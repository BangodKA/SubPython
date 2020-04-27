#pragma once

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

#define LogicOperation(op, opfunc)\
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

const std::string& ToStringSem(ValueType type);

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
    
  private:
	ValueType type_;
    std::string str_;
    int integral_ = 0;
    double real_ = 0.0;
    bool logic_ = false;

    void CheckIs(ValueType type) const;
};

using VariableName = std::string;
// using VariableValue = int;

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


using OperationIndex = std::size_t;

struct Context {
	OperationIndex operation_index = 0;
	std::stack<StackValue> stack;
	std::unordered_map<VariableName, std::unique_ptr<Variable> > variables;

	std::vector<StackValue> DumpStack() const;

	std::ostream& Show(std::ostream& out) const;

};


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

struct VariableOperation : Operation {
  	VariableOperation(const VariableName& name): name_(name) {}

  	void Do(Context& context) const final;

  private:
  	const VariableName name_;
};

struct AssignOperation : Operation {
  	AssignOperation(const VariableName& name): name_(name) {}

  	void Do(Context& context) const final;

  private:
  	const VariableName name_;
};

struct AddOneOperation : Operation {
  	AddOneOperation(const VariableName& name): name_(name) {}

  	void Do(Context& context) const final;

  private:
  	const VariableName name_;
};

struct AddForVarOperation : Operation {
  	void Do(Context& context) const final;
};

struct GoOperation : Operation {
	GoOperation(OperationIndex index): index_(index) {}

	void Do(Context& context) const override;

	private:
	const OperationIndex index_;
};

// Операция - условный переход по лжи
struct IfOperation : GoOperation {
	IfOperation(OperationIndex index): GoOperation(index) {}

	void Do(Context& context) const final;
};

struct PopOperation : Operation {
  	void Do(Context& context) const final;
};

struct ThrowBadOperand : Operation {
	ThrowBadOperand(std::string str) : str_(str) {} 
  	void Do(Context& context) const final;
  private:
	std::string str_;
};

struct ThrowInvalidArgument : Operation {
	ThrowInvalidArgument(std::string str) : str_(str) {} 
  	void Do(Context& context) const final;
  private:
	std::string str_;
};

template<typename T>
struct UnaryMinusOperation : Operation {
  void Do(Context& context) const final;
};

struct MathOperation : Operation {
    void Do(Context& context) const final;

    virtual StackValue DoMath(StackValue op1, StackValue op2) const = 0;
};

template<typename T1, typename T2>
struct PlusOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
struct MinusOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
struct MulOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
struct MulStrLOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
struct MulStrROperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
struct DivOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
struct ModOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
struct LessOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
struct LessEqOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
struct GreaterOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
struct GreaterEqOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
struct EqualOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
struct NotEqualOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T>
struct BoolCast : Operation {
    void Do(Context& context) const final;
};

template<typename T>
struct BoolStrCast : Operation {
    void Do(Context& context) const final;
};

template<typename T>
struct IntCast : Operation {
    void Do(Context& context) const final;
};

template<typename T>
struct IntStrCast : Operation {
    void Do(Context& context) const final;
};

template<typename T>
struct FloatCast : Operation {
    void Do(Context& context) const final;
};

template<typename T>
struct FloatStrCast : Operation {
    void Do(Context& context) const final;
};

template<typename T>
struct StrCast : Operation {
    void Do(Context& context) const final;
};

template<typename T>
struct StrStrCast : Operation {
    void Do(Context& context) const final;
};

template<typename T>
struct PrintOperation : Operation {
    void Do(Context& context) const final;
};

using Operations = std::vector<std::shared_ptr<Operation>>;

using OperationType = Lexeme::LexemeType;

template<typename T>
Operation* MakeOp();

using OperationBuilder = Operation* (*)();

using UnaryKey = std::tuple<OperationType, ValueType>;

static const std::map<UnaryKey, std::shared_ptr<Operation>> kUnaries {
	UnaryNoStr(UnaryMinus, UnaryMinusOperation)
	UnaryNoStr(Bool, BoolCast)
	UnaryNoStr(Int, IntCast)
	UnaryNoStr(Float, FloatCast)
	UnaryNoStr(Str, StrCast)

	UnaryNoStr(Print, PrintOperation)
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

	LogicOperation(Lexeme::Less, LessOperation)
	LogicOperation(Lexeme::Greater, GreaterOperation)
	LogicOperation(Lexeme::LessEq, LessEqOperation)
	LogicOperation(Lexeme::GreaterEq, GreaterEqOperation)
	LogicOperation(Lexeme::Equal, EqualOperation)
	LogicOperation(Lexeme::NotEqual, NotEqualOperation)
};

} // namespace execution