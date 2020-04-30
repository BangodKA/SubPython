#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

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
	std::stack<ValueType> operand_types;
	std::stack<const execution::OperationIndex> loop_starts;
	std::stack<const execution::OperationIndex> breaks;
	std::unordered_map<VariableName, ValueType> var_types;

	std::tuple<ValueType, ValueType> PrepOperation();

	void PostOp(std::stack<ValueType> &operand_types, ValueType op1, ValueType op2 = Logic);

	std::string TypeToString(Lexeme::LexemeType type) const;

	void CheckLexeme(Lexeme::LexemeType type);

	int ProcessLoop(const execution::OperationIndex label_if, const execution::OperationIndex if_index);

	void ProcessUnaryTypeExceptions(ValueType op, Lexeme::LexemeType type);
	void ProcessBinaryTypeExceptions(ValueType op1, ValueType op2, Lexeme::LexemeType type);

	std::tuple<ValueType, ValueType> AddBinaryOperation(Lexeme::LexemeType type);
	ValueType AddUnaryOperation(Lexeme::LexemeType type);

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

void Parser::CheckLexeme(Lexeme::LexemeType type) {
	if (!lexer_.HasLexeme()) {
		throw std::runtime_error(
			"line " + std::to_string(Lexer::line) + ": SyntaxError: unexpected EOF while parsing");
	}

	if (lexer_.PeekLexeme().type != type) {
		throw std::runtime_error(
			std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": SyntaxError: invalid syntax");
	}
}

int Parser::ProcessLoop(const execution::OperationIndex label_if, const execution::OperationIndex if_index) {
	lexer_.TakeLexeme();
	int breaks_amount = breaks.size();

	int next_block_indent = InnerBlock();
	loop_starts.pop();

	operations.emplace_back(new execution::GoOperation(label_if));

	const execution::OperationIndex label_next = operations.size();
	

	while (breaks.size() != breaks_amount) {
		operations[breaks.top()].reset(new execution::GoOperation(label_next));
		breaks.pop();
	}

	operations[if_index].reset(new execution::IfOperation(label_next));

	return next_block_indent;
}

Parser::Parser(std::istream& input): lexer_(input), indents({0}) {}

std::string Parser::TypeToString(Lexeme::LexemeType type) const {
	std::ostringstream oss;
	oss << type;
	return oss.str();
}

std::tuple<ValueType, ValueType> Parser::PrepOperation() {
	ValueType op2 = operand_types.top();
	operand_types.pop();
	ValueType op1 = operand_types.top();
	operand_types.pop();

	return std::make_tuple(op2, op1);
}

void Parser::PostOp(std::stack<ValueType> &operand_types, ValueType op1, ValueType op2) {
	if ((op1 == Str) || (op2 == Str)) {
		operand_types.emplace(Str);
	}
	else if ((op1 == Real) || (op2 == Real)) {
		operand_types.emplace(Real);
	}
	else if ((op1 == Int) || (op2 == Int)){
		operand_types.emplace(Int);
	}
	else {
		operand_types.emplace(Logic);
	}
}

void Parser::Run() {
	while (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::EOL) {
		lexer_.TakeLexeme();
	}
	if (!lexer_.HasLexeme()) {
		return;
	}
	if (IndentCounter() != 0) {
		throw std::runtime_error(
				"line " + std::to_string(Lexer::line) + ":" + 
				std::to_string(Lexer::pos - lexer_.PeekLexeme().value.length()) + ": IndentationError: unexpected indent");
	}
	int next_block_indent = 0;
	while (lexer_.HasLexeme()) {
		if (next_block_indent != 0) {
			throw std::runtime_error(
				"line " + std::to_string(Lexer::line) + ":" + 
				std::to_string(Lexer::pos - lexer_.PeekLexeme().value.length()) + ": IndentationError: unexpected indent");
		}
		if (lexer_.PeekLexeme().type == Lexeme::EOL) {
			lexer_.TakeLexeme();
		}
		else {
			next_block_indent = Block();
		}
	}
}

