#pragma once
#include <iostream>
#include <memory>
#include <cassert>
#include <unordered_map>
#include <vector>
#include <string.h>
#include <koopa.h>
#include "json/json.h"
#include "symbol_table.hpp"

using namespace std;

class BaseAST {
public:
    virtual ~BaseAST() = default;
    virtual Json::Value dump() const = 0;
    virtual void* toKoopa() const { return nullptr; }
    virtual int calculateExp() const { return 0; }
protected:
    inline static vector<void *> bufferBlocks = {};
    inline static vector<const void *> bufferInsts = {};
};

class CompUnitAST : public BaseAST {
public:
    unique_ptr<BaseAST> func_def;

    CompUnitAST(BaseAST *p);
    Json::Value dump() const override;
    void *toKoopa() const override;
};

class FuncDefAST : public BaseAST {
public:
    unique_ptr<BaseAST> func_type;
    string ident;
    unique_ptr<BaseAST> block;

    FuncDefAST(BaseAST *p1, const char *p2, BaseAST *p3);
    Json::Value dump() const override;
    void* toKoopa() const override;
};

class FuncTypeAST : public BaseAST {
public:
    string type;

    FuncTypeAST(const char *p);
    Json::Value dump() const override;
    void* toKoopa() const override;
};

class BlockAST : public BaseAST {
public:
    unique_ptr<vector<unique_ptr<BaseAST> > >blockItem;

    BlockAST(vector<unique_ptr<BaseAST> >*p);
    Json::Value dump() const override;
    void* toKoopa() const override;
};

class StmtAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> exp;

    StmtAST1(BaseAST *p);
    Json::Value dump() const override;
    void* toKoopa() const override;
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

class IfAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> exp;
    unique_ptr<BaseAST> stmtThen;

    IfAST1(BaseAST *p1, BaseAST *p2);
    Json::Value dump() const override;
    void *toKoopa() const override;
};

class IfAST2: public BaseAST {
public:
    unique_ptr<BaseAST> exp;
    unique_ptr<BaseAST> stmtThen;
    unique_ptr<BaseAST> stmtElse;

    IfAST2(BaseAST *p1, BaseAST *p2, BaseAST *p3);
    Json::Value dump() const override;
    void *toKoopa() const override;
};

class ExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> lOrExp;

    ExpAST(BaseAST *p);
    Json::Value dump() const override;
    void* toKoopa() const override;
    int calculateExp() const override;
};

class PrimaryExpAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> exp;

    PrimaryExpAST1(BaseAST *p);
    Json::Value dump() const override;
    void* toKoopa() const override;
    int calculateExp() const override;
};

class PrimaryExpAST2: public BaseAST {
public:
    int num;

    PrimaryExpAST2(int num);
    Json::Value dump() const override;
    void* toKoopa() const override;
    int calculateExp() const override;
};

class PrimaryExpAST3 : public BaseAST {
public:
    unique_ptr<BaseAST> lVal;

    PrimaryExpAST3(BaseAST *p);
    Json::Value dump() const override;
    void *toKoopa() const override;
    int calculateExp() const override;
};

class UnaryExpAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> primaryExp;

    UnaryExpAST1(BaseAST *p);
    Json::Value dump() const override;
    void* toKoopa() const override;
    int calculateExp() const override;
};

class UnaryExpAST2 : public BaseAST {
public:
    string unaryOp;
    unique_ptr<BaseAST> unaryExp;

    UnaryExpAST2(const char *p1, BaseAST *p2);
    Json::Value dump() const override;
    void* toKoopa() const override;
    int calculateExp() const override;
};

class MulExpAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> unaryExp;

    MulExpAST1(BaseAST *p);
    Json::Value dump() const override;
    void* toKoopa() const override;
    int calculateExp() const override;
};

class MulExpAST2 : public BaseAST {
public:
    unique_ptr<BaseAST> mulExp;
    string mulOp;
    unique_ptr<BaseAST> unaryExp;

