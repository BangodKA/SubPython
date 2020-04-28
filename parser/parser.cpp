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
	std::stack<Lexeme::LexemeType> operators;
	std::stack<const execution::OperationIndex> loop_starts;
	std::stack<const execution::OperationIndex> breaks;
	std::unordered_map<VariableName, ValueType> var_types;

	void PostOp(std::stack<ValueType> &operand_types, ValueType op1, ValueType op2 = Logic);

	std::string TypeToString(Lexeme::LexemeType type) const;

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
	
	// Inner Parts
	int InnerBlock(Context& context);

	void Print(Context& context);

	void Assign(Context& context);

	void Expression(Context& context);
	void OrParts(Context& context);
	void AndParts(Context& context);
	void LogicalParts(Context& context);
	void CompParts(Context& context);
	void SumParts(Context& context);
	void MultParts(Context& context);
	void Cast(Context& context);
	void Members(Context& context);
};

Parser::Parser(std::istream& input): lexer_(input), indents({0}) {}

std::string Parser::TypeToString(Lexeme::LexemeType type) const {
	std::ostringstream oss;
	oss << type;
	return oss.str();
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

void Parser::Run(Context& context) {
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
	try {
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
		std::string exception_mes = "";
		Lexeme::LexemeType type = operators.top();
		if (type != Lexeme::UnaryMinus && type != Lexeme::Not) {
			exception_mes = "line " + std::to_string(Lexer::line) + 
						": TypeError: unsupported operand type(s) for " + 
						TypeToString(type) + ": " + ToStringSem(op1) + " and " + ToStringSem(op2);
		}
		else {
			std::string unmin = "";
			if (type == Lexeme::UnaryMinus) {
				unmin = "unary ";
			}
			exception_mes = "line " + std::to_string(Lexer::line) + 
						": TypeError: unsupported operand type(s) for " + unmin + TypeToString(type) + ": " + ToStringSem(op1);
		}
		operations.emplace_back(new execution::ThrowCustomException(exception_mes));
	}
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

int Parser::Block(Context& context) {
	int next_block_indent = 0;
	const Lexeme lex = lexer_.PeekLexeme();
	if (lex.type == Lexeme::For || lex.type == Lexeme::If || 
		lex.type == Lexeme::While || lex.type == Lexeme::ElIf ||
		lex.type == Lexeme::Else) {
		lexer_.TakeLexeme();
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
				"line " + std::to_string(Lexer::line) + ":" + 
				std::to_string(Lexer::pos) + ": IndentationError: unexpected indent");
		}
		return next_block_indent;
	}
	switch (lexer_.PeekLexeme().type) {
		case Lexeme::Print:
			Print(context);
			break;
		case Lexeme::Identifier:
			Assign(context);
			break;
		case Lexeme::Continue:
			if (!loop_starts.empty()) {
				operations.emplace_back(new execution::GoOperation(loop_starts.top()));
				lexer_.TakeLexeme();
			}
			else {
				throw std::runtime_error("SyntaxError: 'continue' outside loop");
			}
			break;
		case Lexeme::Break:
			if (!loop_starts.empty()) {
				const execution::OperationIndex break_index = operations.size();
				operations.emplace_back(nullptr);
				breaks.emplace(break_index);
				lexer_.TakeLexeme();
			}
			else {
				throw std::runtime_error("SyntaxError: 'break' outside loop");
			}
			break;
		default:
			Expression(context);
			break;
	}
	if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::EOL) {
		lexer_.TakeLexeme();
		next_block_indent = IndentCounter();
		return next_block_indent;
	}
	if (!lexer_.HasLexeme()) {
		return 0;
	}
	throw std::runtime_error (
		"line " + std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) +
		 ": SyntaxError: invalid syntax");
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

			const execution::OperationIndex go_index = operations.size();
			operations.emplace_back(nullptr);
			if_indices.emplace(go_index);

			const execution::OperationIndex label_if = operations.size();
			loop_starts.emplace(label_if);

			operations.emplace_back(new execution::VariableOperation(lex.value));
			operations.emplace_back(new execution::AddOneOperation(lex.value));

			const execution::OperationIndex label_new = operations.size();
			operations[go_index].reset(new execution::GoOperation(label_new));

			operations.emplace_back(new execution::VariableOperation(lex.value));
			operations.emplace_back(new execution::VariableOperation("edge" + std::to_string(loop_starts.size() - 1)));

			operand_types.emplace(Int);
			operand_types.emplace(Int);

			operators.emplace(Lexeme::Less);
			PrepOperation();
			operations.emplace_back(execution::kBinaries.at(std::make_tuple(Lexeme::Less, op1, op2))());

			const execution::OperationIndex if_index = operations.size();
			operations.emplace_back(nullptr);
			if_indices.emplace(if_index);

			if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::Colon) {
				lexer_.TakeLexeme();
				int breaks_amount = breaks.size();

				int next_block_indent = InnerBlock(context);
				loop_starts.pop();

				operations.emplace_back(new execution::GoOperation(label_if));

				const execution::OperationIndex label_next = operations.size();

				while (breaks.size() != breaks_amount) {
					operations[breaks.top()].reset(new execution::GoOperation(label_next));
					breaks.pop();
				}

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
			std::string edge = "edge" + std::to_string(loop_starts.size());	
			operations.emplace_back(new execution::AssignOperation(edge));
			var_types.insert_or_assign(edge, operand_types.top());
			if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::RightParenthesis) {
				lexer_.TakeLexeme();
				return;
			}
		}
	}

	throw std::runtime_error(
		std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": " + "invalid syntax: wrong loop field");
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
						// operations.emplace_back(new execution::ThrowBadOperand("line " + std::to_string(Lexer::line) + 
			 			// ": TypeError: 'str' object cannot be interpreted as an integer"));
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
					// operations.emplace_back(new execution::ThrowBadOperand("line " + std::to_string(Lexer::line) + 
					// ": TypeError: 'str' object cannot be interpreted as an integer"));
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
							// operations.emplace_back(new execution::ThrowBadOperand("line " + std::to_string(Lexer::line) + 
							// ": TypeError: 'str' object cannot be interpreted as an integer"));
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
	loop_starts.emplace(label_if);

	Expression(context);

	const execution::OperationIndex if_index = operations.size();
	operations.emplace_back(nullptr);
	if_indices.emplace(if_index);

	if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::Colon) {
		lexer_.TakeLexeme();

		int breaks_amount = breaks.size();

		int next_block_indent = InnerBlock(context);

		loop_starts.pop();

		operations.emplace_back(new execution::GoOperation(label_if));


		const execution::OperationIndex label_next = operations.size();
		while (breaks.size() != breaks_amount) {
			operations[breaks.top()].reset(new execution::GoOperation(label_next));
			breaks.pop();
		}
		
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
			case Lexeme::Continue:
				if (!loop_starts.empty()) {
					operations.emplace_back(new execution::GoOperation(loop_starts.top()));
					lexer_.TakeLexeme();
				}
				else {
					throw std::runtime_error("SyntaxError: 'continue' not properly in loop");
				}
				break;
			case Lexeme::Break:
				if (!loop_starts.empty()) {
					const execution::OperationIndex break_index = operations.size();
					operations.emplace_back(nullptr);
					breaks.emplace(break_index);
					lexer_.TakeLexeme();
				}
				else {
					throw std::runtime_error("SyntaxError: 'continue' not properly in loop");
				}
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

		if (lexer_.HasLexeme()) {

			if (indent > indents.top()) {
				indents.push(indent);

				int next_block_indent = Block(context);

				while (next_block_indent == indent && lexer_.HasLexeme()) {
					next_block_indent = Block(context);
				}

				if (next_block_indent < indent) {
					indents.pop();
					return next_block_indent;
				}
				throw std::runtime_error(
					"line " + std::to_string(Lexer::line) + ":" + 
						std::to_string(Lexer::pos - lexer_.PeekLexeme().value.length()) + 
						": IndentationError: unexpected indent");
			}
			throw std::runtime_error(
					"line " + std::to_string(Lexer::line) + ":" + 
						std::to_string(Lexer::pos) + 
						": IndentationError: expected an indented block");
		}
		
	}
	throw std::runtime_error(
		"line " + std::to_string(Lexer::line) + ": SyntaxError: unexpected EOF while parsing");
}

