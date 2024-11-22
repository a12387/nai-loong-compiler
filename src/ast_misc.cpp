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
ExpAST::ExpAST(BaseAST *p)
    : lOrExp(p) {}
PrimaryExpAST1::PrimaryExpAST1(BaseAST *p)
    : exp(p) {}
PrimaryExpAST2::PrimaryExpAST2(int num)
    : num(num) {}
PrimaryExpAST3::PrimaryExpAST3(BaseAST *p)
    : lVal(p) {}
UnaryExpAST1::UnaryExpAST1(BaseAST *p)
    : primaryExp(p) {}
UnaryExpAST2::UnaryExpAST2(const char *p1, BaseAST *p2)
    : unaryOp(p1), unaryExp(p2) {}
MulExpAST1::MulExpAST1(BaseAST *p) 
    : unaryExp(p) {}
MulExpAST2::MulExpAST2(BaseAST *p1, const char *p2, BaseAST *p3)
    : mulExp(p1), mulOp(p2), unaryExp(p3) {}
AddExpAST1::AddExpAST1(BaseAST *p)
    : mulExp(p) {}
AddExpAST2::AddExpAST2(BaseAST *p1, const char *p2, BaseAST *p3)
    : addExp(p1), addOp(p2), mulExp(p3) {}
RelExpAST1::RelExpAST1(BaseAST *p)
    : addExp(p) {}
RelExpAST2::RelExpAST2(BaseAST *p1, const char *p2, BaseAST *p3)
    : relExp(p1), relOp(p2), addExp(p3) {}
EqExpAST1::EqExpAST1(BaseAST *p)
    : relExp(p) {}
EqExpAST2::EqExpAST2(BaseAST *p1, const char *p2, BaseAST *p3)
    : eqExp(p1), eqOp(p2), relExp(p3) {}
LAndExpAST1::LAndExpAST1(BaseAST *p)
    : eqExp(p) {}
LAndExpAST2::LAndExpAST2(BaseAST *p1, BaseAST *p2)
    : lAndExp(p1), eqExp(p2) {}
LOrExpAST1::LOrExpAST1(BaseAST *p)
    : lAndExp(p) {}
LOrExpAST2::LOrExpAST2(BaseAST *p1, BaseAST *p2)
    : lOrExp(p1), lAndExp(p2) {} 
DeclAST1::DeclAST1(BaseAST *p)
    : constDecl(p) {}
DeclAST2::DeclAST2(BaseAST *p) 
    : varDecl(p) {}
ConstDeclAST::ConstDeclAST(BaseAST *p1, vector<unique_ptr<BaseAST> > *p2)
    : bType(p1), constDef(p2) {}
ConstDefAST::ConstDefAST(const char *p1, BaseAST *p2)
    : ident(p1), constInitVal(p2) {}
ConstInitValAST::ConstInitValAST(BaseAST *p)
    : constExp(p) {}
VarDeclAST::VarDeclAST(BaseAST *p1, vector<unique_ptr<BaseAST> > *p2)
    : bType(p1), varDef(p2) {}
VarDefAST1::VarDefAST1(const char *p)
    : ident(p) {}
VarDefAST2::VarDefAST2(const char *p1, BaseAST *p2)
    : ident(p1), initVal(p2) {}
InitValAST::InitValAST(BaseAST *p)
    : exp(p) {}
BTypeAST::BTypeAST(const char *p)
    : type(p) {}
LValAST::LValAST(const char *p)
    : ident(p) {}
ConstExpAST::ConstExpAST(BaseAST *p)
    : exp(p) {}
// end constructor

// calculateExp
int ExpAST::calculateExp() const {
    return lOrExp->calculateExp();
}
int PrimaryExpAST1::calculateExp() const {
    return exp->calculateExp();
}
int PrimaryExpAST2::calculateExp() const {
    return num;
}
int PrimaryExpAST3::calculateExp() const {
    return lVal->calculateExp();
}
int UnaryExpAST1::calculateExp() const {
    return primaryExp->calculateExp();
}
int UnaryExpAST2::calculateExp() const {
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
int MulExpAST1::calculateExp() const {
    return unaryExp->calculateExp();
}
int MulExpAST2::calculateExp() const {
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
int AddExpAST1::calculateExp() const {
    return mulExp->calculateExp();
}
int AddExpAST2::calculateExp() const {
    if(addOp[0] == '+')
        return addExp->calculateExp() + mulExp->calculateExp();
    else if(addOp[0] == '-')
        return addExp->calculateExp() - mulExp->calculateExp();
    return 0;
}
int RelExpAST1::calculateExp() const {
    return addExp->calculateExp();
}
int RelExpAST2::calculateExp() const {
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
int EqExpAST1::calculateExp() const {
    return relExp->calculateExp();
}
int EqExpAST2::calculateExp() const {
    if(eqOp == "==")
        return eqExp->calculateExp() == relExp->calculateExp();
    else if(eqOp == "!=")
        return eqExp->calculateExp() != relExp->calculateExp();
    return 0;
}
int LAndExpAST1::calculateExp() const {
    return eqExp->calculateExp();
}
int LAndExpAST2::calculateExp() const {
    return lAndExp->calculateExp() && eqExp->calculateExp();
}
int LOrExpAST1::calculateExp() const {
    return lAndExp->calculateExp();
}
int LOrExpAST2::calculateExp() const {
    return lOrExp->calculateExp() || lAndExp->calculateExp();
}
int ConstInitValAST::calculateExp() const {
    return constExp->calculateExp();
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
int ConstExpAST::calculateExp() const {
    return exp->calculateExp();
}
// end calculateExp