// std::tuple<ValueType, ValueType> Parser::AddBinaryOperation(Lexeme::LexemeType type) {
// 	std::tuple<ValueType, ValueType> ops = PrepOperation();
// 	try {
// 		operations.emplace_back(execution::kBinaries.at(std::make_tuple(type, std::get<1>(ops), std::get<0>(ops))));
// 	}
// 	catch (std::out_of_range& e) {
// 		ProcessBinaryTypeExceptions(std::get<1>(ops), std::get<0>(ops), type);
// 	}

// 	return ops;
// }

ValueType Parser::AddUnaryOperation(Lexeme::LexemeType type) {
	ValueType op = operand_types.top();
	operand_types.pop();
	try {
		operations.emplace_back(kUnaries.at(std::make_tuple(type, op)));
	}
	catch (std::out_of_range) {
		ProcessUnaryTypeExceptions(op, type);
	}
	return op;
}

void Parser::ProcessUnaryTypeExceptions(ValueType op, Lexeme::LexemeType type) {
	std::string exception_mes = "";
	std::string unmin = "";
	if (type == Lexeme::UnaryMinus) {
		unmin = "unary ";
	}
	exception_mes = "line " + std::to_string(Lexer::line) + 
				": TypeError: unsupported operand type(s) for " + unmin + TypeToString(type) + ": " + ToStringSem(op);
	operations.emplace_back(new execution::ThrowCustomException(exception_mes));
}

void Parser::ProcessBinaryTypeExceptions(ValueType op1, ValueType op2, Lexeme::LexemeType type) {
	std::string exception_mes = "";
	exception_mes = "line " + std::to_string(Lexer::line) + 
				": TypeError: unsupported operand type(s) for " + 
				TypeToString(type) + ": " + ToStringSem(op1) + " and " + ToStringSem(op2);
	operations.emplace_back(new execution::ThrowCustomException(exception_mes));
}

int Parser::IndentCounter() {
	int indent = 0;
	while (lexer_.HasLexeme()) {
		if (lexer_.PeekLexeme().type == Lexeme::IndentSpace) {
			lexer_.TakeLexeme();
			indent++;
			continue;
		}
		if (lexer_.PeekLexeme().type == Lexeme::EOL) {
			lexer_.TakeLexeme();
			indent = 0;
			continue;
		}
		return indent;
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
				std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": SyntaxError: invalid syntax");
		default:
			break;
	}

	if (indents.top() == 0 && next_block_indent != 0) {
		throw std::runtime_error(
			"line " + std::to_string(Lexer::line) + ":" + 
			std::to_string(Lexer::pos) + ": IndentationError: unexpected indent");
	}
	return next_block_indent;
}

void Parser::CheckBreak() {
	if (!loop_starts.empty()) {
		const execution::OperationIndex break_index = operations.size();
		operations.emplace_back(nullptr);
		breaks.emplace(break_index);
		lexer_.TakeLexeme();
	}
	else {
		throw std::runtime_error("SyntaxError: 'break' outside loop");
	}
}

void Parser::CheckContinue() {
	if (!loop_starts.empty()) {
		operations.emplace_back(new execution::GoOperation(loop_starts.top()));
		lexer_.TakeLexeme();
	}
	else {
		throw std::runtime_error("SyntaxError: 'continue' outside loop");
	}
}

int Parser::ExprParts() {
	switch (lexer_.PeekLexeme().type) {
		case Lexeme::Print:
			Print();
			break;
		case Lexeme::Identifier:
			Assign();
			break;
		case Lexeme::Continue:
			CheckContinue();
			break;
		case Lexeme::Break:
			CheckBreak();
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
		"line " + std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) +
		 ": SyntaxError: invalid syntax");
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

	// AddBinaryOperation(Lexeme::Less);
	operations.emplace_back(new execution::ExecuteOperation(Lexeme::Less));


	const execution::OperationIndex if_index = operations.size();
	operations.emplace_back(nullptr);

	CheckLexeme(Lexeme::Colon);
	return ProcessLoop(label_if, if_index);
}

