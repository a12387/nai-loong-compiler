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

static void ** helper = nullptr;

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
            KOOPA_RSIK_TYPE
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

        for(int i = 0; i < raw->bbs.len; i++) {
            auto ptr0 = reinterpret_cast<koopa_raw_basic_block_t>(raw->bbs.buffer[i]);
            

            for(int j = 0; j < ptr0->insts.len; j++) {
                auto ptr1 = (koopa_raw_value_data_t *)(ptr0->insts.buffer[j]);
                ptr1->ty = raw->ty->data.function.ret;
            }
        }

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

        raw->name = "%entry";
        raw->params = {
            nullptr,
            0,
            KOOPA_RSIK_VALUE
        };
        raw->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_VALUE
        };

        auto buf = new void*;
        helper = buf;
        *helper = stmt->toKoopa();
        raw->insts = {
            (const void **)buf,
            (unsigned)(helper - buf),
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
        auto value = (koopa_raw_value_data_t *)exp->toKoopa();
        value->used_by.buffer[value->used_by.len++] = raw;
        raw->kind.data.ret.value = value;

        raw->name = "return";
        raw->ty = nullptr;
        raw->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_VALUE
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
        raw->name = nullptr;
        raw->ty = nullptr;

        auto buf = new void*;
        raw->used_by = {
            (const void**)buf,
            0,
            KOOPA_RSIK_VALUE
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

        *helper = raw;
        helper++;
        raw->kind.tag = KOOPA_RVT_BINARY;

        auto lhs= new koopa_raw_value_data_t;
        lhs->kind.tag = KOOPA_RVT_INTEGER;
        lhs->kind.data.integer.value = 0;
        lhs->name = nullptr;
        lhs->ty = nullptr;
        auto buf_lhs = new void*;
        buf_lhs[0] = raw;
        lhs->used_by = {
            (const void **)buf_lhs,
            1,
            KOOPA_RSIK_VALUE
        };
        raw->kind.data.binary.lhs = lhs;

        auto rhs = (koopa_raw_value_data_t *)unaryExp->toKoopa();
        rhs->used_by.buffer[rhs->used_by.len++] = raw;
        raw->kind.data.binary.rhs = rhs;
        raw->kind.data.binary.op = op_map[unaryOp];
        raw->name = nullptr;
        raw->ty = nullptr;
        auto buf = new void*;
        raw->used_by = {
            (const void**)buf,
            0,
            KOOPA_RSIK_VALUE
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

        *helper = raw;
        helper++;
        raw->kind.tag = KOOPA_RVT_BINARY;

        auto lhs = (koopa_raw_value_data_t *)mulExp->toKoopa();
        lhs->used_by.buffer[lhs->used_by.len++] = raw;
        raw->kind.data.binary.lhs = lhs;

        auto rhs = (koopa_raw_value_data_t *)unaryExp->toKoopa();
        rhs->used_by.buffer[rhs->used_by.len++] = raw;
        raw->kind.data.binary.rhs = rhs;
        raw->kind.data.binary.op = op_map[mulOp];
        raw->name = nullptr;
        raw->ty = nullptr;
        auto buf = new void*;
        raw->used_by = {
            (const void**)buf,
            0,
            KOOPA_RSIK_VALUE
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

        *helper = raw;
        helper++;

        raw->kind.tag = KOOPA_RVT_BINARY;
        
        auto lhs = (koopa_raw_value_data_t *)addExp->toKoopa();
        lhs->used_by.buffer[lhs->used_by.len++] = raw;
        raw->kind.data.binary.lhs = lhs;

        auto rhs = (koopa_raw_value_data_t *)mulExp->toKoopa();
        rhs->used_by.buffer[rhs->used_by.len++] = raw;
        raw->kind.data.binary.rhs = rhs;
        raw->kind.data.binary.op = op_map[addOp];
        raw->name = nullptr;
        raw->ty = nullptr;

        auto buf = new void*;
        raw->used_by = {
            (const void**)buf,
            0,
            KOOPA_RSIK_VALUE
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

        *helper = raw;
        helper++;
        
        raw->kind.tag = KOOPA_RVT_BINARY;
        
        auto lhs = (koopa_raw_value_data_t *)relExp->toKoopa();
        lhs->used_by.buffer[lhs->used_by.len++] = raw;
        raw->kind.data.binary.lhs = lhs;

        auto rhs = (koopa_raw_value_data_t *)addExp->toKoopa();
        rhs->used_by.buffer[rhs->used_by.len++] = raw;
        raw->kind.data.binary.rhs = rhs;

        raw->kind.data.binary.op = op_map[relOp];
        raw->name = nullptr;
        raw->ty = nullptr;

        auto buf = new void*;
        raw->used_by = {
            (const void**)buf,
            0,
            KOOPA_RSIK_VALUE
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

        *helper = raw;
        helper++;
        
        raw->kind.tag = KOOPA_RVT_BINARY;
        
        auto lhs = (koopa_raw_value_data_t *)eqExp->toKoopa();
        lhs->used_by.buffer[lhs->used_by.len++] = raw;
        raw->kind.data.binary.lhs = lhs;

        auto rhs = (koopa_raw_value_data_t *)relExp->toKoopa();
        rhs->used_by.buffer[rhs->used_by.len++] = raw;
        raw->kind.data.binary.rhs = rhs;
        raw->kind.data.binary.op = op_map[eqOp];
        raw->name = nullptr;
        raw->ty = nullptr;

        auto buf = new void*;
        raw->used_by = {
            (const void**)buf,
            0,
            KOOPA_RSIK_VALUE
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

        *helper = raw1;
        helper++;
        
        raw1->kind.tag = KOOPA_RVT_BINARY;

        auto lhs1= new koopa_raw_value_data_t;
        lhs1->kind.tag = KOOPA_RVT_INTEGER;
        lhs1->kind.data.integer.value = 0;
        lhs1->name = nullptr;
        lhs1->ty = nullptr;
        auto buf_lhs1 = new void*;
        buf_lhs1[0] = raw1;
        lhs1->used_by = {
            (const void**)buf_lhs1,
            1,
            KOOPA_RSIK_VALUE
        };
        raw1->kind.data.binary.lhs = lhs1;

        auto rhs1 = (koopa_raw_value_data_t *)lAndExp->toKoopa();
        rhs1->used_by.buffer[rhs1->used_by.len++] = raw1;
        raw1->kind.data.binary.rhs = rhs1;

        raw1->kind.data.binary.op = op_map["!"];
        raw1->name = nullptr;
        raw1->ty = nullptr;

        auto buf1 = new void*;
        raw1->used_by = {
            (const void**)buf1,
            0,
            KOOPA_RSIK_VALUE
        };

        auto raw2 = new koopa_raw_value_data_t;

        *helper = raw2;
        helper++;

        raw2->kind.tag = KOOPA_RVT_BINARY;

        auto lhs2= new koopa_raw_value_data_t;
        lhs2->kind.tag = KOOPA_RVT_INTEGER;
        lhs2->kind.data.integer.value = 0;
        lhs2->name = nullptr;
        lhs2->ty = nullptr;
        auto buf_lhs2 = new void*;
        buf_lhs2[0] = raw2;
        lhs2->used_by = {
            (const void**)buf_lhs2,
            1,
            KOOPA_RSIK_VALUE
        };
        raw2->kind.data.binary.lhs = lhs2;

        auto rhs2 = (koopa_raw_value_data_t *)eqExp->toKoopa();
        rhs2->used_by.buffer[rhs2->used_by.len++] = raw2;
        raw2->kind.data.binary.rhs = rhs2;
        raw2->kind.data.binary.op = op_map["!"];
        raw2->name = nullptr;
        raw2->ty = nullptr;

        auto buf2 = new void*;
        raw2->used_by = {
            (const void**)buf2,
            0,
            KOOPA_RSIK_VALUE
        };

        auto raw = new koopa_raw_value_data_t;

        *helper = raw;
        helper++;

        raw->kind.tag = KOOPA_RVT_BINARY;
        raw1->used_by.buffer[raw1->used_by.len++] = raw;
        raw->kind.data.binary.lhs = raw1;
        raw2->used_by.buffer[raw2->used_by.len++] = raw;
        raw->kind.data.binary.rhs = raw2;
        raw->kind.data.binary.op = op_map["&&"];
        raw->name = nullptr;
        raw->ty = nullptr;
        
        auto buf = new void *;
        raw->used_by = {
            (const void**)buf,
            0,
            KOOPA_RSIK_VALUE
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

        *helper = raw0;
        helper++;

        raw0->kind.tag = KOOPA_RVT_BINARY;
        auto lhs0 = (koopa_raw_value_data_t *)lOrExp->toKoopa();
        lhs0->used_by.buffer[lhs0->used_by.len++] = raw0;
        raw0->kind.data.binary.lhs = lhs0;
        auto rhs0 = (koopa_raw_value_data_t *)lAndExp->toKoopa();
        rhs0->used_by.buffer[rhs0->used_by.len++] = raw0;
        raw0->kind.data.binary.rhs = rhs0;
        raw0->kind.data.binary.op = op_map["||"];
        raw0->name = nullptr;
        raw0->ty = nullptr;

        auto buf0 = new void*;
        raw0->used_by = {
            (const void **)buf0,
            0,
            KOOPA_RSIK_VALUE
        };

        auto raw = new koopa_raw_value_data_t;

        *helper = raw;
        helper++;

        raw->kind.tag = KOOPA_RVT_BINARY;

        auto lhs= new koopa_raw_value_data_t;
        lhs->kind.tag = KOOPA_RVT_INTEGER;
        lhs->kind.data.integer.value = 0;
        lhs->name = nullptr;
        lhs->ty = nullptr;
        auto buf_lhs = new void*;
        buf_lhs[0] = raw;
        lhs->used_by = {
            (const void**)buf_lhs,
            1,
            KOOPA_RSIK_VALUE
        };
        raw->kind.data.binary.lhs = lhs;

        raw0->used_by.buffer[raw0->used_by.len++] = raw;
        raw->kind.data.binary.rhs = raw0;

        raw->kind.data.binary.op = op_map["!="];
        raw->name = nullptr;
        raw->ty = nullptr;

        auto buf = new void*;
        raw->used_by = {
            (const void **)buf,
            0,
            KOOPA_RSIK_VALUE
        };

        return raw0;
    }
};