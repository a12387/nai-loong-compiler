#pragma once
#include <iostream>
#include <memory>
#include <cassert>
#include <unordered_map>
#include <vector>
#include <deque>
#include <string.h>
#include <koopa.h>
#include "json/json.h"
#include "symbol_table.hpp"

using namespace std;

class BaseAST {
public:
    virtual ~BaseAST() = default;
    virtual Json::Value dump() const = 0;
    virtual void *toKoopa() const { return nullptr; }
    virtual int calculateExp() const { return 0; }
protected:
    inline static vector<void *> bufferBlocks = {};
    inline static vector<const void *> bufferInsts = {};
    inline static vector<const void *> bufferFuncs = {};
    inline static vector<const void *> bufferGlobalValues = {};
    inline static deque<const void *> stackLoop = {};
    inline static bool insideFunc = false;
    static void endBlock();
    static bool checkBlock(koopa_raw_basic_block_data_t *dest);
    static bool checkBlock(int value);
    static bool checkBlock();
    static void initLibFuncs();
};

class CompUnitAST : public BaseAST {
public:
    unique_ptr<vector<unique_ptr<BaseAST> > > defs;

    CompUnitAST(vector<unique_ptr<BaseAST> > *p);
    Json::Value dump() const override;
    void *toKoopa() const override;
};

class FuncDefAST : public BaseAST {
public:
    unique_ptr<BaseAST> func_type;
    string ident;
    unique_ptr<vector<unique_ptr<BaseAST> > > fparams;
    unique_ptr<BaseAST> block;

    FuncDefAST(BaseAST *p1, const char *p2, vector<unique_ptr<BaseAST> > *p3, BaseAST *p4);
    Json::Value dump() const override;
    void *toKoopa() const override;
};

class FuncFParamAST : public BaseAST {
public:
    unique_ptr<BaseAST> bType;
    string ident;

    FuncFParamAST(BaseAST *p1, const char *p2);
    Json::Value dump() const override;
    void *toKoopa() const override;
};

class TypeAST : public BaseAST {
public:
    string type;

    TypeAST(const char *p);
    Json::Value dump() const override;
    void *toKoopa() const override;
};

class BlockAST : public BaseAST {
public:
    unique_ptr<vector<unique_ptr<BaseAST> > > blockItem;

    BlockAST(vector<unique_ptr<BaseAST> >*p);
    Json::Value dump() const override;
    void *toKoopa() const override;
};

class StmtAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> exp;

    StmtAST1(BaseAST *p);
    Json::Value dump() const override;
    void *toKoopa() const override;
};

class StmtAST2 : public BaseAST {
public:
    unique_ptr<BaseAST> lVal;
    unique_ptr<BaseAST> exp;

    StmtAST2(BaseAST *p1, BaseAST *p2);
    Json::Value dump() const override;
    void *toKoopa() const override;
};

class StmtAST3 : public BaseAST {
public:
    unique_ptr<BaseAST> exp;

    StmtAST3(BaseAST *p);
    Json::Value dump() const override;
    void *toKoopa() const override;
};

class StmtAST4 : public BaseAST {
public:
    unique_ptr<BaseAST> block;

    StmtAST4(BaseAST *p);
    Json::Value dump() const override;
    void *toKoopa() const override;
};

class StmtAST5 : public BaseAST {
public:
    StmtAST5();
    Json::Value dump() const override;
    void *toKoopa() const override;
};

class StmtAST6 : public BaseAST {
public:
    StmtAST6();
    Json::Value dump() const override;
    void *toKoopa() const override;
};

class IfAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> exp;
    unique_ptr<BaseAST> stmtThen;

    IfAST1(BaseAST *p1, BaseAST *p2);
    Json::Value dump() const override;
    void *toKoopa() const override;
};

class IfAST2 : public BaseAST {
public:
    unique_ptr<BaseAST> exp;
    unique_ptr<BaseAST> stmtThen;
    unique_ptr<BaseAST> stmtElse;

    IfAST2(BaseAST *p1, BaseAST *p2, BaseAST *p3);
    Json::Value dump() const override;
    void *toKoopa() const override;
};

class WhileAST : public BaseAST {
public:
    unique_ptr<BaseAST> exp;
    unique_ptr<BaseAST> stmt;

