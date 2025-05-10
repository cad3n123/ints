// Copyright 2025 Caden Crowson

#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

#include "parser/parse.h"
#include "runtime/value.h"

class Scope {
 public:
    explicit Scope(std::weak_ptr<Scope> parent = std::weak_ptr<Scope>());
    bool has(const std::string& name) const;
    bool hasRecursive(const std::string& name) const;
    const std::variant<Value, std::shared_ptr<FunctionDefinitionNode>>& get(
        const std::string& name) const;
    void set(const std::string& name,
             const std::variant<Value, std::shared_ptr<FunctionDefinitionNode>>&
                 value);
    void define(
        const std::string& name,
        const std::variant<Value, std::shared_ptr<FunctionDefinitionNode>>&
            value);

 private:
    std::weak_ptr<Scope> parent;
    std::unordered_map<
        std::string,
        std::variant<Value, std::shared_ptr<FunctionDefinitionNode>>>
        variables;
};

void interpret(const RootNode& root);
