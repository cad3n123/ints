// Copyright 2025 Caden Crowson

#include "parser/parse.h"

#include <iostream>
#include <memory>
#include <stack>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "util/error.h"

static const Token& expect(const std::vector<Token>& tokens, size_t i,
                           const std::string& source, TokenType type,
                           const std::string& expectedValue = "") {
    if (i >= tokens.size()) {
        throw UnexpectedEOFError(
            source, expectedValue.empty() ? "token" : expectedValue);
    }
    const Token& token = tokens[i];
    if (token.getType() != type) {
        throw UnexpectedTokenError(
            source, token.getValue(),
            expectedValue.empty() ? tokenTypeToString(type) : expectedValue);
    }
    if (!expectedValue.empty() && token.getValue() != expectedValue) {
        throw UnexpectedTokenError(source, token.getValue(), expectedValue);
    }
    return token;
}

static std::vector<std::shared_ptr<ExpressionNode>> parseExpressions(
    std::vector<Token>& tokens, size_t& i, char end = ')') {
    std::vector<std::shared_ptr<ExpressionNode>> expressions;
    while (i < tokens.size() && !(tokens[i].getType() == TokenType::SYMBOL &&
                                  tokens[i].getValue()[0] == end)) {
        expressions.push_back(
            std::make_shared<ExpressionNode>(ExpressionNode::parse(tokens, i)));
        if (i >= tokens.size())
            throw UnexpectedEOFError("Expression",
                                     ", or " + std::string(1, end));
        if (tokens[i].getType() == TokenType::SYMBOL &&
            tokens[i].getValue()[0] == ',')
            ++i;
    }
    return expressions;
}

static std::string nTabs(size_t n) {
    std::string result;
    std::string tab = "    ";
    result.reserve(tab.size());
    for (; n > 0; n--) result += tab;
    return result;
}

RootNode RootNode::parse(std::vector<Token>& tokens) {
    std::vector<std::variant<
        std::shared_ptr<VariableBindingNode>, std::shared_ptr<FunctionCallNode>,
        std::shared_ptr<FunctionDefinitionNode>, std::shared_ptr<UseNode>>>
        values;
    size_t numTokens = tokens.size();
    size_t i = 0;
    while (i < numTokens) {
        switch (tokens[i].getType()) {
            case TokenType::IDENTIFIER:
                if (tokens[i].getValue() == "fn") {
                    values.push_back(std::make_shared<FunctionDefinitionNode>(
                        FunctionDefinitionNode::parse(tokens, i)));
                    break;
                } else if (tokens[i].getValue() == "use") {
                    values.push_back(
                        std::make_shared<UseNode>(UseNode::parse(tokens, i)));
                    break;
                } else {
                    if (i + 1 < tokens.size() &&
                        tokens[i + 1] == Token(TokenType::SYMBOL, "(")) {
                        values.push_back(std::make_shared<FunctionCallNode>(
                            FunctionCallNode::parse(tokens, i)));
                    } else {
                        values.push_back(std::make_shared<VariableBindingNode>(
                            VariableBindingNode::parse(tokens, i)));
                    }

                    expect(tokens, i, "TODO", TokenType::SYMBOL, ";");
                    ++i;
                    break;
                }
                [[fallthrough]];
            default:
                throw std::runtime_error("Unexpected value " +
                                         tokens[i].getValue() +
                                         ". Expected let, use, or fn");
        }
    }
    return RootNode(std::move(values));
}

RootNode::operator std::string() const {
    std::string result;
    for (auto& value : values) {
        std::visit(
            [&result](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                constexpr bool isVariableBinding =
                    std::is_same_v<T, std::shared_ptr<VariableBindingNode>>;
                constexpr bool isFunctionCall =
                    std::is_same_v<T, std::shared_ptr<FunctionCallNode>>;
                constexpr bool isFunctionDef =
                    std::is_same_v<T, std::shared_ptr<FunctionDefinitionNode>>;
                constexpr bool isUse =
                    std::is_same_v<T, std::shared_ptr<UseNode>>;
                if constexpr (isVariableBinding) {
                    result += std::string(*arg) + "\n";
                } else if constexpr (isFunctionCall) {
                    result += std::string(*arg) + "\n";
                } else if constexpr (isFunctionDef) {
                    result += arg->toStringIndented(0) + "\n";
                } else if constexpr (isUse) {
                    result += arg->toStringIndented(0) + "\n";
                }
            },
            value);
    }
    return result;
}

RootNode::RootNode(
    std::vector<std::variant<
        std::shared_ptr<VariableBindingNode>, std::shared_ptr<FunctionCallNode>,
        std::shared_ptr<FunctionDefinitionNode>, std::shared_ptr<UseNode>>>
        values)
    : values(std::move(values)) {}

FunctionParameterNode FunctionParameterNode::parse(std::vector<Token>& tokens,
                                                   size_t& i) {
    auto identifier =
        expect(tokens, i, "Function Parameter", TokenType::IDENTIFIER)
            .getValue();
    ++i;

    expect(tokens, i, "Function Definition", TokenType::SYMBOL, ":");
    ++i;
    return FunctionParameterNode(identifier, ArrayDescriptor::parse(tokens, i));
}

FunctionParameterNode::FunctionParameterNode(std::string identifier,
                                             ArrayDescriptor descriptor)
    : identifier(std::move(identifier)), descriptor(std::move(descriptor)) {}

FunctionDefinitionNode::FunctionDefinitionNode(
    std::string identifier,
    std::vector<std::shared_ptr<FunctionParameterNode>> params,
    ArrayDescriptor output, std::shared_ptr<BodyNode> body)
    : identifier(std::move(identifier)),
      params(params),
      output(output),
      body(std::move(body)) {}

FunctionDefinitionNode FunctionDefinitionNode::parse(std::vector<Token>& tokens,
                                                     size_t& i) {
    expect(tokens, i, "Function Definition", TokenType::IDENTIFIER, "fn");
    ++i;

    auto identifier =
        expect(tokens, i, "Function Definition", TokenType::IDENTIFIER)
            .getValue();
    ++i;

    expect(tokens, i, "Function Definition", TokenType::SYMBOL, "(");
    ++i;

    std::vector<std::shared_ptr<FunctionParameterNode>> params;
    while (i < tokens.size() && !(tokens[i] == Token(TokenType::SYMBOL, ")"))) {
        params.emplace_back(std::make_shared<FunctionParameterNode>(
            FunctionParameterNode::parse(tokens, i)));
        if (i < tokens.size() && tokens[i] == Token(TokenType::SYMBOL, ","))
            ++i;
    }
    expect(tokens, i, "Function Definition", TokenType::SYMBOL, ")");
    ++i;

    expect(tokens, i, "Function Definition", TokenType::SYMBOL, "-");
    ++i;
    expect(tokens, i, "Function Definition", TokenType::SYMBOL, ">");
    ++i;

    ArrayDescriptor output = ArrayDescriptor::parse(tokens, i);
    std::shared_ptr<BodyNode> body = BodyNode::parse(tokens, i);

    return FunctionDefinitionNode(identifier, params, output, std::move(body));
}

