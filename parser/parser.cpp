#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <stack>
#include <map>
#include <sstream>

#define PrepOperation()\
op2 = operand_types.top();\
operand_types.pop();\
op1 = operand_types.top();\
operand_types.pop();\

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

std::string Lexeme::ToString() const {
	std::ostringstream oss;
	oss << this->value;
	return oss.str();
}

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

Parser::Parser(std::istream& input): lexer_(input), indents({0}) {}

// void Parser::ReturnLexeme() {
// 	lexer_.input_.unget();
// 	while (lexer_.input_.get() != ' ') {
// 		lexer_.input_.unget();
// 		lexer_.input_.unget();
// 	}
// }

void Parser::PostOp(std::stack<ValueType> &operand_types, ValueType op1, ValueType op2) {
	if ((op1 == Str) || (op2 == Str)) {
		operand_types.emplace(Str);
	}
	else if ((op1 == Real) || (op2 == Real)) {
		operand_types.emplace(Real);
	}
	else {
		operand_types.emplace(Int);
	}
}

void Parser::Run(Context& context) {
	while (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::EOL) {
		lexer_.TakeLexeme();
	}
	if (!lexer_.HasLexeme()) {
		return;
	}
	if (IndentCounter() != 0) {
		throw std::runtime_error(
				std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": " + "IndentationError: unexpected indent");
	}
	try {
		int next_block_indent = Block(context);
		while (lexer_.HasLexeme()) {
			if (next_block_indent != 0) {
				throw std::runtime_error(
					std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": " + "IndentationError: unexpected indent");
			}
			if (lexer_.PeekLexeme().type == Lexeme::EOL) {
				lexer_.TakeLexeme();
			}
			else {
				next_block_indent = Block(context);
			}
		}
	}
	catch (std::out_of_range& e) {
		while (!if_indices.empty()) {
			auto if_index = if_indices.top();
			operations[if_index].reset(new execution::IfOperation(operations.size()));
			if_indices.pop();
    	}
		operations.emplace_back(new execution::ThrowBadOperand("line " + std::to_string(Lexer::line) + 
						": TypeError: unsupported operand type(s) for -: " + ToStringSem(op1) + " and " + ToStringSem(op2)));
	}
}

int Parser::IndentCounter() {
	int indent = 0;
	while (lexer_.HasLexeme()) {
		if (lexer_.PeekLexeme().type == Lexeme::IndentSpace) {
			lexer_.TakeLexeme();
			indent++;
		}
		else {
			if (lexer_.PeekLexeme().type == Lexeme::EOL) {
				lexer_.TakeLexeme();
				indent = 0;
			}
			else {
				Lexeme lex = lexer_.PeekLexeme();
				Lexer::pos -= lex.value.length();
				break;
			}
		}
	}
	return indent;
}

int Parser::Block(Context& context) {
	int next_block_indent = -1;
	if (lexer_.HasLexeme()) { 
		if (lexer_.PeekLexeme().type == Lexeme::For || 
			lexer_.PeekLexeme().type == Lexeme::If || 
			lexer_.PeekLexeme().type == Lexeme::While || 
			lexer_.PeekLexeme().type == Lexeme::ElIf ||
			lexer_.PeekLexeme().type == Lexeme::Else) {
			Lexeme lex = lexer_.TakeLexeme();
			switch (lex.type) {
			case Lexeme::For:
				next_block_indent = ForBlock(context);
				break;
			case Lexeme::While:
				next_block_indent = WhileBlock(context);
				break;
			case Lexeme::If:
				next_block_indent = IfBlock(context);
				break;
			case Lexeme::Else:
			case Lexeme::ElIf:
				throw std::runtime_error(
					std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": SyntaxError: invalid syntax");
			default:
				break;
			}

			if (indents.top() == 0 && next_block_indent != 0 && next_block_indent != -1) {
				throw std::runtime_error(
					std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": " + "invalid indent_1");
			}
			return next_block_indent;
		}
		else {
			switch (lexer_.PeekLexeme().type) {
			case Lexeme::Print:
				Print(context);
				break;
			case Lexeme::Identifier:
				Assign(context);
				break;
			default:
				Expression(context);
				break;
			}
			if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::EOL) {
				lexer_.TakeLexeme();
				int next_block_indent = IndentCounter();
				return next_block_indent;
			}
			if (!lexer_.HasLexeme()) {
				return -1;
			}
		}
	}
	throw std::runtime_error (
		std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": " + "invalid syntax_10");
}

// For

