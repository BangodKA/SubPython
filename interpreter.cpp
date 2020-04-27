#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <stack>

#include <ctime>

#include "parser/parser.hpp"

std::ostream& operator<<(std::ostream& out, const Lexeme& lex) {
  switch (lex.type) {
    case Lexeme::Add:
		out << "+";
		break;
	case Lexeme::UnaryMinus:
    case Lexeme::Sub:
		out << "-";
		break;
    case Lexeme::Mul:
		out << "*";
		break;
    case Lexeme::Div:
		out << "/";
		break;
	case Lexeme::Mod:
		out << "%";
		break;
    case Lexeme::Less:
		out << "<";
		break;
    case Lexeme::LessEq:
		out << "<=";
		break;
	case Lexeme::Greater:
		out << ">";
		break;
    case Lexeme::GreaterEq:
		out << ">=";
		break;
	case Lexeme::Equal:
		out << "==";
		break;
	case Lexeme::NotEqual:
		out << "!=";
		break;
	case Lexeme::Assign:
		out << "=";
		break;
    case Lexeme::LeftParenthesis:
		out << "(";
		break;
    case Lexeme::RightParenthesis:
		out << ")";
		break;
    case Lexeme::IntegerConst:
		out << lex.value;
		break;
	case Lexeme::FloatConst:
		out << lex.value;
		break;
    case Lexeme::Identifier:
		out << lex.value;
		break;
	case Lexeme::StringConst:
		out << lex.value;
		break;
	case Lexeme::BoolConst:
		out << lex.value;
		break;
	case Lexeme::EOL:
		out << "End of line";
		break;
	case Lexeme::IndentSpace:
		out << "Space indentation";
		break;
	case Lexeme::Comma:
		out << "Comma ,";
		break;
    case Lexeme::Colon:
		out << "Colon :";
		break;
	case Lexeme::BackSlash:
		out << "BackSlash \\";
		break;
	case Lexeme::Bool:
		out << "bool()";
		break;
    case Lexeme::Int:
		out << "int()";
		break;
	case Lexeme::Float:
		out << "float()";
		break;
	case Lexeme::Str:
		out << "str()";
		break;
	case Lexeme::While:
		out << "while";
		break;
	case Lexeme::For:
		out << "for";
		break;
	case Lexeme::Break:
		out << "break";
		break;
	case Lexeme::Continue:
		out << "continue";
		break;
	case Lexeme::If:
		out << "if";
		break;
	case Lexeme::Else:
		out << "else";
		break;
	case Lexeme::ElIf:
		out << "elif";
		break;
	case Lexeme::In:
		out << "in";
		break;
	case Lexeme::Range:
		out << "range";
		break;
	case Lexeme::And:
		out << "and";
		break;
	case Lexeme::Or:
		out << "or";
		break;
	case Lexeme::Not:
		out << "not";
		break;
	case Lexeme::Print:
		out << "print";
		break;
  }
  return out;
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
    	std::cerr << "expected argument" << std::endl;
    	return 1;
  	}

	try {
		std::ifstream input(argv[1]);

		execution::Context context;
		Parser parser(input);
		parser.Run(context);
		std::cout << "correct" << std::endl;
		

		while (context.operation_index < parser.operations.size()) {
			// context.Show(std::cout); 
			const auto& operation = parser.operations[context.operation_index];
			++context.operation_index;
			operation->Do(context);
		}
		// context.Show(std::cout); 
    	return 0;
	}
	catch (CustomException& e) {
		std::cerr << e.what();
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}