void Parser::Print(Context& context) {
	lexer_.TakeLexeme();
	if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::LeftParenthesis) {
		lexer_.TakeLexeme();
		if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::RightParenthesis) {
			lexer_.TakeLexeme();
			operations.emplace_back(new execution::ValueOperation(""));
			operations.emplace_back(kUnaries.at(std::make_tuple(Lexeme::Print, Str)));
			return;
		}
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
	OrParts(context);
	while (lexer_.HasLexeme()) {
		if (lexer_.PeekLexeme().type != Lexeme::Or) {
			break;
		}
		lexer_.TakeLexeme();
		OrParts(context);

		operators.emplace(Lexeme::Or);
		PrepOperation();
		operations.emplace_back(execution::kBinaries.at(std::make_tuple(Lexeme::Or, op1, op2))());
		
		operand_types.emplace(Logic);
	}
}

void Parser::OrParts(Context& context) {
	AndParts(context);
	while (lexer_.HasLexeme()) {
		if (lexer_.PeekLexeme().type != Lexeme::And) {
			break;
		}
		lexer_.TakeLexeme();
		AndParts(context);

		operators.emplace(Lexeme::And);
		PrepOperation();
		operations.emplace_back(execution::kBinaries.at(std::make_tuple(Lexeme::And, op1, op2))());
		
		operand_types.emplace(Logic);
	}
}

