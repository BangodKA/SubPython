#include "lexemes.hpp"
#include "print.hpp"

Lexeme::operator bool() const {
    assert (type == BoolConst);
    bool result = value == "True" ? true : false;
    return result;
}

Lexeme::operator double() const {
    return std::stod(value);
}

Lexeme::operator int() const {
    return std::stoi(value);
}

Lexeme::operator std::string() const {
    return value;
}

std::string Lexeme::TypeToString(Lexeme::LexemeType type) {
	std::ostringstream oss;
	oss << type;
	return oss.str();
}