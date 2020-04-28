all:
	clang++ -Wall interpreter.cpp poliz/poliz.cpp parser/parser.cpp lexer/lexer.cpp base_files/lexemes.cpp base_files/print.cpp -o interpreter -std=c++17 && ./interpreter prog_files/prog.py