    MulExpAST2(BaseAST *p1, const char *p2, BaseAST *p3);
    Json::Value dump() const override;
    void* toKoopa() const override;
    int calculateExp() const override;
};

class AddExpAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> mulExp;

    AddExpAST1(BaseAST *p);
    Json::Value dump() const override;
    void* toKoopa() const override;
    int calculateExp() const override;
};

class AddExpAST2 : public BaseAST {
public:
    unique_ptr<BaseAST> addExp;
    string addOp;
    unique_ptr<BaseAST> mulExp;

    AddExpAST2(BaseAST *p1, const char *p2, BaseAST *p3);
    Json::Value dump() const override;
    void* toKoopa() const override;
    int calculateExp() const override;
};

class RelExpAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> addExp;

    RelExpAST1(BaseAST *p);
    Json::Value dump() const override;
    void* toKoopa() const override;
    int calculateExp() const override;
};

class RelExpAST2 : public BaseAST {
public:
    unique_ptr<BaseAST> relExp;
    string relOp;
    unique_ptr<BaseAST> addExp;
    
    RelExpAST2(BaseAST *p1, const char *p2, BaseAST *p3);
    Json::Value dump() const override;
    void* toKoopa() const override;
    int calculateExp() const override;
};

class EqExpAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> relExp;

    EqExpAST1(BaseAST *p);
    Json::Value dump() const override;
    void* toKoopa() const override;
    int calculateExp() const override;
};

class EqExpAST2 : public BaseAST {
public:
    unique_ptr<BaseAST> eqExp;
    string eqOp;
    unique_ptr<BaseAST> relExp;

    EqExpAST2(BaseAST *p1, const char *p2, BaseAST *p3);
    Json::Value dump() const override;
    void* toKoopa() const override;
    int calculateExp() const override;
};

class LAndExpAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> eqExp;

    LAndExpAST1(BaseAST *p);
    Json::Value dump() const override;
    void* toKoopa() const override;
    int calculateExp() const override;
};

class LAndExpAST2 : public BaseAST {
public:
    unique_ptr<BaseAST> lAndExp;
    unique_ptr<BaseAST> eqExp;

    LAndExpAST2(BaseAST *p1, BaseAST *p2);
    Json::Value dump() const override;
    void* toKoopa() const override;
    int calculateExp() const override;
};

class LOrExpAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> lAndExp;

    LOrExpAST1(BaseAST *p);
    Json::Value dump() const override;
    void* toKoopa() const override;
    int calculateExp() const override;
};

class LOrExpAST2 : public BaseAST {
public:
    unique_ptr<BaseAST> lOrExp;
    unique_ptr<BaseAST> lAndExp;

    LOrExpAST2(BaseAST *p1, BaseAST *p2);
    Json::Value dump() const override;
    void* toKoopa() const override;
    int calculateExp() const override;
};

class DeclAST1 : public BaseAST {
public:
    unique_ptr<BaseAST> constDecl;

    DeclAST1(BaseAST *p);
    Json::Value dump() const override;
    void *toKoopa() const override;
};

class DeclAST2 : public BaseAST {
public:
    unique_ptr<BaseAST> varDecl;

    DeclAST2(BaseAST *p);
    Json::Value dump() const override;
    void *toKoopa() const override;
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

class ConstInitValAST : public BaseAST {
public:
    unique_ptr<BaseAST> constExp;

    ConstInitValAST(BaseAST *p);
    Json::Value dump() const override;
    int calculateExp() const override;
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

class InitValAST : public BaseAST {
public:
    unique_ptr<BaseAST> exp;

    InitValAST(BaseAST *p);
    Json::Value dump() const override;
    void *toKoopa() const override;
};

class BTypeAST : public BaseAST {
public:
    string type;

    BTypeAST(const char *p);
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

class ConstExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> exp;

    ConstExpAST(BaseAST *p);
    Json::Value dump() const override;
    int calculateExp() const override;
};