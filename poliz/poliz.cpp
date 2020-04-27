#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <stack>
#include <map>

#include <type_traits>

#include "../lexer/lexer.hpp"

#define UnaryNoStr(op, opfunc)\
{{Lexeme::op, Int}, std::shared_ptr<Operation>(new opfunc<int>)},\
{{Lexeme::op, Real}, std::shared_ptr<Operation>(new opfunc<double>)},\
{{Lexeme::op, Logic}, std::shared_ptr<Operation>(new opfunc<bool>)},\

#define NumOperation(op, opfunc)\
{{op, Int, Int}, &MakeOp<opfunc<int, int>>},\
{{op, Int, Real}, &MakeOp<opfunc<int, double>>},\
{{op, Real, Int}, &MakeOp<opfunc<double, int>>},\
{{op, Real, Real}, &MakeOp<opfunc<double, double>>},\
{{op, Logic, Real}, &MakeOp<opfunc<bool, double>>},\
{{op, Real, Logic}, &MakeOp<opfunc<double, bool>>},\
{{op, Logic, Int}, &MakeOp<opfunc<bool, int>>},\
{{op, Int, Logic}, &MakeOp<opfunc<int, bool>>},\
{{op, Logic, Logic}, &MakeOp<opfunc<bool, bool>>},

#define CompOperation(op, opfunc)\
{{op, Int, Int}, &MakeOp<opfunc<int, int>>},\
{{op, Int, Real}, &MakeOp<opfunc<int, double>>},\
{{op, Real, Int}, &MakeOp<opfunc<double, int>>},\
{{op, Real, Real}, &MakeOp<opfunc<double, double>>},\
{{op, Logic, Real}, &MakeOp<opfunc<bool, double>>},\
{{op, Real, Logic}, &MakeOp<opfunc<double, bool>>},\
{{op, Logic, Int}, &MakeOp<opfunc<bool, int>>},\
{{op, Int, Logic}, &MakeOp<opfunc<int, bool>>},\
{{op, Logic, Logic}, &MakeOp<opfunc<bool, bool>>},\
{{op, Str, Str}, &MakeOp<opfunc<std::string, std::string>>},

#define LogicOperation(op, opfunc)\
{{op, Int, Int}, &MakeOp<opfunc<int, int>>},\
{{op, Int, Real}, &MakeOp<opfunc<int, double>>},\
{{op, Real, Int}, &MakeOp<opfunc<double, int>>},\
{{op, Real, Real}, &MakeOp<opfunc<double, double>>},\
{{op, Logic, Real}, &MakeOp<opfunc<bool, double>>},\
{{op, Real, Logic}, &MakeOp<opfunc<double, bool>>},\
{{op, Logic, Int}, &MakeOp<opfunc<bool, int>>},\
{{op, Int, Logic}, &MakeOp<opfunc<int, bool>>},\
{{op, Logic, Logic}, &MakeOp<opfunc<bool, bool>>},

struct CustomException : public std::exception {
	CustomException(std::string str) : str_(str) {}
	const char * what () const throw () {
		return (str_ + "\n").c_str();
	}
  private:
	std::string str_;
};

namespace execution {

enum ValueType { Str, Int, Real, Logic };

const std::string& ToStringSem(ValueType type) {
  static const std::unordered_map<ValueType, std::string> kResults{
    {Str, "string"}, {Int, "int"}, {Real, "double"}, {Logic, "bool"},
  };
  return kResults.at(type);
}

class PolymorphicValue {
  public:
    PolymorphicValue(const char* str): type_(Str), str_(str) {}
    PolymorphicValue(const std::string& str): type_(Str), str_(str) {}
    PolymorphicValue(int integral): type_(Int), integral_(integral) {}
    PolymorphicValue(double real): type_(Real), real_(real) {}
    PolymorphicValue(bool logic): type_(Logic), logic_(logic) {}

    operator std::string() const { CheckIs(Str); return str_; }
    operator int() const { CheckIs(Int); return integral_; }
    operator double() const { CheckIs(Real); return real_; }
    operator bool() const;

	ValueType type_;
    
  private:
    
    std::string str_;
    int integral_ = 0;
    double real_ = 0.0;
    bool logic_ = false;

