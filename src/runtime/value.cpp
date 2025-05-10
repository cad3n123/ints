// Copyright 2025 Caden Crowson

#include "runtime/value.h"

#include <memory>
#include <stdexcept>
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
            if constexpr (isVector) {
                DynamicArray result(value.size());
                for (size_t i = 0; i < value.size(); i++) result[i] = value[i];
                return result;
            } else if constexpr (isDynamic) {
                return DynamicArray(value);
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

Value::Value(std::variant<std::vector<int>, DynamicArray> value, size_t minimum)
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
                        if constexpr (std::is_same_v<T, std::vector<int>>) {
                            if (this->minimum > other_arg.size())
                                throw std::runtime_error(
                                    "Cannot set value. Destination minimum is "
                                    "larger than the sources length");
                            this_arg = other_arg;
                        } else if constexpr (std::is_same_v<T, DynamicArray>) {
                            if (this->minimum > other.minimum)
                                throw std::runtime_error(
                                    "Cannot set value. Destination minimum (" +
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
                        }
                    },
                    other.value);
            } else if constexpr (std::is_same_v<T, DynamicArray>) {
                std::visit(
                    [this, &other, &this_arg](auto&& other_arg) {
                        using T = std::decay_t<decltype(other_arg)>;
                        if constexpr (std::is_same_v<T, std::vector<int>>) {
                            if (this->minimum != other_arg.size())
                                throw std::runtime_error(
                                    "Cannot set value. Destination length is "
                                    "not equal to the sources length");
                            for (size_t i = 0; i < minimum; i++)
                                this_arg[i] = other_arg[i];
                        } else if constexpr (std::is_same_v<T, DynamicArray>) {
                            if (this->minimum != other.minimum)
                                throw std::runtime_error(
                                    "Cannot set value. Destination length is "
                                    "not equal to the sources length");
                            for (size_t i = 0; i < minimum; i++)
                                this_arg[i] = other_arg[i];
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