int Parser::ForBlock(Context& context) {
	if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::Identifier) {
		Lexeme lex = lexer_.TakeLexeme();
		// operations.emplace_back(new execution::VariableOperation(lex));
		if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::In) {
			lexer_.TakeLexeme();
			Range(context);

			operations.emplace_back(new execution::AssignOperation(lex.value));
			var_types.insert_or_assign(lex.value, operand_types.top());

			const execution::OperationIndex label_if = operations.size();

			operations.emplace_back(new execution::VariableOperation(lex.value));
			operations.emplace_back(new execution::VariableOperation("edge"));

			operand_types.emplace(Int);
			operand_types.emplace(Int);

			PrepOperation();
			operations.emplace_back(execution::kBinaries.at(std::make_tuple(Lexeme::Less, op1, op2))());

			const execution::OperationIndex if_index = operations.size();
			operations.emplace_back(nullptr);
			if_indices.emplace(if_index);

			if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::Colon) {
				lexer_.TakeLexeme();
				int next_block_indent = InnerBlock(context);

				operations.emplace_back(new execution::VariableOperation(lex.value));
				operations.emplace_back(new execution::AddOneOperation(lex.value));

				operations.emplace_back(new execution::GoOperation(label_if));

				const execution::OperationIndex label_next = operations.size();
				operations[if_index].reset(new execution::IfOperation(label_next));
				
				if_indices.pop();

				operations.emplace_back(new PopOperation());

				return next_block_indent;
			}
		}
	}
	throw std::runtime_error (std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": " + "invalid syntax");
}

void Parser::Range(Context& context) {
	if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::Range) {
		lexer_.TakeLexeme();
		if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::LeftParenthesis) {
			lexer_.TakeLexeme();
			Interval(context);	
			operations.emplace_back(new execution::AssignOperation("edge"));
			var_types.insert_or_assign("edge", operand_types.top());
			if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::RightParenthesis) {
				lexer_.TakeLexeme();
				return;
			}
		}
	}

	// else if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::StringConst) {
	// 	Lexeme lexeme = lexer_.TakeLexeme();
	// 	operand_types.emplace(Str);
	// 	operations.emplace_back(new execution::ValueOperation(std::string(lexeme)));
	// 	// operations.emplace_back(new execution::AssignOperation(lexe))
	// 	return;
	// }

	throw std::runtime_error(
		std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": " + "invalid syntax: wrong cycle field");
}

void Parser::Interval(Context& context) {
	if (lexer_.HasLexeme() 
		&& (lexer_.PeekLexeme().type == Lexeme::Identifier || lexer_.PeekLexeme().type == Lexeme::IntegerConst ||
			lexer_.PeekLexeme().type == Lexeme::BoolConst)) {
			Lexeme lex = lexer_.TakeLexeme();
			if (lexer_.HasLexeme() && lexer_.PeekLexeme().type != Lexeme::Comma) {
				operand_types.emplace(Int);
				operations.emplace_back(new execution::ValueOperation(0));
				if (lex.type == Lexeme::IntegerConst) {
					operand_types.emplace(Int);
					operations.emplace_back(new execution::ValueOperation(int(lex)));
				}
				if (lex.type == Lexeme::BoolConst) {
					operand_types.emplace(Logic);
					operations.emplace_back(new execution::ValueOperation(bool(lex)));
				}
				if (lex.type == Lexeme::Identifier) {
					if ((var_types[lex.value] != Int) && (var_types[lex.value] != Logic)) {
						operations.emplace_back(new execution::ThrowBadOperand("line " + std::to_string(Lexer::line) + 
			 			": TypeError: 'str' object cannot be interpreted as an integer"));
					}
					operand_types.emplace(var_types[lex.value]);
					operations.emplace_back(new execution::VariableOperation(lex.value));
				}
				return;
			}
			if (lex.type == Lexeme::IntegerConst) {
					operand_types.emplace(Int);
					operations.emplace_back(new execution::ValueOperation(int(lex)));
				}
			if (lex.type == Lexeme::BoolConst) {
				operand_types.emplace(Logic);
				operations.emplace_back(new execution::ValueOperation(bool(lex)));
			}
			if (lex.type == Lexeme::Identifier) {
				if ((var_types[lex.value] != Int) && (var_types[lex.value] != Logic)) {
					operations.emplace_back(new execution::ThrowBadOperand("line " + std::to_string(Lexer::line) + 
					": TypeError: 'str' object cannot be interpreted as an integer"));
				}
				operand_types.emplace(var_types[lex.value]);
				operations.emplace_back(new execution::VariableOperation(lex.value));
			}
			lex = lexer_.TakeLexeme();
			if (lexer_.HasLexeme() 
				&& (lexer_.PeekLexeme().type == Lexeme::Identifier || lexer_.PeekLexeme().type == Lexeme::IntegerConst ||
					lexer_.PeekLexeme().type == Lexeme::BoolConst)) {
					lex = lexer_.TakeLexeme();
					if (lex.type == Lexeme::IntegerConst) {
						operand_types.emplace(Int);
						operations.emplace_back(new execution::ValueOperation(int(lex)));
					}
					if (lex.type == Lexeme::BoolConst) {
						operand_types.emplace(Logic);
						operations.emplace_back(new execution::ValueOperation(bool(lex)));
					}
					if (lex.type == Lexeme::Identifier) {
						if ((var_types[lex.value] != Int) && (var_types[lex.value] != Logic)) {
							operations.emplace_back(new execution::ThrowBadOperand("line " + std::to_string(Lexer::line) + 
							": TypeError: 'str' object cannot be interpreted as an integer"));
						}
						operand_types.emplace(var_types[lex.value]);
						operations.emplace_back(new execution::VariableOperation(lex.value));
					}
					return;
			}
	}

	throw std::runtime_error(
		std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": " + "invalid syntax: wrong range");
}

