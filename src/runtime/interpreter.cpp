// Copyright 2025 Caden Crowson

#include "runtime/interpreter.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

DynamicArray::DynamicArray(const DynamicArray& dynamicArray)
    : DynamicArray(dynamicArray.size) {
    for (size_t i = 0; i < size; i++) data[i] = dynamicArray.data[i];
}

DynamicArray::DynamicArray(size_t size)
    : data(std::make_unique<int[]>(size)), size(size) {}

DynamicArray::DynamicArray(std::unique_ptr<int[]> data, size_t size)
    : data(std::move(data)), size(size) {}

DynamicArray DynamicArray::fromValue(const Value& value) {
    return std::visit(
        [](auto&& value) -> DynamicArray {
            using T = std::decay_t<decltype(value)>;
            constexpr bool isVector = std::is_same_v<T, std::vector<int>>;
            constexpr bool isDynamic = std::is_same_v<T, DynamicArray>;
            constexpr bool isFunctionDef =
                std::is_same_v<T, std::shared_ptr<FunctionDefinitionNode>>;
            if constexpr (isVector) {
                DynamicArray result(value.size());
                for (size_t i = 0; i < value.size(); i++) result[i] = value[i];
                return result;
            } else if constexpr (isDynamic) {
                return DynamicArray(value);
            } else if constexpr (isFunctionDef) {
                return DynamicArray(0);
            }
        },
        value.value);
}

int& DynamicArray::at(size_t i) {
    if (i >= size) {
        throw std::out_of_range("Index " + std::to_string(i) +
                                " is out of bounds");
    }
    return data[i];
}

const int& DynamicArray::at(size_t i) const {
    if (i >= size) {
        throw std::out_of_range("Index " + std::to_string(i) +
                                " is out of bounds");
    }
    return data[i];
}

DynamicArray::operator std::string() const {
    std::string result = "[ ";
    for (size_t i = 0; i < size; i++)
        result += std::to_string(data[i]) + (i + 1 < size ? ", " : "");

    result += " ]";
    return result;
}

int& DynamicArray::operator[](size_t i) { return data[i]; }

const int& DynamicArray::operator[](size_t i) const { return data[i]; }

DynamicArray DynamicArray::operator+(const DynamicArray& other) {
    if (size != other.size)
        throw std::runtime_error("Cannot add arrays with different sizes");
    DynamicArray result(size);
    for (size_t i = 0; i < size; i++) result[i] = data[i] + other[i];
    return result;
}

DynamicArray DynamicArray::operator-(const DynamicArray& other) {
    if (size != other.size)
        throw std::runtime_error("Cannot subtract arrays with different sizes");
    DynamicArray result(size);
    for (size_t i = 0; i < size; i++) result[i] = data[i] - other[i];
    return result;
}

DynamicArray DynamicArray::operator*(const DynamicArray& other) {
    if (size != other.size)
        throw std::runtime_error("Cannot multiply arrays with different sizes");
    DynamicArray result(size);
    for (size_t i = 0; i < size; i++) result[i] = data[i] * other[i];
    return result;
}

DynamicArray DynamicArray::operator/(const DynamicArray& other) {
    if (size != other.size)
        throw std::runtime_error("Cannot divide arrays with different sizes");
    DynamicArray result(size);
    for (size_t i = 0; i < size; i++) result[i] = data[i] / other[i];
    return result;
}

bool DynamicArray::operator==(const DynamicArray& other) const {
    if (size != other.size) return false;
    for (size_t i = 0; i < size; i++)
        if (data[i] != other[i]) return false;
    return true;
}

bool DynamicArray::operator!=(const DynamicArray& other) const {
    if (size != other.size) return false;
    for (size_t i = 0; i < size; i++)
        if (data[i] == other[i]) return false;
    return true;
}

bool DynamicArray::operator<(const DynamicArray& other) const {
    if (size != other.size) return false;
    for (size_t i = 0; i < size; i++)
        if (data[i] >= other[i]) return false;
    return true;
}