    void CheckIs(ValueType type) const;
};

PolymorphicValue::operator bool() const {
	if (type_ == Logic) {
		return logic_;
	}
	else if (type_ == Int) {
		return integral_;
	}
	else if (type_ == Str) {
		if (str_ == "") {
			return false;
		}
		return true;
	}
	return real_;	
}

void PolymorphicValue::CheckIs(ValueType type) const {
	if (type != type_) {
		throw std::logic_error("type mismatch expected " + ToStringSem(type) +
								" != actual " + ToStringSem(type_));
	}
}

using VariableName = std::string;

struct Variable {
  VariableName name;
  PolymorphicValue value;

  Variable(const VariableName& name, const PolymorphicValue& value):
    name(name), value(value) {}
};

class StackValue {
  public:
    StackValue(Variable* variable): variable_(variable), value_(0) {}
    StackValue(PolymorphicValue value): variable_(nullptr), value_(value) {}

	PolymorphicValue Get() const;

    StackValue SetValue(const StackValue& value);

	std::ostream& ShowEx(std::ostream& out) const;

  private:
    Variable* variable_;
    PolymorphicValue value_;

};

std::ostream& StackValue::ShowEx(std::ostream& out) const {
    if (variable_) {
      	out << "&" << variable_->name;
    } 
	else {
		switch (value_.type_) {
		case Int:
			out << int(value_);
			break;
		case Real:
			out << double(value_);
			break;
		case Logic:
			out << bool(value_);
			break;
		case Str:
			out << std::string(value_);
			break;
		default:
			break;
		}
    }
    return out;
}

StackValue StackValue::SetValue(const StackValue& value) {
    if (!variable_) {
      throw std::logic_error("stack value is not variable");
    }
    variable_->value = value.Get();
    return variable_->value;
}

PolymorphicValue StackValue::Get() const {
    return variable_ != nullptr ? variable_->value : value_;
}

using OperationIndex = std::size_t;

struct Context {
	OperationIndex operation_index = 0;
	std::stack<StackValue> stack;
	std::unordered_map<VariableName, std::unique_ptr<Variable> > variables;

	std::vector<StackValue> DumpStack() const;

	std::ostream& Show(std::ostream& out) const;

};

std::ostream& Context::Show(std::ostream& out) const {
    out << "index = " << std::setw(2) << operation_index;

    out << ", variables = {";
    bool first = true;
    for (const auto& [name, variable] : variables) {
      if (first) {
        first = false;
      } else {
        out << ", ";
      }
	//   auto variable = std::get<0>(var).get();
	  ValueType type = variable->value.type_;
	  switch (variable->value.type_) {
		case Int:
			out << ToStringSem(type) << ' ' << name << " = " <<  int(variable->value);
			break;
		case Real:
			out << ToStringSem(type) << ' ' << name << " = " << double(variable->value);
			break;
		case Logic:
			out << ToStringSem(type) << ' ' << name << " = " << bool(variable->value);
			break;
		case Str:
			out << ToStringSem(type) << ' ' << name << " = " << std::string(variable->value);
			break;
		default:
			break;
		}
    }
    out << "}";

    out << ", stack = [";
    first = true;
    for (const StackValue& value : DumpStack()) {
      if (first) {
        first = false;
      } else {
        out << ", ";
      }
      value.ShowEx(out);
    }

    out << "]" << std::endl;
    return out;
  }

std::vector<StackValue> Context::DumpStack() const {
    std::vector<StackValue> result;
    std::stack<StackValue> copy = stack;
    while (!copy.empty()) {
        result.push_back(copy.top());
        copy.pop();
    }
    std::reverse(result.begin(), result.end());
    return result;
}

struct Operation {
  virtual ~Operation() {}
  virtual void Do(Context& context) const = 0;
};

struct ValueOperation : Operation {
    ValueOperation(PolymorphicValue value): value_(value) {}

    void Do(Context& context) const final;

 private:
    const PolymorphicValue value_;
};

void ValueOperation::Do(Context& context) const {
    context.stack.emplace(value_);
}

struct VariableOperation : Operation {
  VariableOperation(const VariableName& name): name_(name) {}

  void Do(Context& context) const final;

 private:
  const VariableName name_;
};

void VariableOperation::Do(Context& context) const {
    context.stack.emplace(context.variables.at(name_).get());
}


struct AssignOperation : Operation {
  	AssignOperation(const VariableName& name): name_(name) {}

