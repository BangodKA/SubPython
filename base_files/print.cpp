#include "lexemes.hpp"

std::ostream& operator<<(std::ostream& out, const Lexeme::LexemeType& type) {
  switch (type) {
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
    case Lexeme::LeftParenthesis:
    case Lexeme::RightParenthesis:
    case Lexeme::IntegerConst:
		case Lexeme::FloatConst:
		case Lexeme::Identifier:
		case Lexeme::StringConst:
		case Lexeme::BoolConst:
		case Lexeme::EOL:
		case Lexeme::IndentSpace:
		case Lexeme::Comma:
		case Lexeme::Colon:
		case Lexeme::While:
		case Lexeme::For:
		case Lexeme::Break:
		case Lexeme::Continue:
		case Lexeme::If:
		case Lexeme::Else:
		case Lexeme::ElIf:
		case Lexeme::In:
		case Lexeme::Print:
			break;
  }
  return out;
}