#pragma once
#include <iostream>
#include <fstream>
#include <memory>
#include <cassert>
#include <stack>
#include <unordered_map>
#include <koopa.h>
#include <vector>
#include <string.h>
#include <algorithm>
#include <variant>
#include "helper.hpp"
#include "json/json.h"

using namespace std;
typedef variant<int, void *> term;
static unordered_map<string, koopa_raw_binary_op_t> op_map = {
    {"+", KOOPA_RBO_ADD},
    {"-", KOOPA_RBO_SUB},
    {"*", KOOPA_RBO_MUL},
    {"/", KOOPA_RBO_DIV},
    {"%", KOOPA_RBO_MOD},
    {"!", KOOPA_RBO_EQ},
    {"<", KOOPA_RBO_LT},
    {">", KOOPA_RBO_GT},
    {"<=", KOOPA_RBO_LE},
    {">=", KOOPA_RBO_GE},
    {"==", KOOPA_RBO_EQ},
    {"!=", KOOPA_RBO_NOT_EQ},
    {"&&", KOOPA_RBO_AND},
    {"||", KOOPA_RBO_OR}
};

static unordered_map<string, term> symbolTable;

class BaseAST {
public:
    virtual ~BaseAST() = default;
    virtual Json::Value dump() const = 0;
    virtual void* toKoopa() const { return nullptr; }
    virtual void* toKoopa(vector<const void *> &buffer) const { return nullptr; }
    virtual int calculateExp() const { return 0; }
};

class CompUnitAST : public BaseAST {
public:
    unique_ptr<BaseAST> func_def;

    Json::Value dump() const override {
        Json::Value comp_unit;
        comp_unit["FuncDef"] = func_def->dump();
        return comp_unit;
    }

    void* toKoopa() const override {
        auto raw = new koopa_raw_program_t;

        raw->values.buffer = nullptr;
        raw->values.kind = KOOPA_RSIK_VALUE;
        raw->values.len = 0;

        vector<const void *> buffer;
        buffer.push_back(func_def->toKoopa());
        raw->funcs.buffer = new const void *[buffer.size()];
        copy(buffer.begin(), buffer.end(), raw->funcs.buffer);
        raw->funcs.kind = KOOPA_RSIK_FUNCTION;
        raw->funcs.len = buffer.size();

        return raw;
    }
};

class FuncDefAST : public BaseAST {
public:
    unique_ptr<BaseAST> func_type;
    string ident;
    unique_ptr<BaseAST> block;

    Json::Value dump() const override {
        Json::Value func_def;

        func_def["FuncType"] = func_type->dump();
        func_def["Ident"] = ident;
        func_def["Block"] = block->dump();

        return func_def;
    }

    void* toKoopa() const override {
        auto raw = new koopa_raw_function_data_t;

        char *name = new char[ident.size() + 2];
        strcpy(name, ("@"+ident).c_str());
        raw->name = name;
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
        ty->data.function.ret = (const koopa_raw_type_kind_t *)func_type->toKoopa();
        raw->ty = ty;

        vector<const void *> buffer;
        buffer.push_back(block->toKoopa());
        raw->bbs.buffer = new const void *[buffer.size()];
        copy(buffer.begin(),buffer.end(),raw->bbs.buffer);
        raw->bbs.kind = KOOPA_RSIK_BASIC_BLOCK;
        raw->bbs.len = buffer.size();

        return raw;
    }
};

class FuncTypeAST : public BaseAST {
public:
    string type;

    Json::Value dump() const override {
        Json::Value func_type;
        func_type["Type"] = type;
        return func_type;
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
    unique_ptr<vector<unique_ptr<BaseAST> > >blockItem;

    Json::Value dump() const override {
        Json::Value block;
        Json::Value blockItem_json;
        for(int i = 0; i < blockItem->size(); i++) {
            blockItem_json[i] = (*blockItem)[i]->dump();
        }
        block["BlockItem"] = blockItem_json;
        return block;
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

        vector<const void *> buffer;
        int i = 0;
        for(; i < blockItem->size(); i++) {
            auto ptr = (koopa_raw_value_data_t *)(*blockItem)[i]->toKoopa(buffer);
            if(ptr != nullptr && ptr->kind.tag == KOOPA_RVT_RETURN) {
                i++;
                break;
            }
        }
        
        raw->insts.buffer = new const void *[buffer.size()];
        copy(buffer.begin(), buffer.end(), raw->insts.buffer);
        raw->insts.kind = KOOPA_RSIK_VALUE;
        raw->insts.len = buffer.size();

        return raw;
    }
};

class StmtAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> exp;

