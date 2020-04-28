all:
	clang++ -Wall python.cpp poliz/poliz.cpp parser/parser.cpp lexer/lexer.cpp base_files/lexemes.cpp base_files/print.cpp -o python -std=c++17 && ./python prog_files/prog.py