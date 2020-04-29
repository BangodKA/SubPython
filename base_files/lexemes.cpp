#include "lexemes.hpp"

Lexeme::operator bool() const {
    assert (type == BoolConst);
    return (value == "True");
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

std::string Lexeme::ToString() const {
	std::ostringstream oss;
	oss << value;
	return oss.str();
}
