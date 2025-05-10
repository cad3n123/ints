// Copyright 2025 Caden Crowson

#include "../include/file.h"
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>

std::string readCode(const std::string &path) {
    std::ifstream file(path,
        std::ios::in  // Read
        | std::ios::binary  // Exact byte for byte read
        | std::ios::ate);  // Immediately seek to end of file

    if (!file)
        throw std::runtime_error("Failed to open file: " + path);

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string result(size, '\0');
    if (!file.read(&result[0], size))
        throw std::runtime_error("Failed to read file: " + path);

    return result;
}
