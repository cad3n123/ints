// Copyright 2025 Caden Crowson

#pragma once

#include <memory>
#include <string>

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
    Value(std::variant<std::vector<int>, DynamicArray> value, size_t minimum);
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
    std::variant<std::vector<int>, DynamicArray> value;
    size_t minimum;
};