bool DynamicArray::operator<=(const DynamicArray& other) const {
    if (size != other.size) return false;
    for (size_t i = 0; i < size; i++)
        if (data[i] > other[i]) return false;
    return true;
}

bool DynamicArray::operator>(const DynamicArray& other) const {
    if (size != other.size) return false;
    for (size_t i = 0; i < size; i++)
        if (data[i] <= other[i]) return false;
    return true;
}

bool DynamicArray::operator>=(const DynamicArray& other) const {
    if (size != other.size) return false;
    for (size_t i = 0; i < size; i++)
        if (data[i] < other[i]) return false;
    return true;
}

Value::Value(const Value& value) : value(value.value), minimum(value.minimum) {}

Value::Value(std::variant<std::vector<int>, DynamicArray,
                          std::shared_ptr<FunctionDefinitionNode>>
                 value,
             size_t minimum)
    : value(std::move(value)), minimum(minimum) {}

Value Value::fromDescriptor(const ArrayDescriptor& descriptor,
                            std::optional<Value> value) {
    if (descriptor.getCanGrow()) {
        std::vector<int> dynamicArray;
        if (descriptor.getSize().has_value()) {
            dynamicArray.reserve(descriptor.getSize().value());
        }
        Value result(dynamicArray, dynamicArray.size());
        if (value.has_value()) result = value.value();
        return result;
    } else {
        if (descriptor.getSize().has_value()) {
            auto size = descriptor.getSize().value();
            Value result(DynamicArray(size), size);
            if (value.has_value()) result = value.value();
            return result;
        } else {
            if (value.has_value()) return Value(value.value());
            throw std::runtime_error(
                "Static array cannot be defined without a value");
        }
    }
}

size_t Value::getSize() const { return DynamicArray::fromValue(*this).size; }

Value::operator std::string() const {
    return std::string(DynamicArray::fromValue(*this));
}

Value& Value::operator=(const Value& other) {
    std::visit(
        [this, &other](auto&& this_arg) {
            using T = std::decay_t<decltype(this_arg)>;
            if constexpr (std::is_same_v<T, std::vector<int>>) {
                std::visit(
                    [this, &other, &this_arg](auto&& other_arg) {
                        using T = std::decay_t<decltype(other_arg)>;
                        constexpr bool isFunctionDefinition =
                            std::is_same_v<T, FunctionDefinitionNode*>;
                        if constexpr (std::is_same_v<T, std::vector<int>>) {
                            if (this->minimum > other_arg.size())
                                throw std::runtime_error(
                                    "1Cannot set value. Destination minimum is "
                                    "larger than the sources length");
                            this_arg = other_arg;
                        } else if constexpr (std::is_same_v<T, DynamicArray>) {
                            if (this->minimum > other.minimum)
                                throw std::runtime_error(
                                    "2Cannot set value. Destination minimum (" +
                                    std::to_string(this->minimum) +
                                    ") is larger than the sources length (" +
                                    std::to_string(other.minimum) + ")");
                            for (size_t i = 0; i < this_arg.size(); i++) try {
                                    this_arg[i] = other_arg.at(i);
                                } catch (const std::out_of_range& e) {
                                    this_arg[i] = 0;
                                }
                            for (size_t i = this_arg.size(); i < other_arg.size;
                                 i++)
                                this_arg.push_back(other_arg[i]);
                        } else if constexpr (isFunctionDefinition) {
                            throw std::runtime_error(
                                "Cannot set dynamic array to function "
                                "definition");
                        }
                    },
                    other.value);
            } else if constexpr (std::is_same_v<T, DynamicArray>) {
                std::visit(
                    [this, &other, &this_arg](auto&& other_arg) {
                        using T = std::decay_t<decltype(other_arg)>;
                        constexpr bool isFunctionDefinition =
                            std::is_same_v<T, FunctionDefinitionNode*>;
                        if constexpr (std::is_same_v<T, std::vector<int>>) {
                            if (this->minimum != other_arg.size())
                                throw std::runtime_error(
                                    "3Cannot set value. Destination length is "
                                    "not equal to the sources length");
                            for (size_t i = 0; i < minimum; i++)
                                this_arg[i] = other_arg[i];
                        } else if constexpr (std::is_same_v<T, DynamicArray>) {
                            if (this->minimum != other.minimum)
                                throw std::runtime_error(
                                    "4Cannot set value. Destination length is "
                                    "not equal to the sources length");
                            for (size_t i = 0; i < minimum; i++)
                                this_arg[i] = other_arg[i];
                        } else if constexpr (isFunctionDefinition) {
                            throw std::runtime_error(
                                "Cannot set array to function "
                                "definition");
                        }
                    },
                    other.value);
            } else if constexpr (std::is_same_v<T, FunctionDefinitionNode*>) {
                std::visit(
                    [this, &other, &this_arg](auto&& other_arg) {
                        using T = std::decay_t<decltype(other_arg)>;
                        constexpr bool isFunctionDefinition =
                            std::is_same_v<T, FunctionDefinitionNode*>;
                        if constexpr (std::is_same_v<T, std::vector<int>> ||
                                      std::is_same_v<T, DynamicArray>) {
                            throw std::runtime_error(
                                "Cannot identifier associated with a function "
                                "to an array.");
                        } else if constexpr (isFunctionDefinition) {
                            this_arg = other_arg;
                        }
                    },
                    other.value);
            }
        },
        value);
    return *this;
}

