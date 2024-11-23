#include "ast.hpp"
using namespace std;

// constructor
CompUnitAST::CompUnitAST(BaseAST *p) 
    : func_def(p) {}
FuncDefAST::FuncDefAST(BaseAST *p1, const char *p2, BaseAST *p3)
    : func_type(p1), ident(p2), block(p3) {}
FuncTypeAST::FuncTypeAST(const char *p)
    : type(p) {}
BlockAST::BlockAST(vector<unique_ptr<BaseAST> > *p)
    : blockItem(p) {}
StmtAST1::StmtAST1(BaseAST *p)
    : exp(p) {}
StmtAST2::StmtAST2(BaseAST *p1, BaseAST *p2)
    : lVal(p1), exp(p2) {}
StmtAST3::StmtAST3(BaseAST *p)
    : exp(p) {}
StmtAST4::StmtAST4(BaseAST *p)
    : block(p) {}
IfAST1::IfAST1(BaseAST *p1, BaseAST *p2)
    : exp(p1), stmtThen(p2) {}
IfAST2::IfAST2(BaseAST *p1, BaseAST *p2, BaseAST *p3)
    : exp(p1), stmtThen(p2), stmtElse(p3) {}
PrimaryExpAST::PrimaryExpAST(int num)
    : num(num) {}
UnaryExpAST::UnaryExpAST(const char *p1, BaseAST *p2)
    : unaryOp(p1), unaryExp(p2) {}
MulExpAST::MulExpAST(BaseAST *p1, const char *p2, BaseAST *p3)
    : mulExp(p1), mulOp(p2), unaryExp(p3) {}
AddExpAST::AddExpAST(BaseAST *p1, const char *p2, BaseAST *p3)
    : addExp(p1), addOp(p2), mulExp(p3) {}
RelExpAST::RelExpAST(BaseAST *p1, const char *p2, BaseAST *p3)
    : relExp(p1), relOp(p2), addExp(p3) {}
EqExpAST::EqExpAST(BaseAST *p1, const char *p2, BaseAST *p3)
    : eqExp(p1), eqOp(p2), relExp(p3) {}
LAndExpAST::LAndExpAST(BaseAST *p1, BaseAST *p2)
    : lAndExp(p1), eqExp(p2) {}
LOrExpAST::LOrExpAST(BaseAST *p1, BaseAST *p2)
    : lOrExp(p1), lAndExp(p2) {} 
ConstDeclAST::ConstDeclAST(BaseAST *p1, vector<unique_ptr<BaseAST> > *p2)
    : bType(p1), constDef(p2) {}
ConstDefAST::ConstDefAST(const char *p1, BaseAST *p2)
    : ident(p1), constInitVal(p2) {}
VarDeclAST::VarDeclAST(BaseAST *p1, vector<unique_ptr<BaseAST> > *p2)
    : bType(p1), varDef(p2) {}
VarDefAST1::VarDefAST1(const char *p)
    : ident(p) {}
VarDefAST2::VarDefAST2(const char *p1, BaseAST *p2)
    : ident(p1), initVal(p2) {}
BTypeAST::BTypeAST(const char *p)
    : type(p) {}
LValAST::LValAST(const char *p)
    : ident(p) {}
// end constructor

// calculateExp
int PrimaryExpAST::calculateExp() const {
    return num;
}
int UnaryExpAST::calculateExp() const {
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
int MulExpAST::calculateExp() const {
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
int AddExpAST::calculateExp() const {
    if(addOp[0] == '+')
        return addExp->calculateExp() + mulExp->calculateExp();
    else if(addOp[0] == '-')
        return addExp->calculateExp() - mulExp->calculateExp();
    return 0;
}
int RelExpAST::calculateExp() const {
    if(relOp == "<") 
        return relExp->calculateExp() < addExp->calculateExp();
    else if(relOp == "<=")
        return relExp->calculateExp() <= addExp->calculateExp();
    else if(relOp == ">")  
        return relExp->calculateExp() > addExp->calculateExp();
    else if(relOp == ">=")
        return relExp->calculateExp() >= addExp->calculateExp();
    return 0;
}
int EqExpAST::calculateExp() const {
    if(eqOp == "==")
        return eqExp->calculateExp() == relExp->calculateExp();
    else if(eqOp == "!=")
        return eqExp->calculateExp() != relExp->calculateExp();
    return 0;
}
int LAndExpAST::calculateExp() const {
    return lAndExp->calculateExp() && eqExp->calculateExp();
}
int LOrExpAST::calculateExp() const {
    return lOrExp->calculateExp() || lAndExp->calculateExp();
}
int LValAST::calculateExp() const {
    auto i = SymbolTable::getItem(ident);
    if(i.type == SYMBOLTABLE_ITEM_CONST) 
        return i.data.c;
    else {
        //常量求值里不能有查询到变量
        cerr << "Required no variables in const declaration" << endl;
        exit(1);
    }
}
// end calculateExp
