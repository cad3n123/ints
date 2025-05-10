// Copyright 2025 Caden Crowson

#include <iostream>
#include <string>
#include <utility>

#include "lexer/tokenize.h"
#include "parser/parse.h"
#include "runtime/interpreter.h"
#include "util/file.h"

int main() {
    const std::string code = readCode("test.ints");
    auto tokens = tokenize(code);
    auto root = RootNode::parse(tokens);
    // std::cout << std::string(root) << '\n';
    interpret(root);

    return 0;
}