ArrayDescriptor ArrayDescriptor::parse(std::vector<Token>& tokens, size_t& i) {
    expect(tokens, i, "Array Descriptor", TokenType::SYMBOL, "[");
    ++i;

    std::optional<size_t> size;
    if (i < tokens.size() && tokens[i].getType() == TokenType::INT_LIT) {
        size = std::stoi(tokens[i].getValue());
        ++i;
    }

    expect(tokens, i, "Array Descriptor", TokenType::SYMBOL);
    bool canGrow =
        tokens[i].getValue().size() > 0 && tokens[i].getValue()[0] == '+';
    if (canGrow) ++i;
    expect(tokens, i, "Array Descriptor", TokenType::SYMBOL, "]");
    ++i;

    return ArrayDescriptor(size, canGrow);
}

ArrayDescriptor::ArrayDescriptor(std::optional<size_t> size, bool canGrow)
    : size(std::move(size)), canGrow(canGrow) {}

std::shared_ptr<BodyNode> BodyNode::parse(std::vector<Token>& tokens,
                                          size_t& i) {
    expect(tokens, i, "Body", TokenType::SYMBOL, "{");
    ++i;
    std::vector<std::shared_ptr<StatementNode>> statements;
    while (i < tokens.size() && !(tokens[i].getType() == TokenType::SYMBOL &&
                                  tokens[i].getValue()[0] == '}')) {
        statements.push_back(StatementNode::parse(tokens, i));
    }
    expect(tokens, i, "Body", TokenType::SYMBOL, "}");
    ++i;
    return std::make_shared<BodyNode>(std::move(statements));
}

BodyNode::BodyNode(std::vector<std::shared_ptr<StatementNode>> statements)
    : statements(std::move(statements)) {}

StatementNode::StatementNode(
    std::variant<std::shared_ptr<VariableBindingNode>,
                 std::shared_ptr<ForLoopNode>, std::shared_ptr<IfNode>,
                 std::shared_ptr<WhileNode>, std::shared_ptr<FunctionCallNode>,
                 std::shared_ptr<ReturnNode>>
        value)
    : value(std::move(value)) {}

std::shared_ptr<StatementNode> StatementNode::parse(std::vector<Token>& tokens,
                                                    size_t& i) {
    if (i >= tokens.size()) throw UnexpectedEOFError("Statement", "token");

    std::shared_ptr<StatementNode> result;
    if (tokens[i].getType() == TokenType::IDENTIFIER) {
        auto identifier = tokens[i].getValue();
        if (identifier == "if") {
            result = std::make_shared<StatementNode>(
                std::make_shared<IfNode>(IfNode::parse(tokens, i)));
        } else if (identifier == "for") {
            result = std::make_shared<StatementNode>(
                std::make_shared<ForLoopNode>(ForLoopNode::parse(tokens, i)));
        } else if (identifier == "while") {
            result = std::make_shared<StatementNode>(
                std::make_shared<WhileNode>(WhileNode::parse(tokens, i)));
        } else if (identifier == "return") {
            result = std::make_shared<StatementNode>(
                std::make_shared<ReturnNode>(ReturnNode::parse(tokens, i)));
        } else {
            if (i + 1 < tokens.size() &&
                tokens[i + 1] == Token(TokenType::SYMBOL, "(")) {
                result = std::make_shared<StatementNode>(
                    std::make_shared<FunctionCallNode>(
                        FunctionCallNode::parse(tokens, i)));
            } else {
                result = std::make_shared<StatementNode>(
                    std::make_shared<VariableBindingNode>(
                        VariableBindingNode::parse(tokens, i)));
            }

            expect(tokens, i, "Statement", TokenType::SYMBOL, ";");
            ++i;
        }
    } else {
        throw UnexpectedTokenError(
            "Statement", tokens[i].getValue(),
            "Identifier. Previous token: " + tokens[i - 1].getValue());
    }

    return result;
}

ReturnNode ReturnNode::parse(std::vector<Token>& tokens, size_t& i) {
    expect(tokens, i, "Return", TokenType::IDENTIFIER, "return");
    ++i;
    auto expression =
        std::make_shared<ExpressionNode>(ExpressionNode::parse(tokens, i));
    expect(tokens, i, "Return", TokenType::SYMBOL, ";");
    ++i;
    return ReturnNode(std::move(expression));
}

ReturnNode::ReturnNode(std::shared_ptr<ExpressionNode> value)
    : value(std::move(value)) {}

IfNode::IfNode(std::variant<std::shared_ptr<IfCompareNode>,
                            std::shared_ptr<IfDeclarationNode>>
                   condition,
               std::shared_ptr<BodyNode> body,
               std::optional<std::shared_ptr<IfNode>> elseIfBranches,
               std::optional<std::shared_ptr<BodyNode>> elseBody)
    : condition(std::move(condition)),
      body(std::move(body)),
      elseIfBranches(std::move(elseIfBranches)),
      elseBody(std::move(elseBody)) {}

IfNode IfNode::parse(std::vector<Token>& tokens, size_t& i) {
    expect(tokens, i, "If", TokenType::IDENTIFIER, "if");
    ++i;
    if (i >= tokens.size()) throw UnexpectedEOFError("If", "token");

    std::variant<std::shared_ptr<IfCompareNode>,
                 std::shared_ptr<IfDeclarationNode>>
        condition =
            (tokens[i].getType() == TokenType::IDENTIFIER &&
             tokens[i].getValue() == "let")
                ? std::variant<
                      std::shared_ptr<IfCompareNode>,
                      std::shared_ptr<IfDeclarationNode>>{std::make_shared<
                      IfDeclarationNode>(IfDeclarationNode::parse(tokens, i))}
                : std::variant<std::shared_ptr<IfCompareNode>,
                               std::shared_ptr<IfDeclarationNode>>{
                      std::make_shared<IfCompareNode>(
                          IfCompareNode::parse(tokens, i))};

    std::shared_ptr<BodyNode> body = BodyNode::parse(tokens, i);
    std::optional<std::shared_ptr<IfNode>> elseIfBranches;
    std::optional<std::shared_ptr<BodyNode>> elseBody;

    if (i < tokens.size() && tokens[i].getType() == TokenType::IDENTIFIER &&
        tokens[i].getValue() == "else") {
        ++i;
        if (i >= tokens.size())
            throw UnexpectedEOFError("If", "if or { after else");
        if (tokens[i].getType() == TokenType::SYMBOL &&
            tokens[i].getValue()[0] == '{') {
            elseBody = BodyNode::parse(tokens, i);
        } else {
            elseIfBranches = std::make_shared<IfNode>(IfNode::parse(tokens, i));
        }
    }

    return IfNode(std::move(condition), std::move(body),
                  std::move(elseIfBranches), std::move(elseBody));
}

