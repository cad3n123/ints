// Copyright 2025 Caden Crowson

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "lexer/tokenize.h"

class ExpressionNode;

class ArrayRangeNode {
 public:
    static ArrayRangeNode parse(std::vector<Token> &tokens, size_t &i);
    operator std::string() const;
    const std::optional<std::variant<size_t, std::shared_ptr<ExpressionNode>>> &
    getStart() const;
    const std::optional<std::variant<size_t, std::shared_ptr<ExpressionNode>>> &
    getEnd() const;

 private:
    ArrayRangeNode(
        std::optional<std::variant<size_t, std::shared_ptr<ExpressionNode>>>
            start,
        std::optional<std::variant<size_t, std::shared_ptr<ExpressionNode>>>
            end);
    std::optional<std::variant<size_t, std::shared_ptr<ExpressionNode>>> start;
    std::optional<std::variant<size_t, std::shared_ptr<ExpressionNode>>> end;
};

class ArithmeticNode {
 public:
    enum Type {
        TYPE_LEFT_PARENTHESIS,
        TYPE_ADDITION,
        TYPE_SUBTRACTION,
        TYPE_MULTIPLICATION,
        TYPE_DIVISION,
    };
    enum Precedence {
        LEFT_PARENTHESIS = 0,
        ADD_SUB = 1,
        MULT_DIV = 2,
    };
    ArithmeticNode(std::shared_ptr<ExpressionNode> left,
                   std::shared_ptr<ExpressionNode> right, Type type);
    operator std::string() const;
    std::shared_ptr<ExpressionNode> left;
    std::shared_ptr<ExpressionNode> right;
    Type type;

 private:
};

class MethodNode {
 public:
    MethodNode(std::string identifier,
               std::vector<std::shared_ptr<ExpressionNode>> parameters);
    static MethodNode parse(std::vector<Token> &tokens, size_t &i);
    operator std::string() const;
    const std::string &getIdentifier() const;
    const std::vector<std::shared_ptr<ExpressionNode>> &getParameters() const;

 private:
    std::string identifier;
    std::vector<std::shared_ptr<ExpressionNode>> parameters;
};

class FunctionCallNode {
 public:
    FunctionCallNode(std::string identifier,
                     std::vector<std::shared_ptr<ExpressionNode>> parameters);
    static FunctionCallNode parse(std::vector<Token> &tokens, size_t &i);
    operator std::string() const;
    const std::string &getIdentifier() const;
    const std::vector<std::shared_ptr<ExpressionNode>> &getParameters() const;

 private:
    std::string identifier;
    std::vector<std::shared_ptr<ExpressionNode>> parameters;
};

class ArrayNode {
 public:
    explicit ArrayNode(std::variant<std::vector<int>, std::string,
                                    std::shared_ptr<FunctionCallNode>>
                           value);
    static ArrayNode parse(std::vector<Token> &tokens, size_t &i);
    static std::vector<int> stringToInts(const std::string &string);
    operator std::string() const;
    const std::variant<std::vector<int>, std::string,
                       std::shared_ptr<FunctionCallNode>> &
    getValue() const;

 private:
    std::variant<std::vector<int>, std::string,
                 std::shared_ptr<FunctionCallNode>>
        value;
};

class ArrayPostFixNode {
 public:
    explicit ArrayPostFixNode(
        std::vector<std::variant<std::shared_ptr<ArrayRangeNode>,
                                 std::shared_ptr<MethodNode>>>
            values);
    static ArrayPostFixNode parse(std::vector<Token> &tokens, size_t &i);
    operator std::string() const;
    const std::vector<std::variant<std::shared_ptr<ArrayRangeNode>,
                                   std::shared_ptr<MethodNode>>> &
    getValues() const;

 private:
    std::vector<std::variant<std::shared_ptr<ArrayRangeNode>,
                             std::shared_ptr<MethodNode>>>
        values;
};

class ExpressionNode {
 public:
    ExpressionNode(std::variant<std::shared_ptr<ArithmeticNode>,
                                std::shared_ptr<ArrayNode>>
                       primary,
                   ArrayPostFixNode postfix);
    explicit ExpressionNode(std::vector<int> values);
    static ExpressionNode parse(std::vector<Token> &tokens, size_t &i);
    operator std::string() const;
    const std::variant<std::shared_ptr<ArithmeticNode>,
                       std::shared_ptr<ArrayNode>> &
    getPrimary() const;
    const ArrayPostFixNode &getPostfix() const;

 private:
    std::variant<std::shared_ptr<ArithmeticNode>, std::shared_ptr<ArrayNode>>
        primary;
    ArrayPostFixNode postfix;
};

class ArrayDescriptor {
 public:
    static ArrayDescriptor parse(std::vector<Token> &tokens, size_t &i);
    operator std::string() const;
    const std::optional<size_t> &getSize() const;
    const bool &getCanGrow() const;

 private:
    ArrayDescriptor(std::optional<size_t> size, bool canGrow);
    std::optional<size_t> size;
    bool canGrow;
};

