#include "lexemes.hpp"

std::ostream& operator<<(std::ostream& out, const Lexeme& lex) {
  switch (lex.type) {
    case Lexeme::Add:
		out << "Add +";
		break;
    case Lexeme::Sub:
		out << "Sub -";
		break;
    case Lexeme::Mul:
		out << "Mul *";
		break;
    case Lexeme::Div:
		out << "Div /";
		break;
	case Lexeme::Mod:
		out << "Mod %";
		break;
    case Lexeme::Less:
		out << "Less <";
		break;
    case Lexeme::LessEq:
		out << "LessOrEqual <=";
		break;
	case Lexeme::Greater:
		out << "Greater >";
		break;
    case Lexeme::GreaterEq:
		out << "GreaterOrEqual >=";
		break;
	case Lexeme::Equal:
		out << "Equal ==";
		break;
	case Lexeme::Assign:
		out << "Assign =";
		break;
    case Lexeme::LeftParenthesis:
		out << "LeftParenthesis (";
		break;
    case Lexeme::RightParenthesis:
		out << "RightParenthesis )";
		break;
    case Lexeme::IntegerConst:
		out << "IntegerConst" << "	(" << lex.value << ')';
		break;
	case Lexeme::FloatConst:
		out << "FloatConst" << "	(" << lex.value << ')';
		break;
    case Lexeme::Identifier:
		out << "Identifier" << "	(" << lex.value << ')';
		break;
	case Lexeme::StringConst:
		out << "StringConst" << "	(" << lex.value << ')';
		break;
	case Lexeme::BoolConst:
		out << "BoolConst" << "	(" << lex.value << ')';
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
		out << "bool";
		break;
    case Lexeme::Int:
		out << "int";
		break;
	case Lexeme::None:
		out << "None";
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