    WhileAST(BaseAST *p1, BaseAST *p2);
    Json::Value dump() const override;
    void *toKoopa() const override;
};

class PrimaryExpAST : public BaseAST {
public:
    int num;

    PrimaryExpAST(int num);
    Json::Value dump() const override;
    void *toKoopa() const override;
    int calculateExp() const override;
};

class UnaryExpAST1 : public BaseAST {
public:
    string unaryOp;
    unique_ptr<BaseAST> unaryExp;

    UnaryExpAST1(const char *p1, BaseAST *p2);
    Json::Value dump() const override;
    void *toKoopa() const override;
    int calculateExp() const override;
};

class UnaryExpAST2 : public BaseAST {
public:
    string ident;
    unique_ptr<vector<unique_ptr<BaseAST> > > rparams;

    UnaryExpAST2(const char *p1, vector<unique_ptr<BaseAST> > *p2);
    Json::Value dump() const override;
    void *toKoopa() const override;
};

class MulExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> mulExp;
    string mulOp;
    unique_ptr<BaseAST> unaryExp;

    MulExpAST(BaseAST *p1, const char *p2, BaseAST *p3);
    Json::Value dump() const override;
    void *toKoopa() const override;
    int calculateExp() const override;
};

class AddExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> addExp;
    string addOp;
    unique_ptr<BaseAST> mulExp;

    AddExpAST(BaseAST *p1, const char *p2, BaseAST *p3);
    Json::Value dump() const override;
    void *toKoopa() const override;
    int calculateExp() const override;
};

class RelExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> relExp;
    string relOp;
    unique_ptr<BaseAST> addExp;
    
    RelExpAST(BaseAST *p1, const char *p2, BaseAST *p3);
    Json::Value dump() const override;
    void *toKoopa() const override;
    int calculateExp() const override;
};

class EqExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> eqExp;
    string eqOp;
    unique_ptr<BaseAST> relExp;

    EqExpAST(BaseAST *p1, const char *p2, BaseAST *p3);
    Json::Value dump() const override;
    void *toKoopa() const override;
    int calculateExp() const override;
};

class LAndExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> lAndExp;
    unique_ptr<BaseAST> eqExp;

    LAndExpAST(BaseAST *p1, BaseAST *p2);
    Json::Value dump() const override;
    void *toKoopa() const override;
    int calculateExp() const override;
};

class LOrExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> lOrExp;
    unique_ptr<BaseAST> lAndExp;

    LOrExpAST(BaseAST *p1, BaseAST *p2);
    Json::Value dump() const override;
    void *toKoopa() const override;
    int calculateExp() const override;
};

class ConstDeclAST : public BaseAST {
public:
    unique_ptr<BaseAST> bType;
    unique_ptr<vector<unique_ptr<BaseAST> > > constDef;

    ConstDeclAST(BaseAST *p1, vector<unique_ptr<BaseAST> > *p2);
    Json::Value dump() const override;
    void *toKoopa() const override;
};

class ConstDefAST : public BaseAST {
public:
    string ident;
    unique_ptr<BaseAST> constInitVal;

    ConstDefAST(const char *p1, BaseAST *p2);
    Json::Value dump() const override;
    void *toKoopa() const override;
};

class VarDeclAST : public BaseAST {
public:
    unique_ptr<BaseAST> bType;
    unique_ptr<vector<unique_ptr<BaseAST> > > varDef;

    VarDeclAST(BaseAST *p1, vector<unique_ptr<BaseAST> > *p2);
    Json::Value dump() const override;
    void *toKoopa() const override; 
};

class VarDefAST1 : public BaseAST {
public:
    string ident;

    VarDefAST1(const char *p);
    Json::Value dump() const override;
    void *toKoopa() const override;
};

class VarDefAST2 : public BaseAST {
public:
    string ident;
    unique_ptr<BaseAST> initVal;

    VarDefAST2(const char *p1, BaseAST *p2);
    Json::Value dump() const override;
    void *toKoopa() const override;
};

class LValAST : public BaseAST {
public:
    string ident;

    LValAST(const char *p);
    Json::Value dump() const override;
    void *toKoopa() const override;
    int calculateExp() const override;
};
