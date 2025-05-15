// Copyright 2025 Caden Crowson

#include "runtime/interpreter.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif
#include <csignal>
#include <cstdlib>

#include "lexer/tokenize.h"
#include "parser/parse.h"
#include "util/file.h"

Scope::Scope(std::weak_ptr<Scope> parent) : parent(parent) {}

bool Scope::has(const std::string& name) const {
    return variables.find(name) != variables.end();
}

bool Scope::hasRecursive(const std::string& name) const {
    if (has(name)) {
        return true;
    }
    if (auto parent_locked = parent.lock()) {
        return parent_locked->hasRecursive(name);
    }
    return false;
}

const std::variant<std::shared_ptr<Value>,
                   std::shared_ptr<FunctionDefinitionNode>>&
Scope::get(const std::string& name) const {
    if (has(name)) return variables.at(name);
    if (auto parent_locked = parent.lock()) {
        return parent_locked->get(name);
    }
    throw std::runtime_error("Undefined variable: " + name);
}

void Scope::set(
    const std::string& name,
    const std::variant<std::shared_ptr<Value>,
                       std::shared_ptr<FunctionDefinitionNode>>& value) {
    if (has(name)) {
        variables.insert_or_assign(name, value);
    } else if (auto parent_locked = parent.lock()) {
        parent_locked->set(name, value);
    } else {
        throw std::runtime_error("Undefined variable for assignment: " + name);
    }
}

void Scope::define(
    const std::string& name,
    const std::variant<std::shared_ptr<Value>,
                       std::shared_ptr<FunctionDefinitionNode>>& value) {
    variables.emplace(name, value);
}

static void interpretFunctionDefinition(
    std::shared_ptr<FunctionDefinitionNode> functionDefinition,
    std::weak_ptr<Scope> parent) {
    if (auto lockedParent = parent.lock()) {
        lockedParent->define(functionDefinition->getIdentifier(),
                             functionDefinition);
    } else {
        throw std::runtime_error("Error interpreting function definition");
    }
}

static Value interpretExpression(
    const std::shared_ptr<ExpressionNode>& expression,
    std::weak_ptr<Scope> scope);

static std::vector<std::shared_ptr<Value>> interpretParameters(
    const std::vector<std::shared_ptr<ExpressionNode>>& parameters,
    std::weak_ptr<Scope> scope) {
    std::vector<std::shared_ptr<Value>> result;
    for (auto& parameter : parameters)
        result.push_back(
            std::make_shared<Value>(interpretExpression(parameter, scope)));
    return result;
}

static Value interpretArray(const std::shared_ptr<ArrayNode>& array,
                            std::weak_ptr<Scope> scope) {
    if (auto lockedScope = scope.lock()) {
        return std::visit(
            [&scope, &lockedScope](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                constexpr bool isVector = std::is_same_v<T, std::vector<int>>;
                constexpr bool isString = std::is_same_v<T, std::string>;
                constexpr bool isFunctionCall =
                    std::is_same_v<T, std::shared_ptr<FunctionCallNode>>;
                if constexpr (isVector) {
                    return Value(arg, arg.size());
                } else if constexpr (isString) {
                    if (auto value = *std::get_if<std::shared_ptr<Value>>(
                            &lockedScope->get(arg)))
                        return *value;
                    else
                        throw std::runtime_error(
                            "Cannot use " + arg +
                            " as an array, as it is defined as a function");
                } else if constexpr (isFunctionCall) {
                    return interpretFunctionCall(arg, scope);
                }
            },
            array->getValue());
    }
    throw std::runtime_error("Error interpreting array");
}

static Value interpretArithmetic(
    const std::shared_ptr<ArithmeticNode>& arithmetic,
    std::weak_ptr<Scope> scope) {
    Value left = interpretExpression(arithmetic->left, scope);
    Value right = interpretExpression(arithmetic->right, scope);
    switch (arithmetic->type) {
        case ArithmeticNode::TYPE_ADDITION:
            return left + right;
        case ArithmeticNode::TYPE_SUBTRACTION:
            return left - right;
        case ArithmeticNode::TYPE_MULTIPLICATION:
            return left * right;
        case ArithmeticNode::TYPE_DIVISION:
            return left / right;
        default:
            throw std::runtime_error("Error interpreting arithmetic");
    }
}

