#pragma once
#include <iostream>
#include <fstream>
#include <memory>
#include <cassert>
#include <stack>
#include <unordered_map>
#include <koopa.h>
using namespace std;

static unordered_map<string, koopa_raw_binary_op_t> op_map = {
    {"+", KOOPA_RBO_ADD},
    {"-", KOOPA_RBO_SUB},
    {"*", KOOPA_RBO_MUL},
    {"/", KOOPA_RBO_DIV},
    {"%", KOOPA_RBO_MOD},
    {"!", KOOPA_RBO_NOT_EQ},
    {"<", KOOPA_RBO_LT},
    {">", KOOPA_RBO_GT},
    {"<=", KOOPA_RBO_LE},
    {">=", KOOPA_RBO_GE},
    {"==", KOOPA_RBO_EQ},
    {"!=", KOOPA_RBO_NOT_EQ},
    {"&&", KOOPA_RBO_AND},
    {"||", KOOPA_RBO_OR}
};


class BaseAST {
public:
    virtual ~BaseAST() = default;
    virtual void dump() const = 0;
    virtual void* toKoopa() const = 0;
};

class CompUnitAST : public BaseAST {
public:
    unique_ptr<BaseAST> func_def;

    void dump() const override {
        cout << "CompUnit { ";
        func_def->dump();
        cout << " } \n";
    }

    void* toKoopa() const override {
        auto raw = new koopa_raw_program_t;

        raw->values.buffer = nullptr;
        raw->values.kind = KOOPA_RSIK_VALUE;
        raw->values.len = 0;

        auto buf = new void*;
        *buf = func_def->toKoopa();
        raw->funcs.buffer = (const void **)(buf);
        raw->funcs.kind = KOOPA_RSIK_FUNCTION;
        raw->funcs.len = 1;

        return raw;
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

    void* toKoopa() const override {
        auto raw = new koopa_raw_function_data_t;

        raw->name = ("@"+ident).c_str();
        raw->params = {
            nullptr,
            0,
            KOOPA_RSIK_VALUE
        };
        
        auto ty = new koopa_raw_type_kind_t;
        ty->tag = KOOPA_RTT_FUNCTION;
        ty->data.function.params = {
            nullptr,
            0,
            KOOPA_RSIK_VALUE
        };
        ty->data.function.ret = (koopa_raw_type_kind_t *)func_type->toKoopa();
        raw->ty = ty;


        auto buf = new void*;
        *buf = block->toKoopa();
        raw->bbs = {
            (const void **)buf,
            1,
            KOOPA_RSIK_BASIC_BLOCK
        };

        return raw;
    }
};

class FuncTypeAST : public BaseAST {
public:
    string type;

    void dump() const override {
        cout << "FuncType { " + type + " } ";
    }

