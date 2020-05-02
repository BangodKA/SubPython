#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "../lexer/lexer.hpp"
#include "../poliz/poliz.hpp"
#include "parser.hpp"

using namespace execution;

Parser::Parser(std::istream& input): lexer_(input), indents({0}) {}

void Parser::Run() {
	while (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::EOL) {
		lexer_.TakeLexeme();
	}
	if (!lexer_.HasLexeme()) {
		return;
	}
	if (IndentCounter() != 0) {
		throw std::runtime_error(
				"line " + std::to_string(lexer_.GetLine()) + ":" + 
				std::to_string(lexer_.GetPos() - lexer_.PeekLexeme().value.length()) + ": IndentationError: unexpected indent");
	}
	int next_block_indent = 0;
	while (lexer_.HasLexeme()) {
		if (next_block_indent != 0) {
			throw std::runtime_error(
				"line " + std::to_string(lexer_.GetLine()) + ":" + 
				std::to_string(lexer_.GetPos() - lexer_.PeekLexeme().value.length()) + ": IndentationError: unexpected indent");
		}
		if (lexer_.PeekLexeme().type == Lexeme::EOL) {
			lexer_.TakeLexeme();
		}
		else {
			next_block_indent = Block();
		}
	}
}

void Parser::CheckLexeme(Lexeme::LexemeType type) {
	if (!lexer_.HasLexeme()) {
		throw std::runtime_error(
			"line " + std::to_string(lexer_.GetLine()) + ": SyntaxError: unexpected EOF while parsing");
	}

	if (lexer_.PeekLexeme().type != type) {
		throw std::runtime_error(
			std::to_string(lexer_.GetLine()) + ":" + std::to_string(lexer_.GetPos()) + ": SyntaxError: invalid syntax");
	}
}

int Parser::ProcessLoop(const execution::OperationIndex label_if, const execution::OperationIndex if_index, 
										bool for_loop, std::string value) {
	lexer_.TakeLexeme();
	int breaks_amount = breaks.size();
	int continues_amount = continues.size();

	int next_block_indent = InnerBlock();
	loop_starts.pop();

	const execution::OperationIndex label_cont = operations.size();

	while (continues.size() != continues_amount) {
		operations[continues.top()].reset(new execution::GoOperation(label_cont));
		continues.pop();
	}

	if (for_loop) {
		operations.emplace_back(new execution::VariableOperation(value, lexer_.GetPos(), lexer_.GetLine()));
		operations.emplace_back(new execution::AddOneOperation(value));
	}

	operations.emplace_back(new execution::GoOperation(label_if));

	const execution::OperationIndex label_next = operations.size();
	

	while (breaks.size() != breaks_amount) {
		operations[breaks.top()].reset(new execution::GoOperation(label_next));
		breaks.pop();
	}

	operations[if_index].reset(new execution::IfOperation(label_next));

	return next_block_indent;
}

int Parser::IndentCounter() {
	if (lexer_.HasLexeme()) {
		if (lexer_.PeekLexeme().type == Lexeme::IndentSpace) {
			Lexeme lex = lexer_.TakeLexeme();
			return lex.indent_amount;
		}
	}
	return 0;
}

int Parser::BlockParts() {
	int next_block_indent = 0;
	const Lexeme lex = lexer_.PeekLexeme();
	lexer_.TakeLexeme();
	switch (lex.type) {
		case Lexeme::For:
			next_block_indent = ForBlock();
			break;
		case Lexeme::While:
			next_block_indent = WhileBlock();
			break;
		case Lexeme::If:
			next_block_indent = IfBlock();
			break;
		case Lexeme::Else:
		case Lexeme::ElIf:
			throw std::runtime_error(
				std::to_string(lexer_.GetLine()) + ":" + std::to_string(lexer_.GetPos()) + ": SyntaxError: invalid syntax");
		default:
			break;
	}

	if (indents.top() == 0 && next_block_indent != 0) {
		throw std::runtime_error(
			"line " + std::to_string(lexer_.GetLine()) + ":" + 
			std::to_string(lexer_.GetPos()) + ": IndentationError: unexpected indent");
	}
	return next_block_indent;
}

void Parser::CheckBreakContinue(Lexeme::LexemeType shift) {
	if (!loop_starts.empty()) {
		const execution::OperationIndex shift_index = operations.size();
		operations.emplace_back(nullptr);
		if (shift == Lexeme::Continue) {
			continues.emplace(shift_index);
		}
		else {
			breaks.emplace(shift_index);
		}
		lexer_.TakeLexeme();
	}
	else {
		throw std::runtime_error("line " + std::to_string(lexer_.GetLine()) + 
								": SyntaxError: '" + Lexeme::TypeToString(shift) + "' outside loop");
	}
}