static Value applyAppend(const Value& value,
                         std::vector<std::shared_ptr<Value>> parameters) {
    if (parameters.size() != 1)
        throw std::runtime_error("append expects 1 argument with type []");
    auto& param1 = parameters[0];
    size_t size1 = value.getSize();
    size_t size2 = param1->getSize();
    DynamicArray result(size1 + size2);
    std::visit(
        [&result, &size1](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            constexpr bool isVector = std::is_same_v<T, std::vector<int>>;
            constexpr bool isDynamic = std::is_same_v<T, DynamicArray>;
            if constexpr (isVector) {
                for (size_t i = 0; i < size1; i++) result[i] = arg[i];
            } else if constexpr (isDynamic) {
                for (size_t i = 0; i < size1; i++) result[i] = arg[i];
            }
        },
        value.value);
    std::visit(
        [&result, &size1, &size2](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            constexpr bool isVector = std::is_same_v<T, std::vector<int>>;
            constexpr bool isDynamic = std::is_same_v<T, DynamicArray>;
            if constexpr (isVector) {
                for (size_t i = 0; i < size2; i++) result[size1 + i] = arg[i];
            } else if constexpr (isDynamic) {
                for (size_t i = 0; i < size2; i++) result[size1 + i] = arg[i];
            }
        },
        param1->value);
    return Value(result, size1 + size2);
}

static Value applySqrt(const Value& value,
                       std::vector<std::shared_ptr<Value>> parameters) {
    if (parameters.size() != 0)
        throw std::runtime_error("sqrt expects 0 arguments");
    DynamicArray fixedValue = DynamicArray::fromValue(value);
    DynamicArray result(fixedValue.size);
    for (size_t i = 0; i < fixedValue.size; i++)
        result[i] = sqrt(fixedValue[i]);
    return Value(result, result.size);
}

static Value applySize(const Value& value,
                       std::vector<std::shared_ptr<Value>> parameters) {
    if (parameters.size() != 0)
        throw std::runtime_error("size expects 0 arguments");
    DynamicArray result(1);
    result[0] = value.getSize();
    return Value(result, 1);
}

static Value applyMethod(const Value& value,
                         const std::shared_ptr<MethodNode>& method,
                         std::weak_ptr<Scope> scope) {
    auto parameters = interpretParameters(method->getParameters(), scope);
    if (method->getIdentifier() == "append")
        return applyAppend(value, parameters);
    else if (method->getIdentifier() == "sqrt")
        return applySqrt(value, parameters);
    else if (method->getIdentifier() == "size")
        return applySize(value, parameters);
    else
        throw std::runtime_error("Unknown method " + method->getIdentifier());
}

static size_t interpretArrayRangeBound(
    const std::variant<size_t, std::shared_ptr<ExpressionNode>>& value,
    std::weak_ptr<Scope> scope) {
    return std::visit(
        [&scope](auto&& value) -> size_t {
            using T = std::decay_t<decltype(value)>;
            constexpr bool isSizeT = std::is_same_v<T, size_t>;
            constexpr bool isExpression =
                std::is_same_v<T, std::shared_ptr<ExpressionNode>>;
            if constexpr (isSizeT) {
                return value;
            } else if constexpr (isExpression) {
                auto result =
                    DynamicArray::fromValue(interpretExpression(value, scope));
                if (result.size != 1 || result[0] < 0)
                    throw std::runtime_error(
                        "Array Bounds value must be an integer or evaluate to "
                        "an array with 1 positive value");
                return result[0];
            }
        },
        value);
}