bool Value::sameSize(const Value& other) const {
    return DynamicArray::fromValue(*this).size ==
           DynamicArray::fromValue(other).size;
}

Value Value::operator+(const Value& other) {
    if (!sameSize(other))
        throw std::runtime_error("Cannot add arrays with different sizes");
    DynamicArray left = DynamicArray::fromValue(*this);
    DynamicArray right = DynamicArray::fromValue(other);
    return Value(left + right, left.size);
}

Value Value::operator-(const Value& other) {
    if (!sameSize(other))
        throw std::runtime_error("Cannot subtract arrays with different sizes");
    DynamicArray left = DynamicArray::fromValue(*this);
    DynamicArray right = DynamicArray::fromValue(other);
    return Value(left - right, left.size);
}

Value Value::operator*(const Value& other) {
    if (!sameSize(other))
        throw std::runtime_error("Cannot multiply arrays with different sizes");
    DynamicArray left = DynamicArray::fromValue(*this);
    DynamicArray right = DynamicArray::fromValue(other);
    return Value(left * right, left.size);
}

Value Value::operator/(const Value& other) {
    if (!sameSize(other))
        throw std::runtime_error("Cannot divide arrays with different sizes");
    DynamicArray left = DynamicArray::fromValue(*this);
    DynamicArray right = DynamicArray::fromValue(other);
    return Value(left / right, left.size);
}

bool Value::operator==(const Value& other) const {
    if (!sameSize(other)) return false;
    return DynamicArray::fromValue(*this) == DynamicArray::fromValue(other);
}

bool Value::operator!=(const Value& other) const {
    if (!sameSize(other)) return false;
    return DynamicArray::fromValue(*this) != DynamicArray::fromValue(other);
}

bool Value::operator<(const Value& other) const {
    if (!sameSize(other)) return false;
    return DynamicArray::fromValue(*this) < DynamicArray::fromValue(other);
}

bool Value::operator<=(const Value& other) const {
    if (!sameSize(other)) return false;
    return DynamicArray::fromValue(*this) <= DynamicArray::fromValue(other);
}

bool Value::operator>(const Value& other) const {
    if (!sameSize(other)) return false;
    return DynamicArray::fromValue(*this) > DynamicArray::fromValue(other);
}

bool Value::operator>=(const Value& other) const {
    if (!sameSize(other)) return false;
    return DynamicArray::fromValue(*this) >= DynamicArray::fromValue(other);
}

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

const Value& Scope::get(const std::string& name) const {
    if (has(name)) return variables.at(name);
    if (auto parent_locked = parent.lock()) {
        return parent_locked->get(name);
    }
    throw std::runtime_error("Undefined variable: " + name);
}