int Parser::ExprParts() {
	Lexeme lex = lexer_.PeekLexeme();
	switch (lex.type) {
		case Lexeme::Print:
			Print();
			break;
		case Lexeme::Identifier:
			Assign();
			break;
		case Lexeme::Continue:
		case Lexeme::Break:
			CheckBreakContinue(lex.type);
			break;
		default:
			Expression();
			break;
	}
	if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::EOL) {
		lexer_.TakeLexeme();
		return IndentCounter();
	}
	if (!lexer_.HasLexeme()) {
		return 0;
	}
	throw std::runtime_error (
		"line " + std::to_string(lexer_.GetLine()) + ":" + std::to_string(lexer_.GetPos()) +
		 ": SyntaxError5: invalid syntax");
}

int Parser::Block() {
	const Lexeme lex = lexer_.PeekLexeme();
	if (lex.type == Lexeme::For || lex.type == Lexeme::If || 
		lex.type == Lexeme::While || lex.type == Lexeme::ElIf ||
		lex.type == Lexeme::Else) {
		return BlockParts();
	}
	return ExprParts();
}

// For

int Parser::ForBlock() {
	CheckLexeme(Lexeme::Identifier);
	Lexeme lex = lexer_.TakeLexeme();
	
	CheckLexeme(Lexeme::In);
	lexer_.TakeLexeme();

	Range();

	const execution::OperationIndex label_if = PrepForLoopParams(lex);

	operations.emplace_back(new execution::ExecuteOperation(Lexeme::Less, lexer_.GetPos(), lexer_.GetLine()));


	const execution::OperationIndex if_index = operations.size();
	operations.emplace_back(nullptr);

	CheckLexeme(Lexeme::Colon);
	bool for_loop = true;
	return ProcessLoop(label_if, if_index, for_loop, lex.value);
}

const execution::OperationIndex Parser::PrepForLoopParams(Lexeme lex) {
	operations.emplace_back(new execution::AssignOperation(lex.value));

	const execution::OperationIndex label_if = operations.size();
	loop_starts.emplace(label_if);

	operations.emplace_back(new execution::VariableOperation(lex.value, lexer_.GetPos(), lexer_.GetLine()));
	operations.emplace_back(new execution::VariableOperation(
		"edge" + std::to_string(loop_starts.size() - 1), lexer_.GetPos(), lexer_.GetLine()));

	return label_if;
}

void Parser::Range() {
	CheckLexeme(Lexeme::Range);
	lexer_.TakeLexeme();
	CheckLexeme(Lexeme::LeftParenthesis);
	lexer_.TakeLexeme();
	Interval();

	operations.emplace_back(new execution::GetRangeOperation(lexer_.GetPos(), lexer_.GetLine()));
	
	std::string edge = "edge" + std::to_string(loop_starts.size());	
	operations.emplace_back(new execution::AssignOperation(edge));

	CheckLexeme(Lexeme::RightParenthesis);
	lexer_.TakeLexeme();
}

void Parser::Interval() {
	if (lexer_.HasLexeme()) {
		Expression();
		if (lexer_.HasLexeme()) {
			if (lexer_.PeekLexeme().type != Lexeme::Comma) {
				return;
			}
			Lexeme lex = lexer_.TakeLexeme();
			if (lexer_.HasLexeme()) {
				Expression();
				return;
			}
		}
	}

	throw std::runtime_error(
		std::to_string(lexer_.GetLine()) + ":" + std::to_string(lexer_.GetPos()) + ":  SyntaxError: unexpected EOF while parsing");
}

// If

int Parser::IfBlock() {
	Expression();

	const execution::OperationIndex if_index = operations.size();
	operations.emplace_back(nullptr);
	CheckLexeme(Lexeme::Colon);
	lexer_.TakeLexeme();
	int next_block_indent = InnerBlock();

	const execution::OperationIndex truth_index = operations.size();
	operations.emplace_back(nullptr);

	const execution::OperationIndex label = operations.size();
	operations[if_index].reset(new execution::IfOperation(label));

	if (next_block_indent == indents.top()) {

		if (lexer_.HasLexeme() && 
			(lexer_.PeekLexeme().type == Lexeme::ElIf || lexer_.PeekLexeme().type == Lexeme::Else)) {
			Lexeme lex = lexer_.TakeLexeme();
		
			switch (lex.type) {
				case Lexeme::Else:
					next_block_indent = Parser::ElseBlock();
					break;
				case Lexeme::ElIf:
					next_block_indent = Parser::IfBlock();
					break;
				default:
					break;
			}
		}
	}
	const execution::OperationIndex label_n = operations.size();
	operations[truth_index].reset(new execution::GoOperation(label_n));
	return next_block_indent;
}

int Parser::ElseBlock() {
	CheckLexeme(Lexeme::Colon);
	lexer_.TakeLexeme();
	int next_block_indent = InnerBlock();
	return next_block_indent;

}