static Value interpretArrayRange(const std::shared_ptr<ArrayRangeNode>& range,
                                 const Value& value,
                                 std::weak_ptr<Scope> scope) {
    size_t size = value.getSize();
    auto startV = range->getStart().value_or(static_cast<size_t>(0));
    auto endV = range->getEnd().value_or(size);
    size_t start = interpretArrayRangeBound(startV, scope);
    size_t end = interpretArrayRangeBound(endV, scope);
    if (end < start)
        throw std::runtime_error(
            "Array Range upper bound must be greater than or equal to the "
            "lower bound");
    size_t newSize = end - start;
    DynamicArray result(newSize);
    if (end > size)
        throw std::runtime_error(
            "Array range bounds must be smaller than the "
            "length of the array");
    auto staticValue = DynamicArray::fromValue(value);
    for (size_t i = 0; i < newSize; i++) {
        result[i] = staticValue[i + start];
    }
    return Value(result, newSize);
}

static Value applyPostfix(const Value& value,
                          const ArrayPostFixNode& postfixNode,
                          std::weak_ptr<Scope> scope) {
    Value* resultValue = new Value(value);
    for (auto& postfix : postfixNode.getValues()) {
        resultValue = new Value(std::visit(
            [&scope, &resultValue](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                constexpr bool isArrayRange =
                    std::is_same_v<T, std::shared_ptr<ArrayRangeNode>>;
                constexpr bool isMethod =
                    std::is_same_v<T, std::shared_ptr<MethodNode>>;
                if constexpr (isArrayRange) {
                    return interpretArrayRange(arg, *resultValue, scope);
                } else if constexpr (isMethod) {
                    return applyMethod(*resultValue, arg, scope);
                }
            },
            postfix));
    }
    return *resultValue;
}

static std::optional<Value> interpretBody(const std::shared_ptr<BodyNode>& body,
                                          std::weak_ptr<Scope> scope);
static Value interpretFunctionCall(
    const std::shared_ptr<FunctionCallNode>& functionCall,
    std::weak_ptr<Scope> parent);

static Value interpretExpression(
    const std::shared_ptr<ExpressionNode>& expression,
    std::weak_ptr<Scope> scope) {
    Value value = std::visit(
        [&scope, &expression](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            constexpr bool isArithmetic =
                std::is_same_v<T, std::shared_ptr<ArithmeticNode>>;
            constexpr bool isArray =
                std::is_same_v<T, std::shared_ptr<ArrayNode>>;
            if constexpr (isArithmetic) {
                return interpretArithmetic(arg, scope);
            } else if constexpr (isArray) {
                return interpretArray(arg, scope);
            }
        },
        expression->getPrimary());
    return applyPostfix(value, expression->getPostfix(), scope);
}

static void interpretVariableDeclaration(
    const std::shared_ptr<VariableDeclarationNode>& variableDeclaration,
    std::weak_ptr<Scope> scope) {
    if (auto lockedScope = scope.lock()) {
        std::optional<Value> value;
        if (variableDeclaration->getValue().has_value())
            value = interpretExpression(variableDeclaration->getValue().value(),
                                        scope);
        lockedScope->define(variableDeclaration->getIdentifier(),
                            std::make_shared<Value>(Value::fromDescriptor(
                                variableDeclaration->getDescriptor(), value)));
    } else {
        throw std::runtime_error("Error interpreting variable declaration");
    }
}

static void interpretVariableAssignment(
    const std::shared_ptr<VariableAssignmentNode>& variableAssignment,
    std::weak_ptr<Scope> scope) {
    if (auto lockedScope = scope.lock()) {
        if (!lockedScope->hasRecursive(variableAssignment->getLeft()))
            throw std::runtime_error(variableAssignment->getLeft() +
                                     " has not been defined");

        auto right = std::make_shared<Value>(
            interpretExpression(variableAssignment->getRight(), scope));
        lockedScope->set(variableAssignment->getLeft(), right);
    } else {
        throw std::runtime_error("Error interpreting variable assignment");
    }
}

static void interpretVariableBinding(
    const std::shared_ptr<VariableBindingNode>& variableBinding,
    std::weak_ptr<Scope> scope) {
    std::visit(
        [&scope](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            constexpr bool isDeclaration =
                std::is_same_v<T, std::shared_ptr<VariableDeclarationNode>>;
            constexpr bool isAssignment =
                std::is_same_v<T, std::shared_ptr<VariableAssignmentNode>>;
            if constexpr (isDeclaration) {
                interpretVariableDeclaration(arg, scope);
            } else if constexpr (isAssignment) {
                interpretVariableAssignment(arg, scope);
            }
        },
        variableBinding->getValue());
}

