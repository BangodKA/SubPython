#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <stack>
#include <map>
#include <queue>

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
    operator bool() const { CheckIs(Logic); return logic_; }
    
    ValueType GetType();

  private:
    ValueType type_;
    std::string str_;
    int integral_ = 0;
    double real_ = 0.0;
    bool logic_ = false;

    void CheckIs(ValueType type) const;
};

ValueType PolymorphicValue::GetType() {
    return type_;
}

// PolymorphicValue::operator bool() const {
// 	if (type_ == Logic) {
// 		return logic_;
// 	}
// 	else if (type_ == Int) {
// 		return integral_;
// 	}
// 	else if (type_ == Str) {
// 		if (str_ == "") {
// 			return false;
// 		}
// 		return true;
// 	}
// 	return real_;	
// }

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

  private:
    Variable* variable_;
    PolymorphicValue value_;

};

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
    std::queue<int> position;

	std::vector<StackValue> DumpStack() const;

	std::ostream& Show(std::ostream& out) const;

};

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
  VariableOperation(const VariableName& name, int pos, int line): name_(name), pos_(pos), line_(line) {}

  void Do(Context& context) const final;

 private:
  const VariableName name_;
  int pos_;
  int line_;
};

void VariableOperation::Do(Context& context) const {
    try {
        context.stack.emplace(context.variables.at(name_).get());
    }
    catch (std::out_of_range) {
        throw CustomException("line " + std::to_string(line_) + ":" +
        std::to_string(pos_) + ": NameError: name '" + name_ +"' is not defined");
    }

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
    context.stack.pop();

	if (!value.Get()) {
		GoOperation::Do(context);
	}
}

struct ThrowCustomException : Operation {
	ThrowCustomException(std::string str) : str_(str) {} 
  	void Do(Context& context) const final;
  private:
	std::string str_;
};