    Json::Value dump() const override {
        Json::Value stmt;
        stmt["Exp"] = exp->dump();
        Json::Value v;
        v["Return"] = stmt;
        return v;
    }

    void* toKoopa(vector<const void *> & buffer) const override {
        auto raw = new koopa_raw_value_data_t;
        
        raw->kind.tag = KOOPA_RVT_RETURN;
        auto value = (koopa_raw_value_data_t *)exp->toKoopa(buffer);
        addItemToSlice(value->used_by, raw);
        raw->kind.data.ret.value = value;
        raw->ty = value->ty;
        raw->name = nullptr;
        raw->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_VALUE
        };
        buffer.push_back(raw);
        return raw;
    }
};

class StmtAST2 : public BaseAST {
public:
    unique_ptr<BaseAST> lVal;
    unique_ptr<BaseAST> exp;

    Json::Value dump() const override {
        Json::Value stmt;
        stmt["LVal"] = lVal->dump();
        stmt["Exp"] = exp->dump();
        Json::Value v;
        v["Var"] = stmt;
        return v;
    }
    void *toKoopa(vector<const void *> &buffer) const override {
        auto raw = new koopa_raw_value_data_t;

        raw->kind.tag = KOOPA_RVT_STORE;

        auto dest = (koopa_raw_value_data_t  *)lVal->toKoopa();
        raw->kind.data.store.dest = dest;
        addItemToSlice(dest->used_by, raw);

        auto value = (koopa_raw_value_data_t *)exp->toKoopa(buffer);
        raw->kind.data.store.value = value;
        addItemToSlice(value->used_by, raw);

        raw->name = nullptr;
        auto ty = new koopa_raw_type_kind_t;
        ty->tag = KOOPA_RTT_UNIT;
        raw->ty = ty;
        raw->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_VALUE
        };
        buffer.push_back(raw);
        return raw;
    }
};

class ExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> lOrExp;

    Json::Value dump() const override {
        return lOrExp->dump();
    }
    void* toKoopa(vector<const void *> &buffer) const override {
        return lOrExp->toKoopa(buffer);
    }
    int calculateExp() const override {
        return lOrExp->calculateExp();
    }
};

class PrimaryExpAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> exp;

    Json::Value dump() const override {
        return exp->dump();
    }
    void* toKoopa(vector<const void *> &buffer) const override {
        return exp->toKoopa(buffer);
    }
    int calculateExp() const override {
        return exp->calculateExp();
    }
};

class PrimaryExpAST2: public BaseAST {
public:
    int num;
    Json::Value dump() const override {
        Json::Value primaryExp;
        primaryExp["Number"] = num;
        return primaryExp;
    }
    void* toKoopa(vector<const void *> &buffer) const override {
        auto raw = new koopa_raw_value_data_t;

        raw->kind.tag = KOOPA_RVT_INTEGER;
        raw->kind.data.integer.value = num;
        raw->name = nullptr;
        
        auto ty = new koopa_raw_type_kind_t;
        ty->tag = KOOPA_RTT_INT32;
        raw->ty = ty;

        raw->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_VALUE
        };

        return raw;
    }
    int calculateExp() const override {
        return num;
    }
};

class PrimaryExpAST3 : public BaseAST {
public:
    unique_ptr<BaseAST> lVal;

    Json::Value dump() const override {
        Json::Value primaryExp;
        primaryExp["LVal"] = lVal->dump();
        return primaryExp;
    }
    void *toKoopa(vector<const void *> &buffer) const override {
        return lVal->toKoopa(buffer);
    }
    int calculateExp() const override {
        return lVal->calculateExp();
    }
};

class UnaryExpAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> primaryExp;

    Json::Value dump() const override {
        return primaryExp->dump();
    }
    void* toKoopa(vector<const void *> &buffer) const override {
        return primaryExp->toKoopa(buffer);
    }
    int calculateExp() const override {
        return primaryExp->calculateExp();
    }
};

class UnaryExpAST2 : public BaseAST {
public:
    string unaryOp;
    unique_ptr<BaseAST> unaryExp;

    Json::Value dump() const override {
        Json::Value unaryExp_json;
        unaryExp_json["Operator"] = unaryOp;
        unaryExp_json["UnaryExp"] = unaryExp->dump();
        return unaryExp_json;
    }
    void* toKoopa(vector<const void *> &buffer) const override {
        if(unaryOp == "+")
            return unaryExp->toKoopa(buffer);

        auto raw = new koopa_raw_value_data_t;

        raw->kind.tag = KOOPA_RVT_BINARY;

        auto lhs= new koopa_raw_value_data_t;
        lhs->kind.tag = KOOPA_RVT_INTEGER;
        lhs->kind.data.integer.value = 0;
        lhs->name = nullptr;

        auto buf_lhs = new const void*;
        buf_lhs[0] = raw;
        lhs->used_by = {
            buf_lhs,
            1,
            KOOPA_RSIK_VALUE
        };
        raw->kind.data.binary.lhs = lhs;

        auto rhs = (koopa_raw_value_data_t *)unaryExp->toKoopa(buffer);
        addItemToSlice(rhs->used_by, raw);
        raw->kind.data.binary.rhs = rhs;
        raw->kind.data.binary.op = op_map[unaryOp];
        raw->name = nullptr;

        lhs->ty = rhs->ty;
        raw->ty = rhs->ty;

        raw->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_VALUE
        };

        buffer.push_back(raw);

        return raw;
    }
    int calculateExp() const override {
        switch(unaryOp[0]) {
        case '+':
            return unaryExp->calculateExp();
        case '-':
            return -unaryExp->calculateExp();
        case '!':
            return !unaryExp->calculateExp();
        }
        return 0;
    }
};

class MulExpAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> unaryExp;

    Json::Value dump() const override {
        return unaryExp->dump();
    }
    void* toKoopa(vector<const void *> &buffer) const override {
        return unaryExp->toKoopa(buffer);
    }
    int calculateExp() const override {
        return unaryExp->calculateExp();
    }
};

class MulExpAST2 : public BaseAST {
public:
    unique_ptr<BaseAST> mulExp;
    string mulOp;
    unique_ptr<BaseAST> unaryExp;

    Json::Value dump() const override {
        Json::Value mulExp_json;
        mulExp_json["MulExp"] = mulExp->dump();
        mulExp_json["Operator"] = mulOp;
        mulExp_json["UnaryExp"] = unaryExp->dump();
        return mulExp_json;
    }
    void* toKoopa(vector<const void *> &buffer) const override {
        auto raw = new koopa_raw_value_data_t;
        raw->kind.tag = KOOPA_RVT_BINARY;

        auto lhs = (koopa_raw_value_data_t *)mulExp->toKoopa(buffer);
        addItemToSlice(lhs->used_by, raw);
        raw->kind.data.binary.lhs = lhs;

        auto rhs = (koopa_raw_value_data_t *)unaryExp->toKoopa(buffer);
        addItemToSlice(rhs->used_by, raw);
        raw->kind.data.binary.rhs = rhs;

        raw->kind.data.binary.op = op_map[mulOp];
        raw->name = nullptr;
        raw->ty = lhs->ty;
        raw->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_VALUE
        };

        buffer.push_back(raw);

        return raw;
    }
    int calculateExp() const override {
        switch(mulOp[0]) {
        case '*':
            return mulExp->calculateExp() * unaryExp->calculateExp();
        case '/':
            return mulExp->calculateExp() / unaryExp->calculateExp();
        case '%':
            return mulExp->calculateExp() % unaryExp->calculateExp();
        }
        return 0;
    }
};

class AddExpAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> mulExp;

    Json::Value dump() const override {
        return mulExp->dump();
    }
    void* toKoopa(vector<const void*> &buffer) const override {
        return mulExp->toKoopa(buffer);
    }
    int calculateExp() const override {
        return mulExp->calculateExp();
    }
};

class AddExpAST2 : public BaseAST {
public:
    unique_ptr<BaseAST> addExp;
    string addOp;
    unique_ptr<BaseAST> mulExp;

    Json::Value dump() const override {
        Json::Value addExp_json;
        addExp_json["AddExp"] = addExp->dump();
        addExp_json["Operator"] = addOp;
        addExp_json["MulExp"] = mulExp->dump();
        return addExp_json;
    }
    void* toKoopa(vector<const void*> &buffer) const override {
        auto raw = new koopa_raw_value_data_t;

        raw->kind.tag = KOOPA_RVT_BINARY;
        
        auto lhs = (koopa_raw_value_data_t *)addExp->toKoopa(buffer);
        addItemToSlice(lhs->used_by, raw);
        raw->kind.data.binary.lhs = lhs;

        auto rhs = (koopa_raw_value_data_t *)mulExp->toKoopa(buffer);
        addItemToSlice(rhs->used_by, raw);
        raw->kind.data.binary.rhs = rhs;
        raw->kind.data.binary.op = op_map[addOp];
        raw->name = nullptr;
        raw->ty = lhs->ty;

        raw->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_VALUE
        };

        buffer.push_back(raw);

        return raw;
    }
    int calculateExp() const override {
        if(addOp[0] == '+')
            return addExp->calculateExp() + mulExp->calculateExp();
        else if(addOp[0] == '-')
            return addExp->calculateExp() - mulExp->calculateExp();
        return 0;
    }
};

class RelExpAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> addExp;

    Json::Value dump() const override {
        return addExp->dump();
    }
    void* toKoopa(vector<const void *> &buffer) const override {
        return addExp->toKoopa(buffer);
    }
    int calculateExp() const override {
        return addExp->calculateExp();
    }
};

class RelExpAST2 : public BaseAST {
public:
    unique_ptr<BaseAST> relExp;
    string relOp;
    unique_ptr<BaseAST> addExp;
    
    Json::Value dump() const override {
        Json::Value relExp_json;
        relExp_json["RelExp"] = relExp->dump();
        relExp_json["Operator"] = relOp;
        relExp_json["AddExp"] = addExp->dump();
        return relExp_json;
    }
    void* toKoopa(vector<const void *> &buffer) const override {
        auto raw = new koopa_raw_value_data_t;

        raw->kind.tag = KOOPA_RVT_BINARY;
        
        auto lhs = (koopa_raw_value_data_t *)relExp->toKoopa(buffer);
        addItemToSlice(lhs->used_by, raw);
        raw->kind.data.binary.lhs = lhs;

        auto rhs = (koopa_raw_value_data_t *)addExp->toKoopa(buffer);
        addItemToSlice(rhs->used_by, raw);
        raw->kind.data.binary.rhs = rhs;

        raw->kind.data.binary.op = op_map[relOp];
        raw->name = nullptr;
        raw->ty = lhs->ty;

        raw->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_VALUE
        };

        buffer.push_back(raw);

        return raw;
    }
    int calculateExp() const override {
        if(relOp[0] == '<') {
            if(relOp.size() == 1)
                return relExp->calculateExp() < addExp->calculateExp();
            else
                return relExp->calculateExp() <= addExp->calculateExp();
        }
        else if(relOp[0] == '>') {
            if(relOp.size() == 1) 
                return relExp->calculateExp() > addExp->calculateExp();
            else
                return relExp->calculateExp() <= addExp->calculateExp();
        }
        return 0;
    }
};

class EqExpAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> relExp;

    Json::Value dump() const override {
        return relExp->dump();
    }
    void* toKoopa(vector<const void *> &buffer) const override {
        return relExp->toKoopa(buffer);
    }
    int calculateExp() const override {
        return relExp->calculateExp();
    }
};

class EqExpAST2 : public BaseAST {
public:
    unique_ptr<BaseAST> eqExp;
    string eqOp;
    unique_ptr<BaseAST> relExp;