static bool interpretIfDeclaration(
    const std::shared_ptr<IfDeclarationNode>& condition,
    std::weak_ptr<Scope> scope) {
    auto expression = condition->getVariableDeclaration()->getValue();

    if (!expression.has_value()) {
        interpretVariableDeclaration(condition->getVariableDeclaration(),
                                     scope);
        return true;
    } else if (auto lockedScope = scope.lock()) {
        auto value = interpretExpression(expression.value(), scope);
        auto descriptor = condition->getVariableDeclaration()->getDescriptor();
        if (descriptor.getSize() == value.getSize() ||
            (descriptor.getSize() < value.getSize() &&
             descriptor.getCanGrow())) {
            lockedScope->define(
                condition->getVariableDeclaration()->getIdentifier(),
                std::make_shared<Value>(
                    Value::fromDescriptor(descriptor, value)));
            return true;
        } else {
            return false;
        }
    } else {
        throw std::runtime_error("Error interpreting if declaration");
    }
}

static bool interpretIfCompare(const std::shared_ptr<IfCompareNode>& condition,
                               std::weak_ptr<Scope> scope) {
    Value left = interpretExpression(condition->getLeft(), scope);
    Value right = interpretExpression(condition->getRight(), scope);
    switch (condition->getType()) {
        case IfCompareNode::Type::EQ:
            return left == right;
        case IfCompareNode::Type::NE:
            return left != right;
        case IfCompareNode::Type::LT:
            return left < right;
        case IfCompareNode::Type::LE:
            return left <= right;
        case IfCompareNode::Type::GT:
            return left > right;
        case IfCompareNode::Type::GE:
            return left >= right;
    }
    throw std::runtime_error("Error interpreting if comparison");
}

static bool interpretIfCondition(
    const std::variant<std::shared_ptr<IfCompareNode>,
                       std::shared_ptr<IfDeclarationNode>>& variantCondition,
    std::weak_ptr<Scope> scope) {
    return std::visit(
        [&scope](auto&& condition) -> bool {
            using T = std::decay_t<decltype(condition)>;
            constexpr bool isCompare =
                std::is_same_v<T, std::shared_ptr<IfCompareNode>>;
            constexpr bool isDeclaration =
                std::is_same_v<T, std::shared_ptr<IfDeclarationNode>>;
            if constexpr (isCompare)
                return interpretIfCompare(condition, scope);
            else if constexpr (isDeclaration)
                return interpretIfDeclaration(condition, scope);
        },
        variantCondition);
}

static std::optional<Value> interpretWhile(
    const std::shared_ptr<WhileNode>& whileNode,
    std::weak_ptr<Scope> parentScope) {
    auto scope = std::make_shared<Scope>(parentScope);
    while (interpretIfCondition(whileNode->getCondition(), scope)) {
        auto result = interpretBody(whileNode->getBody(), scope);
        if (result.has_value()) return result.value();
    }
    return std::nullopt;
}

static std::optional<Value> interpretForLoop(
    const std::shared_ptr<ForLoopNode>& forLoop,
    std::weak_ptr<Scope> parentScope) {
    auto iterable = interpretExpression(forLoop->getIterable(), parentScope);
    size_t iterableSize = iterable.getSize();
    return std::visit(
        [&parentScope, &forLoop,
         &iterableSize](auto&& iterable) -> std::optional<Value> {
            using T = std::decay_t<decltype(iterable)>;
            constexpr bool isVector = std::is_same_v<T, std::vector<int>>;
            constexpr bool isDynamicArray = std::is_same_v<T, DynamicArray>;
            if constexpr (isVector) {
                for (int element : iterable) {
                    auto scope = std::make_shared<Scope>(parentScope);
                    DynamicArray elementArray(1);
                    elementArray[0] = element;
                    scope->define(forLoop->getElement(),
                                  std::make_shared<Value>(elementArray, 1));
                    std::optional<Value> returnValue =
                        interpretBody(forLoop->getBody(), scope);
                    if (returnValue.has_value()) return returnValue.value();
                }
                return std::nullopt;
            } else if constexpr (isDynamicArray) {
                for (size_t i = 0; i < iterableSize; i++) {
                    int element = iterable[i];
                    auto scope = std::make_shared<Scope>(parentScope);
                    DynamicArray elementArray(1);
                    elementArray[0] = element;
                    scope->define(forLoop->getElement(),
                                  std::make_shared<Value>(elementArray, 1));
                    std::optional<Value> returnValue =
                        interpretBody(forLoop->getBody(), scope);
                    if (returnValue.has_value()) return returnValue.value();
                }
                return std::nullopt;
            }
        },
        iterable.value);
}

