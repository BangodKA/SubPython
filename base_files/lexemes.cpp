#include <iostream>
#include <sstream>
#include <string>

struct Lexeme {
    enum LexemeType {
        // Constants
        // String consists of ASCII only;
        StringConst,  
        BoolConst, 
        IntegerConst, 
        FloatConst,

        // Keywords
        Bool, 
        Int, 
        Str, 
        Float, 
        While,  
        For, 
        Break, 
        Continue, 
        If, 
        Else, 
        ElIf, 
        In, 
        Range, 
        And,
        Or,
        Not,
        Print,

        // Punctuators/Operators
        EOL, 
        IndentSpace, 
        Comma, 
        Colon, 
        LeftParenthesis, 
        RightParenthesis, 
        Assign, 
        Equal, 
        NotEqual, 
        Less, 
        LessEq,  
        Greater, 
        GreaterEq, 
        Add, 
        Sub,  
        Mul,  
        Div, 
        Mod, 

        UnaryMinus, // Special operation, not lexeme

        Identifier, 
    };
	Lexeme() = default;
    LexemeType type;

    std::string value;

    operator bool() const;
    operator double() const;
    operator int() const;
    operator std::string() const;

	std::string ToString() const;
};

Lexeme::operator bool() const {
    if (value == "True") {
        return true;
    }
    return false;
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
	oss << this->value;
	return oss.str();
}