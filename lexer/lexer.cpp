#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "../base_files/operators.hpp"
#include "../base_files/lexemes.hpp"
#include "lexer.hpp"

int Lexer::GetLine() const {
	return line_;
}
int Lexer::GetPos() const {
	return pos_;
}

void Lexer::Unget() {
	input_.unget();
	pos_--;
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
		pos_++;
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
		lexeme_.indent_amount++;
		return false;
	}

	if (c == '\n') {
		pos_ = 0;
		line_++;
		lexeme_.indent_amount = 0;
		return false;
	}

	if (c == std::istream::traits_type::eof()) {
		return false;
	}

	if (c == '#') {
		state_ = &Lexer::LineComments;
		return false;
	}

	lexeme_.type = Lexeme::IndentSpace;

	state_ = &Lexer::Initial;
	Unget();
	return true;
}

bool Lexer::Initial(Lexer::Char c) {
	if (c == ' ' || c == std::istream::traits_type::eof()) {
    	return false;
  	}

	if (c == '\n') {
		lexeme_.type = Lexeme::EOL;
		state_ = &Lexer::LineStart;
		Unget();
		return true;
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
		state_ = &Lexer::LineComments;
		return false;
	}

	throw std::runtime_error(std::to_string(line_) + ":" + std::to_string(pos_) +
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
		throw std::runtime_error(std::to_string(line_) + ":" + std::to_string(pos_) +
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

	throw std::runtime_error(std::to_string(line_) + ":" + std::to_string(pos_) +
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
		pos_++;
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

	throw std::runtime_error(std::to_string(line_) + ":" + std::to_string(pos_) +
				": LexicalError invalid character " + std::string(1, c));
}

bool Lexer::EFloat(Lexer::Char c) {
	if (c >= '0' && c <= '9') {
		lexeme_.value.push_back(c);
		state_ = &Lexer::FullEFloat;
		return false;
	}

	throw std::runtime_error(std::to_string(line_) + ":" + std::to_string(pos_) +
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
		throw std::runtime_error(std::to_string(line_) + ":" + std::to_string(pos_) +
				": LexicalError invalid character " + std::string(1, c));
	}

	throw std::runtime_error(std::to_string(line_) + ":" + std::to_string(pos_ - 1) +
			": lexical : missing terminating " + std::string(1, lexeme_.value[0]) + " character");
}

bool Lexer::ScreenSymbol(Lexer::Char c) {
	state_ = &Lexer::String;
	lexeme_.value.push_back(c);
	return false;
}

bool Lexer::LineComments(Lexer::Char c) {
	if (c == '\n' || c == std::istream::traits_type::eof()) {
		Unget();
		state_ = &Lexer::LineStart;
	}
	return false;
}