// While
int Parser::WhileBlock() {
	const execution::OperationIndex label_if = operations.size();
	loop_starts.emplace(label_if);

	Expression();

	const execution::OperationIndex if_index = operations.size();
	operations.emplace_back(nullptr);

	CheckLexeme(Lexeme::Colon);

	return ProcessLoop(label_if, if_index);
}



int Parser::InnerBlock() {
	if (lexer_.HasLexeme()) {
		if (lexer_.PeekLexeme().type != Lexeme::EOL) {
			return ExprParts();
		}

		while (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::EOL) {
			lexer_.TakeLexeme();
		}
		int indent = IndentCounter();

		if (lexer_.HasLexeme()) {
			return ProcessInnerBlock(indent);
		}
	}
	throw std::runtime_error(
		"line " + std::to_string(lexer_.GetLine()) + ":" + std::to_string(lexer_.GetPos()) + ": SyntaxError: unexpected EOF while parsing");
}

int Parser::ProcessInnerBlock(int indent) {
	if (indent > indents.top()) {
		indents.push(indent);

		int next_block_indent = Block();

		while (next_block_indent == indent && lexer_.HasLexeme()) {
			next_block_indent = Block();
		}

		if (next_block_indent < indent) {
			indents.pop();
			return next_block_indent;
		}
		throw std::runtime_error("line " + std::to_string(lexer_.GetLine()) + ":" + 
				std::to_string(lexer_.GetPos() - lexer_.PeekLexeme().value.length()) + 
				": IndentationError: unexpected indent");
	}
	throw std::runtime_error("line " + std::to_string(lexer_.GetLine()) + ":" + 
			std::to_string(lexer_.GetPos()) + ": IndentationError: expected an indented block");
}

void Parser::Print() {
	lexer_.TakeLexeme();
	if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::LeftParenthesis) {
		lexer_.TakeLexeme();
		if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::RightParenthesis) {
			lexer_.TakeLexeme();
			operations.emplace_back(new execution::ValueOperation("", Lexeme::StringConst, lexer_.GetPos(), lexer_.GetLine()));

				operations.emplace_back(new execution::PrintOperation());
			return;
		}
		Expression();
		int comma_cnt = GatherPrints();

		if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::RightParenthesis) {

			PrintGathered(comma_cnt);
			
			return;
		}
	}

	throw std::runtime_error(std::to_string(lexer_.GetLine()) + ":" + std::to_string(lexer_.GetPos()) + ": SyntaxError: invalid syntax");
}

int Parser::GatherPrints() {
	int comma_cnt = 0;
	while (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::Comma) {
		comma_cnt++;
		lexer_.TakeLexeme();

		operations.emplace_back(new execution::StrCast());

		operations.emplace_back(new execution::ValueOperation(" ", Lexeme::StringConst, lexer_.GetPos(), lexer_.GetLine()));
		Expression();
	}
	return comma_cnt;
}

void Parser::PrintGathered(int comma_cnt) {
	lexer_.TakeLexeme();
	if (comma_cnt) {
		operations.emplace_back(new execution::StrCast());
	}

	for (int i = 0; i < comma_cnt * 2; ++i) {
		operations.emplace_back(new execution::ExecuteOperation(Lexeme::Add, lexer_.GetPos(), lexer_.GetLine()));
	}

	operations.emplace_back(new execution::PrintOperation());
}

void Parser::Assign() {
	Lexeme lex = lexer_.TakeLexeme();
	if (lexer_.HasLexeme()) {
		if (lexer_.PeekLexeme().type == Lexeme::Assign) {
			lexer_.TakeLexeme();
			Expression();
			operations.emplace_back(new execution::AssignOperation(lex.value));
			return;
		} 
		if (lexer_.PeekLexeme().type == Lexeme::EOL) {
			return;
		} 
		throw std::runtime_error(
			std::to_string(lexer_.GetLine()) + ":" + std::to_string(lexer_.GetPos()) + ": " + "expected =");
	} 
	return;
}

void Parser::Expression() {
	OrParts();
	while (lexer_.HasLexeme()) {
		if (lexer_.PeekLexeme().type != Lexeme::Or) {
			break;
		}
		lexer_.TakeLexeme();
		OrParts();

		operations.emplace_back(new execution::ExecuteOperation(Lexeme::Or, lexer_.GetPos(), lexer_.GetLine()));
	}
}

void Parser::OrParts() {
	AndParts();
	while (lexer_.HasLexeme()) {
		if (lexer_.PeekLexeme().type != Lexeme::And) {
			break;
		}
		lexer_.TakeLexeme();
		AndParts();

		operations.emplace_back(new execution::ExecuteOperation(Lexeme::And, lexer_.GetPos(), lexer_.GetLine()));
	}
}