class VariableAssignmentNode {
 public:
    static VariableAssignmentNode parse(std::vector<Token> &tokens, size_t &i);
    operator std::string() const;
    const std::string &getLeft() const;
    const std::shared_ptr<ExpressionNode> &getRight() const;

 private:
    VariableAssignmentNode(std::string left,
                           std::shared_ptr<ExpressionNode> right);
    std::string left;
    std::shared_ptr<ExpressionNode> right;
};

class VariableDeclarationNode {
 public:
    static VariableDeclarationNode parse(std::vector<Token> &tokens, size_t &i);
    operator std::string() const;
    const std::string &getIdentifier() const;
    const ArrayDescriptor &getDescriptor() const;
    const std::optional<std::shared_ptr<ExpressionNode>> &getValue() const;

 private:
    VariableDeclarationNode(
        std::string identifier, ArrayDescriptor descriptor,
        std::optional<std::shared_ptr<ExpressionNode>> value);
    std::string identifier;
    ArrayDescriptor descriptor;
    std::optional<std::shared_ptr<ExpressionNode>> value;
};

class VariableBindingNode {
 public:
    explicit VariableBindingNode(
        std::variant<std::shared_ptr<VariableDeclarationNode>,
                     std::shared_ptr<VariableAssignmentNode>>
            value);
    static VariableBindingNode parse(std::vector<Token> &tokens, size_t &i);
    operator std::string() const;
    const std::variant<std::shared_ptr<VariableDeclarationNode>,
                       std::shared_ptr<VariableAssignmentNode>> &
    getValue() const;

 private:
    std::variant<std::shared_ptr<VariableDeclarationNode>,
                 std::shared_ptr<VariableAssignmentNode>>
        value;
};

class StatementNode;

class ReturnNode {
 public:
    static ReturnNode parse(std::vector<Token> &tokens, size_t &i);
    operator std::string() const;
    const std::shared_ptr<ExpressionNode> &getValue() const;

 private:
    explicit ReturnNode(std::shared_ptr<ExpressionNode> value);
    std::shared_ptr<ExpressionNode> value;
};

class BodyNode {
 public:
    explicit BodyNode(std::vector<std::shared_ptr<StatementNode>> statements);
    static std::shared_ptr<BodyNode> parse(std::vector<Token> &tokens,
                                           size_t &i);
    std::string toStringIndented(size_t indent) const;
    const std::vector<std::shared_ptr<StatementNode>> &getStatements() const;

 private:
    std::vector<std::shared_ptr<StatementNode>> statements;
};

class IfCompareNode {
 public:
    enum Type { EQ, NE, LT, LE, GT, GE };
    static IfCompareNode parse(std::vector<Token> &tokens, size_t &i);
    operator std::string() const;
    const Type &getType() const;
    const std::shared_ptr<ExpressionNode> &getLeft() const;
    const std::shared_ptr<ExpressionNode> &getRight() const;

 private:
    IfCompareNode(Type type, std::shared_ptr<ExpressionNode> left,
                  std::shared_ptr<ExpressionNode> right);
    Type type;
    std::shared_ptr<ExpressionNode> left;
    std::shared_ptr<ExpressionNode> right;
};

class IfDeclarationNode {
 public:
    static IfDeclarationNode parse(std::vector<Token> &tokens, size_t &i);
    operator std::string() const;
    const std::shared_ptr<VariableDeclarationNode> &getVariableDeclaration()
        const;

 private:
    explicit IfDeclarationNode(
        std::shared_ptr<VariableDeclarationNode> variableDeclaration);
    std::shared_ptr<VariableDeclarationNode> variableDeclaration;
};

class IfNode {
 public:
    IfNode(std::variant<std::shared_ptr<IfCompareNode>,
                        std::shared_ptr<IfDeclarationNode>>
               condition,
           std::shared_ptr<BodyNode> body,
           std::optional<std::shared_ptr<IfNode>> elseIfBranches,
           std::optional<std::shared_ptr<BodyNode>> elseBody);
    static IfNode parse(std::vector<Token> &tokens, size_t &i);
    std::string toStringIndented(size_t indent) const;
    const std::variant<std::shared_ptr<IfCompareNode>,
                       std::shared_ptr<IfDeclarationNode>> &
    getCondition() const;
    const std::shared_ptr<BodyNode> &getBody() const;
    const std::optional<std::shared_ptr<IfNode>> &getElseIfBranches() const;
    const std::optional<std::shared_ptr<BodyNode>> &getElseBody() const;

 private:
    std::variant<std::shared_ptr<IfCompareNode>,
                 std::shared_ptr<IfDeclarationNode>>
        condition;
    std::shared_ptr<BodyNode> body;
    std::optional<std::shared_ptr<IfNode>> elseIfBranches;
    std::optional<std::shared_ptr<BodyNode>> elseBody;
};

