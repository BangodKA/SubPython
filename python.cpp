#include <iostream>
#include <stdexcept>

#include "parser/parser.hpp"
#include "base_files/interpret.hpp"

int main(int argc, char* argv[]) {
	return Python(argc, argv);
}