void Parser::AndParts() {
	bool not_ = false;
	while (lexer_.HasLexeme()) {
		if (lexer_.PeekLexeme().type != Lexeme::Not) {
			break;
		}
		not_ ^= true;
		lexer_.TakeLexeme();
	}

	LogicalParts();

	if (not_) {
		operations.emplace_back(new execution::NotOperation);
	}
}

void Parser::LogicalParts() {
	CompParts();
	while (lexer_.HasLexeme()) {
		const Lexeme::LexemeType op_type = lexer_.PeekLexeme().type;
		if (op_type != Lexeme::Less && op_type != Lexeme::LessEq &&
			op_type != Lexeme::Greater && op_type != Lexeme::GreaterEq && 
			op_type != Lexeme::Equal && op_type != Lexeme::NotEqual) {
			break;
		}
		lexer_.TakeLexeme();
		CompParts();

		operations.emplace_back(new execution::ExecuteOperation(op_type, lexer_.GetPos(), lexer_.GetLine()));
	}
}

void Parser::CompParts() {
	SumParts();
	while (lexer_.HasLexeme()) {
		const Lexeme::LexemeType op_type = lexer_.PeekLexeme().type;
		if (op_type != Lexeme::Add && op_type != Lexeme::Sub) {
		break;
		}
		lexer_.TakeLexeme();
		SumParts();

		operations.emplace_back(new execution::ExecuteOperation(op_type, lexer_.GetPos(), lexer_.GetLine()));

	}
}

void Parser::SumParts() {
	MultParts();
	while (lexer_.HasLexeme()) {
		const Lexeme::LexemeType op_type = lexer_.PeekLexeme().type;
		if (op_type != Lexeme::Mul && op_type != Lexeme::Div &&
			op_type != Lexeme::Mod) {
			break;
		}
		lexer_.TakeLexeme();
		MultParts();

		operations.emplace_back(new execution::ExecuteOperation(op_type, lexer_.GetPos(), lexer_.GetLine()));
	}
}

void Parser::MultParts() {
	bool minus = false;
	while (lexer_.HasLexeme()) {
		const Lexeme::LexemeType sign_type = lexer_.PeekLexeme().type;
		if (sign_type != Lexeme::Add && sign_type != Lexeme::Sub) {
			break;
		}
		if (sign_type == Lexeme::Sub) {
			minus ^= true;
		}
		lexer_.TakeLexeme();
	}

	Cast();

	if (minus) {

		operations.emplace_back(new execution::UnaryMinusOperation(lexer_.GetPos(), lexer_.GetLine()));

	}
}

void Parser::Cast() {
	if (lexer_.HasLexeme()) {
		bool cast = false;
		const Lexeme::LexemeType cast_type = lexer_.PeekLexeme().type;
		if (cast_type == Lexeme::Bool || cast_type == Lexeme::Int ||
			cast_type == Lexeme::Str || cast_type == Lexeme::Float) {
			cast = true;

			lexer_.TakeLexeme();
			CheckLexeme(Lexeme::LeftParenthesis);
			lexer_.TakeLexeme();
			Expression();

			if (cast) {
				CheckLexeme(Lexeme::RightParenthesis);

				lexer_.TakeLexeme();

				operations.emplace_back(new execution::Cast(cast_type, lexer_.GetPos(), lexer_.GetLine()));
			}
		}
		else {
			Members();
		}
	}
	else {
		throw std::runtime_error("line " + std::to_string(lexer_.GetLine()) + ":" + std::to_string(lexer_.GetPos()) 
		+ ": SyntaxError: unexpected EOF while parsing");
	}
}

void Parser::Members() {
	if (!lexer_.HasLexeme()) {
		throw std::runtime_error("line " + std::to_string(lexer_.GetLine()) + ":" + std::to_string(lexer_.GetPos()) 
			+ ": SyntaxError: unexpected EOF while parsing");
	}

	const Lexeme lexeme = lexer_.TakeLexeme();

	switch (lexeme.type) {
		case Lexeme::Identifier:
			operations.emplace_back(new execution::VariableOperation(lexeme.value, lexer_.GetPos(), lexer_.GetLine()));
			return;
		case Lexeme::IntegerConst:
		case Lexeme::BoolConst:
		case Lexeme::FloatConst:
		case Lexeme::StringConst:
			operations.emplace_back(new ValueOperation(lexeme.value, lexeme.type, lexer_.GetPos(), lexer_.GetLine()));
			return;
		default:
			break;
	}

	if (lexeme.type != Lexeme::LeftParenthesis) {
		throw std::runtime_error(
			std::to_string(lexer_.GetLine()) + ":" + std::to_string(lexer_.GetPos()) + ": SyntaxError: invalid syntax");
	}

	Expression();

	CheckLexeme(Lexeme::RightParenthesis);

	lexer_.TakeLexeme();
}
