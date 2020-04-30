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
#include "poliz.hpp"

namespace execution {

const std::string& ToStringSem(ValueType type) {
  static const std::unordered_map<ValueType, std::string> kResults{
    {Str, "string"}, {Int, "int"}, {Real, "double"}, {Logic, "bool"},
  };
  return kResults.at(type);
}

PolymorphicValue::PolymorphicValue(const char* str): type_(Str), str_(str) {}
PolymorphicValue::PolymorphicValue(const std::string& str): type_(Str), str_(str) {}
PolymorphicValue::PolymorphicValue(int integral): type_(Int), integral_(integral) {}
PolymorphicValue::PolymorphicValue(double real): type_(Real), real_(real) {}
PolymorphicValue::PolymorphicValue(bool logic): type_(Logic), logic_(logic) {}

ValueType PolymorphicValue::GetType() {
    return type_;
}

PolymorphicValue::operator std::string() const { CheckIs(Str); return str_; }
PolymorphicValue::operator double() const { CheckIs(Real); return real_; }

PolymorphicValue::operator int() const {
	if (type_ == Int) {
		return integral_;
	}
	else if (type_ == Logic) {
		return logic_ == false ? 0 : 1;
	}
	else if (type_ == Str) {
        throw std::logic_error("type mismatch expected " + ToStringSem(type_) +
						" != actual " + ToStringSem(Int));
	}
	throw std::logic_error("type mismatch expected " + ToStringSem(Real) +
						" != actual " + ToStringSem(Int));
}

PolymorphicValue::operator bool() const {
	if (type_ == Logic) {
		return logic_;
	}
	else if (type_ == Int) {
		return integral_ == 0 ? false : true;
	}
	else if (type_ == Str) {
        return str_ == "" ? false : true;
	}
	return real_ == 0 ? false : true;
}

void PolymorphicValue::CheckIs(ValueType type) const {
	if (type != type_) {
		throw std::logic_error("type mismatch expected " + ToStringSem(type) +
								" != actual " + ToStringSem(type_));
	}
}

Variable::Variable(const VariableName& name, const PolymorphicValue& value):
    name(name), value(value) {}

StackValue::StackValue(Variable* variable): variable_(variable), value_(0) {}

StackValue::StackValue(PolymorphicValue value): variable_(nullptr), value_(value) {}

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

Operation::~Operation() {}

ValueOperation::ValueOperation(std::string value, Lexeme::LexemeType type, int pos, int line):
             value_(value), type_(type), pos_(pos), line_(line) {}

void ValueOperation::Do(Context& context) const {
    switch (type_) {
		case Lexeme::IntegerConst:
			try {
                context.stack.emplace(std::stoi(value_));
            } catch (std::out_of_range) {
                throw std::runtime_error("line " + std::to_string(line_) + ":" + std::to_string(pos_) + 
				": RangeError: " + value_ + " is too big for int");
            }
			break;
		case Lexeme::BoolConst:
			context.stack.emplace((value_ == "True" ? true : false));
			break;
		case Lexeme::FloatConst:
			try {
				context.stack.emplace(std::stod(value_));
			} catch (std::out_of_range) {
                throw std::runtime_error("line " + std::to_string(line_) + ":" + std::to_string(pos_) + 
				": RangeError: " + value_ + " is too precise for double");
            }
			break;
		case Lexeme::StringConst:
			context.stack.emplace(value_);
			break;
        default:
            break;
	}
}

VariableOperation::VariableOperation(const VariableName& name, int pos, int line): 
                                name_(name), pos_(pos), line_(line) {}

void VariableOperation::Do(Context& context) const {
    try {
        context.stack.emplace(context.variables.at(name_).get());
    }
    catch (std::out_of_range) {
        throw std::runtime_error("line " + std::to_string(line_) + ":" +
        std::to_string(pos_) + ": NameError: name '" + name_ +"' is not defined");
    }
}

AssignOperation::AssignOperation(const VariableName& name): name_(name) {}

void AssignOperation::Do(Context& context) const {
    StackValue value = context.stack.top();
    context.stack.pop();

	context.variables[name_].reset(new Variable(name_, value.Get()));
}

AddOneOperation::AddOneOperation(const VariableName& name): name_(name) {}

void AddOneOperation::Do(Context& context) const {
    StackValue value = context.stack.top();
    context.stack.pop();

	context.variables[name_].reset(new Variable(name_, int(value.Get()) + 1));
}

GoOperation::GoOperation(OperationIndex index): index_(index) {}

void GoOperation::Do(Context& context) const {
	context.operation_index = index_;
}

IfOperation::IfOperation(OperationIndex index): GoOperation(index) {}

void IfOperation::Do(Context& context) const {
  	StackValue value = context.stack.top();
    context.stack.pop();

	if (!bool(value.Get())) {
		GoOperation::Do(context);
	}
}

UnaryMinusOperation::UnaryMinusOperation(int pos, int line): pos_(pos), line_(line) {}

void UnaryMinusOperation::Do(Context& context) const {
    StackValue op = context.stack.top();
    context.stack.pop();

    switch (op.Get().GetType()) {
        case Logic:
        case Int: 
            context.stack.push(StackValue(-int(op.Get())));
            break;
        case Real: 
            context.stack.push(StackValue(-double(op.Get())));
            break;
        case Str:
            throw std::runtime_error("line " + std::to_string(line_) + ":" + std::to_string(pos_) + 
				": TypeError: unsupported operand type(s) for unary -: " + ToStringSem(op.Get().GetType()));
    }
	
}

void NotOperation::Do(Context& context) const {
	const PolymorphicValue new_value(!(bool(context.stack.top().Get())));
	context.stack.pop();
	context.stack.push(new_value);
}

GetRangeOperation::GetRangeOperation(int pos, int line): pos_(pos), line_(line) {}

void GetRangeOperation::Do(Context& context) const {
    StackValue op2 = context.stack.top();
    context.stack.pop();
    if (context.stack.size() == 0) {
        if (op2.Get().GetType() != Int && op2.Get().GetType() != Logic) {
            throw std::runtime_error("line " + std::to_string(line_) + ":" + std::to_string(pos_) + 
				": TypeError: unsupported operand type(s) for range" + ": " +  ToStringSem(op2.Get().GetType()));
        }
        context.stack.emplace(StackValue(0));
        context.stack.emplace(op2);
    }
    else {
        StackValue op1 = context.stack.top();
        if ((op1.Get().GetType() != Int && op1.Get().GetType() != Logic) ||
            (op2.Get().GetType() != Int && op2.Get().GetType() != Logic)) {
            throw std::runtime_error("line " + std::to_string(line_) + ":" + std::to_string(pos_) + 
				": TypeError: unsupported operand type(s) for range: " +
                ToStringSem(op1.Get().GetType()) + " and " + ToStringSem(op2.Get().GetType()));
        }
        context.stack.emplace(op2);
    }
}

void MathOperation::Do(Context& context) const {}

ExecuteOperation::ExecuteOperation(Lexeme::LexemeType type, int pos, int line):
                         type_(type), pos_(pos), line_(line) {}

template<typename T1, typename T2>
StackValue PlusOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(static_cast<T1>(op1.Get()) + static_cast<T2>(op2.Get()));
}