static std::pair<std::optional<Value>, bool> interpretIf(
    const std::shared_ptr<IfNode>& ifNode, std::weak_ptr<Scope> parentScope) {
    auto scope = std::make_shared<Scope>(parentScope);
    bool isConditionTrue = interpretIfCondition(ifNode->getCondition(), scope);
    if (isConditionTrue) return {interpretBody(ifNode->getBody(), scope), true};

    auto elseIf = ifNode->getElseIfBranches();
    if (elseIf.has_value()) {
        auto elseIfResult = interpretIf(elseIf.value(), scope);
        if (elseIfResult.second) return {elseIfResult.first, true};
    }
    auto elseBody = ifNode->getElseBody();
    if (elseBody.has_value())
        return {interpretBody(elseBody.value(), scope), true};

    return {std::nullopt, false};
}

static Value interpretReturn(const std::shared_ptr<ReturnNode>& returnNode,
                             std::weak_ptr<Scope> scope) {
    return interpretExpression(returnNode->getValue(), scope);
}

static std::optional<Value> interpretStatement(
    const std::shared_ptr<StatementNode>& statement,
    std::weak_ptr<Scope> scope) {
    return std::visit(
        [&scope](auto&& arg) -> std::optional<Value> {
            using T = std::decay_t<decltype(arg)>;
            constexpr bool isVariableBinding =
                std::is_same_v<T, std::shared_ptr<VariableBindingNode>>;
            constexpr bool isForLoop =
                std::is_same_v<T, std::shared_ptr<ForLoopNode>>;
            constexpr bool isWhile =
                std::is_same_v<T, std::shared_ptr<WhileNode>>;
            constexpr bool isIfNode =
                std::is_same_v<T, std::shared_ptr<IfNode>>;
            constexpr bool isFunctionCall =
                std::is_same_v<T, std::shared_ptr<FunctionCallNode>>;
            constexpr bool isReturn =
                std::is_same_v<T, std::shared_ptr<ReturnNode>>;
            if constexpr (isVariableBinding) {
                interpretVariableBinding(arg, scope);
                return std::nullopt;
            } else if constexpr (isForLoop) {
                return interpretForLoop(arg, scope);
            } else if constexpr (isWhile) {
                return interpretWhile(arg, scope);
            } else if constexpr (isIfNode) {
                return interpretIf(arg, scope).first;
            } else if constexpr (isFunctionCall) {
                interpretFunctionCall(arg, scope);
                return std::nullopt;
            } else if constexpr (isReturn) {
                return interpretReturn(arg, scope);
            }
        },
        statement->getValue());
}

static std::optional<Value> interpretBody(const std::shared_ptr<BodyNode>& body,
                                          std::weak_ptr<Scope> scope) {
    for (auto& statement : body->getStatements()) {
        auto returnValue = interpretStatement(statement, scope);
        if (returnValue.has_value()) return returnValue.value();
    }
    return std::nullopt;
}

static std::string valueToString(const Value& value) {
    auto array = DynamicArray::fromValue(value);
    std::string result;
    result.reserve(array.size);
    for (size_t i = 0; i < array.size; i++)
        result += static_cast<char>(array.data[i]);
    return result;
}