    void* toKoopa() const override {
        auto ret = new koopa_raw_type_kind_t;
        if(type == "int")
            ret->tag = KOOPA_RTT_INT32;
        else
            ret->tag = KOOPA_RTT_UNIT;

        return ret;
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

    void* toKoopa() const override {
        auto raw = new koopa_raw_basic_block_data_t;

        raw->name = "entry";
        raw->params = {
            nullptr,
            0,
            KOOPA_RSIK_VALUE
        };
        raw->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_UNKNOWN
        };

        auto buf = new void*;
        *buf = stmt->toKoopa();
        raw->insts = {
            (const void **)buf,
            1,
            KOOPA_RSIK_VALUE
        };
        
        return raw;
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

    void* toKoopa() const override {
        auto raw = new koopa_raw_value_data_t;
        
        raw->kind.tag = KOOPA_RVT_RETURN;
        raw->kind.data.ret.value = (koopa_raw_value_data_t *)exp->toKoopa();

        raw->name = "I am Return!";
        raw->ty = nullptr;
        raw->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_UNKNOWN
        };

        return raw;
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

    void* toKoopa() const override {
        return lOrExp->toKoopa();
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
    void* toKoopa() const override {
        return exp->toKoopa();
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
    void* toKoopa() const override {
        auto raw = new koopa_raw_value_data_t;

        raw->kind.tag = KOOPA_RVT_INTEGER;
        raw->kind.data.integer.value = num;
        raw->name = "I FOUND NUMBERS!";
        raw->ty = nullptr;
        raw->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_UNKNOWN
        };

        return raw;
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
    void* toKoopa() const override {
        return primaryExp->toKoopa();
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
    void* toKoopa() const override {
        if(unaryOp == "+")
            return unaryExp->toKoopa();

        auto raw = new koopa_raw_value_data_t;

        raw->kind.tag = KOOPA_RVT_BINARY;

        auto lhs= new koopa_raw_value_data_t;
        lhs->kind.tag = KOOPA_RVT_INTEGER;
        lhs->kind.data.integer.value = 0;
        lhs->name = "Just A Zero";
        lhs->ty = nullptr;
        lhs->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_UNKNOWN
        };
        raw->kind.data.binary.lhs = lhs;
        raw->kind.data.binary.rhs = (koopa_raw_value_data_t *)unaryExp->toKoopa();
        raw->kind.data.binary.op = op_map[unaryOp];
        raw->name = "Here is an unary expression";
        raw->ty = nullptr;
        raw->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_UNKNOWN
        };

        return raw;
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
    void* toKoopa() const override {
        return unaryExp->toKoopa();
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
    void* toKoopa() const override {
        auto raw = new koopa_raw_value_data_t;

        raw->kind.tag = KOOPA_RVT_BINARY;
        raw->kind.data.binary.lhs = (koopa_raw_value_data_t *)mulExp->toKoopa();
        raw->kind.data.binary.rhs = (koopa_raw_value_data_t *)unaryExp->toKoopa();
        raw->kind.data.binary.op = op_map[mulOp];
        raw->name = "Here is a mul expression";
        raw->ty = nullptr;
        raw->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_UNKNOWN
        };

        return raw;
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
    void* toKoopa() const override {
        return mulExp->toKoopa();
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
    void* toKoopa() const override {
        auto raw = new koopa_raw_value_data_t;

        raw->kind.tag = KOOPA_RVT_BINARY;
        raw->kind.data.binary.lhs = (koopa_raw_value_data_t *)addExp->toKoopa();
        raw->kind.data.binary.rhs = (koopa_raw_value_data_t *)mulExp->toKoopa();
        raw->kind.data.binary.op = op_map[addOp];
        raw->name = "Here is an add expression";
        raw->ty = nullptr;
        raw->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_UNKNOWN
        };

        return raw;
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
    void* toKoopa() const override {
        return addExp->toKoopa();
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
    void* toKoopa() const override {
        auto raw = new koopa_raw_value_data_t;

        raw->kind.tag = KOOPA_RVT_BINARY;
        raw->kind.data.binary.lhs = (koopa_raw_value_data_t *)relExp->toKoopa();
        raw->kind.data.binary.rhs = (koopa_raw_value_data_t *)addExp->toKoopa();
        raw->kind.data.binary.op = op_map[relOp];
        raw->name = "Here is an rel expression";
        raw->ty = nullptr;
        raw->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_UNKNOWN
        };

        return raw;
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
    void* toKoopa() const override {
        return relExp->toKoopa();
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
    void* toKoopa() const override {
        auto raw = new koopa_raw_value_data_t;

        raw->kind.tag = KOOPA_RVT_BINARY;
        raw->kind.data.binary.lhs = (koopa_raw_value_data_t *)eqExp->toKoopa();
        raw->kind.data.binary.rhs = (koopa_raw_value_data_t *)relExp->toKoopa();
        raw->kind.data.binary.op = op_map[eqOp];
        raw->name = "Here is an eq expression";
        raw->ty = nullptr;
        raw->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_UNKNOWN
        };

        return raw;
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
    void* toKoopa() const override {
        return eqExp->toKoopa();
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
    void* toKoopa() const override {
        auto raw1 = new koopa_raw_value_data_t;

        raw1->kind.tag = KOOPA_RVT_BINARY;

        auto lhs1= new koopa_raw_value_data_t;
        lhs1->kind.tag = KOOPA_RVT_INTEGER;
        lhs1->kind.data.integer.value = 0;
        lhs1->name = "Just A Zero";
        lhs1->ty = nullptr;
        lhs1->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_UNKNOWN
        };
        raw1->kind.data.binary.lhs = lhs1;
        raw1->kind.data.binary.rhs = (koopa_raw_value_data_t *)lAndExp->toKoopa();
        raw1->kind.data.binary.op = op_map["!"];
        raw1->name = "Here is an unary expression";
        raw1->ty = nullptr;
        raw1->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_UNKNOWN
        };

        auto raw2 = new koopa_raw_value_data_t;

        raw2->kind.tag = KOOPA_RVT_BINARY;

        auto lhs2= new koopa_raw_value_data_t;
        lhs2->kind.tag = KOOPA_RVT_INTEGER;
        lhs2->kind.data.integer.value = 0;
        lhs2->name = "Just A Zero";
        lhs2->ty = nullptr;
        lhs2->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_UNKNOWN
        };
        raw2->kind.data.binary.lhs = lhs2;
        raw2->kind.data.binary.rhs = (koopa_raw_value_data_t *)eqExp->toKoopa();
        raw2->kind.data.binary.op = op_map["!"];
        raw2->name = "Here is an unary expression";
        raw2->ty = nullptr;
        raw2->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_UNKNOWN
        };

        auto raw = new koopa_raw_value_data_t;

        raw->kind.tag = KOOPA_RVT_BINARY;
        raw->kind.data.binary.lhs = raw1;
        raw->kind.data.binary.rhs = raw2;
        raw->kind.data.binary.op = op_map["&&"];
        raw->name = "Here is an eq expression";
        raw->ty = nullptr;
        raw->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_UNKNOWN
        };

        return raw;
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
    void* toKoopa() const override {
        return lAndExp->toKoopa();
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
    void* toKoopa() const override {
        auto raw0 = new koopa_raw_value_data_t;

        raw0->kind.tag = KOOPA_RVT_BINARY;
        raw0->kind.data.binary.lhs = (koopa_raw_value_data_t *)lOrExp->toKoopa();
        raw0->kind.data.binary.rhs = (koopa_raw_value_data_t *)lAndExp->toKoopa();
        raw0->kind.data.binary.op = op_map["||"];
        raw0->name = "Here is an eq expression";
        raw0->ty = nullptr;
        raw0->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_UNKNOWN
        };

        auto raw = new koopa_raw_value_data_t;

        raw->kind.tag = KOOPA_RVT_BINARY;

        auto lhs= new koopa_raw_value_data_t;
        lhs->kind.tag = KOOPA_RVT_INTEGER;
        lhs->kind.data.integer.value = 0;
        lhs->name = "Just A Zero";
        lhs->ty = nullptr;
        lhs->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_UNKNOWN
        };
        raw->kind.data.binary.lhs = lhs;
        raw->kind.data.binary.rhs = raw0;
        raw->kind.data.binary.op = op_map["!="];
        raw->name = "Here is an unary expression";
        raw->ty = nullptr;
        raw->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_UNKNOWN
        };

        return raw0;
    }
};