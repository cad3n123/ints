// Copyright 2025 Caden Crowson

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "lexer/tokenize.h"
#include "parser/parse.h"
#include "runtime/interpreter.h"
#include "util/file.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filename> [args...]\n";
        return 1;
    }

    const std::string filename = argv[1];
    std::vector<std::string> args;
    for (int i = 2; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    const std::string code = readCode(filename);
    auto tokens = tokenize(code);
    auto root = RootNode::parse(tokens);
    interpret(root, args);

    return 0;
}
