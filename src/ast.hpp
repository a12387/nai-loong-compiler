#pragma once
#include <iostream>
#include <fstream>
#include <memory>
#include <cassert>
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
        cout << " } \n";
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
    unique_ptr<BaseAST> exp;

    void dump() const override {
        cout << "Stmt { ";
        exp->dump();
        cout << " } ";
    }

    void toKoopa(string &out) const override {
        out += "    ret ";
        string s = "";
        exp->toKoopa(s);
        out += s;
        out += "\n";
    }
};

class ExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> unaryExp;

    void dump() const override {
        cout << "Exp { ";
        unaryExp->dump();
        cout << " } ";
    }

    void toKoopa(string &out) const override {
        unaryExp->toKoopa(out);
    }
};

class PrimaryExpAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> exp;

    void dump() const override {
        cout << "PrimaryExp { ";
        exp->dump();
        cout << " } ";
    }
    void toKoopa(string &out) const override {
        exp->toKoopa(out);
    }
};

class PrimaryExpAST2: public BaseAST {
public:
    int num;
    void dump() const override {
        cout << "Primary { ";
        cout << num;
        cout << " } ";
    }
    void toKoopa(string &out) const override {
        out = to_string(num);
    }
};

class UnaryExpAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> primaryExp;

    void dump() const override {
        cout << "UnaryExp { ";
        primaryExp->dump();
        cout << " } ";
    }
    void toKoopa(string &out) const override {
        primaryExp->toKoopa(out);
    }
};

class UnaryExpAST2 : public BaseAST {
public:
    string unaryOp;
    unique_ptr<BaseAST> unaryExp;

    void dump() const override {
        cout << "UnaryExp { ";
        cout << "\'" + unaryOp + "\' ";
        unaryExp->dump();
        cout << " } ";
    }
    void toKoopa(string &out) const override {
        unaryExp->toKoopa(out);
        int num;
        sscanf(out.c_str(),"%d",&num);
        switch(unaryOp[0]) {
        case '+':
            out = to_string(num);
            break;
        case '-':
            out = to_string(-num);
            break;
        case '!':
            out = to_string(!num);
            break;
        default:
            assert(false);
        }
    }
};