void ThrowCustomException::Do(Context& context) const {
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

template<typename T1, typename T2>
struct GetRangeOperation : Operation {
    void Do(Context& context) const final;
};

template<typename T1, typename T2>
void GetRangeOperation<T1, T2>::Do(Context& context) const {
    if (context.stack.size() == 1) {
        StackValue op2 = context.stack.top();
        context.stack.pop();
        context.stack.emplace(StackValue(0));
        context.stack.emplace(op2);
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
struct EqualStrOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
StackValue EqualStrOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(false);
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

struct BoolCast : Operation {
    void Do(Context& context) const final;
};

void BoolCast::Do(Context& context) const {
    StackValue op = context.stack.top();
    context.stack.pop();
	switch (op.Get().GetType()) {
        case Logic:
            context.stack.emplace(bool(op.Get()));
            break;
        case Str:
            std::string(op.Get()) == "" ? context.stack.emplace(false) : context.stack.emplace(true);
            break;
        case Int:
            int(op.Get()) == 0 ? context.stack.emplace(false) : context.stack.emplace(true);
            break;
        case Real:
            double(op.Get()) == 0 ? context.stack.emplace(false) : context.stack.emplace(true);
            break;
        default:
            break;
    }
}

struct IntCast : Operation {
    void Do(Context& context) const final;
};

void IntCast::Do(Context& context) const {
    StackValue op = context.stack.top();
    context.stack.pop();
	switch (op.Get().GetType()) {
        case Logic:
            context.stack.emplace(int(bool(op.Get())));
            break;
        case Str:
            context.stack.emplace(std::stoi(std::string(op.Get())));
            break;
        case Int:
            context.stack.emplace(int(op.Get()));
            break;
        case Real:
            context.stack.emplace(int(double(op.Get())));
            break;
        default:
            break;
    }
}

struct FloatCast : Operation {
    void Do(Context& context) const final;
};

void FloatCast::Do(Context& context) const {
    StackValue op = context.stack.top();
    context.stack.pop();
    switch (op.Get().GetType()) {
        case Logic:
            context.stack.emplace(double(bool(op.Get())));
            break;
        case Str:
            if (std::string(op.Get()) == "True" || std::string(op.Get()) == "False") {
                std::string(op.Get()) == "False" ? context.stack.emplace(0.0) : context.stack.emplace(1.0);
                break;
            }
            context.stack.emplace(std::stod(std::string(op.Get())));
            break;
        case Int:
            context.stack.emplace(double(int(op.Get())));
            break;
        case Real:
            context.stack.emplace(double(op.Get()));
            break;
        default:
            break;
    }
}

struct StrCast : Operation {
    void Do(Context& context) const final;
};

void StrCast::Do(Context& context) const {
    StackValue op = context.stack.top();
    context.stack.pop();
    switch (op.Get().GetType())
    {
    case Logic:
        bool(op.Get()) == false ? context.stack.emplace("False") : context.stack.emplace("True");
        break;
    case Str:
        context.stack.emplace(std::string(op.Get()));
        break;
    case Int:
        context.stack.emplace(std::to_string(int(op.Get())));
        break;
    case Real:
        context.stack.emplace(std::to_string(double(op.Get())));
        break;
    default:
        break;
    }
}

struct Cast : Operation {
    Cast(Lexeme::LexemeType cast_type): cast_type_(cast_type) {}
    void Do(Context& context) const final;
 private:
    Lexeme::LexemeType cast_type_;
};

void Cast::Do(Context& context) const {
    switch (cast_type_) {
        case Lexeme::Str:
            StrCast().Do(context);
            break;
        case Lexeme::Float:
            FloatCast().Do(context);
            break;
        case Lexeme::Bool:
            BoolCast().Do(context);
            break;
        case Lexeme::Int:
            IntCast().Do(context);
            break;
        default:
            break;
    }
}

struct PrintOperation : Operation {
    void Do(Context& context) const final;
};


void PrintOperation::Do(Context& context) const {
    StackValue op = context.stack.top();
    context.stack.pop();
    switch (op.Get().GetType()) {
        case Logic:
            std::cout << (bool(op.Get()) == 1 ? "True" : "False") << std::endl;
            break;
        case Str:
            std::cout << std::string(op.Get()) << std::endl;
            break;
        case Int:
            std::cout << int(op.Get()) << std::endl;
            break;
        case Real:
            std::cout << double(op.Get()) << std::endl;
            break;
        default:
            break;
    }
	
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
	// UnaryNoStr(Bool, BoolCast)
	// UnaryNoStr(Int, IntCast)
	// UnaryNoStr(Float, FloatCast)
	// UnaryNoStr(Str, StrCast)

	UnaryNoStr(Not, NotOperation)

	{{Lexeme::Not, Str}, std::shared_ptr<Operation>(new NotStrOperation<std::string>)},
	// {{Lexeme::Bool, Str}, std::shared_ptr<Operation>(new BoolStrCast<std::string>)},
	// {{Lexeme::Int, Str}, std::shared_ptr<Operation>(new IntStrCast<std::string>)},
	// {{Lexeme::Float, Str}, std::shared_ptr<Operation>(new FloatStrCast<std::string>)},
	// {{Lexeme::Str, Str}, std::shared_ptr<Operation>(new StrStrCast<std::string>)},
  
};

using BinaryKey = std::tuple<OperationType, ValueType, ValueType>;

static const std::map<BinaryKey, OperationBuilder> kBinaries {
	{{Lexeme::Add, Str, Str}, &MakeOp<PlusOperation<std::string, std::string>>},
	{{Lexeme::Mul, Str, Int}, &MakeOp<MulStrROperation<std::string, int>>},
	{{Lexeme::Mul, Int, Str}, &MakeOp<MulStrLOperation<int, std::string>>},

    {{Lexeme::Equal, Str, Int}, &MakeOp<EqualStrOperation<std::string, int>>},
	{{Lexeme::Equal, Int, Str}, &MakeOp<EqualStrOperation<int, std::string>>},
    {{Lexeme::Equal, Str, Real}, &MakeOp<EqualStrOperation<std::string, double>>},
	{{Lexeme::Equal, Real, Str}, &MakeOp<EqualStrOperation<double, std::string>>},
    {{Lexeme::Equal, Str, Logic}, &MakeOp<EqualStrOperation<std::string, bool>>},
	{{Lexeme::Equal, Logic, Str}, &MakeOp<EqualStrOperation<bool, std::string>>},

	NumOperation(Lexeme::Add, PlusOperation)
	NumOperation(Lexeme::Sub, MinusOperation)
	NumOperation(Lexeme::Mul, MulOperation)
	NumOperation(Lexeme::Div, DivOperation)

	{{Lexeme::Mod, Int, Int}, &MakeOp<ModOperation<int, int>>},
	{{Lexeme::Mod, Logic, Int}, &MakeOp<ModOperation<bool, int>>},
	{{Lexeme::Mod, Int, Logic}, &MakeOp<ModOperation<int, bool>>},
	{{Lexeme::Mod, Logic, Logic}, &MakeOp<ModOperation<bool, bool>>},

    {{Lexeme::Range, Int, Int}, &MakeOp<GetRangeOperation<int, int>>},

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