    Json::Value dump() const override {
        Json::Value eqExp_json;
        eqExp_json["EqExp"] = eqExp->dump();
        eqExp_json["Operator"] = eqOp;
        eqExp_json["RelExp"] = relExp->dump();
        return eqExp_json;
    }
    void* toKoopa(vector<const void *> &buffer) const override {
        auto raw = new koopa_raw_value_data_t;
        
        raw->kind.tag = KOOPA_RVT_BINARY;
        
        auto lhs = (koopa_raw_value_data_t *)eqExp->toKoopa(buffer);
        addItemToSlice(lhs->used_by, raw);
        raw->kind.data.binary.lhs = lhs;

        auto rhs = (koopa_raw_value_data_t *)relExp->toKoopa(buffer);
        addItemToSlice(rhs->used_by, raw);
        raw->kind.data.binary.rhs = rhs;
        raw->kind.data.binary.op = op_map[eqOp];
        raw->name = nullptr;
        raw->ty = lhs->ty;

        raw->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_VALUE
        };

        buffer.push_back(raw);

        return raw;
    }
    int calculateExp() const override {
        if(eqOp == "==")
            return eqExp->calculateExp() == relExp->calculateExp();
        else if(eqOp == "!=")
            return eqExp->calculateExp() != relExp->calculateExp();
        return 0;
    }
};

class LAndExpAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> eqExp;

    Json::Value dump() const override {
        return eqExp->dump();
    }
    void* toKoopa(vector<const void *> &buffer) const override {
        return eqExp->toKoopa(buffer);
    }
    int calculateExp() const override {
        return eqExp->calculateExp();
    }
};

class LAndExpAST2 : public BaseAST {
public:
    unique_ptr<BaseAST> lAndExp;
    unique_ptr<BaseAST> eqExp;

    Json::Value dump() const override {
        Json::Value lAndExp_json;
        lAndExp_json["LAndExp"] = lAndExp->dump();
        lAndExp_json["Operator"] = "&&";
        lAndExp_json["EqExp"] = eqExp->dump();
        return lAndExp_json;
    }
    void* toKoopa(vector<const void *> &buffer) const override {
        auto raw1 = new koopa_raw_value_data_t;

        raw1->kind.tag = KOOPA_RVT_BINARY;

        auto lhs1= new koopa_raw_value_data_t;
        lhs1->kind.tag = KOOPA_RVT_INTEGER;
        lhs1->kind.data.integer.value = 0;
        lhs1->name = nullptr;
        auto buf_lhs1 = new const void*;
        buf_lhs1[0] = raw1;
        lhs1->used_by = {
            buf_lhs1,
            1,
            KOOPA_RSIK_VALUE
        };
        raw1->kind.data.binary.lhs = lhs1;

        auto rhs1 = (koopa_raw_value_data_t *)lAndExp->toKoopa(buffer);
        addItemToSlice(rhs1->used_by, raw1);
        raw1->kind.data.binary.rhs = rhs1;

        raw1->kind.data.binary.op = op_map["!="];
        raw1->name = nullptr;

        lhs1->ty = rhs1->ty;
        raw1->ty = rhs1->ty;

        raw1->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_VALUE
        };

        buffer.push_back(raw1);

        auto raw2 = new koopa_raw_value_data_t;

        raw2->kind.tag = KOOPA_RVT_BINARY;

        auto lhs2= new koopa_raw_value_data_t;
        lhs2->kind.tag = KOOPA_RVT_INTEGER;
        lhs2->kind.data.integer.value = 0;
        lhs2->name = nullptr;
        auto buf_lhs2 = new const void*;
        buf_lhs2[0] = raw2;
        lhs2->used_by = {
            buf_lhs2,
            1,
            KOOPA_RSIK_VALUE
        };
        raw2->kind.data.binary.lhs = lhs2;

        auto rhs2 = (koopa_raw_value_data_t *)eqExp->toKoopa(buffer);
        addItemToSlice(rhs2->used_by, raw2);
        raw2->kind.data.binary.rhs = rhs2;
        raw2->kind.data.binary.op = op_map["!="]; 
        raw2->name = nullptr;