const execution::OperationIndex Parser::PrepForLoopParams(Lexeme lex) {
	operations.emplace_back(new execution::AssignOperation(lex.value));
	var_types.insert_or_assign(lex.value, operand_types.top());

	const execution::OperationIndex go_index = operations.size();
	operations.emplace_back(nullptr);

	const execution::OperationIndex label_if = operations.size();
	loop_starts.emplace(label_if);

	operations.emplace_back(new execution::VariableOperation(lex.value, Lexer::pos, Lexer::line));
	operations.emplace_back(new execution::AddOneOperation(lex.value));

	const execution::OperationIndex label_new = operations.size();
	operations[go_index].reset(new execution::GoOperation(label_new));

	operations.emplace_back(new execution::VariableOperation(lex.value, Lexer::pos, Lexer::line));
	operations.emplace_back(new execution::VariableOperation(
		"edge" + std::to_string(loop_starts.size() - 1), Lexer::pos, Lexer::line));

	operand_types.emplace(Int);
	operand_types.emplace(Int);

	return label_if;
}

void Parser::Range() {
	CheckLexeme(Lexeme::Range);
	lexer_.TakeLexeme();
	CheckLexeme(Lexeme::LeftParenthesis);
	lexer_.TakeLexeme();
	Interval();

	operations.emplace_back(new execution::GetRangeOperation(Lexer::pos, Lexer::line));
	// std::tuple<ValueType, ValueType> ops = AddBinaryOperation(Lexeme::Range);
	
	// operand_types.emplace(std::get<1>(ops));
	// operand_types.emplace(std::get<0>(ops));
	
	std::string edge = "edge" + std::to_string(loop_starts.size());	
	operations.emplace_back(new execution::AssignOperation(edge));
	var_types.insert_or_assign(edge, operand_types.top());

	CheckLexeme(Lexeme::RightParenthesis);
	lexer_.TakeLexeme();
}

void Parser::Interval() {
	if (lexer_.HasLexeme()) {
		Expression();
		if (lexer_.HasLexeme()) {
			if (lexer_.PeekLexeme().type != Lexeme::Comma) {
				operand_types.emplace(Int);
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
		std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ":  SyntaxError: unexpected EOF while parsing");
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
		"line " + std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": SyntaxError: unexpected EOF while parsing");
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
		throw std::runtime_error("line " + std::to_string(Lexer::line) + ":" + 
				std::to_string(Lexer::pos - lexer_.PeekLexeme().value.length()) + 
				": IndentationError: unexpected indent");
	}
	throw std::runtime_error("line " + std::to_string(Lexer::line) + ":" + 
			std::to_string(Lexer::pos) + ": IndentationError: expected an indented block");
}

void Parser::Print() {
	lexer_.TakeLexeme();
	if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::LeftParenthesis) {
		lexer_.TakeLexeme();
		if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::RightParenthesis) {
			lexer_.TakeLexeme();
			operations.emplace_back(new execution::ValueOperation(""));
				// operations.emplace_back(kUnaries.at(std::make_tuple(Lexeme::Print, Str)));
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

	throw std::runtime_error(std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": SyntaxError: invalid syntax");
}

int Parser::GatherPrints() {
	int comma_cnt = 0;
	while (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::Comma) {
		comma_cnt++;
		lexer_.TakeLexeme();

		// AddUnaryOperation(Lexeme::Str);
		operations.emplace_back(new execution::StrCast());

		operations.emplace_back(new execution::ValueOperation(" "));
		Expression();
	}
	return comma_cnt;
}

