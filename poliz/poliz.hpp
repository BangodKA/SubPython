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

#define NumOperation(op, opfunc)\
{{op, Int, Int}, std::shared_ptr<MathOperation>(new opfunc<int, int>)},\
{{op, Int, Real}, std::shared_ptr<MathOperation>(new opfunc<int, double>)},\
{{op, Real, Int}, std::shared_ptr<MathOperation>(new opfunc<double, int>)},\
{{op, Real, Real}, std::shared_ptr<MathOperation>(new opfunc<double, double>)},\
{{op, Logic, Real}, std::shared_ptr<MathOperation>(new opfunc<bool, double>)},\
{{op, Real, Logic}, std::shared_ptr<MathOperation>(new opfunc<double, bool>)},\
{{op, Logic, Int}, std::shared_ptr<MathOperation>(new opfunc<bool, int>)},\
{{op, Int, Logic}, std::shared_ptr<MathOperation>(new opfunc<int, bool>)},\
{{op, Logic, Logic}, std::shared_ptr<MathOperation>(new opfunc<bool, bool>)},

#define CompOperation(op, opfunc)\
{{op, Int, Int}, std::shared_ptr<MathOperation>(new opfunc<int, int>)},\
{{op, Int, Real}, std::shared_ptr<MathOperation>(new opfunc<int, double>)},\
{{op, Real, Int}, std::shared_ptr<MathOperation>(new opfunc<double, int>)},\
{{op, Real, Real}, std::shared_ptr<MathOperation>(new opfunc<double, double>)},\
{{op, Logic, Real}, std::shared_ptr<MathOperation>(new opfunc<bool, double>)},\
{{op, Real, Logic}, std::shared_ptr<MathOperation>(new opfunc<double, bool>)},\
{{op, Logic, Int}, std::shared_ptr<MathOperation>(new opfunc<bool, int>)},\
{{op, Int, Logic}, std::shared_ptr<MathOperation>(new opfunc<int, bool>)},\
{{op, Logic, Logic}, std::shared_ptr<MathOperation>(new opfunc<bool, bool>)},\
{{op, Str, Str}, std::shared_ptr<MathOperation>(new opfunc<std::string, std::string>)},

#define LogicOperation(op, opfunc)\
{{op, Int, Int}, std::shared_ptr<MathOperation>(new opfunc<int, int>)},\
{{op, Int, Real}, std::shared_ptr<MathOperation>(new opfunc<int, double>)},\
{{op, Real, Int}, std::shared_ptr<MathOperation>(new opfunc<double, int>)},\
{{op, Real, Real}, std::shared_ptr<MathOperation>(new opfunc<double, double>)},\
{{op, Logic, Real}, std::shared_ptr<MathOperation>(new opfunc<bool, double>)},\
{{op, Real, Logic}, std::shared_ptr<MathOperation>(new opfunc<double, bool>)},\
{{op, Logic, Int}, std::shared_ptr<MathOperation>(new opfunc<bool, int>)},\
{{op, Int, Logic}, std::shared_ptr<MathOperation>(new opfunc<int, bool>)},\
{{op, Logic, Logic}, std::shared_ptr<MathOperation>(new opfunc<bool, bool>)},

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
    operator int() const;
    operator double() const { CheckIs(Real); return real_; }
    operator bool() const;

	ValueType GetType();

  private:
	ValueType type_;
    std::string str_;
    int integral_ = 0;
    double real_ = 0.0;
    bool logic_ = false;

    void CheckIs(ValueType type) const;
};

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
	VariableOperation(const VariableName& name, int pos, int line): name_(name), pos_(pos), line_(line) {}

	void Do(Context& context) const final;

	private:
	const VariableName name_;
	int pos_;
	int line_;
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

struct UnaryMinusOperation : Operation {
  void Do(Context& context) const final;
};

struct NotOperation : Operation {
  	void Do(Context& context) const final;
};

struct GetRangeOperation : Operation {
    GetRangeOperation(int pos, int line): pos_(pos), line_(line) {}
    void Do(Context& context) const final;   
 private:
    int pos_;
    int line_;
};

// struct MathOperation : Operation {
//     void Do(Context& context) const final;

//     virtual StackValue DoMath(StackValue op1, StackValue op2) const = 0;
// };

struct MathOperation : Operation {
    void Do(Context& context) const final{}

    virtual StackValue DoMath(StackValue op1, StackValue op2) const = 0;
    StackValue NDo(Context& context, Lexeme::LexemeType type) const;
};