ForLoopNode ForLoopNode::parse(std::vector<Token>& tokens, size_t& i) {
    expect(tokens, i, "For Loop", TokenType::IDENTIFIER, "for");
    ++i;
    auto elementIdentifier =
        expect(tokens, i, "For Loop", TokenType::IDENTIFIER).getValue();
    ++i;
    expect(tokens, i, "For Loop", TokenType::SYMBOL, ":");
    ++i;
    auto iterable =
        std::make_shared<ExpressionNode>(ExpressionNode::parse(tokens, i));
    return ForLoopNode(std::move(elementIdentifier), std::move(iterable),
                       BodyNode::parse(tokens, i));
}

ForLoopNode::ForLoopNode(std::string element,
                         std::shared_ptr<ExpressionNode> iterable,
                         std::shared_ptr<BodyNode> body)
    : element(std::move(element)),
      iterable(std::move(iterable)),
      body(std::move(body)) {}

MethodNode MethodNode::parse(std::vector<Token>& tokens, size_t& i) {
    expect(tokens, i, "Method", TokenType::SYMBOL, ".");
    ++i;
    auto identifier =
        expect(tokens, i, "Method", TokenType::IDENTIFIER).getValue();
    ++i;
    expect(tokens, i, "Method", TokenType::SYMBOL, "(");
    ++i;
    std::vector<std::shared_ptr<ExpressionNode>> parameters =
        parseExpressions(tokens, i);
    expect(tokens, i, "Method", TokenType::SYMBOL, ")");
    ++i;
    return MethodNode(identifier, std::move(parameters));
}

MethodNode::MethodNode(std::string identifier,
                       std::vector<std::shared_ptr<ExpressionNode>> parameters)
    : identifier(std::move(identifier)), parameters(std::move(parameters)) {}

VariableBindingNode VariableBindingNode::parse(std::vector<Token>& tokens,
                                               size_t& i) {
    auto ident1 =
        expect(tokens, i, "Variable Bind", TokenType::IDENTIFIER).getValue();
    if (ident1 == "let") {
        return VariableBindingNode(std::make_shared<VariableDeclarationNode>(
            VariableDeclarationNode::parse(tokens, i)));
    } else {
        return VariableBindingNode(std::make_shared<VariableAssignmentNode>(
            VariableAssignmentNode::parse(tokens, i)));
    }
}

VariableBindingNode::VariableBindingNode(
    std::variant<std::shared_ptr<VariableDeclarationNode>,
                 std::shared_ptr<VariableAssignmentNode>>
        value)
    : value(std::move(value)) {}

FunctionCallNode FunctionCallNode::parse(std::vector<Token>& tokens,
                                         size_t& i) {
    auto identifier =
        expect(tokens, i, "Function Call", TokenType::IDENTIFIER).getValue();
    ++i;
    expect(tokens, i, "Function Call", TokenType::SYMBOL, "(");
    ++i;

    std::vector<std::shared_ptr<ExpressionNode>> parameters =
        parseExpressions(tokens, i);

    expect(tokens, i, "Function Call", TokenType::SYMBOL, ")");
    ++i;

    if (i >= tokens.size())
        throw UnexpectedEOFError("Function Call", "; or . after function call");

    return FunctionCallNode(identifier, std::move(parameters));
}

FunctionCallNode::FunctionCallNode(
    std::string identifier,
    std::vector<std::shared_ptr<ExpressionNode>> parameters)
    : identifier(std::move(identifier)), parameters(std::move(parameters)) {}

IfCompareNode IfCompareNode::parse(std::vector<Token>& tokens, size_t& i) {
    std::shared_ptr<ExpressionNode> left =
        std::make_shared<ExpressionNode>(ExpressionNode::parse(tokens, i));
    char symbol1 =
        expect(tokens, i, "If Comparison", TokenType::SYMBOL).getValue()[0];
    ++i;
    bool nextIsEq = i < tokens.size() &&
                    tokens[i].getType() == TokenType::SYMBOL &&
                    tokens[i].getValue()[0] == '=';
    if (nextIsEq) ++i;
    Type type;
    if (nextIsEq) {
        switch (symbol1) {
            case '<':
                type = Type::LE;
                break;
            case '=':
                type = Type::EQ;
                break;
            case '>':
                type = Type::GE;
                break;
            case '!':
                type = Type::NE;
                break;
            default:
                throw UnexpectedTokenError(
                    "If Comparison",
                    tokenTypeToString(tokens[i - 1].getType()) + " " +
                        std::string(1, symbol1),
                    "Comparison Operator");
                break;
        }
    } else {
        switch (symbol1) {
            case '<':
                type = Type::LT;
                break;
            case '>':
                type = Type::GT;
                break;
            default:
                throw UnexpectedTokenError(
                    "If Comparison",
                    tokenTypeToString(tokens[i].getType()) + " " +
                        std::string(1, symbol1),
                    "Comparison Operator");
                break;
        }
    }
    std::shared_ptr<ExpressionNode> right =
        std::make_shared<ExpressionNode>(ExpressionNode::parse(tokens, i));
    return IfCompareNode(type, std::move(left), std::move(right));
}

IfCompareNode::IfCompareNode(Type type, std::shared_ptr<ExpressionNode> left,
                             std::shared_ptr<ExpressionNode> right)
    : type(type), left(std::move(left)), right(std::move(right)) {}

IfDeclarationNode IfDeclarationNode::parse(std::vector<Token>& tokens,
                                           size_t& i) {
    return IfDeclarationNode(std::make_shared<VariableDeclarationNode>(
        VariableDeclarationNode::parse(tokens, i)));
}

