#pragma once
#include <iostream>
#include <fstream>
#include <memory>

using namespace std;

class BaseAST {
public:
    virtual ~BaseAST() = default;
    virtual void dump() const = 0;
    virtual void toKoopa(string &out) const = 0;
};

class CompUnitAST : public BaseAST {
public:
    unique_ptr<BaseAST> func_def;

    void dump() const override {
        cout << "CompUnit { ";
        func_def->dump();
        cout << " } ";
    }

    void toKoopa(string &out) const override {
        func_def->toKoopa(out);
    }
};

class FuncDefAST : public BaseAST {
public:
    unique_ptr<BaseAST> func_type;
    string ident;
    unique_ptr<BaseAST> block;

    void dump() const override {
        cout << "FuncDef { ";
        func_type->dump();
        cout << "," + ident + ",";
        block->dump();
        cout << " } ";
    }

    void toKoopa(string &out) const override {
        out += "fun @" + ident + "(): ";
        func_type->toKoopa(out);
        block->toKoopa(out);
    }
};

class FuncTypeAST : public BaseAST {
public:
    string type;

    void dump() const override {
        cout << "FuncType { " + type + " } ";
    }

    void toKoopa(string &out) const override {
        if(type == "int")
            out += "i32 ";
    }
};

class BlockAST : public BaseAST {
public:
    unique_ptr<BaseAST> stmt;

    void dump() const override {
        cout << "Block { ";
        stmt->dump();
        cout << " } ";
    }

    void toKoopa(string &out) const override {
        out += "{\n%entry:\n";
        stmt->toKoopa(out);
        out += "}";
    }
};

class StmtAST : public BaseAST {
public:
    int number;

    void dump() const override {
        cout << "Stmt { " + to_string(number) + " } ";
    }

    void toKoopa(string &out) const override {
        out += "    ret " + to_string(number) + "\n";
    }
};