static Value interpretPrint(
    const std::shared_ptr<FunctionCallNode>& functionCall,
    std::weak_ptr<Scope> scope) {
    if (functionCall->getParameters().size() != 1)
        throw std::runtime_error(
            "Function print expected 1 argument but received " +
            functionCall->getParameters().size());
    auto param1 = interpretExpression(functionCall->getParameters()[0], scope);
    std::cout << valueToString(param1);

    return Value(DynamicArray(0), 0);
}

static Value interpretRead(
    const std::shared_ptr<FunctionCallNode>& functionCall,
    std::weak_ptr<Scope> scope) {
    if (functionCall->getParameters().size() != 1)
        throw std::runtime_error(
            "Function read expected 1 argument but received " +
            functionCall->getParameters().size());
    auto param1 = interpretExpression(functionCall->getParameters()[0], scope);
    auto filename = valueToString(param1);
    return interpretArray(std::make_shared<ArrayNode>(
                              ArrayNode::stringToInts(readCode(filename))),
                          scope);
}

static char getCharImmediate() {
#ifdef _WIN32
    char ch = _getch();
#else
    struct termios oldt, newt;
    char ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);  // raw mode
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
    if (ch == 3)  // Ctrl+C
        std::raise(SIGINT);
    return ch;
}

static Value interpretGetchar(
    const std::shared_ptr<FunctionCallNode>& functionCall,
    std::weak_ptr<Scope> scope) {
    if (functionCall->getParameters().size() != 0)
        throw std::runtime_error(
            "Function getchar expected 0 argument but received " +
            functionCall->getParameters().size());
    return interpretArray(std::make_shared<ArrayNode>(ArrayNode::stringToInts(
                              std::string(1, getCharImmediate()))),
                          scope);
}

static void clearTerminal() {
#ifdef _WIN32
    std::system("cls");  // Windows
#else
    std::system("clear");  // Unix/Linux/macOS
#endif
}

static Value interpretClear(
    const std::shared_ptr<FunctionCallNode>& functionCall) {
    if (functionCall->getParameters().size() != 0)
        throw std::runtime_error(
            "Function clear expected 0 argument but received " +
            functionCall->getParameters().size());
    clearTerminal();
    return Value(DynamicArray(0), 0);
}

static Value interpretRange(
    const std::shared_ptr<FunctionCallNode>& functionCall,
    std::weak_ptr<Scope> scope) {
    if (functionCall->getParameters().size() != 1)
        throw std::runtime_error(
            "Function range expected 1 argument but received " +
            functionCall->getParameters().size());
    auto param1 = DynamicArray::fromValue(
        interpretExpression(functionCall->getParameters()[0], scope));
    if (param1.size != 1)
        throw std::runtime_error(
            "Function range expected 1 argument with size [1] but received [" +
            std::to_string(param1.size) + "]");
    int length = param1[0];
    if (length < 0)
        throw std::runtime_error(
            "Function range expected 1 non-negative argument with size [1] but "
            "received the value " +
            std::string(param1));
    DynamicArray result(static_cast<size_t>(length));
    for (size_t i = 0; i < result.size; i++) result[i] = i;

    return Value(result, result.size);
}

static Value interpretExit(
    const std::shared_ptr<FunctionCallNode>& functionCall,
    std::weak_ptr<Scope> scope) {
    if (functionCall->getParameters().size() != 1)
        throw std::runtime_error(
            "Function range expected 1 argument but received " +
            functionCall->getParameters().size());
    auto param1 = DynamicArray::fromValue(
        interpretExpression(functionCall->getParameters()[0], scope));
    exit(param1[0]);
}

