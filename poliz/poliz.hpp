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

#include "../lexer/lexer.hpp"

namespace execution {

enum ValueType { Str, Int, Real, Logic };

const std::string& ToStringSem(ValueType type);

class PolymorphicValue {
  public:
	PolymorphicValue(const char* str);
	PolymorphicValue(const std::string& str);
	PolymorphicValue(int integral);
	PolymorphicValue(double real);
	PolymorphicValue(bool logic);

	operator std::string() const;
	operator int() const;
	operator double() const;
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

	Variable(const VariableName& name, const PolymorphicValue& value);
};

class StackValue {
  public:
	StackValue(Variable* variable);
	StackValue(PolymorphicValue value);

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
};

struct Operation {
	virtual ~Operation();
	virtual void Do(Context& context) const = 0;
};

struct ValueOperation : Operation {
	ValueOperation(std::string value, Lexeme::LexemeType type, int pos, int line);

	void Do(Context& context) const final;

  private:
	std::string value_;
	Lexeme::LexemeType type_;
	int pos_;
	int line_;
};

struct VariableOperation : Operation {
	VariableOperation(const VariableName& name, int pos, int line);

	void Do(Context& context) const final;

	private:
	const VariableName name_;
	int pos_;
	int line_;
};

struct AssignOperation : Operation {
	AssignOperation(const VariableName& name);

	void Do(Context& context) const final;

  private:
	const VariableName name_;
};

struct AddOneOperation : Operation {
	AddOneOperation(const VariableName& name);
	void Do(Context& context) const final;

  private:
	const VariableName name_;
};

struct GoOperation : Operation {
	GoOperation(OperationIndex index);

	void Do(Context& context) const override;

	private:
	const OperationIndex index_;
};

struct IfOperation : GoOperation {
	IfOperation(OperationIndex index);

	void Do(Context& context) const final;
};

struct UnaryMinusOperation : Operation {
	UnaryMinusOperation(int pos, int line);
	void Do(Context& context) const final;
  private:
	int pos_;
	int line_;
};

struct NotOperation : Operation {
	void Do(Context& context) const final;
};

struct GetRangeOperation : Operation {
	GetRangeOperation(int pos, int line);
	void Do(Context& context) const final;
 private:
	int pos_;
	int line_;
};

struct MathOperation : Operation {
	void Do(Context& context) const final;
	virtual StackValue DoMath(StackValue op1, StackValue op2) const = 0;
	StackValue NDo(Context& context, Lexeme::LexemeType type) const;
};

struct ExecuteOperation : Operation {
	ExecuteOperation(Lexeme::LexemeType type, int pos, int line);
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
	IntCast(int pos, int line);
	void Do(Context& context) const final;
  private:
	int pos_;
	int line_;
};

struct FloatCast : Operation {
	FloatCast(int pos, int line);
	void Do(Context& context) const final;
  private:
	int pos_;
	int line_;
};

struct StrCast : Operation {
	void Do(Context& context) const final;
};

struct Cast : Operation {
	Cast(Lexeme::LexemeType cast_type, int pos, int line);
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

} // namespace execution