IfDeclarationNode::IfDeclarationNode(
    std::shared_ptr<VariableDeclarationNode> variableDeclaration)
    : variableDeclaration(std::move(variableDeclaration)) {}

VariableDeclarationNode VariableDeclarationNode::parse(
    std::vector<Token>& tokens, size_t& i) {
    expect(tokens, i, "Variable Declaration", TokenType::IDENTIFIER, "let");
    ++i;
    auto identifier =
        expect(tokens, i, "Variable Declaration", TokenType::IDENTIFIER)
            .getValue();
    ++i;
    expect(tokens, i, "Variable Declaration", TokenType::SYMBOL, ":");
    ++i;
    auto descriptor = ArrayDescriptor::parse(tokens, i);
    std::optional<std::shared_ptr<ExpressionNode>> value;
    if (i < tokens.size() && tokens[i].getType() == TokenType::SYMBOL &&
        tokens[i].getValue()[0] == '=') {
        ++i;
        value =
            std::make_shared<ExpressionNode>(ExpressionNode::parse(tokens, i));
    }
    return VariableDeclarationNode(identifier, descriptor, std::move(value));
}

VariableDeclarationNode::VariableDeclarationNode(
    std::string identifier, ArrayDescriptor descriptor,
    std::optional<std::shared_ptr<ExpressionNode>> value)
    : identifier(std::move(identifier)),
      descriptor(descriptor),
      value(std::move(value)) {}

// VariableMethodNode VariableMethodNode::parse(std::vector<Token>& tokens,
//                                              size_t& i) {
//     auto identifier =
//         expect(tokens, i, "Variable Method",
//         TokenType::IDENTIFIER).getValue();
//     ++i;
//     return VariableMethodNode(identifier, MethodNode::parse(tokens, i));
// }

// VariableMethodNode::VariableMethodNode(std::string identifier,
//                                        MethodNode method)
//     : identifier(std::move(identifier)), method(std::move(method)) {}

VariableAssignmentNode VariableAssignmentNode::parse(std::vector<Token>& tokens,
                                                     size_t& i) {
    auto left = expect(tokens, i, "Variable Assignment", TokenType::IDENTIFIER)
                    .getValue();
    ++i;
    expect(tokens, i, "Variable Assignment", TokenType::SYMBOL, "=");
    ++i;
    return VariableAssignmentNode(
        std::move(left), std::make_shared<ExpressionNode>(
                             std::move(ExpressionNode::parse(tokens, i))));
}

VariableAssignmentNode::VariableAssignmentNode(
    std::string left, std::shared_ptr<ExpressionNode> right)
    : left(std::move(left)), right(std::move(right)) {}

ExpressionNode ExpressionNode::parse(std::vector<Token>& tokens, size_t& i) {
    unsigned numLeftParentheses = 0;
    std::stack<std::variant<ArithmeticNode::Type, ExpressionNode>> output;
    std::stack<std::pair<std::optional<ArithmeticNode::Type>,
                         ArithmeticNode::Precedence>>
        operators;
    while (i < tokens.size() && !(numLeftParentheses == 0 &&
                                  tokens[i] == Token(TokenType::SYMBOL, ")"))) {
        auto& token = tokens[i];
        switch (token.getType()) {
            case TokenType::IDENTIFIER:
            case TokenType::STRING_LIT: {
                auto arrayNode = ArrayNode::parse(tokens, i);
                auto postFix = ArrayPostFixNode::parse(tokens, i);
                output.push(ExpressionNode(
                    std::make_shared<ArrayNode>(std::move(arrayNode)),
                    std::move(postFix)));
                --i;
                break;
            }
            case TokenType::INT_LIT:
                throw std::runtime_error(
                    "Unexpected int literal in array expression.");
                break;
            case TokenType::SYMBOL: {
                char c = token.getValue()[0];
                if (c == '[') {
                    auto arrayNode = ArrayNode::parse(tokens, i);
                    auto postFix = ArrayPostFixNode::parse(tokens, i);
                    output.push(ExpressionNode(
                        std::make_shared<ArrayNode>(std::move(arrayNode)),
                        std::move(postFix)));
                    --i;
                } else if (c == ')') {
                    while (operators.size() > 0 &&
                           operators.top().second !=
                               ArithmeticNode::Precedence::LEFT_PARENTHESIS) {
                        output.push(operators.top().first.value());
                        operators.pop();
                    }
                    if (operators.size() == 0)
                        throw std::runtime_error(
                            "More ) than ( in array expression.");
                    operators.pop();
                    --numLeftParentheses;
                } else {
                    std::optional<ArithmeticNode::Type> newOperator;
                    ArithmeticNode::Precedence newPrecedence;
                    switch (c) {
                        case '+':
                            newOperator = ArithmeticNode::Type::TYPE_ADDITION;
                            newPrecedence = ArithmeticNode::Precedence::ADD_SUB;
                            break;
                        case '-':
                            newOperator =
                                ArithmeticNode::Type::TYPE_SUBTRACTION;
                            newPrecedence = ArithmeticNode::Precedence::ADD_SUB;
                            break;
                        case '*':
                            newOperator =
                                ArithmeticNode::Type::TYPE_MULTIPLICATION;
                            newPrecedence =
                                ArithmeticNode::Precedence::MULT_DIV;
                            break;
                        case '/':
                            newOperator = ArithmeticNode::Type::TYPE_DIVISION;
                            newPrecedence =
                                ArithmeticNode::Precedence::MULT_DIV;
                            break;
                        case '(':
                            newPrecedence =
                                ArithmeticNode::Precedence::LEFT_PARENTHESIS;
                            ++numLeftParentheses;
                            break;
                        default:
                            goto exit_loop;
                            break;
                    }
                    while (operators.size() > 0 &&
                           operators.top().second >= newPrecedence) {
                        output.push(operators.top().first.value());
                        operators.pop();
                    }
                    operators.push(std::make_pair(newOperator, newPrecedence));
                }
            } break;
        }
        ++i;
    }
exit_loop: {}
    while (!operators.empty()) {
        output.push(operators.top().first.value());
        operators.pop();
    }
    if (output.size() == 0) throw std::runtime_error("Empty expression.");
    auto top = std::move(output.top());
    output.pop();
    if (std::holds_alternative<ExpressionNode>(top)) {
        if (output.size() > 0)
            throw std::runtime_error("Invalid array expression.");
        return std::move(std::get<ExpressionNode>(top));
    }
    std::stack<ArithmeticNode> result;
    result.push(
        ArithmeticNode(nullptr, nullptr, std::get<ArithmeticNode::Type>(top)));
    while (output.size() > 0) {
        auto top = std::move(output.top());
        output.pop();
        if (std::holds_alternative<ExpressionNode>(top)) {
            if (result.top().right == nullptr) {
                result.top().right = std::make_unique<ExpressionNode>(
                    std::move(std::get<ExpressionNode>(top)));
            } else if (result.top().left == nullptr) {
                result.top().left = std::make_unique<ExpressionNode>(
                    std::move(std::get<ExpressionNode>(top)));

                while (result.size() > 1) {
                    auto current = std::move(result.top());
                    result.pop();
                    if (result.top().right == nullptr) {
                        result.top().right = std::make_unique<ExpressionNode>(
                            std::make_shared<ArithmeticNode>(
                                std::move(current)),
                            ArrayPostFixNode(
                                std::vector<std::variant<
                                    std::shared_ptr<ArrayRangeNode>,
                                    std::shared_ptr<MethodNode>>>()));
                        break;
                    } else if (result.top().left == nullptr) {
                        result.top().left = std::make_unique<ExpressionNode>(
                            std::make_shared<ArithmeticNode>(
                                std::move(current)),
                            ArrayPostFixNode(
                                std::vector<std::variant<
                                    std::shared_ptr<ArrayRangeNode>,
                                    std::shared_ptr<MethodNode>>>()));
                    } else {
                        throw std::runtime_error("Invalid array expression");
                    }
                }
            } else {
                throw std::runtime_error("Invalid array expression");
            }
        } else {
            result.push(ArithmeticNode(nullptr, nullptr,
                                       std::get<ArithmeticNode::Type>(top)));
        }
    }
    if (result.size() != 1)
        throw std::runtime_error("Invalid array expression");
    return ExpressionNode(
        std::make_shared<ArithmeticNode>(std::move(result.top())),
        ArrayPostFixNode(
            std::vector<std::variant<std::shared_ptr<ArrayRangeNode>,
                                     std::shared_ptr<MethodNode>>>()));
}

