#pragma once
#include <iostream>
#include <memory>

class BaseAST {
public:
    virtual ~BaseAST() = default;
    virtual void dump() const = 0;
};

class CompUnitAST : public BaseAST {
public:
    // 用智能指针管理对象
    std::unique_ptr<BaseAST> func_def;

    void dump() const override {
        std::cout << "CompUnit { ";
        func_def->dump();
        std::cout << " } ";
    }
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void dump() const override {
        std::cout << "FuncDef { ";
        func_type->dump();
        std::cout << "," + ident + ",";
        block->dump();
        std::cout << " } ";
    }
};

class FuncTypeAST : public BaseAST {
public:
    std::string type;

    void dump() const override {
        std::cout << "FuncType { " + type + " } ";
    }
};

class BlockAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> stmt;

    void dump() const override {
        std::cout << "Block { ";
        stmt->dump();
        std::cout << " } ";
    }
};

class StmtAST : public BaseAST {
public:
    int number;

    void dump() const override {
        std::cout << "Stmt { " + to_string(number) + " } ";
    }
};