class WhileNode {
 public:
    WhileNode(std::variant<std::shared_ptr<IfCompareNode>,
                           std::shared_ptr<IfDeclarationNode>>
                  condition,
              std::shared_ptr<BodyNode> body);
    static WhileNode parse(std::vector<Token> &tokens, size_t &i);
    std::string toStringIndented(size_t indent) const;
    const std::variant<std::shared_ptr<IfCompareNode>,
                       std::shared_ptr<IfDeclarationNode>> &
    getCondition() const;
    const std::shared_ptr<BodyNode> &getBody() const;

 private:
    std::variant<std::shared_ptr<IfCompareNode>,
                 std::shared_ptr<IfDeclarationNode>>
        condition;
    std::shared_ptr<BodyNode> body;
};

class ForLoopNode {
 public:
    static ForLoopNode parse(std::vector<Token> &tokens, size_t &i);
    std::string toStringIndented(size_t indent) const;
    const std::string &getElement() const;
    const std::shared_ptr<ExpressionNode> &getIterable() const;
    const std::shared_ptr<BodyNode> &getBody() const;

 private:
    ForLoopNode(std::string element, std::shared_ptr<ExpressionNode> iterable,
                std::shared_ptr<BodyNode> body);
    std::string element;
    std::shared_ptr<ExpressionNode> iterable;
    std::shared_ptr<BodyNode> body;
};

class StatementNode {
 public:
    explicit StatementNode(
        std::variant<
            std::shared_ptr<VariableBindingNode>, std::shared_ptr<ForLoopNode>,
            std::shared_ptr<IfNode>, std::shared_ptr<WhileNode>,
            std::shared_ptr<FunctionCallNode>, std::shared_ptr<ReturnNode>>
            value);
    static std::shared_ptr<StatementNode> parse(std::vector<Token> &tokens,
                                                size_t &i);
    std::string toStringIndented(size_t indent) const;
    const std::variant<
        std::shared_ptr<VariableBindingNode>, std::shared_ptr<ForLoopNode>,
        std::shared_ptr<IfNode>, std::shared_ptr<WhileNode>,
        std::shared_ptr<FunctionCallNode>, std::shared_ptr<ReturnNode>> &
    getValue() const;

 private:
    std::variant<std::shared_ptr<VariableBindingNode>,
                 std::shared_ptr<ForLoopNode>, std::shared_ptr<IfNode>,
                 std::shared_ptr<WhileNode>, std::shared_ptr<FunctionCallNode>,
                 std::shared_ptr<ReturnNode>>
        value;
};

class FunctionParameterNode {
 public:
    static FunctionParameterNode parse(std::vector<Token> &tokens, size_t &i);
    operator std::string() const;
    const std::string &getIdentifier() const;
    const ArrayDescriptor &getDescriptor() const;

 private:
    FunctionParameterNode(std::string identifier, ArrayDescriptor descriptor);
    std::string identifier;
    ArrayDescriptor descriptor;
};

class FunctionDefinitionNode {
 public:
    static FunctionDefinitionNode parse(std::vector<Token> &tokens, size_t &i);
    std::string toStringIndented(size_t indent) const;
    const std::string &getIdentifier() const;
    const std::vector<std::shared_ptr<FunctionParameterNode>> &getParams()
        const;
    const ArrayDescriptor &getOutput() const;
    const std::shared_ptr<BodyNode> &getBody() const;

 private:
    FunctionDefinitionNode(
        std::string identifier,
        std::vector<std::shared_ptr<FunctionParameterNode>> input,
        ArrayDescriptor output, std::shared_ptr<BodyNode> body);
    std::string identifier;
    std::vector<std::shared_ptr<FunctionParameterNode>> params;
    ArrayDescriptor output;
    std::shared_ptr<BodyNode> body;
};

class UseNode {
 public:
    enum Type { PATH, STANDARD_HEADER };
    static UseNode parse(std::vector<Token> &tokens, size_t &i);
    const std::shared_ptr<ArrayNode> &getValue() const;
    const Type &getType() const;
    std::string toStringIndented(size_t indent) const;

 private:
    UseNode(std::shared_ptr<ArrayNode> value, Type type);
    std::shared_ptr<ArrayNode> value;
    Type type;
};

class RootNode {
 public:
    static RootNode parse(std::vector<Token> &tokens);
    operator std::string() const;
    const std::vector<std::variant<
        std::shared_ptr<VariableBindingNode>, std::shared_ptr<FunctionCallNode>,
        std::shared_ptr<FunctionDefinitionNode>, std::shared_ptr<UseNode>>> &
    getValues() const;

 private:
    explicit RootNode(
        std::vector<std::variant<std::shared_ptr<VariableBindingNode>,
                                 std::shared_ptr<FunctionCallNode>,
                                 std::shared_ptr<FunctionDefinitionNode>,
                                 std::shared_ptr<UseNode>>>
            values);
    std::vector<std::variant<
        std::shared_ptr<VariableBindingNode>, std::shared_ptr<FunctionCallNode>,
        std::shared_ptr<FunctionDefinitionNode>, std::shared_ptr<UseNode>>>
        values;
};