// If

int Parser::IfBlock(Context& context) {
	Expression(context);

	const execution::OperationIndex if_index = operations.size();
	operations.emplace_back(nullptr);
	if_indices.emplace(if_index);
	  
	if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::Colon) {
		lexer_.TakeLexeme();
		int next_block_indent = InnerBlock(context);

		const execution::OperationIndex truth_index = operations.size();
		operations.emplace_back(nullptr);
		if_indices.emplace(truth_index);

		const execution::OperationIndex label = operations.size();
		operations[if_index].reset(new execution::IfOperation(label));

		if (next_block_indent == indents.top()) {

			if (lexer_.HasLexeme() && 
				(lexer_.PeekLexeme().type == Lexeme::ElIf || lexer_.PeekLexeme().type == Lexeme::Else)) {
				Lexeme lex = lexer_.TakeLexeme();
			
				switch (lex.type) {
					case Lexeme::Else:
						next_block_indent = Parser::ElseBlock(context);
						break;
					case Lexeme::ElIf:
						next_block_indent = Parser::IfBlock(context);
						break;
					default:
						break;
				}
			}
			operations.emplace_back(new PopOperation());
		}
		const execution::OperationIndex label_n = operations.size();
		operations[truth_index].reset(new execution::GoOperation(label_n));
		if_indices.pop();
		if_indices.pop();
		return next_block_indent;
	}
	throw std::runtime_error (std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": " + "invalid syntax");
}

int Parser::ElseBlock(Context& context) {
	if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::Colon) {
		lexer_.TakeLexeme();
		int next_block_indent = InnerBlock(context);
		return next_block_indent;
	}
	throw std::runtime_error (std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": " + "invalid syntax");
}

// While
int Parser::WhileBlock(Context& context) {
	const execution::OperationIndex label_if = operations.size();

	Expression(context);

	const execution::OperationIndex if_index = operations.size();
	operations.emplace_back(nullptr);
	if_indices.emplace(if_index);

	if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::Colon) {
		lexer_.TakeLexeme();
		int next_block_indent = InnerBlock(context);

		operations.emplace_back(new execution::GoOperation(label_if));

		const execution::OperationIndex label_next = operations.size();
  		operations[if_index].reset(new execution::IfOperation(label_next));
		
		if_indices.pop();

		operations.emplace_back(new PopOperation());

		return next_block_indent;
	}
	throw std::runtime_error (std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": " + "invalid syntax");
}



int Parser::InnerBlock(Context& context) {
	if (lexer_.HasLexeme()) {
		if (lexer_.PeekLexeme().type != Lexeme::EOL) {
			switch (lexer_.PeekLexeme().type) {
			case Lexeme::Print:
				Print(context);
				break;
			case Lexeme::Identifier:
				Assign(context);
				break;
			default:
				Expression(context);
				break;
			}
			if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::EOL) {
				lexer_.TakeLexeme();
				return IndentCounter();
			}
			throw std::runtime_error (std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": " + "invalid syntax: expected EOL");
		}

		while (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::EOL) {
			lexer_.TakeLexeme();
		}

		int indent = IndentCounter();

		if (indent > indents.top()) {
			indents.push(indent);

			int next_block_indent = Block(context);

			while (next_block_indent == indent) {
				next_block_indent = Block(context);
			}

			if (next_block_indent < indent) {
				indents.pop();
				return next_block_indent;
			}
		}
		throw std::runtime_error(std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": " + "invalid indent_3");
	}
	throw std::runtime_error(std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": " + "invalid indent_4");
	return -1;
}

void Parser::Print(Context& context) {
	lexer_.TakeLexeme();
	if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::LeftParenthesis) {
		lexer_.TakeLexeme();
		Expression(context);
		
		if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::RightParenthesis) {
			lexer_.TakeLexeme();
			ValueType op = operand_types.top();
			operand_types.pop();
			operations.emplace_back(kUnaries.at(std::make_tuple(Lexeme::Print, op)));
			
			return;
		}
	}

	throw std::runtime_error(std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": " + "invalid syntax_4");
}

