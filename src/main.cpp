// Copyright 2025 Caden Crowson

#include <iostream>
#include <string>
#include <utility>

#include "../include/file.h"
#include "../include/interpreter.h"
#include "../include/parse.h"
#include "../include/tokenize.h"

int main() {
    const std::string code = readCode("test.ints");
    auto tokens = tokenize(code);
    auto root = RootNode::parse(tokens);
    // std::cout << std::string(root) << '\n';
    interpret(root);

    return 0;
}
