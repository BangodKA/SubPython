#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <stack>
#include <map>

// #include "../lexer/lexer.hpp"
#include "../poliz/poliz.hpp"

// struct BadOperand : public std::exception {
// 	BadOperand(std::string str) : str_(str) {}
// 	const char * what () const throw () {
// 		return (str_ + "\n").c_str();
// 	}
//   private:
// 	std::string str_;
// };

using namespace execution;

class Parser{
 public:
	explicit Parser(std::istream& input);
	Operations operations;
	void Run(Context& context);

 private:

	ValueType op2;
	ValueType op1;
 	
	Lexer lexer_;
	std::stack<int> indents;
	std::stack<OperationIndex> if_indices;
	std::stack<ValueType> operand_types;
	std::stack<Lexeme::LexemeType> type_cast;
	std::unordered_map<VariableName, ValueType> var_types;

	void PostOp(std::stack<ValueType> &operand_types, ValueType op1, ValueType op2 = Int);


	// void ReturnLexeme();

	int IndentCounter();

	int Block(Context& context);

	// For
	int ForBlock(Context& context);
	void Range(Context& context);
	void Interval(Context& context);

	// If
	int IfBlock(Context& context);
	int ElseBlock(Context& context);

	// While
	int WhileBlock(Context& context);
	
	int InnerBlock(Context& context);

	void Print(Context& context);

	void Assign(Context& context);

	void Expression(Context& context);
	void CompParts(Context& context);
	void SumParts(Context& context);
	void MultParts(Context& context);
	void Cast(Context& context);
	void Members(Context& context);
};