        lhs2->ty = rhs2->ty;
        raw2->ty = rhs2->ty;

        raw2->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_VALUE
        };

        buffer.push_back(raw2);

        auto raw = new koopa_raw_value_data_t;

        raw->kind.tag = KOOPA_RVT_BINARY;
        addItemToSlice(raw1->used_by, raw);
        raw->kind.data.binary.lhs = raw1;
        addItemToSlice(raw2->used_by, raw);
        raw->kind.data.binary.rhs = raw2;
        raw->kind.data.binary.op = op_map["&&"];
        raw->name = nullptr;
        raw->ty = raw1->ty;
        
        raw->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_VALUE
        };

        buffer.push_back(raw);

        return raw;
    }
    int calculateExp() const override {
        return lAndExp->calculateExp() && eqExp->calculateExp();
    }
};

class LOrExpAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> lAndExp;

    Json::Value dump() const override {
        return lAndExp->dump();
    }
    void* toKoopa(vector<const void *> &buffer) const override {
        return lAndExp->toKoopa(buffer);
    }
    int calculateExp() const override {
        return lAndExp->calculateExp();
    }
};

class LOrExpAST2 : public BaseAST {
public:
    unique_ptr<BaseAST> lOrExp;
    unique_ptr<BaseAST> lAndExp;

    Json::Value dump() const override {
        Json::Value lOrExp_json;
        lOrExp_json["LOrExp"] = lOrExp->dump();
        lOrExp_json["Operator"] = "||";
        lOrExp_json["LAndExp"] = lAndExp->dump();
        return lOrExp_json;
    }
    void* toKoopa(vector<const void *> &buffer) const override {
        auto raw0 = new koopa_raw_value_data_t;

        raw0->kind.tag = KOOPA_RVT_BINARY;
        auto lhs0 = (koopa_raw_value_data_t *)lOrExp->toKoopa(buffer);
        addItemToSlice(lhs0->used_by, raw0);
        raw0->kind.data.binary.lhs = lhs0;
        auto rhs0 = (koopa_raw_value_data_t *)lAndExp->toKoopa(buffer);
        addItemToSlice(rhs0->used_by, raw0);
        raw0->kind.data.binary.rhs = rhs0;
        raw0->kind.data.binary.op = op_map["||"];
        raw0->name = nullptr;
        raw0->ty = lhs0->ty;

        raw0->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_VALUE
        };

        buffer.push_back(raw0);

        auto raw = new koopa_raw_value_data_t;

        raw->kind.tag = KOOPA_RVT_BINARY;

        auto lhs= new koopa_raw_value_data_t;
        lhs->kind.tag = KOOPA_RVT_INTEGER;
        lhs->kind.data.integer.value = 0;
        lhs->name = nullptr;
        auto buf_lhs = new const void*;
        buf_lhs[0] = raw;
        lhs->used_by = {
            buf_lhs,
            1,
            KOOPA_RSIK_VALUE
        };
        raw->kind.data.binary.lhs = lhs;

        addItemToSlice(raw0->used_by, raw);
        raw->kind.data.binary.rhs = raw0;

        raw->kind.data.binary.op = op_map["!="];
        raw->name = nullptr;

        lhs->ty = raw0->ty;
        raw->ty = raw0->ty;

        raw->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_VALUE
        };

        buffer.push_back(raw);

        return raw;
    }
    int calculateExp() const override {
        return lOrExp->calculateExp() || lAndExp->calculateExp();
    }
};

class DeclAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> constDecl;

    Json::Value dump() const override {
        Json::Value v;
        v["ConstDecl"] = constDecl->dump();
        return v;
    }
    void *toKoopa(vector<const void*> &buffer) const override {
        constDecl->toKoopa();
        return nullptr;
    }
};

class DeclAST2 : public BaseAST {
public:
    unique_ptr<BaseAST> varDecl;

    Json::Value dump() const override {
        Json::Value v;
        v["VarDecl"] = varDecl->dump();
        return v;
    }
    void *toKoopa(vector<const void*> &buffer) const override {
        return varDecl->toKoopa(buffer);
    }
};