ExpressionNode::ExpressionNode(
    std::variant<std::shared_ptr<ArithmeticNode>, std::shared_ptr<ArrayNode>>
        primary,
    ArrayPostFixNode postfix)
    : primary(std::move(primary)), postfix(std::move(postfix)) {}

ArithmeticNode::ArithmeticNode(std::shared_ptr<ExpressionNode> left,
                               std::shared_ptr<ExpressionNode> right, Type type)
    : left(std::move(left)), right(std::move(right)), type(type) {}

ArrayNode ArrayNode::parse(std::vector<Token>& tokens, size_t& i) {
    std::variant<std::vector<int>, std::string,
                 std::shared_ptr<FunctionCallNode>>
        value;
    if (tokens[i].getType() == TokenType::IDENTIFIER) {
        if (i + 1 < tokens.size() &&
            tokens[i + 1] == Token(TokenType::SYMBOL, "(")) {
            value = std::make_shared<FunctionCallNode>(
                std::move(FunctionCallNode::parse(tokens, i)));
        } else {
            value = std::move(tokens[i].getValue());
            ++i;
        }
    } else {
        std::vector<int> ints;
        if (i < tokens.size() && tokens[i].getType() == TokenType::STRING_LIT) {
            ints = ArrayNode::stringToInts(tokens[i].getValue());
            ++i;
        } else {
            expect(tokens, i, "Array", TokenType::SYMBOL, "[");
            ++i;
            while (i < tokens.size() &&
                   !(tokens[i].getType() == TokenType::SYMBOL &&
                     tokens[i].getValue()[0] == ']')) {
                ints.push_back(std::stoi(
                    expect(tokens, i, "Array", TokenType::INT_LIT).getValue()));
                ++i;
                if (!(tokens[i].getType() == TokenType::SYMBOL &&
                      tokens[i].getValue()[0] == ']')) {
                    expect(tokens, i, "Array", TokenType::SYMBOL, ",");
                    ++i;
                }
            }
            expect(tokens, i, "Array", TokenType::SYMBOL, "]");
            ++i;
        }
        value = std::move(ints);
    }
    return ArrayNode(std::move(value));
}

ArrayNode::ArrayNode(std::variant<std::vector<int>, std::string,
                                  std::shared_ptr<FunctionCallNode>>
                         value)
    : value(std::move(value)) {}

ArrayRangeNode ArrayRangeNode::parse(std::vector<Token>& tokens, size_t& i) {
    expect(tokens, i, "Array Range", TokenType::SYMBOL, "[");
    ++i;
    std::optional<std::variant<size_t, std::shared_ptr<ExpressionNode>>> start;
    std::optional<std::variant<size_t, std::shared_ptr<ExpressionNode>>> end;
    if (i + 1 >= tokens.size())
        throw UnexpectedEOFError("Array Range", "Lower bound or :");
    if (tokens[i].getType() == TokenType::INT_LIT) {
        size_t startAsSizeT = std::stoi(tokens[i].getValue());
        ++i;
        if (i < tokens.size() && tokens[i] == Token(TokenType::SYMBOL, "]")) {
            ++i;
            return ArrayRangeNode(startAsSizeT, startAsSizeT + 1);
        }
        start = startAsSizeT;
    } else if (!(tokens[i] == Token(TokenType::SYMBOL, ":"))) {
        ExpressionNode startAsExpression = ExpressionNode::parse(tokens, i);
        if (i < tokens.size() && tokens[i] == Token(TokenType::SYMBOL, "]")) {
            ++i;
            ExpressionNode endAsExpression = ExpressionNode(
                std::make_shared<ArithmeticNode>(
                    std::make_shared<ExpressionNode>(startAsExpression),
                    std::make_shared<ExpressionNode>((std::vector<int>){1}),
                    ArithmeticNode::TYPE_ADDITION),
                ArrayPostFixNode(
                    std::vector<std::variant<std::shared_ptr<ArrayRangeNode>,
                                             std::shared_ptr<MethodNode>>>()));
        }
        start = std::make_shared<ExpressionNode>(startAsExpression);
    }
    expect(tokens, i, "Array Range", TokenType::SYMBOL, ":");
    ++i;
    if (i + 1 >= tokens.size())
        throw UnexpectedEOFError("Array Range", "Lower bound or :");
    if (tokens[i].getType() == TokenType::INT_LIT) {
        end = static_cast<size_t>(std::stoi(tokens[i].getValue()));
        ++i;
    } else if (!(tokens[i] == Token(TokenType::SYMBOL, "]"))) {
        end =
            std::make_shared<ExpressionNode>(ExpressionNode::parse(tokens, i));
    }
    expect(tokens, i, "Array Range", TokenType::SYMBOL, "]");
    ++i;

    return ArrayRangeNode(start, end);
}

