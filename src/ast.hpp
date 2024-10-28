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
    unique_ptr<BaseAST> lOrExp;

    void dump() const override {
        cout << "Exp { ";
        lOrExp->dump();
        cout << " } ";
    }

    void toKoopa(string &out) const override {
        lOrExp->toKoopa(out);
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

class MulExpAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> unaryExp;

    void dump() const override {
        cout << "MulExp1 { ";
        unaryExp->dump();
        cout << " } ";
    }
    void toKoopa(string &out) const override {
        unaryExp->toKoopa(out);
    }
};

class MulExpAST2 : public BaseAST {
public:
    unique_ptr<BaseAST> mulExp;
    string mulOp;
    unique_ptr<BaseAST> unaryExp;

    void dump() const override {
        cout << "MulExp2 { ";
        mulExp->dump();
        cout << " " << mulOp << " ";
        unaryExp->dump();
        cout << " } ";
    }
    void toKoopa(string &out) const override {
        int v0 = variable_counter - 1;

        string s1 = "";
        mulExp->toKoopa(s1);
        int v1 = variable_counter - 1;

        string s2 = "";
        unaryExp->toKoopa(s2);
        int v2 = variable_counter - 1;

        string out1 = "";
        out1 += "    %" + to_string(variable_counter++) + " = ";
        switch(mulOp[0]) {
        case '*':
            out1 += "mul ";
            break;
        case '/':
            out1 += "div ";
            break;
        case '%':
            out1 += "mod ";
            break;
        default:
            assert(false);
        }
        
        if(v1 == v0) {
            out1 += s1;
        }
        else {
            out += s1;
            out1 += "%" + to_string(v1);
        }
        out1 += ", ";
        if(v2 == v1) {
            out1 += s2;
        }
        else {
            out += s2;
            out1 += "%" + to_string(v2);
        }
        out += out1 + "\n";
    }
};

class AddExpAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> mulExp;

    void dump() const override {
        cout << "AddExp1 { ";
        mulExp->dump();
        cout << " } ";
    }
    void toKoopa(string &out) const override {
        mulExp->toKoopa(out);
    }
};

class AddExpAST2 : public BaseAST {
public:
    unique_ptr<BaseAST> addExp;
    string addOp;
    unique_ptr<BaseAST> mulExp;

    void dump() const override {
        cout << "AddExp2 { ";
        addExp->dump();
        cout << " " << addOp << " ";
        mulExp->dump();
        cout << " } ";
    }
    void toKoopa(string &out) const override {
        int v0 = variable_counter - 1;

        string s1 = "";
        addExp->toKoopa(s1);
        int v1 = variable_counter - 1;

        string s2 = "";
        mulExp->toKoopa(s2);
        int v2 = variable_counter - 1;

        string out1 = "";
        out1 += "    %" + to_string(variable_counter++) + " = ";
        switch(addOp[0]) {
        case '+':
            out1 += "add ";
            break;
        case '-':
            out1 += "sub ";
            break;
        default:
            assert(false);
        }
        
        if(v1 == v0) {
            out1 += s1;
        }
        else {
            out += s1;
            out1 += "%" + to_string(v1);
        }
        out1 += ", ";
        if(v2 == v1) {
            out1 += s2;
        }
        else {
            out += s2;
            out1 += "%" + to_string(v2);
        }
        out += out1 + "\n";
    }
};

class RelExpAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> addExp;

    void dump() const override {
        cout << "RelExp1 { ";
        addExp->dump();
        cout << " } ";
    }
    void toKoopa(string &out) const override {
        addExp->toKoopa(out);
    }
};

class RelExpAST2 : public BaseAST {
public:
    unique_ptr<BaseAST> relExp;
    string relOp;
    unique_ptr<BaseAST> addExp;
    
    void dump() const override {
        cout << "RelExp2 { ";
        addExp->dump();
        cout << " " << relOp << " ";
        addExp->dump();
        cout << " } ";
    }
    void toKoopa(string &out) const override {

    }
};

class EqExpAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> relExp;

    void dump() const override {
        cout << "EqExp1 { ";
        relExp->dump();
        cout << " } ";
    }
    void toKoopa(string &out) const override {
        relExp->toKoopa(out);
    }
};

class EqExpAST2 : public BaseAST {
public:
    unique_ptr<BaseAST> eqExp;
    string eqOp;
    unique_ptr<BaseAST> relExp;

    void dump() const override {
        cout << "EqExp2 { ";
        eqExp->dump();
        cout << " " << eqOp << " ";
        relExp->dump();
        cout << " } ";
    }
    void toKoopa(string &out) const override {

    }
};

class LAndExpAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> eqExp;

    void dump() const override {
        cout << "LAndExp1 { ";
        eqExp->dump();
        cout << " } ";
    }
    void toKoopa(string &out) const override {
        eqExp->toKoopa(out);
    }
};

class LAndExpAST2 : public BaseAST {
public:
    unique_ptr<BaseAST> lAndExp;
    unique_ptr<BaseAST> eqExp;

    void dump() const override {
        cout << "LAndExp2 { ";
        lAndExp->dump();
        cout << " && ";
        eqExp->dump();
        cout << " } ";
    }
    void toKoopa(string &out) const override {

    }
};

class LOrExpAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> lAndExp;

    void dump() const override {
        cout << "LOrExp1 { ";
        lAndExp->dump();
        cout << " } ";
    }
    void toKoopa(string &out) const override {
        lAndExp->toKoopa(out);
    }
};

class LOrExpAST2 : public BaseAST {
public:
    unique_ptr<BaseAST> lOrExp;
    unique_ptr<BaseAST> lAndExp;

    void dump() const override {
        cout << "LOrExp2 { ";
        lOrExp->dump();
        cout << " || ";
        lAndExp->dump();
        cout << " } ";
    }
    void toKoopa(string &out) const override {

    }
};