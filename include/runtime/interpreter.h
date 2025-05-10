// Copyright 2025 Caden Crowson

#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

#include "parser/parse.h"

class Value;

struct DynamicArray {
    std::unique_ptr<int[]> data;
    size_t size;

    DynamicArray(const DynamicArray& dynamicArray);
    explicit DynamicArray(size_t n);
    DynamicArray(std::unique_ptr<int[]> data, size_t size);
    static DynamicArray fromValue(const Value& value);
    int& at(size_t i);
    const int& at(size_t i) const;
    operator std::string() const;
    int& operator[](size_t i);
    const int& operator[](size_t i) const;
    DynamicArray operator+(const DynamicArray& other);
    DynamicArray operator-(const DynamicArray& other);
    DynamicArray operator*(const DynamicArray& other);
    DynamicArray operator/(const DynamicArray& other);
    bool operator==(const DynamicArray& other) const;
    bool operator!=(const DynamicArray& other) const;
    bool operator<(const DynamicArray& other) const;
    bool operator<=(const DynamicArray& other) const;
    bool operator>(const DynamicArray& other) const;
    bool operator>=(const DynamicArray& other) const;
};

class Value {
 public:
    Value(const Value& value);
    Value(std::variant<std::vector<int>, DynamicArray,
                       std::shared_ptr<FunctionDefinitionNode>>
              value,
          size_t minimum);
    static Value fromDescriptor(const ArrayDescriptor& descriptor,
                                std::optional<Value> value);
    bool sameSize(const Value& other) const;
    size_t getSize() const;
    operator std::string() const;
    Value& operator=(const Value& other);
    Value operator+(const Value& other);
    Value operator-(const Value& other);
    Value operator*(const Value& other);
    Value operator/(const Value& other);
    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const;
    bool operator<(const Value& other) const;
    bool operator<=(const Value& other) const;
    bool operator>(const Value& other) const;
    bool operator>=(const Value& other) const;
    std::variant<std::vector<int>, DynamicArray,
                 std::shared_ptr<FunctionDefinitionNode>>
        value;
    size_t minimum;
};

class Scope {
 public:
    explicit Scope(std::weak_ptr<Scope> parent = std::weak_ptr<Scope>());
    bool has(const std::string& name) const;
    bool hasRecursive(const std::string& name) const;
    const Value& get(const std::string& name) const;
    void set(const std::string& name, const Value& value);
    void define(const std::string& name, const Value& value);

 private:
    std::weak_ptr<Scope> parent;
    std::unordered_map<std::string, Value> variables;
};

void interpret(const RootNode& root);