ArrayRangeNode::ArrayRangeNode(
    std::optional<std::variant<size_t, std::shared_ptr<ExpressionNode>>> start,
    std::optional<std::variant<size_t, std::shared_ptr<ExpressionNode>>> end)
    : start(std::move(start)), end(std::move(end)) {}

std::string FunctionDefinitionNode::toStringIndented(size_t indent) const {
    std::string result = nTabs(indent) + "fn " + identifier + "(";
    for (auto& param : params) result += std::string(*param);

    result +=
        ") -> " + std::string(output) + " " + body->toStringIndented(indent);
    return result;
}

std::string BodyNode::toStringIndented(size_t indent) const {
    std::string result = "{\n";
    for (auto& statement : statements)
        result += statement->toStringIndented(indent + 1) + "\n";
    result += nTabs(indent) + "}";
    return result;
}

std::string StatementNode::toStringIndented(size_t indent) const {
    std::string result = nTabs(indent);
    std::visit(
        [&result, indent](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            constexpr bool isForLoop =
                std::is_same_v<T, std::shared_ptr<ForLoopNode>>;
            constexpr bool isWhile =
                std::is_same_v<T, std::shared_ptr<WhileNode>>;
            constexpr bool isIf = std::is_same_v<T, std::shared_ptr<IfNode>>;
            constexpr bool isFunctionCall =
                std::is_same_v<T, std::shared_ptr<FunctionCallNode>>;
            constexpr bool isReturn =
                std::is_same_v<T, std::shared_ptr<ReturnNode>>;
            if constexpr (std::is_same_v<
                              T, std::shared_ptr<VariableBindingNode>>) {
                result += std::string(*arg) + ";";
            } else if constexpr (isForLoop) {
                result += arg->toStringIndented(indent);
            } else if constexpr (isWhile) {
                result += arg->toStringIndented(indent);
            } else if constexpr (isIf) {
                result += arg->toStringIndented(indent);
            } else if constexpr (isFunctionCall) {
                result += std::string(*arg) + ";";
            } else if constexpr (isReturn) {
                result += std::string(*arg) + ";";
            }
        },
        value);
    return result;
}

std::string ForLoopNode::toStringIndented(size_t indent) const {
    return "for " + element + " : " + std::string(*iterable) + " " +
           body->toStringIndented(indent);
}

VariableBindingNode::operator std::string() const {
    std::string result;
    std::visit(
        [&result](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            constexpr bool isDeclaration =
                std::is_same_v<T, std::shared_ptr<VariableDeclarationNode>>;
            constexpr bool isAssignment =
                std::is_same_v<T, std::shared_ptr<VariableAssignmentNode>>;
            if constexpr (isDeclaration) {
                result = std::string(*arg);
            } else if constexpr (isAssignment) {
                result = std::string(*arg);
            }
        },
        value);
    return result;
}

VariableDeclarationNode::operator std::string() const {
    std::string result = "let " + identifier + ": " + std::string(descriptor);
    if (value.has_value()) {
        result += " = " + std::string(*value.value());
    }
    return result;
}

VariableAssignmentNode::operator std::string() const {
    return left + " = " + std::string(*right);
}

MethodNode::operator std::string() const {
    std::string result = "." + identifier + "(";
    size_t numParameters = parameters.size();
    for (size_t i = 0; i < numParameters; i++) {
        result +=
            std::string(*parameters[i]) + (i + 1 < numParameters ? ", " : "");
    }
    result += ")";
    return result;
}

std::string IfNode::toStringIndented(size_t indent) const {
    std::string result = "if ";
    std::visit(
        [&result](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            constexpr bool isCompare =
                std::is_same_v<T, std::shared_ptr<IfCompareNode>>;
            constexpr bool isDeclaration =
                std::is_same_v<T, std::shared_ptr<IfDeclarationNode>>;
            if constexpr (isCompare) {
                result += std::string(*arg);
            } else if constexpr (isDeclaration) {
                result += std::string(*arg);
            }
        },
        condition);
    result += " " + body->toStringIndented(indent);
    if (elseIfBranches.has_value()) {
        result += " else " + elseIfBranches.value()->toStringIndented(indent);
    }
    if (elseBody.has_value()) {
        result += " else " + elseBody.value()->toStringIndented(indent);
    }
    return result;
}

IfCompareNode::operator std::string() const {
    std::string comparison;
    switch (type) {
        case Type::EQ:
            comparison = " == ";
            break;
        case Type::NE:
            comparison = " != ";
            break;

        case Type::LT:
            comparison = " < ";
            break;
        case Type::LE:
            comparison = " <= ";
            break;
        case Type::GT:
            comparison = " > ";
            break;
        case Type::GE:
            comparison = " >= ";
            break;
    }

    return std::string(*left) + comparison + std::string(*right);
}

IfDeclarationNode::operator std::string() const {
    return std::string(*variableDeclaration);
}

ReturnNode::operator std::string() const {
    return "return " + std::string(*value);
}

ExpressionNode::operator std::string() const {
    std::string result;
    std::visit(
        [&result](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            constexpr bool isArithmetic =
                std::is_same_v<T, std::shared_ptr<ArithmeticNode>>;
            constexpr bool isArray =
                std::is_same_v<T, std::shared_ptr<ArrayNode>>;
            if constexpr (isArithmetic) {
                result = std::string(*arg);
            } else if constexpr (isArray) {
                result = std::string(*arg);
            }
        },
        primary);
    result += postfix;
    return result;
}

ArithmeticNode::operator std::string() const {
    std::string math_op;
    switch (type) {
        case Type::TYPE_ADDITION:
            math_op += " + ";
            break;
        case Type::TYPE_SUBTRACTION:
            math_op += " - ";
            break;
        case Type::TYPE_MULTIPLICATION:
            math_op += " * ";
            break;
        case Type::TYPE_DIVISION:
            math_op += " / ";
            break;
        default:
            break;
    }
    return "(" + std::string(*left) + math_op + std::string(*right) + ")";
}