  	void Do(Context& context) const final;

  private:
  	const VariableName name_;
};

void AssignOperation::Do(Context& context) const {
    StackValue value = context.stack.top();
    context.stack.pop();

	context.variables[name_].reset(new Variable(name_, value.Get()));
}

struct AddOneOperation : Operation {
  	AddOneOperation(const VariableName& name): name_(name) {}

  	void Do(Context& context) const final;

  private:
  	const VariableName name_;
};

void AddOneOperation::Do(Context& context) const {
    StackValue value = context.stack.top();
    context.stack.pop();

	context.variables[name_].reset(new Variable(name_, int(value.Get()) + 1));
}


struct AddForVarOperation : Operation {
  	void Do(Context& context) const final;
};

void AddForVarOperation::Do(Context& context) const {
	StackValue op2 = context.stack.top();
	context.stack.pop();

	StackValue op1 = context.stack.top();
	context.stack.pop();

	context.variables[op1.Get()].reset(new Variable(op1.Get(), op2.Get()));

	context.stack.push(op1.SetValue(op2));
}

struct GoOperation : Operation {
	GoOperation(OperationIndex index): index_(index) {}

	void Do(Context& context) const override;

	private:
	const OperationIndex index_;
};

void GoOperation::Do(Context& context) const {
	context.operation_index = index_;
}

struct IfOperation : GoOperation {
	IfOperation(OperationIndex index): GoOperation(index) {}

	void Do(Context& context) const final;
};

void IfOperation::Do(Context& context) const {
  	StackValue value = context.stack.top();
//   context.stack.pop();

	if (!value.Get()) {
		GoOperation::Do(context);
	}
}

struct PopOperation : Operation {
  	void Do(Context& context) const final;
};

void PopOperation::Do(Context& context) const {
    context.stack.pop();
}

struct ThrowBadOperand : Operation {
	ThrowBadOperand(std::string str) : str_(str) {} 
  	void Do(Context& context) const final;
  private:
	std::string str_;
};

void ThrowBadOperand::Do(Context& context) const {
    throw CustomException(str_);
}

struct ThrowTypeError : Operation {
	ThrowTypeError(std::string str) : str_(str) {} 
  	void Do(Context& context) const final;
  private:
	std::string str_;
};

void ThrowTypeError::Do(Context& context) const {
    throw CustomException(str_);
}

template<typename T>
struct UnaryMinusOperation : Operation {
  	void Do(Context& context) const final;
};

template<typename T>
void UnaryMinusOperation<T>::Do(Context& context) const {
	const PolymorphicValue new_value(-static_cast<T>(context.stack.top().Get()));
	context.stack.pop();
	context.stack.push(new_value);
}

template<typename T>
struct NotOperation : Operation {
  	void Do(Context& context) const final;
};

template<typename T>
void NotOperation<T>::Do(Context& context) const {
	const PolymorphicValue new_value(!(static_cast<T>(context.stack.top().Get())));
	context.stack.pop();
	context.stack.push(new_value);
}

// template<typename T>
// void NotOperation<T>::Do(Context& context) const {
// 	T val = static_cast<T>(context.stack.top().Get());
// 	context.stack.pop();
// 	if (val == 0) {
// 		context.stack.push(StackValue(true));
// 	}
// 	else {
// 		context.stack.push(StackValue(false));
// 	}
// }

template<typename T>
struct NotStrOperation : Operation {
    void Do(Context& context) const final;
};

template<typename T>
void NotStrOperation<T>::Do(Context& context) const {
    StackValue op = context.stack.top();
    context.stack.pop();
    if (static_cast<T>(op.Get()) != "") {
      	context.stack.emplace(false);
    }
    else {
      	context.stack.emplace(true);
    }
}

struct MathOperation : Operation {
    void Do(Context& context) const final;

    virtual StackValue DoMath(StackValue op1, StackValue op2) const = 0;
};

void MathOperation::Do(Context& context) const {
    StackValue op2 = context.stack.top();
    context.stack.pop();

    StackValue op1 = context.stack.top();
    context.stack.pop();

    context.stack.push(DoMath(op1, op2));
}

template<typename T1, typename T2>
struct PlusOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
StackValue PlusOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(static_cast<T1>(op1.Get()) + static_cast<T2>(op2.Get()));
}

