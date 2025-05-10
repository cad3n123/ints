// Copyright 2025 Caden Crowson

#pragma once

#include <exception>
#include <string>

class UnexpectedTokenError : public std::exception {
    std::string message;

 public:
    UnexpectedTokenError(const std::string& source,
                         const std::string& unexpected,
                         const std::string& expected);
    const char* what() const noexcept override;
};

class UnexpectedEOFError : public std::exception {
    std::string message;

 public:
    explicit UnexpectedEOFError(const std::string& source,
                                const std::string& expected);
    const char* what() const noexcept override;
};