ArrayNode::operator std::string() const {
    std::string result;
    std::visit(
        [&result](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            constexpr bool isVector = std::is_same_v<T, std::vector<int>>;
            constexpr bool isString = std::is_same_v<T, std::string>;
            constexpr bool isFunctionCall =
                std::is_same_v<T, std::shared_ptr<FunctionCallNode>>;
            if constexpr (isVector) {
                size_t numInts = arg.size();
                result = "[";
                for (size_t i = 0; i < numInts; i++) {
                    result +=
                        std::to_string(arg[i]) + (i + 1 < numInts ? ", " : "");
                }
                result += "]";
            } else if constexpr (isString) {
                result = arg;
            } else if constexpr (isFunctionCall) {
                result = std::string(*arg);
            }
        },
        value);
    return result;
}

FunctionCallNode::operator std::string() const {
    std::string result = identifier + "(";
    size_t numParams = parameters.size();
    for (size_t i = 0; i < numParams; i++) {
        result += std::string(*parameters[i]) + (i + 1 < numParams ? ", " : "");
    }
    result += ")";
    return result;
}

ArrayDescriptor::operator std::string() const {
    std::string result = "[";
    if (size.has_value()) result += std::to_string(size.value());
    if (canGrow) result += "+";
    result += "]";
    return result;
}

ArrayPostFixNode ArrayPostFixNode::parse(std::vector<Token>& tokens,
                                         size_t& i) {
    std::vector<std::variant<std::shared_ptr<ArrayRangeNode>,
                             std::shared_ptr<MethodNode>>>
        result;
    while (i < tokens.size() && tokens[i].getType() == TokenType::SYMBOL &&
           (tokens[i].getValue()[0] == '[' || tokens[i].getValue()[0] == '.'))
        if (tokens[i].getValue()[0] == '[')
            result.push_back(std::make_shared<ArrayRangeNode>(
                ArrayRangeNode::parse(tokens, i)));
        else
            result.push_back(
                std::make_shared<MethodNode>(MethodNode::parse(tokens, i)));
    return ArrayPostFixNode(std::move(result));
}

ArrayPostFixNode::ArrayPostFixNode(
    std::vector<std::variant<std::shared_ptr<ArrayRangeNode>,
                             std::shared_ptr<MethodNode>>>
        values)
    : values(std::move(values)) {}

ArrayPostFixNode::operator std::string() const {
    std::string result;
    for (auto& value : values) {
        std::visit(
            [&result](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                constexpr bool isArrayRange =
                    std::is_same_v<T, std::shared_ptr<ArrayRangeNode>>;
                constexpr bool isMethod =
                    std::is_same_v<T, std::shared_ptr<MethodNode>>;
                if constexpr (isArrayRange) {
                    result += std::string(*arg);
                } else if constexpr (isMethod) {
                    result += std::string(*arg);
                }
            },
            value);
    }
    return result;
}

ArrayRangeNode::operator std::string() const {
    std::string result(1, '[');
    if (start.has_value()) {
        result += std::visit(
            [](auto&& start) -> std::string {
                using T = std::decay_t<decltype(start)>;
                constexpr bool isSizeT = std::is_same_v<T, size_t>;
                constexpr bool isExpression =
                    std::is_same_v<T, std::shared_ptr<ExpressionNode>>;
                if constexpr (isSizeT) {
                    return std::to_string(start);
                } else if constexpr (isExpression) {
                    return std::string(*start);
                }
            },
            start.value());
    }
    result.push_back(':');
    if (end.has_value()) {
        result += std::visit(
            [](auto&& end) -> std::string {
                using T = std::decay_t<decltype(end)>;
                constexpr bool isSizeT = std::is_same_v<T, size_t>;
                constexpr bool isExpression =
                    std::is_same_v<T, std::shared_ptr<ExpressionNode>>;
                if constexpr (isSizeT) {
                    return std::to_string(end);
                } else if constexpr (isExpression) {
                    return std::string(*end);
                }
            },
            end.value());
    }
    result.push_back(']');
    return result;
}

const std::vector<std::variant<
    std::shared_ptr<VariableBindingNode>, std::shared_ptr<FunctionCallNode>,
    std::shared_ptr<FunctionDefinitionNode>, std::shared_ptr<UseNode>>>&
RootNode::getValues() const {
    return values;
}

const std::string& FunctionDefinitionNode::getIdentifier() const {
    return identifier;
}

const std::vector<std::shared_ptr<FunctionParameterNode>>&
FunctionDefinitionNode::getParams() const {
    return params;
}

const ArrayDescriptor& FunctionDefinitionNode::getOutput() const {
    return output;
}

const std::shared_ptr<BodyNode>& FunctionDefinitionNode::getBody() const {
    return body;
}

const std::vector<std::shared_ptr<StatementNode>>& BodyNode::getStatements()
    const {
    return statements;
}

const std::variant<
    std::shared_ptr<VariableBindingNode>, std::shared_ptr<ForLoopNode>,
    std::shared_ptr<IfNode>, std::shared_ptr<WhileNode>,
    std::shared_ptr<FunctionCallNode>, std::shared_ptr<ReturnNode>>&
StatementNode::getValue() const {
    return value;
}

FunctionParameterNode::operator std::string() const {
    return identifier + ": " + std::string(descriptor);
}

const ArrayDescriptor& FunctionParameterNode::getDescriptor() const {
    return descriptor;
}

const std::string& FunctionParameterNode::getIdentifier() const {
    return identifier;
}

const std::optional<size_t>& ArrayDescriptor::getSize() const { return size; }

const bool& ArrayDescriptor::getCanGrow() const { return canGrow; }

const std::variant<std::shared_ptr<ArithmeticNode>, std::shared_ptr<ArrayNode>>&
ExpressionNode::getPrimary() const {
    return primary;
}

const ArrayPostFixNode& ExpressionNode::getPostfix() const { return postfix; }

const std::string& VariableDeclarationNode::getIdentifier() const {
    return identifier;
}

const ArrayDescriptor& VariableDeclarationNode::getDescriptor() const {
    return descriptor;
}

const std::optional<std::shared_ptr<ExpressionNode>>&
VariableDeclarationNode::getValue() const {
    return value;
}

const std::variant<std::shared_ptr<VariableDeclarationNode>,
                   std::shared_ptr<VariableAssignmentNode>>&
VariableBindingNode::getValue() const {
    return value;
}

const std::vector<
    std::variant<std::shared_ptr<ArrayRangeNode>, std::shared_ptr<MethodNode>>>&
ArrayPostFixNode::getValues() const {
    return values;
}

const std::variant<std::vector<int>, std::string,
                   std::shared_ptr<FunctionCallNode>>&