void Parser::Assign(Context& context) {
	Lexeme lex = lexer_.TakeLexeme();
	if (lexer_.HasLexeme()) {
		if (lexer_.PeekLexeme().type == Lexeme::Assign) {
			lexer_.TakeLexeme();
			Expression(context);
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

void Parser::Expression(Context& context) {
	CompParts(context);
	while (lexer_.HasLexeme()) {
		const Lexeme::LexemeType op_type = lexer_.PeekLexeme().type;
		if (op_type != Lexeme::Less && op_type != Lexeme::LessEq &&
			op_type != Lexeme::Greater && op_type != Lexeme::GreaterEq && 
			op_type != Lexeme::Equal && op_type != Lexeme::NotEqual) {
			break;
		}
		lexer_.TakeLexeme();
		CompParts(context);

		PrepOperation();

		operations.emplace_back(execution::kBinaries.at(std::make_tuple(op_type, op1, op2))());
		
		operand_types.emplace(Logic);
	}
}

void Parser::CompParts(Context& context) {
	SumParts(context);
	while (lexer_.HasLexeme()) {
		const Lexeme::LexemeType op_type = lexer_.PeekLexeme().type;
		if (op_type != Lexeme::Add && op_type != Lexeme::Sub) {
		break;
		}
		lexer_.TakeLexeme();
		SumParts(context);
		PrepOperation();

		operations.emplace_back(execution::kBinaries.at(std::make_tuple(op_type, op1, op2))());


		PostOp(operand_types, op1, op2);
	}
}

void Parser::SumParts(Context& context) {
	MultParts(context);
	while (lexer_.HasLexeme()) {
		const Lexeme::LexemeType op_type = lexer_.PeekLexeme().type;
		if (op_type != Lexeme::Mul && op_type != Lexeme::Div) {
			break;
		}
		lexer_.TakeLexeme();
		MultParts(context);
		PrepOperation();

		operations.emplace_back(execution::kBinaries.at(std::make_tuple(op_type, op1, op2))());	

		PostOp(operand_types, op1, op2);
	}
}

void Parser::MultParts(Context& context) {
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

	Cast(context);

	if (minus) {
		ValueType op = operand_types.top();
		operand_types.pop();
		try {
			operations.emplace_back(kUnaries.at(std::make_tuple(Lexeme::UnaryMinus, op)));
	
			PostOp(operand_types, op);
		}
		catch (std::out_of_range& e) {
			operations.emplace_back(new execution::ThrowBadOperand("line " + std::to_string(Lexer::line) + 
			 			": TypeError: bad operand type(s) for unary -: " + ToStringSem(op)));
		}
	}
}

void Parser::Cast(Context& context) {
	while (lexer_.HasLexeme()) {
		const Lexeme::LexemeType cast_type = lexer_.PeekLexeme().type;
		if (cast_type != Lexeme::Bool && cast_type != Lexeme::Int &&
			cast_type != Lexeme::Str && cast_type != Lexeme::Float) {
			break;
		}
		type_cast.emplace(cast_type);
		lexer_.TakeLexeme();
		if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::LeftParenthesis) {
			lexer_.TakeLexeme();
		}	
		else {
			throw std::runtime_error(std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": " + 
				"expected left parenthesis");
		}
	}
	Members(context);

	while (!type_cast.empty()) {
		if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::RightParenthesis) {
			lexer_.TakeLexeme();
		}	
		else {
			throw std::runtime_error(std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": " + 
				"expected right parenthesis");
		}
		ValueType op = operand_types.top();
		operand_types.pop();
		Lexeme::LexemeType cast_to = type_cast.top();
		type_cast.pop();
		operations.emplace_back(kUnaries.at(std::make_tuple(cast_to, op)));
		switch (cast_to) {
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

void Parser::Members(Context& context) {
	if (!lexer_.HasLexeme()) {
		throw std::runtime_error("expected operand");
	}

	const Lexeme lexeme = lexer_.TakeLexeme();

	switch (lexeme.type) {
		case Lexeme::Identifier:
			operand_types.emplace(var_types[lexeme.value]);
			operations.emplace_back(new execution::VariableOperation(lexeme.value));

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
			std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": " + 
			"expected left parenthesis instead of " + lexeme.ToString());
	}

	Expression(context);

	if (!lexer_.HasLexeme()) {
		throw std::runtime_error(
			std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": " + "expected right parenthesis instead of end");
	}

	if (lexer_.PeekLexeme().type != Lexeme::RightParenthesis) {
		throw std::runtime_error(
			std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": " + "expected right parenthesis instead of " +
			lexer_.PeekLexeme().ToString());
	}

	lexer_.TakeLexeme();
}