void Parser::AndParts(Context& context) {
	bool not_ = false;
	while (lexer_.HasLexeme()) {
		if (lexer_.PeekLexeme().type != Lexeme::Not) {
			break;
		}
		not_ ^= true;
		lexer_.TakeLexeme();
	}

	LogicalParts(context);

	if (not_) {
		operators.emplace(Lexeme::Not);
		op1 = operand_types.top();
		operand_types.pop();
		// try {
			operations.emplace_back(kUnaries.at(std::make_tuple(Lexeme::Not, op1)));
	
			PostOp(operand_types, Logic);
		// }
		// catch (std::out_of_range& e) {
		// 	operations.emplace_back(new execution::ThrowBadOperand("line " + std::to_string(Lexer::line) + 
		// 	 			": TypeError: bad operand type(s) for unary -: " + ToStringSem(op)));
		// }
	}
}

void Parser::LogicalParts(Context& context) {
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

		operators.emplace(op_type);

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

		operators.emplace(op_type);
		PrepOperation();

		operations.emplace_back(execution::kBinaries.at(std::make_tuple(op_type, op1, op2))());


		PostOp(operand_types, op1, op2);
	}
}

void Parser::SumParts(Context& context) {
	MultParts(context);
	while (lexer_.HasLexeme()) {
		const Lexeme::LexemeType op_type = lexer_.PeekLexeme().type;
		if (op_type != Lexeme::Mul && op_type != Lexeme::Div &&
			op_type != Lexeme::Mod) {
			break;
		}
		lexer_.TakeLexeme();
		MultParts(context);

		operators.emplace(op_type);
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
		operators.emplace(Lexeme::UnaryMinus);
		op1 = operand_types.top();
		operand_types.pop();
		// try {
			operations.emplace_back(kUnaries.at(std::make_tuple(Lexeme::UnaryMinus, op1)));
	
			PostOp(operand_types, op1);
		// }
		// catch (std::out_of_range& e) {
		// 	operations.emplace_back(new execution::ThrowBadOperand("line " + std::to_string(Lexer::line) + 
		// 	 			": TypeError: bad operand type(s) for unary -: " + ToStringSem(op)));
		// }
	}
}

void Parser::Cast(Context& context) {
	if (lexer_.HasLexeme()) {
		bool cast = false;
		const Lexeme::LexemeType cast_type = lexer_.PeekLexeme().type;
		if (cast_type == Lexeme::Bool || cast_type == Lexeme::Int ||
			cast_type == Lexeme::Str || cast_type == Lexeme::Float) {
			cast = true;
		// type_cast.emplace(cast_type);
			lexer_.TakeLexeme();
			if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::LeftParenthesis) {
				lexer_.TakeLexeme();
				Expression(context);
			}	
			else {
				throw std::runtime_error(std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": " + 
					"expected left parenthesis");
			}


			if (cast) {
				if (lexer_.HasLexeme() && lexer_.PeekLexeme().type == Lexeme::RightParenthesis) {
					lexer_.TakeLexeme();
				}	
				else {
					throw std::runtime_error(std::to_string(Lexer::line) + ":" + std::to_string(Lexer::pos) + ": " + 
						"expected right parenthesis");
				}

				ValueType op = operand_types.top();
				operand_types.pop();

				operations.emplace_back(kUnaries.at(std::make_tuple(cast_type, op)));
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
			Members(context);
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
