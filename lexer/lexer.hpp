#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "../base_files/lexemes.hpp"

class Lexer {
 public:
	explicit Lexer(std::istream& input);
	using Lexemes = std::vector <Lexeme>;

	static int line;
	static int pos;
	std::istream& input_;

	bool HasLexeme();
	const Lexeme& PeekLexeme() const;
	Lexeme TakeLexeme();

 private:
	using Char = std::istream::int_type;
	using State = bool (Lexer::*)(Char c);

	void Unget();

	bool Initial(Char c);

	// Indents influencers
	bool LineStart(Char c);
	bool LineBreak(Char c);
	bool BlockStart(Char c);

	// Comparison/Assign
	bool CompSigns(Char c);

	// Number
	bool NoIntegerPart(Char c);
	bool Float(Char c);
	bool Zero(Char c);
	bool Integer(Char c);
	bool SignEFloat(Char c);
	bool EFloat(Char c);
	bool FullEFloat(Char c);
	
	// Variable/Keywords
	bool Variable(Char c);

	// String
	bool String(Char c);
	bool ScreenSymbol(Char c);

	// Comments
	bool LineComments(Char c);

	bool has_lexeme_;
	Lexeme lexeme_;
	Lexemes lexemes_;
	State state_;

	static bool IsVar(Char c);
	static bool IsRelevant(Char c);

	static const std::unordered_map<Char, Lexeme::LexemeType> Comparison;
	static const std::unordered_map<std::string, Lexeme::LexemeType> EqComparison;
	static const std::unordered_map<Char, Lexeme::LexemeType> kSingleSeparators;
	static const std::unordered_map<std::string, Lexeme::LexemeType> ReservedWords;
};