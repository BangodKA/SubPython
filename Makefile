all:
	clang++ -Wall interpreter.cpp poliz/poliz.cpp parser/parser.cpp lexer/lexer.cpp base_files/lexemes.cpp -o interpreter -std=c++17 && ./interpreter prog_files/prog.py