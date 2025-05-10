// Copyright 2025 Caden Crowson

#pragma once

#include <string>
#include <vector>

enum class TokenType { IDENTIFIER, INT_LIT, STRING_LIT, SYMBOL };
class Token {
 public:
    Token(TokenType type, std::string value);
    operator std::string();
    bool operator==(const Token& other) const;
    const TokenType& getType() const;
    const std::string& getValue() const;

 private:
    TokenType type;
    std::string value;
};

std::vector<Token> tokenize(const std::string& code);
std::string tokenTypeToString(const TokenType& type);