ArrayNode::getValue() const {
    return value;
}

std::vector<int> ArrayNode::stringToInts(const std::string& string) {
    std::vector<int> ints;
    ints.reserve(string.size());
    for (size_t i = 0; i < string.size(); i++) ints.push_back(string[i]);
    return ints;
}

const std::string& FunctionCallNode::getIdentifier() const {
    return identifier;
}

const std::vector<std::shared_ptr<ExpressionNode>>&
FunctionCallNode::getParameters() const {
    return parameters;
}

const std::string& MethodNode::getIdentifier() const { return identifier; }

const std::vector<std::shared_ptr<ExpressionNode>>& MethodNode::getParameters()
    const {
    return parameters;
}

const std::optional<std::variant<size_t, std::shared_ptr<ExpressionNode>>>&
ArrayRangeNode::getStart() const {
    return start;
}

const std::optional<std::variant<size_t, std::shared_ptr<ExpressionNode>>>&
ArrayRangeNode::getEnd() const {
    return end;
}

const std::string& VariableAssignmentNode::getLeft() const { return left; }

const std::shared_ptr<ExpressionNode>& VariableAssignmentNode::getRight()
    const {
    return right;
}

const std::string& ForLoopNode::getElement() const { return element; }

const std::shared_ptr<ExpressionNode>& ForLoopNode::getIterable() const {
    return iterable;
}

const std::shared_ptr<BodyNode>& ForLoopNode::getBody() const { return body; }

const std::variant<std::shared_ptr<IfCompareNode>,
                   std::shared_ptr<IfDeclarationNode>>&
IfNode::getCondition() const {
    return condition;
}

const std::shared_ptr<BodyNode>& IfNode::getBody() const { return body; }

const std::optional<std::shared_ptr<IfNode>>& IfNode::getElseIfBranches()
    const {
    return elseIfBranches;
}

const std::optional<std::shared_ptr<BodyNode>>& IfNode::getElseBody() const {
    return elseBody;
}

const IfCompareNode::Type& IfCompareNode::getType() const { return type; }

const std::shared_ptr<ExpressionNode>& IfCompareNode::getLeft() const {
    return left;
}

const std::shared_ptr<ExpressionNode>& IfCompareNode::getRight() const {
    return right;
}

const std::shared_ptr<VariableDeclarationNode>&
IfDeclarationNode::getVariableDeclaration() const {
    return variableDeclaration;
}

const std::shared_ptr<ExpressionNode>& ReturnNode::getValue() const {
    return value;
}

ExpressionNode::ExpressionNode(std::vector<int> values)
    : primary(std::make_shared<ArrayNode>(values)),
      postfix(ArrayPostFixNode(
          std::vector<std::variant<std::shared_ptr<ArrayRangeNode>,
                                   std::shared_ptr<MethodNode>>>())) {}

UseNode UseNode::parse(std::vector<Token>& tokens, size_t& i) {
    expect(tokens, i, "use", TokenType::IDENTIFIER, "use");
    ++i;
    if (i >= tokens.size())
        throw UnexpectedEOFError("use",
                                 "\"array literal\" or <standard_header>");
    if (tokens[i] == Token(TokenType::SYMBOL, "<")) {
        ++i;
        auto standardHeader =
            expect(tokens, i, "use", TokenType::IDENTIFIER).getValue();
        ++i;
        expect(tokens, i, "use", TokenType::SYMBOL, ">");
        ++i;
        return UseNode(std::make_shared<ArrayNode>(
                           ArrayNode::stringToInts(standardHeader)),
                       Type::STANDARD_HEADER);
    }
    return UseNode(std::make_shared<ArrayNode>(ArrayNode::parse(tokens, i)),
                   Type::PATH);
}

const std::shared_ptr<ArrayNode>& UseNode::getValue() const { return value; }

const UseNode::Type& UseNode::getType() const { return type; }

UseNode::UseNode(std::shared_ptr<ArrayNode> value, Type type)
    : value(std::move(value)), type(type) {}

std::string UseNode::toStringIndented(size_t indent) const {
    char begin, end;
    begin = end = '"';
    if (type == Type::STANDARD_HEADER) {
        begin = '<';
        end = '>';
    }
    return nTabs(indent) + " " + std::string(1, begin) + std::string(*value) +
           std::string(1, end);
}

WhileNode::WhileNode(std::variant<std::shared_ptr<IfCompareNode>,
                                  std::shared_ptr<IfDeclarationNode>>
                         condition,
                     std::shared_ptr<BodyNode> body)
    : condition(std::move(condition)), body(std::move(body)) {}

WhileNode WhileNode::parse(std::vector<Token>& tokens, size_t& i) {
    expect(tokens, i, "while", TokenType::IDENTIFIER, "while");
    ++i;
    if (i >= tokens.size()) throw UnexpectedEOFError("while", "token");

    std::variant<std::shared_ptr<IfCompareNode>,
                 std::shared_ptr<IfDeclarationNode>>
        condition =
            (tokens[i].getType() == TokenType::IDENTIFIER &&
             tokens[i].getValue() == "let")
                ? std::variant<
                      std::shared_ptr<IfCompareNode>,
                      std::shared_ptr<IfDeclarationNode>>{std::make_shared<
                      IfDeclarationNode>(IfDeclarationNode::parse(tokens, i))}
                : std::variant<std::shared_ptr<IfCompareNode>,
                               std::shared_ptr<IfDeclarationNode>>{
                      std::make_shared<IfCompareNode>(
                          IfCompareNode::parse(tokens, i))};

    std::shared_ptr<BodyNode> body = BodyNode::parse(tokens, i);

    return WhileNode(condition, body);
}

std::string WhileNode::toStringIndented(size_t indent) const {
    std::string result = "while ";
    std::visit(
        [&result](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            constexpr bool isCompare =
                std::is_same_v<T, std::shared_ptr<IfCompareNode>>;
            constexpr bool isDeclaration =
                std::is_same_v<T, std::shared_ptr<IfDeclarationNode>>;
            if constexpr (isCompare) {
                result += std::string(*arg);
            } else if constexpr (isDeclaration) {
                result += std::string(*arg);
            }
        },
        condition);
    result += " " + body->toStringIndented(indent);
    return result;
}

const std::variant<std::shared_ptr<IfCompareNode>,
                   std::shared_ptr<IfDeclarationNode>>&
WhileNode::getCondition() const {
    return condition;
}

const std::shared_ptr<BodyNode>& WhileNode::getBody() const { return body; }
