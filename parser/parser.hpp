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

// #include "../lexer/lexer.hpp" // todo -
#include "../poliz/poliz.hpp"

using namespace execution;

class Parser{
 public:
	explicit Parser(std::istream& input);
	Operations operations;
	void Run();

 private:

	Lexer lexer_;
	std::stack<int> indents;
	std::stack<const execution::OperationIndex> loop_starts;
	std::stack<const execution::OperationIndex> breaks;

	void CheckLexeme(Lexeme::LexemeType type);

	int ProcessLoop(const execution::OperationIndex label_if, const execution::OperationIndex if_index);

	int IndentCounter();

	int Block();
	int BlockParts();
	int ExprParts();

	// For
	int ForBlock();
	const execution::OperationIndex PrepForLoopParams(Lexeme lex);
	void Range();
	void Interval();

	// If
	int IfBlock();
	int ElseBlock();

	// While
	int WhileBlock();

	void CheckBreak();
	void CheckContinue();
	
	// Inner Parts
	int InnerBlock();
	int ProcessInnerBlock(int indent);

	void Print();
	int GatherPrints();
	void PrintGathered(int comma_cnt);

	void Assign();

	void Expression();
	void OrParts();
	void AndParts();
	void LogicalParts();
	void CompParts();
	void SumParts();
	void MultParts();
	void Cast();
	void Members();
};