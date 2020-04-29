#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "../base_files/print.hpp"

class Lexer {
  public:
	explicit Lexer(std::istream& input);

	static int line;
	static int pos;

	bool HasLexeme();
	const Lexeme& PeekLexeme() const;
	Lexeme TakeLexeme();


  private:
	using Char = std::istream::int_type;
	using State = bool (Lexer::*)(Char c);

	 std::istream& input_;

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
	// bool LineComments(Char c);

	bool has_lexeme_;
	Lexeme lexeme_;
	State state_;

	static bool IsVar(Char c);
	static bool IsRelevant(Char c);

	static const std::unordered_map<Char, Lexeme::LexemeType> Comparison;
	static const std::unordered_map<std::string, Lexeme::LexemeType> EqComparison;
	static const std::unordered_map<Char, Lexeme::LexemeType> kSingleSeparators;
	static const std::unordered_map<std::string, Lexeme::LexemeType> ReservedWords;
};

int Lexer::line = 1;
int Lexer::pos = 0;

void Lexer::Unget() {
	input_.unget();
	pos--;
}

inline bool Lexer::IsVar(Char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

inline bool Lexer::IsRelevant(Char c) {
	return (c >= 32 && c <= 127) || c == '\n' || c == '\n' || c == 9;
}


const std::unordered_map<Lexer::Char, Lexeme::LexemeType> Lexer::Comparison {
	{'>', Lexeme::Greater},
	{'<', Lexeme::Less},
	{'=', Lexeme::Assign},
	{'!', Lexeme::NotEqual},
};

const std::unordered_map<std::string, Lexeme::LexemeType> Lexer::EqComparison {
	{"<=", Lexeme::LessEq},
    {">=", Lexeme::GreaterEq},
    {"==", Lexeme::Equal},
	{"!=", Lexeme::NotEqual},
};

const std::unordered_map<Lexer::Char, Lexeme::LexemeType> Lexer::kSingleSeparators {
	{'+', Lexeme::Add},
	{'-', Lexeme::Sub},
	{'*', Lexeme::Mul},
	{'/', Lexeme::Div},
	{'%', Lexeme::Mod},
	{'(', Lexeme::LeftParenthesis},
	{')', Lexeme::RightParenthesis},
	{',', Lexeme::Comma},
};

const std::unordered_map<std::string, Lexeme::LexemeType> Lexer::ReservedWords {
	{"bool", Lexeme::Bool},
	{"True", Lexeme::BoolConst},
	{"False", Lexeme::BoolConst},
    {"int", Lexeme::Int},
	{"str", Lexeme::Str},
	{"float", Lexeme::Float},
	{"while", Lexeme::While},
	{"for", Lexeme::For},
	{"break", Lexeme::Break},
	{"continue", Lexeme::Continue},
	{"if", Lexeme::If},
	{"else", Lexeme::Else},
	{"elif", Lexeme::ElIf},
	{"in", Lexeme::In},
	{"range", Lexeme::Range},
	{"and", Lexeme::And},
	{"or", Lexeme::Or},
	{"not", Lexeme::Not},
	{"print", Lexeme::Print},
};

Lexer::Lexer(std::istream& input):
	input_(input),
	has_lexeme_(false),
	state_(nullptr){}

bool Lexer::HasLexeme() {
	if (has_lexeme_) {
		return true;
	}
	if (!input_) {
		return false;
	}

	if (!state_) {
		state_ = &Lexer::LineStart;
	}

	lexeme_.value.clear();
	Char c;
	do {
		c = input_.get();
		pos++;
		if ((this->*state_)(c)) {
			has_lexeme_ = true;
			return true;
		}
	} while (c != std::istream::traits_type::eof());

	return false;
}

const Lexeme& Lexer::PeekLexeme() const {
	if (!has_lexeme_) {
		throw std::logic_error("PeekLexeme before check HasLexeme");
	}
	return lexeme_;
}

Lexeme Lexer::TakeLexeme() {
	if (!has_lexeme_) {
		throw std::logic_error("TakeLexeme before check HasLexeme");
	}
	has_lexeme_ = false;
	return lexeme_;
}

bool Lexer::LineStart(Lexer::Char c) {
	if (c == ' ') {
		lexeme_.type = Lexeme::IndentSpace;
		return true;
	}

	if (c == '\n') {
		pos = 0;
		line++;
		lexeme_.type = Lexeme::EOL;
		return true;
	}

	if (c == std::istream::traits_type::eof()) {
    	return false;
  	}

	state_ = &Lexer::Initial;
	Unget();
	return false;
}

bool Lexer::Initial(Lexer::Char c) {
	if (c == ' ' || c == std::istream::traits_type::eof()) {
    	return false;
  	}

	if (c == '\n') {
		state_ = &Lexer::LineStart;
		Unget();
		return false;
	}

	auto sep = kSingleSeparators.find(c);
	if (sep != kSingleSeparators.end()) {
		lexeme_.type = sep->second;
		lexeme_.value = c;
		return true;
	}

	auto comp = Comparison.find(c);
	if (comp != Comparison.end()) {
		state_ = &Lexer::CompSigns;
		lexeme_.type = comp->second;
		lexeme_.value = c;
		return false;
	}

	if (c == ':') {
		lexeme_.type = Lexeme::Colon;
		state_ = &Lexer::BlockStart;
		return true;
	}

	if (c == '0') {
		state_ = &Lexer::Zero;
		lexeme_.value = c;
		return false;
	}

	if (c >= '1' && c <= '9') {
		lexeme_.value = c;
		state_ = &Lexer::Integer;
		return false;
	}

	if (c == '.') {
		lexeme_.value = "0.";
		state_ = &Lexer::NoIntegerPart;
		return false;
	}

	if (IsVar(c)) {
		state_ = &Lexer::Variable;
		lexeme_.value = c;
		return false;
	}

	if (c == '\'' || c == '"') {
		lexeme_.value = c;
		state_ = &Lexer::String;
		return false;
	}

	if (c == '#') {
		// todo - fix
		state_ = &Lexer::LineStart;
		while (c != '\n' && c != std::istream::traits_type::eof()) c = input_.get();
		Unget();
		return false;
	}

	throw std::runtime_error(std::to_string(line) + ":" + std::to_string(pos) +
				": LexicalError invalid character " + std::string(1, c));
}

bool Lexer::LineBreak(Lexer::Char c) {
	if (c == '\n') {
		lexeme_.type = Lexeme::EOL;
		lexeme_.value = c;
		state_ = &Lexer::Initial;
		return true;
	}

	throw std::runtime_error(std::to_string(line) + ":" + std::to_string(pos) +
				": LexicalError invalid character " + std::string(1, c));
}

bool Lexer::CompSigns(Lexer::Char c) {
	state_ = &Lexer::Initial;
	if (c == '=') {
		lexeme_.value.push_back(c);
		auto comp = EqComparison.find(lexeme_.value);
		lexeme_.type = comp->second;
		return true;
	}
	if (lexeme_.value[0] == '!') {
		throw std::runtime_error(std::to_string(line) + ":" + std::to_string(pos) +
				 ": LexicalError invalid character " + std::string(1, c));
	}
	Unget();
	return true;
}

bool Lexer::BlockStart(Lexer::Char c) {
	if (c != ' ') {
		Unget();
		state_ = &Lexer::Initial;
	}
	return false;
}

bool Lexer::NoIntegerPart(Lexer::Char c) {
	if (c >= '0' && c <= '9') {
		lexeme_.value.push_back(c);
		state_ = &Lexer::Float;
		return false;
	}

	throw std::runtime_error(std::to_string(line) + ":" + std::to_string(pos) +
				": LexicalError invalid character " + std::string(1, c));
}

bool Lexer::Float(Lexer::Char c) {
	if (c >= '0' && c <= '9') {
		lexeme_.value.push_back(c);
		state_ = &Lexer::Float;
		return false;
	}
	if (c == 'e' || c == 'E') {
		lexeme_.value.push_back(c);
		state_ = &Lexer::SignEFloat;
		return false;
	}
	if (lexeme_.value.back() == '.') {
		lexeme_.value.push_back('0');
	}
	lexeme_.type = Lexeme::FloatConst;
	state_ = &Lexer::Initial;
    Unget();
	return true;
}

bool Lexer::Zero(Lexer::Char c) {
	if (c == '.') {
		lexeme_.value.push_back(c);
		state_ = &Lexer::Float;
		return false;
	}
	if (c == 'e' || c == 'E') {
		lexeme_.value.push_back(c);
		state_ = &Lexer::SignEFloat;
		return false;
	}
	if (c == '0') {
		pos++;
		c = input_.get();
		return false;
	}
	lexeme_.type = Lexeme::IntegerConst;
	state_ = &Lexer::Initial;
    Unget();
	return true;
}

bool Lexer::Integer(Lexer::Char c) {
	if (c >= '0' && c <= '9') {
		lexeme_.value.push_back(c);
		state_ = &Lexer::Integer;
		return false;
	}
	if (c == 'e' || c == 'E') {
		lexeme_.value.push_back(c);
		state_ = &Lexer::SignEFloat;
		return false;
	}
	if (c == '.') {
		lexeme_.value.push_back(c);
		state_ = &Lexer::Float;
		return false;
	}
	lexeme_.type = Lexeme::IntegerConst;
	state_ = &Lexer::Initial;
    Unget();
	return true;
}

bool Lexer::SignEFloat(Lexer::Char c) {
	if (c == '+' || c == '-') {
		lexeme_.value.push_back(c);
		state_ = &Lexer::EFloat;
		return false;
	}
	if (c >= '0' && c <= '9') {
		lexeme_.value.push_back(c);
		state_ = &Lexer::FullEFloat;
		return false;
	}

	throw std::runtime_error(std::to_string(line) + ":" + std::to_string(pos) +
				": LexicalError invalid character " + std::string(1, c));
}

bool Lexer::EFloat(Lexer::Char c) {
	if (c >= '0' && c <= '9') {
		lexeme_.value.push_back(c);
		state_ = &Lexer::FullEFloat;
		return false;
	}

	throw std::runtime_error(std::to_string(line) + ":" + std::to_string(pos) +
				": LexicalError invalid character " + std::string(1, c));
}

bool Lexer::FullEFloat(Char c) {
	if (c >= '0' && c <= '9') {
		lexeme_.value.push_back(c);
		return false;
	}

	Unget();
	state_ = &Lexer::Initial;
	lexeme_.type = Lexeme::FloatConst;
	return true;
}

bool Lexer::Variable(Lexer::Char c) {
	if (IsVar(c) || (c >= '0' && c <= '9')) {
		lexeme_.value.push_back(c);
		return false;
	}

	auto keyword = ReservedWords.find(lexeme_.value);
	if (keyword != ReservedWords.end()) {
		lexeme_.type = keyword->second;
	}
	else {
		lexeme_.type = Lexeme::Identifier;
	}

	state_ = &Lexer::Initial;
    Unget();
	return true;
}

bool Lexer::String(Lexer::Char c) {
	if (c == '\\') {
		state_ = &Lexer::ScreenSymbol;
		return false;
	}

	if (c == lexeme_.value[0]) {
		state_ = &Lexer::Initial;
		lexeme_.type = Lexeme::StringConst;
		lexeme_.value.erase(0, 1);
		return true;
	}

	if (c != '\n' && c != std::istream::traits_type::eof() && IsRelevant(c)) {
		lexeme_.value.push_back(c);
		return false;
	}

	if (!IsRelevant(c)) {
		throw std::runtime_error(std::to_string(line) + ":" + std::to_string(pos) +
				": LexicalError invalid character " + std::string(1, c));
	}

	throw std::runtime_error(std::to_string(line) + ":" + std::to_string(pos - 1) +
			": lexical : missing terminating " + std::string(1, lexeme_.value[0]) + " character");
}

bool Lexer::ScreenSymbol(Lexer::Char c) {
	state_ = &Lexer::String;
	lexeme_.value.push_back(c);
	return false;
}

// bool Lexer::LineComments(Lexer::Char c) {
// 	if (c == '\n') {
// 		Unget();
// 		state_ = &Lexer::Initial;
// 	}
// 	return false;
// }