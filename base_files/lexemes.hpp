#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

struct Lexeme {
    enum LexemeType {
        // Constants
        // String consists of ASCII only; long literals can be wrapped up into either ''' or """
        StringConst,  
        // True/False
        BoolConst,
        // Numbers
        IntegerConst, 
        FloatConst,
        // OctintegerConst, // 0o or 0O
        // HexintegerConst, // 0x or 0X
        // BinintegerConst, // 0b or 0B

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
        BackSlash, 
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

        // Special operations, not lexemes
        UnaryMinus,

        Identifier,
    };
	Lexeme() = default;
    LexemeType type;

    // bool boolean_value;
    std::string value;

    operator bool() const;
    operator double() const;
    operator int() const;
    operator std::string() const;

	std::string ToString() const;
};