void Parser::PrintGathered(int comma_cnt) {
	lexer_.TakeLexeme();
	operand_types.pop();
	if (comma_cnt) {
		operations.emplace_back(new execution::StrCast());
	}

	for (int i = 0; i < comma_cnt * 2; ++i) {
		operations.emplace_back(new execution::ExecuteOperation(Lexeme::Add));
		// operations.emplace_back(execution::kBinaries.at(std::make_tuple(Lexeme::Add, Str, Str)));
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
			var_types.insert_or_assign(lex.value, operand_types.top());
			return;
		} 
		if (lexer_.PeekLexeme().type == Lexeme::EOL) {
			return;
		} 
		throw std::runtime_error(
			std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": " + "expected =");
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

		// AddBinaryOperation(Lexeme::Or);
		operations.emplace_back(new execution::ExecuteOperation(Lexeme::Or));
		// operations.emplace_back(execution::kNewBinaries.at(std::make_tuple(Lexeme::Or, std::get<1>(ops), std::get<0>(ops)))());

		
		operand_types.emplace(Logic);
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

		// AddBinaryOperation(Lexeme::And);
		operations.emplace_back(new execution::ExecuteOperation(Lexeme::And));
		
		operand_types.emplace(Logic);
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

		// AddUnaryOperation(Lexeme::Not);
		operations.emplace_back(new execution::NotOperation);
	
		PostOp(operand_types, Logic);
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

		// AddBinaryOperation(op_type);
		operations.emplace_back(new execution::ExecuteOperation(op_type));
		
		operand_types.emplace(Logic);
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

		// std::tuple<ValueType, ValueType> ops = AddBinaryOperation(op_type);
		operations.emplace_back(new execution::ExecuteOperation(op_type));

		// PostOp(operand_types, std::get<1>(ops), std::get<0>(ops));
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

		// std::tuple<ValueType, ValueType> ops = AddBinaryOperation(op_type);
		operations.emplace_back(new execution::ExecuteOperation(op_type));


		// PostOp(operand_types, std::get<1>(ops), std::get<0>(ops));
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

		ValueType op = AddUnaryOperation(Lexeme::UnaryMinus);
	
		PostOp(operand_types, op);
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
				operand_types.pop();

				operations.emplace_back(new execution::Cast(cast_type));
				
				// operations.emplace_back(kUnaries.at(std::make_tuple(cast_type, op)));
				switch (cast_type) {
					case Lexeme::Bool:
						operand_types.emplace(Logic);
						break;
					case Lexeme::Str:
						operand_types.emplace(Str);
						break;
					case Lexeme::Int:
						operand_types.emplace(Int);
						break;
					case Lexeme::Float:
						operand_types.emplace(Real);
						break;
					default:
						break;
				}
			}
		}
		else {
			Members();
		}
	}
	else {
		throw std::runtime_error("line " + std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) 
		+ ": SyntaxError: unexpected EOF while parsing");
	}
}

void Parser::Members() {
	if (!lexer_.HasLexeme()) {
		throw std::runtime_error("line " + std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) 
			+ ": SyntaxError: unexpected EOF while parsing");
	}

	const Lexeme lexeme = lexer_.TakeLexeme();

	switch (lexeme.type) {
		case Lexeme::Identifier:
			operand_types.emplace(var_types[lexeme.value]);
			operations.emplace_back(new execution::VariableOperation(lexeme.value, Lexer::pos, Lexer::line));
			return;
		case Lexeme::IntegerConst:
			operand_types.emplace(Int);
			operations.emplace_back(new execution::ValueOperation(int(lexeme)));
			return;
		case Lexeme::BoolConst:
			operand_types.emplace(Logic);
			operations.emplace_back(new execution::ValueOperation(bool(lexeme)));
			return;
		case Lexeme::FloatConst:
			operand_types.emplace(Real);
			operations.emplace_back(new execution::ValueOperation(double(lexeme)));
			return;
		case Lexeme::StringConst:
			operand_types.emplace(Str);
			operations.emplace_back(new execution::ValueOperation(std::string(lexeme)));
			return;
		default:
			break;
	}

	if (lexeme.type != Lexeme::LeftParenthesis) {
		throw std::runtime_error(
			std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": SyntaxError: invalid syntax");
	}

	Expression();

	CheckLexeme(Lexeme::RightParenthesis);

	lexer_.TakeLexeme();
}