void Scope::set(const std::string& name, const Value& value) {
    if (has(name)) {
        variables.insert_or_assign(name, value);
    } else if (auto parent_locked = parent.lock()) {
        parent_locked->set(name, value);
    } else {
        throw std::runtime_error("Undefined variable for assignment: " + name);
    }
}

void Scope::define(const std::string& name, const Value& value) {
    variables.emplace(name, value);
}

static void interpretFunctionDefinition(
    std::shared_ptr<FunctionDefinitionNode> functionDefinition,
    std::weak_ptr<Scope> parent) {
    if (auto lockedParent = parent.lock()) {
        lockedParent->define(functionDefinition->getIdentifier(),
                             Value(functionDefinition, 0));
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
                    return lockedScope->get(arg);
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
            constexpr bool isFunctionDef =
                std::is_same_v<T, std::shared_ptr<FunctionDefinitionNode>>;
            if constexpr (isVector) {
                for (size_t i = 0; i < size1; i++) result[i] = arg[i];
            } else if constexpr (isDynamic) {
                for (size_t i = 0; i < size1; i++) result[i] = arg[i];
            } else if constexpr (isFunctionDef) {
                throw std::runtime_error("Error interpreting append method");
            }
        },
        value.value);
    std::visit(
        [&result, &size1, &size2](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            constexpr bool isVector = std::is_same_v<T, std::vector<int>>;
            constexpr bool isDynamic = std::is_same_v<T, DynamicArray>;
            constexpr bool isFunctionDef =
                std::is_same_v<T, std::shared_ptr<FunctionDefinitionNode>>;
            if constexpr (isVector) {
                for (size_t i = 0; i < size2; i++) result[size1 + i] = arg[i];
            } else if constexpr (isDynamic) {
                for (size_t i = 0; i < size2; i++) result[size1 + i] = arg[i];
            } else if constexpr (isFunctionDef) {
                throw std::runtime_error("Error interpreting append method");
            }
        },
        param1->value);
    return Value(result, size1 + size2);
}

static Value applySqrt(const Value& value,
                       std::vector<std::shared_ptr<Value>> parameters) {
    if (parameters.size() != 0)
        throw std::runtime_error("sqrt expects 0 argument with type []");
    DynamicArray fixedValue = DynamicArray::fromValue(value);
    DynamicArray result(fixedValue.size);
    for (size_t i = 0; i < fixedValue.size; i++)
        result[i] = sqrt(fixedValue[i]);
    return Value(result, result.size);
}

static Value applyMethod(const Value& value,
                         const std::shared_ptr<MethodNode>& method,
                         std::weak_ptr<Scope> scope) {
    auto parameters = interpretParameters(method->getParameters(), scope);
    if (method->getIdentifier() == "append")
        return applyAppend(value, parameters);
    else if (method->getIdentifier() == "sqrt")
        return applySqrt(value, parameters);
    else
        throw std::runtime_error("Unknown method " + method->getIdentifier());
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
                    size_t size = resultValue->getSize();
                    size_t end = arg->getEnd().value_or(size);
                    size_t start = arg->getStart().value_or(0);
                    if (end <= start)
                        throw std::runtime_error(
                            "Array Range upper bound must be greater than the "
                            "lower bound");
                    size_t newSize = end - start;
                    DynamicArray result(newSize);
                    if (end > size)
                        throw std::runtime_error(
                            "Array range bounds must be smaller than the "
                            "length of the array");
                    std::visit(
                        [&result, &newSize, &start](auto&& valueValue) {
                            using T = std::decay_t<decltype(valueValue)>;
                            constexpr bool isVector =
                                std::is_same_v<T, std::vector<int>>;
                            constexpr bool isArray =
                                std::is_same_v<T, DynamicArray>;
                            constexpr bool isFunctionDefinition =
                                std::is_same_v<
                                    T, std::shared_ptr<FunctionDefinitionNode>>;
                            if constexpr (isVector) {
                                for (size_t i = 0; i < newSize; i++) {
                                    result[i] = valueValue[i + start];
                                }
                            } else if constexpr (isArray) {
                                for (size_t i = 0; i < newSize; i++) {
                                    result[i] = valueValue[i + start];
                                }
                            } else if constexpr (isFunctionDefinition) {
                                throw std::runtime_error(
                                    "Error interpreting postfix");
                            }
                        },
                        resultValue->value);
                    return Value(result, newSize);
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
        lockedScope->define(
            variableDeclaration->getIdentifier(),
            Value::fromDescriptor(variableDeclaration->getDescriptor(), value));
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

        auto right = interpretExpression(variableAssignment->getRight(), scope);
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
            constexpr bool isFunctionDef =
                std::is_same_v<T, std::shared_ptr<FunctionDefinitionNode>>;
            if constexpr (isVector) {
                for (int element : iterable) {
                    auto scope = std::make_shared<Scope>(parentScope);
                    DynamicArray elementArray(1);
                    elementArray[0] = element;
                    scope->define(forLoop->getElement(),
                                  Value(elementArray, 1));
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
                                  Value(elementArray, 1));
                    std::optional<Value> returnValue =
                        interpretBody(forLoop->getBody(), scope);
                    if (returnValue.has_value()) return returnValue.value();
                }
                return std::nullopt;
            } else if constexpr (isFunctionDef) {
                throw std::runtime_error("Error interpreting for loop");
            }
        },
        iterable.value);
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
                Value::fromDescriptor(descriptor, value));
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

