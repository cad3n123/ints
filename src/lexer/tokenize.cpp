// Copyright 2025 Caden Crowson

#include "lexer/tokenize.h"

#include <cctype>
#include <stdexcept>
#include <string>
#include <vector>

#include "util/error.h"

static std::string collectWhile(const std::string& code, size_t& i,
                                size_t& char_num, int (*predicate)(int)) {
    std::string result;
    --i;
    --char_num;
    while (++i < code.size() && predicate(code[i]) != 0) {
        result.push_back(code[i]);
        ++char_num;
    }
    --i;
    --char_num;
    return result;
}

static std::string interpretEscapes(const std::string& raw) {
    std::string result;
    result.reserve(raw.size());  // optimize for performance

    for (size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] == '\\' && i + 1 < raw.size()) {
            char next = raw[i + 1];
            switch (next) {
                case 'n':
                    result += '\n';
                    break;
                case 't':
                    result += '\t';
                    break;
                case 'r':
                    result += '\r';
                    break;
                case '\\':
                    result += '\\';
                    break;
                case '"':
                    result += '\"';
                    break;
                case '\'':
                    result += '\'';
                    break;
                case '0':
                    result += '\0';
                    break;
                default:
                    throw std::runtime_error(
                        "Unexpected character after '\\': '" +
                        std::string(1, next) + "'");
                    break;
            }
            ++i;  // skip next character
        } else {
            result += raw[i];
        }
    }

    return result;
}

std::vector<Token> tokenize(const std::string& code) {
    const std::string symbols = "[]-><{}:+!=*/%;().,";
    std::vector<Token> result;
    size_t char_num = 1;
    size_t line_num = 1;

    const size_t code_length = code.length();
    for (size_t i = 0; i < code_length; i++) {
        const char c = code[i];
        if (std::isalpha(c)) {
            auto identifier = collectWhile(code, i, char_num, std::isalnum);
            result.emplace_back(TokenType::IDENTIFIER, identifier);
        } else if (std::isdigit(c) || (c == '-' && i + 1 < code_length &&
                                       std::isdigit(code[i + 1]))) {
            ++i;
            ++char_num;
            auto int_lit = std::string(1, c) +
                           collectWhile(code, i, char_num, std::isdigit);
            result.emplace_back(TokenType::INT_LIT, int_lit);
        } else if (c == '"') {
            std::string string_lit;
            ++i;
            ++char_num;
            while (i < code_length && (code[i] != '"' || code[i - 1] == '\\')) {
                string_lit += code[i++];
                ++char_num;
            }
            if (i >= code_length)
                throw UnexpectedEOFError(
                    "String Literal at line " + std::to_string(line_num) +
                        ", char " + std::to_string(char_num),
                    "\"");
            result.emplace_back(TokenType::STRING_LIT,
                                interpretEscapes(string_lit));
        } else if (c == '\n') {
            ++line_num;
            char_num = 1;
        } else if (symbols.find(c) != std::string::npos) {
            result.emplace_back(TokenType::SYMBOL, std::string(1, c));
        } else if (!std::isspace(c)) {
            throw std::runtime_error("Unexpected character '" +
                                     std::string(1, c) + "' at line " +
                                     std::to_string(line_num) + ", char " +
                                     std::to_string(char_num));
        }
        char_num++;
    }

    return result;
}

std::string tokenTypeToString(const TokenType& type) {
    switch (type) {
        case TokenType::IDENTIFIER:
            return "IDENTIFIER";
        case TokenType::INT_LIT:
            return "INT_LIT";
        case TokenType::STRING_LIT:
            return "STRING_LIT";
        case TokenType::SYMBOL:
            return "SYMBOL";
        default:
            return "UNKNOWN";
    }
}
Token::Token(TokenType type, std::string value) : type(type), value(value) {}

Token::operator std::string() {
    return tokenTypeToString(type) + " - " + value;
}

bool Token::operator==(const Token& other) const {
    return type == other.getType() && value == other.getValue();
}
const TokenType& Token::getType() const { return type; }

const std::string& Token::getValue() const { return value; }