struct ExecuteOperation : Operation {
    ExecuteOperation(Lexeme::LexemeType type, int pos, int line): type_(type), pos_(pos), line_(line) {}
    void Do(Context& context) const final;
  private:
    Lexeme::LexemeType type_;
    int pos_;
    int line_;
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
struct EqualStrOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

template<typename T1, typename T2>
struct NotEqualOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

struct AndOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

struct OrOperation : MathOperation {
    StackValue DoMath(StackValue op1, StackValue op2) const final;
};

struct BoolCast : Operation {
    void Do(Context& context) const final;
};

struct IntCast : Operation {
    IntCast(int pos, int line): pos_(pos), line_(line) {}
    void Do(Context& context) const final;
  private:
    int pos_;
    int line_;
};

struct FloatCast : Operation {
    FloatCast(int pos, int line): pos_(pos), line_(line) {}
    void Do(Context& context) const final;
  private:
    int pos_;
    int line_;
};

struct StrCast : Operation {
    void Do(Context& context) const final;
};

struct Cast : Operation {
    Cast(Lexeme::LexemeType cast_type, int pos, int line): cast_type_(cast_type), pos_(pos), line_(line) {}
    void Do(Context& context) const final;
 private:
    Lexeme::LexemeType cast_type_;
    int pos_;
    int line_;
};

struct PrintOperation : Operation {
    void Do(Context& context) const final;
};

using Operations = std::vector<std::shared_ptr<Operation>>;

using OperationType = Lexeme::LexemeType;

using BinaryKey = std::tuple<OperationType, ValueType, ValueType>;

static const std::map<BinaryKey, std::shared_ptr<MathOperation>> kMathBinaries {
    {{Lexeme::Or, Int, Int}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Int, Real}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Real, Int}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Real, Real}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Logic, Real}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Real, Logic}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Logic, Int}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Int, Logic}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Logic, Logic}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Str, Str}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Logic, Str}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Int, Str}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Real, Str}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Str, Logic}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Str, Real}, std::shared_ptr<MathOperation>(new OrOperation)},
    {{Lexeme::Or, Str, Int}, std::shared_ptr<MathOperation>(new OrOperation)},

    {{Lexeme::And, Int, Int}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Int, Real}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Real, Int}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Real, Real}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Logic, Real}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Real, Logic}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Logic, Int}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Int, Logic}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Logic, Logic}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Str, Str}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Logic, Str}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Int, Str}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Real, Str}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Str, Logic}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Str, Real}, std::shared_ptr<MathOperation>(new AndOperation)},
    {{Lexeme::And, Str, Int}, std::shared_ptr<MathOperation>(new AndOperation)},

    {{Lexeme::Add, Str, Str}, std::shared_ptr<MathOperation>(new PlusOperation<std::string, std::string>)},
	{{Lexeme::Mul, Str, Int}, std::shared_ptr<MathOperation>(new MulStrROperation<std::string, int>)},
	{{Lexeme::Mul, Int, Str}, std::shared_ptr<MathOperation>(new MulStrLOperation<int, std::string>)},

    {{Lexeme::Equal, Str, Int}, std::shared_ptr<MathOperation>(new EqualStrOperation<std::string, int>)},
	{{Lexeme::Equal, Int, Str}, std::shared_ptr<MathOperation>(new EqualStrOperation<int, std::string>)},
    {{Lexeme::Equal, Str, Real}, std::shared_ptr<MathOperation>(new EqualStrOperation<std::string, double>)},
	{{Lexeme::Equal, Real, Str}, std::shared_ptr<MathOperation>(new EqualStrOperation<double, std::string>)},
    {{Lexeme::Equal, Str, Logic}, std::shared_ptr<MathOperation>(new EqualStrOperation<std::string, bool>)},
	{{Lexeme::Equal, Logic, Str}, std::shared_ptr<MathOperation>(new EqualStrOperation<bool, std::string>)},

	NumOperation(Lexeme::Add, PlusOperation)
	NumOperation(Lexeme::Sub, MinusOperation)
	NumOperation(Lexeme::Mul, MulOperation)
	NumOperation(Lexeme::Div, DivOperation)

	{{Lexeme::Mod, Int, Int}, std::shared_ptr<MathOperation>(new ModOperation<int, int>)},
	{{Lexeme::Mod, Logic, Int}, std::shared_ptr<MathOperation>(new ModOperation<bool, int>)},
	{{Lexeme::Mod, Int, Logic}, std::shared_ptr<MathOperation>(new ModOperation<int, bool>)},
	{{Lexeme::Mod, Logic, Logic}, std::shared_ptr<MathOperation>(new ModOperation<bool, bool>)},

	CompOperation(Lexeme::Less, LessOperation)
	CompOperation(Lexeme::Greater, GreaterOperation)
	CompOperation(Lexeme::LessEq, LessEqOperation)
	CompOperation(Lexeme::GreaterEq, GreaterEqOperation)
	CompOperation(Lexeme::Equal, EqualOperation)
  	CompOperation(Lexeme::NotEqual, NotEqualOperation)
};

} // namespace execution