class ConstDeclAST : public BaseAST {
public:
    unique_ptr<BaseAST> bType;
    unique_ptr<vector<unique_ptr<BaseAST> > > constDef;

    Json::Value dump() const override {
        Json::Value v;
        v["BType"] = bType->dump();
        Json::Value defs;
        for(int i = 0; i < constDef->size(); i++) {
            defs[i] = (*constDef)[i]->dump();
        }
        v["ConstDef"] = defs;
        return v;
    }
    void *toKoopa() const override {
        for(int i = 0; i < constDef->size(); i++) {
            (*constDef)[i]->toKoopa();
        }
        return nullptr;
    }
};

class BTypeAST : public BaseAST {
public:
    string type;

    Json::Value dump() const override {
        Json::Value v;
        v["Type"] = type;
        return v;
    }
    void *toKoopa() const override {
        auto ty = new koopa_raw_type_kind_t;
        if(type == "int")
            ty->tag = KOOPA_RTT_INT32;
        else
            ty->tag = KOOPA_RTT_UNIT;
        return ty;
    }
};

class ConstDefAST : public BaseAST {
public:
    string ident;
    unique_ptr<BaseAST> constInitVal;

    Json::Value dump() const override {
        Json::Value v;
        v["Ident"] = ident;
        v["ConstInitVal"] = constInitVal->dump();
        return v;
    }
    void *toKoopa() const override {
        int *p;
        if(symbolTable.find(ident) != symbolTable.end() && (p = get_if<int>(&symbolTable[ident]))) {
            printf("%p", p);
            assert(false);
        }
        term t = constInitVal->calculateExp();
        symbolTable.insert(make_pair(ident, t));
        return nullptr;
    }
};

class ConstInitValAST : public BaseAST {
public:
    unique_ptr<BaseAST> constExp;

    Json::Value dump() const override {
        return constExp->dump();
    }
    int calculateExp() const override {
        return constExp->calculateExp();
    }
};

class VarDeclAST : public BaseAST {
public:
    unique_ptr<BaseAST> bType;
    unique_ptr<vector<unique_ptr<BaseAST> > > varDef;

    Json::Value dump() const override {
        Json::Value v;
        v["BType"] = bType->dump(); 
        Json::Value defs;
        for(int i = 0; i < varDef->size(); i++) {
            defs[i] = (*varDef)[i]->dump();
        }
        v["VarDef"] = defs;
        return v;
    }
    void *toKoopa(vector<const void *> &buffer) const override {
        auto ty = new koopa_raw_type_kind_t;
        ty->tag = KOOPA_RTT_POINTER;
        ty->data.pointer.base = (koopa_raw_type_t)bType->toKoopa();
        for(int i = 0; i < varDef->size(); i++) {
            auto ptr = (koopa_raw_value_data_t *)(*varDef)[i]->toKoopa(buffer);
            ptr->ty = ty;
        }
        return nullptr;
    } 
};

class VarDefAST1 : public BaseAST {
public:
    string ident;

    Json::Value dump() const override {
        Json::Value v;
        v["Ident"] = ident;
        return v;
    }
    void *toKoopa(vector<const void *> &buffer) const override {
        if(symbolTable.find(ident) != symbolTable.end() && get_if<void *>(&symbolTable[ident]))
            assert(false);
        auto raw = new koopa_raw_value_data_t;

        raw->kind.tag = KOOPA_RVT_ALLOC;
        char *name = new char[ident.size() + 2];
        strcpy(name, ("@" + ident).c_str());
        raw->name = name;
        raw->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_VALUE
        };
        buffer.push_back(raw);

        term t = raw;
        symbolTable.insert(make_pair(ident, t));

        return raw;
    }
};

class VarDefAST2 : public BaseAST {
public:
    string ident;
    unique_ptr<BaseAST> initVal;

