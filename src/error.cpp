// Copyright 2025 Caden Crowson

#include "../include/error.h"

#include <exception>
#include <string>

UnexpectedTokenError::UnexpectedTokenError(const std::string& source,
                                           const std::string& unexpected,
                                           const std::string& expected)
    : message("Unexpected token " + unexpected + " in " + source +
              ". Expected " + expected) {}

const char* UnexpectedTokenError::what() const noexcept {
    return message.c_str();
}

UnexpectedEOFError::UnexpectedEOFError(const std::string& source,
                                       const std::string& expected)
    : message("Unexpected end of file in " + source + ". Expected " +
              expected) {}

const char* UnexpectedEOFError::what() const noexcept {
    return message.c_str();
}