template<typename T1, typename T2>
struct MinusOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
StackValue MinusOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(static_cast<T1>(op1.Get()) - static_cast<T2>(op2.Get()));
}

template<typename T1, typename T2>
struct MulOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
StackValue MulOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(static_cast<T1>(op1.Get()) * static_cast<T2>(op2.Get()));
}

template<typename T1, typename T2>
struct MulStrLOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
StackValue MulStrLOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
	std::string new_str = "";
	std::string old_str = static_cast<T2>(op2.Get());

	for (int i = 0; i < static_cast<T1>(op1.Get()); ++i) {
		new_str += old_str;
	}
    return StackValue(new_str);
}

template<typename T1, typename T2>
struct MulStrROperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
StackValue MulStrROperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
	std::string new_str = "";
	std::string old_str = static_cast<T1>(op1.Get());

	for (int i = 0; i < static_cast<T2>(op2.Get()); ++i) {
		new_str += old_str;
	}
    return StackValue(new_str);
}

template<typename T1, typename T2>
struct DivOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
StackValue DivOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(static_cast<T1>(op1.Get()) / static_cast<T2>(op2.Get()));
}

template<typename T1, typename T2>
struct ModOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
StackValue ModOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(static_cast<T1>(op1.Get()) % static_cast<T2>(op2.Get()));
}

template<typename T1, typename T2>
struct LessOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
StackValue LessOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(static_cast<T1>(op1.Get()) < static_cast<T2>(op2.Get()));
}

template<typename T1, typename T2>
struct LessEqOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
StackValue LessEqOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(static_cast<T1>(op1.Get()) <= static_cast<T2>(op2.Get()));
}

template<typename T1, typename T2>
struct GreaterOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
StackValue GreaterOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(static_cast<T1>(op1.Get()) > static_cast<T2>(op2.Get()));
}

template<typename T1, typename T2>
struct GreaterEqOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
StackValue GreaterEqOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(static_cast<T1>(op1.Get()) >= static_cast<T2>(op2.Get()));
}

template<typename T1, typename T2>
struct EqualOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
StackValue EqualOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(static_cast<T1>(op1.Get()) == static_cast<T2>(op2.Get()));
}

template<typename T1, typename T2>
struct NotEqualOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
StackValue NotEqualOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(static_cast<T1>(op1.Get()) != static_cast<T2>(op2.Get()));
}

template<typename T1, typename T2>
struct AndOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
StackValue AndOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(static_cast<T1>(op1.Get()) && static_cast<T2>(op2.Get()));
}

template<typename T1, typename T2>
struct OrOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
StackValue OrOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(static_cast<T1>(op1.Get()) || static_cast<T2>(op2.Get()));
}

template<typename T>
struct BoolCast : Operation {
    void Do(Context& context) const final;
};

template<typename T>
void BoolCast<T>::Do(Context& context) const {
    StackValue op = context.stack.top();
    context.stack.pop();
	  context.stack.emplace(bool(static_cast<T>(op.Get())));
}

template<typename T>
struct BoolStrCast : Operation {
    void Do(Context& context) const final;
};

template<typename T>
void BoolStrCast<T>::Do(Context& context) const {
    StackValue op = context.stack.top();
    context.stack.pop();
    if (static_cast<T>(op.Get()) != "") {
      	context.stack.emplace(true);
    }
    else {
      	context.stack.emplace(false);
    }
}

template<typename T>
struct IntCast : Operation {
    void Do(Context& context) const final;
};

template<typename T>
void IntCast<T>::Do(Context& context) const {
    StackValue op = context.stack.top();
    context.stack.pop();
	context.stack.emplace(int(static_cast<T>(op.Get())));
}

template<typename T>
struct IntStrCast : Operation {
    void Do(Context& context) const final;
};

template<typename T>
void IntStrCast<T>::Do(Context& context) const {
    StackValue op = context.stack.top();
    context.stack.pop();
	std::string op_str = static_cast<T>(op.Get());

    context.stack.emplace(std::stoi(op_str));
}

template<typename T>
struct FloatCast : Operation {
    void Do(Context& context) const final;
};

template<typename T>
void FloatCast<T>::Do(Context& context) const {
    StackValue op = context.stack.top();
    context.stack.pop();
	context.stack.emplace(float(static_cast<T>(op.Get())));
}

