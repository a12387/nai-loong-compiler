#pragma once
#include <iostream>
#include <fstream>
#include <memory>
#include <cassert>
#include <stack>
using namespace std;

static int variable_counter = 0;
static stack<char> unaryOpStack;

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
        string s = "";
        exp->toKoopa(s);
        if(variable_counter != 0) {
            out += s;
            out += "    ret %" + to_string(variable_counter - 1);
        }
        else {
            out += "    ret " + s;
        }
        out += "\n";
        
        variable_counter = 0;
        assert(unaryOpStack.empty());
        
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
        cout << "PrimaryExp1 { ";
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
        cout << "PrimaryExp2 { ";
        cout << num;
        cout << " } ";
    }
    void toKoopa(string &out) const override {
        while(!unaryOpStack.empty() && unaryOpStack.top() == '+') {
            unaryOpStack.pop();
        }
        
        if(unaryOpStack.empty()) {
            out += to_string(num);
            return;
        }
        out += "    %" + to_string(variable_counter++) + " = "; 
        switch(unaryOpStack.top()) {
        case '-':
            out += "sub 0, " + to_string(num) + "\n";
            break;
        case '!':
            out += "eq " + to_string(num) + ", 0\n";
            break;
        default:
            assert(false);
        }
        unaryOpStack.pop();
    }
};

class UnaryExpAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> primaryExp;

    void dump() const override {
        cout << "UnaryExp1 { ";
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
        cout << "UnaryExp2 { ";
        cout << "\'" + unaryOp + "\' ";
        unaryExp->dump();
        cout << " } ";
    }
    void toKoopa(string &out) const override {
        unaryOpStack.push(unaryOp[0]);
        unaryExp->toKoopa(out);
        while(!unaryOpStack.empty() && unaryOpStack.top() == '+') {
            unaryOpStack.pop();
        }
        if(unaryOpStack.empty()) {
            return;
        }
        switch(unaryOpStack.top()) {
        case '-':
            out += "    %" + to_string(variable_counter) + " = sub 0, %" + to_string(variable_counter - 1) + "\n";
            variable_counter++;
            break;
        case '!':
            out += "    %" + to_string(variable_counter) + " = eq %" + to_string(variable_counter - 1) + ", 0\n";
            variable_counter++;
            break;
        default:
            assert(false);
        }
        unaryOpStack.pop();
    }
};