// Copyright 2025 Caden Crowson

#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "runtime/interpreter.h"

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

    interpret(filename, argc - 2, args);

    while (isGuiRunning())
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    return 0;
}