// static void interpretFunctionCall(
//     const std::shared_ptr<FunctionCallNode>& functionCall,
//     std::weak_ptr<Scope> scope) {
//     if (auto lockedScope = scope.lock()) {
//         if (auto& function =
//                 *std::get_if<std::shared_ptr<FunctionDefinitionNode>>(
//                     &lockedScope->get(functionCall->getIdentifier()).value))
//                     {
//             interpretFunctionCall(
//                 function, scope,
//                 interpretParameters(functionCall->getParameters(), scope));
//         } else {
//             throw std::runtime_error("Unknown function " +
//                                      functionCall->getIdentifier());
//         }
//     } else {
//         throw std::runtime_error("Error interpreting function call");
//     }
// }

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

static Value interpretPrint(
    const std::shared_ptr<FunctionCallNode>& functionCall,
    std::weak_ptr<Scope> scope) {
    if (functionCall->getParameters().size() != 1)
        throw std::runtime_error(
            "Function print expected 1 argument but received " +
            functionCall->getParameters().size());
    auto param1 = DynamicArray::fromValue(
        interpretExpression(functionCall->getParameters()[0], scope));
    std::string result;
    result.reserve(param1.size);
    for (size_t i = 0; i < param1.size; i++)
        result += static_cast<char>(param1.data[i]);
    std::cout << result;

    return Value(DynamicArray(0), 0);
}

static Value interpretFunctionCall(
    const std::shared_ptr<FunctionCallNode>& functionCall,
    std::weak_ptr<Scope> parent) {
    if (auto lockedParent = parent.lock()) {
        if (lockedParent->hasRecursive(functionCall->getIdentifier())) {
            if (auto& functionDefinition =
                    *std::get_if<std::shared_ptr<FunctionDefinitionNode>>(
                        &lockedParent->get(functionCall->getIdentifier())
                             .value)) {
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
                                  Value::fromDescriptor(param->getDescriptor(),
                                                        *argument));
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
        } else {
            throw std::runtime_error("Undefined function '" +
                                     functionCall->getIdentifier() + "'");
        }
    } else {
        throw std::runtime_error("Error interpreting function call");
    }
}

void interpret(const RootNode& root) {
    auto scope = std::make_shared<Scope>();
    for (auto value : root.getValues()) {
        std::visit(
            [&scope](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<
                                  T, std::shared_ptr<FunctionDefinitionNode>>) {
                    interpretFunctionDefinition(arg, scope);
                }
            },
            value);
    }
    if (scope->has("main")) {
        interpretFunctionCall(
            std::make_shared<FunctionCallNode>(
                std::string("main"),
                std::vector<std::shared_ptr<ExpressionNode>>()),
            scope);
    }
}