template<typename T>
struct FloatStrCast : Operation {
    void Do(Context& context) const final;
};

template<typename T>
void FloatStrCast<T>::Do(Context& context) const {
    StackValue op = context.stack.top();
    context.stack.pop();
	std::string op_str = static_cast<T>(op.Get());

    context.stack.emplace(std::stof(op_str));
}

template<typename T>
struct StrCast : Operation {
    void Do(Context& context) const final;
};

template<typename T>
void StrCast<T>::Do(Context& context) const {
    StackValue op = context.stack.top();
    context.stack.pop();
	context.stack.emplace(std::to_string(static_cast<T>(op.Get())));
}

template<typename T>
struct StrStrCast : Operation {
    void Do(Context& context) const final;
};

template<typename T>
void StrStrCast<T>::Do(Context& context) const {
    return;
}

template<typename T>
struct PrintOperation : Operation {
    void Do(Context& context) const final;
};

template<typename T>
void PrintOperation<T>::Do(Context& context) const {
    StackValue op = context.stack.top();
    context.stack.pop();
	
	std::cout << static_cast<T>(op.Get()) << std::endl;
}

using Operations = std::vector<std::shared_ptr<Operation>>;

using OperationType = Lexeme::LexemeType;

template<typename T>
Operation* MakeOp() {
  	return new T();
}

using OperationBuilder = Operation* (*)();

using UnaryKey = std::tuple<OperationType, ValueType>;

static const std::map<UnaryKey, std::shared_ptr<Operation>> kUnaries {
	UnaryNoStr(UnaryMinus, UnaryMinusOperation)
	UnaryNoStr(Bool, BoolCast)
	UnaryNoStr(Int, IntCast)
	UnaryNoStr(Float, FloatCast)
	UnaryNoStr(Str, StrCast)

	UnaryNoStr(Print, PrintOperation)
	UnaryNoStr(Not, NotOperation)

	{{Lexeme::Not, Str}, std::shared_ptr<Operation>(new NotStrOperation<std::string>)},
	{{Lexeme::Print, Str}, std::shared_ptr<Operation>(new PrintOperation<std::string>)},
	{{Lexeme::Bool, Str}, std::shared_ptr<Operation>(new BoolStrCast<std::string>)},
	{{Lexeme::Int, Str}, std::shared_ptr<Operation>(new IntStrCast<std::string>)},
	{{Lexeme::Float, Str}, std::shared_ptr<Operation>(new FloatStrCast<std::string>)},
	{{Lexeme::Str, Str}, std::shared_ptr<Operation>(new StrStrCast<std::string>)},
  
};

using BinaryKey = std::tuple<OperationType, ValueType, ValueType>;

static const std::map<BinaryKey, OperationBuilder> kBinaries {
	{{Lexeme::Add, Str, Str}, &MakeOp<PlusOperation<std::string, std::string>>},
	{{Lexeme::Mul, Str, Int}, &MakeOp<MulStrROperation<std::string, int>>},
	{{Lexeme::Mul, Int, Str}, &MakeOp<MulStrLOperation<int, std::string>>},

	NumOperation(Lexeme::Add, PlusOperation)
	NumOperation(Lexeme::Sub, MinusOperation)
	NumOperation(Lexeme::Mul, MulOperation)
	NumOperation(Lexeme::Div, DivOperation)
	// NumOperation(Mod, ModOperation)

	{{Lexeme::Mod, Int, Int}, &MakeOp<ModOperation<int, int>>},
	{{Lexeme::Mod, Logic, Int}, &MakeOp<ModOperation<bool, int>>},
	{{Lexeme::Mod, Int, Logic}, &MakeOp<ModOperation<int, bool>>},
	{{Lexeme::Mod, Logic, Logic}, &MakeOp<ModOperation<bool, bool>>},

	CompOperation(Lexeme::Less, LessOperation)
	CompOperation(Lexeme::Greater, GreaterOperation)
	CompOperation(Lexeme::LessEq, LessEqOperation)
	CompOperation(Lexeme::GreaterEq, GreaterEqOperation)
	CompOperation(Lexeme::Equal, EqualOperation)
  	CompOperation(Lexeme::NotEqual, NotEqualOperation)

	LogicOperation(Lexeme::And, AndOperation)
	LogicOperation(Lexeme::Or, OrOperation)

};

}