template<typename T1, typename T2>
StackValue MinusOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(static_cast<T1>(op1.Get()) - static_cast<T2>(op2.Get()));
}

template<typename T1, typename T2>
StackValue MulOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(static_cast<T1>(op1.Get()) * static_cast<T2>(op2.Get()));
}

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
StackValue MulStrROperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
	std::string new_str = "";
	std::string old_str = static_cast<T1>(op1.Get());

	for (int i = 0; i < static_cast<T2>(op2.Get()); ++i) {
		new_str += old_str;
	}
    return StackValue(new_str);
}

template<typename T1, typename T2>
StackValue DivOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(static_cast<T1>(op1.Get()) / static_cast<T2>(op2.Get()));
}

template<typename T1, typename T2>
StackValue ModOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(static_cast<T1>(op1.Get()) % static_cast<T2>(op2.Get()));
}

template<typename T1, typename T2>
StackValue LessOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(static_cast<T1>(op1.Get()) < static_cast<T2>(op2.Get()));
}

template<typename T1, typename T2>
StackValue LessEqOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(static_cast<T1>(op1.Get()) <= static_cast<T2>(op2.Get()));
}

template<typename T1, typename T2>
StackValue GreaterOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(static_cast<T1>(op1.Get()) > static_cast<T2>(op2.Get()));
}