    Json::Value dump() const override {
        Json::Value v;
        v["Ident"] = ident;
        v["InitVal"] = initVal->dump();
        return v;
    }
    void *toKoopa(vector<const void *> &buffer) const override {
        if(symbolTable.find(ident) != symbolTable.end() && get_if<void *>(&symbolTable[ident]))
            assert(false);
        auto raw1 = new koopa_raw_value_data_t;
    
        raw1->kind.tag = KOOPA_RVT_ALLOC;
        char *name = new char[ident.size() + 2];
        strcpy(name, ("@" + ident).c_str());
        raw1->name = name;
        raw1->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_VALUE
        };
        buffer.push_back(raw1);

        auto raw2 = new koopa_raw_value_data_t;

        raw2->kind.tag = KOOPA_RVT_STORE;
        
        addItemToSlice(raw1->used_by, raw2);
        raw2->kind.data.store.dest = raw1;

        auto value = (koopa_raw_value_data_t *)initVal->toKoopa(buffer);
        addItemToSlice(value->used_by, raw2);
        raw2->kind.data.store.value = value;
        raw2->name = nullptr;

        auto ty = new koopa_raw_type_kind_t;
        ty->tag = KOOPA_RTT_UNIT;
        raw2->ty = value->ty;

        raw2->used_by = {
            nullptr,
            0,
            KOOPA_RSIK_VALUE
        };

        buffer.push_back(raw2);

        term t = raw1;
        symbolTable.insert(make_pair(ident, t));

        return raw1;
        
    }
};

class InitValAST : public BaseAST {
public:
    unique_ptr<BaseAST> exp;

    Json::Value dump() const override {
        return exp->dump();
    }
    void *toKoopa(vector<const void *> &buffer) const override {
        return exp->toKoopa(buffer);
    }
};

class LValAST : public BaseAST {
public:
    string ident;

    Json::Value dump() const override {
        Json::Value v;
        v["Ident"] = ident;
        return v;
    }
    void *toKoopa() const override {
        //作为左值引用一个符号
        if(symbolTable.find(ident) != symbolTable.end()) {
            if(auto p = get_if<void *>(&symbolTable[ident])) {
                return *p;
            }
            if(auto p = get_if<int>(&symbolTable[ident])) {
                cerr << "lvalue required as left operand of assignment" << endl;
                exit(1);
            }
        }
        return nullptr;
    }
    void *toKoopa(vector<const void *> &buffer) const override {
        //作为右值引用一个符号（如果是变量，必须先Load）
        if(symbolTable.find(ident) != symbolTable.end()) {
            if(auto p = get_if<int>(&symbolTable[ident])) {
                auto raw = new koopa_raw_value_data_t;
                auto ty = new koopa_raw_type_kind_t;
                ty->tag = KOOPA_RTT_INT32;

                raw->kind.tag = KOOPA_RVT_INTEGER;
                raw->kind.data.integer.value = *p;
                raw->name = nullptr;
                raw->ty = ty;
                raw->used_by = {
                    nullptr,
                    0,
                    KOOPA_RSIK_VALUE
                };
                return raw;
            }
            else if (auto p = get_if<void *>(&symbolTable[ident])) {
                auto raw = new koopa_raw_value_data_t;
                auto ty = new koopa_raw_type_kind_t;
                ty->tag = KOOPA_RTT_INT32;

                raw->kind.tag = KOOPA_RVT_LOAD;
                auto src = (koopa_raw_value_data_t *)(*p);
                addItemToSlice(src->used_by, raw);
                raw->kind.data.load.src = src;
                raw->name = nullptr;
                raw->ty = ty;
                raw->used_by = {
                    nullptr,
                    0,
                    KOOPA_RSIK_VALUE
                };
                buffer.push_back(raw);

                return raw;
            }
        }
        else  {
            cerr << "undeclared identifier" << endl;
            exit(1);
            // Make compiler shut up
        }
        return 0;
    }
    int calculateExp() const override {
        int *p;
        if(symbolTable.find(ident) != symbolTable.end() && (p = get_if<int>(&symbolTable[ident]))) {
            return *p;
        }
        //常量求值里不能有查询到变量
        assert(false);
    }
};

class ConstExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> exp;

    Json::Value dump() const override {
        return exp->dump();
    }
    int calculateExp() const override {
        return exp->calculateExp();
    }
};