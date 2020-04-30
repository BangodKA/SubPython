#include "lexemes.hpp"
#include "operators.hpp"

std::string Lexeme::TypeToString(Lexeme::LexemeType type) {
	std::ostringstream oss;
	oss << type;
	return oss.str();
}