template<typename T1, typename T2>
StackValue GreaterEqOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(static_cast<T1>(op1.Get()) >= static_cast<T2>(op2.Get()));
}

template<typename T1, typename T2>
StackValue EqualOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(static_cast<T1>(op1.Get()) == static_cast<T2>(op2.Get()));
}

template<typename T1, typename T2>
StackValue EqualStrOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(false);
}

template<typename T1, typename T2>
StackValue NotEqualOperation<T1, T2>::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(static_cast<T1>(op1.Get()) != static_cast<T2>(op2.Get()));
}

StackValue AndOperation::DoMath(StackValue op1, StackValue op2) const {
    return StackValue(bool((op1.Get())) && bool((op2.Get())));
}

StackValue OrOperation::DoMath(StackValue op1, StackValue op2) const  {
    return StackValue(bool((op1.Get())) || bool((op2.Get())));
}

void BoolCast::Do(Context& context) const {
    StackValue op = context.stack.top();
    context.stack.pop();
    context.stack.emplace(bool(op.Get()));
}

IntCast::IntCast(int pos, int line): pos_(pos), line_(line) {}

void IntCast::Do(Context& context) const {
    StackValue op = context.stack.top();
    context.stack.pop();
	switch (op.Get().GetType()) {
        case Str:
            if (std::string(op.Get()) == "True" || std::string(op.Get()) == "False") {
                std::string(op.Get()) == "False" ? context.stack.emplace(0) : context.stack.emplace(1);
                break;
            }
            try {
                context.stack.emplace(std::stoi(std::string(op.Get())));
            } catch (std::out_of_range) {
                throw std::runtime_error("line " + std::to_string(line_) + ":" + std::to_string(pos_) + 
				": RangeError: " + std::string(op.Get()) + " is too big for int()");
            }
            break;
        case Logic:
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
FloatCast::FloatCast(int pos, int line): pos_(pos), line_(line) {}

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
            try {
                context.stack.emplace(std::stod(std::string(op.Get())));
            } catch (std::out_of_range) {
                throw std::runtime_error("line " + std::to_string(line_) + ":" + std::to_string(pos_) + 
				": RangeError: " + std::string(op.Get()) + " is too precise for float()");
            }
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
Cast::Cast(Lexeme::LexemeType cast_type, int pos, int line): cast_type_(cast_type), pos_(pos), line_(line) {}

void Cast::Do(Context& context) const {
    switch (cast_type_) {
        case Lexeme::Str:
            StrCast().Do(context);
            break;
        case Lexeme::Float:
            FloatCast(pos_, line_).Do(context);
            break;
        case Lexeme::Bool:
            BoolCast().Do(context);
            break;
        case Lexeme::Int:
            IntCast(pos_, line_).Do(context);
            break;
        default:
            break;
    }
}

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

#include "poliz.tpp"

void ExecuteOperation::Do(Context& context) const {
    StackValue op2 = context.stack.top();
    context.stack.pop();

    StackValue op1 = context.stack.top();
    context.stack.pop();
    try {
        context.stack.emplace(
            execution::kMathBinaries.at(
                std::make_tuple(type_, op1.Get().GetType(), op2.Get().GetType()))->DoMath(op1, op2));
    } catch (std::out_of_range) {
        throw std::runtime_error("line " + std::to_string(line_) + ":" + std::to_string(pos_) + 
				": TypeError: unsupported operand type(s) for " + Lexeme::TypeToString(type_) + ": " +  
                ToStringSem(op1.Get().GetType()) + " and " + ToStringSem(op2.Get().GetType()));
    }
}

}