static Value interpretFunctionCall(
    const std::shared_ptr<FunctionCallNode>& functionCall,
    std::weak_ptr<Scope> parent) {
    if (auto lockedParent = parent.lock()) {
        if (lockedParent->hasRecursive(functionCall->getIdentifier())) {
            if (auto& functionDefinition =
                    *std::get_if<std::shared_ptr<FunctionDefinitionNode>>(
                        &lockedParent->get(functionCall->getIdentifier()))) {
                auto arguments =
                    interpretParameters(functionCall->getParameters(), parent);
                auto scope = std::make_shared<Scope>(parent);
                auto& params = functionDefinition->getParams();
                if (params.size() != arguments.size())
                    throw std::runtime_error(
                        "Function " + functionDefinition->getIdentifier() +
                        " expected " + std::to_string(params.size()) +
                        " argument(s) but received " +
                        std::to_string(arguments.size()));
                for (size_t i = 0; i < params.size(); i++) {
                    auto& param = params[i];
                    auto& argument = arguments[i];
                    scope->define(param->getIdentifier(),
                                  std::make_shared<Value>(Value::fromDescriptor(
                                      param->getDescriptor(), *argument)));
                }
                std::optional<Value> returnValue =
                    interpretBody(functionDefinition->getBody(), scope);
                if (returnValue.has_value())
                    return returnValue.value();
                else
                    return Value(DynamicArray(0), 0);
            } else {
                throw std::runtime_error(functionCall->getIdentifier() +
                                         " must be defined as a function.");
            }
        } else if (functionCall->getIdentifier() == "print") {
            return interpretPrint(functionCall, parent);
        } else if (functionCall->getIdentifier() == "read") {
            return interpretRead(functionCall, parent);
        } else if (functionCall->getIdentifier() == "getchar") {
            return interpretGetchar(functionCall, parent);
        } else if (functionCall->getIdentifier() == "clear") {
            return interpretClear(functionCall);
        } else if (functionCall->getIdentifier() == "range") {
            return interpretRange(functionCall, parent);
        } else if (functionCall->getIdentifier() == "exit") {
            return interpretExit(functionCall, parent);
        } else {
            throw std::runtime_error("Undefined function '" +
                                     functionCall->getIdentifier() + "'");
        }
    } else {
        throw std::runtime_error("Error interpreting function call");
    }
}

static void interpretFile(const std::string& filename,
                          std::shared_ptr<Scope> scope,
                          std::vector<std::string>& interpretedFiles);

static void interpretUse(const std::shared_ptr<UseNode>& use,
                         std::shared_ptr<Scope> scope,
                         std::vector<std::string>& interpretedFiles) {
    std::string filename =
        valueToString(interpretArray(use->getValue(), scope));
    if (std::find(interpretedFiles.begin(), interpretedFiles.end(), filename) ==
        interpretedFiles.end()) {
        interpretedFiles.push_back(filename);
        interpretFile(filename, scope, interpretedFiles);
    }
}

static void interpretFile(const std::string& filename,
                          std::shared_ptr<Scope> scope,
                          std::vector<std::string>& interpretedFiles) {
    const std::string code = readCode(filename);
    auto tokens = tokenize(code);
    auto root = RootNode::parse(tokens);
    for (auto value : root.getValues()) {
        std::visit(
            [&scope, &interpretedFiles](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                constexpr bool isFunctionDef =
                    std::is_same_v<T, std::shared_ptr<FunctionDefinitionNode>>;
                constexpr bool isUse =
                    std::is_same_v<T, std::shared_ptr<UseNode>>;
                if constexpr (isFunctionDef) {
                    interpretFunctionDefinition(arg, scope);
                } else if constexpr (isUse) {
                    interpretUse(arg, scope, interpretedFiles);
                }
            },
            value);
    }
}

void interpret(const std::string& filename, int argc,
               std::vector<std::string> args) {
    auto scope = std::make_shared<Scope>();
    std::vector<std::string> interpretedFiles;
    interpretFile(filename, scope, interpretedFiles);
    if (scope->has("main")) {
        std::vector<int> commandLineArgs;
        for (std::string arg : args) {
            commandLineArgs.push_back(arg.size());
            for (char c : arg) commandLineArgs.push_back(c);
        }

        std::vector<std::shared_ptr<ExpressionNode>> mainArgs = {
            std::make_shared<ExpressionNode>(std::vector<int>{argc}),
            std::make_shared<ExpressionNode>(commandLineArgs)};

        try {
            interpretFunctionCall(std::make_shared<FunctionCallNode>(
                                      std::string("main"), mainArgs),
                                  scope);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << '\n';
            exit(1);
        }
    }
}
