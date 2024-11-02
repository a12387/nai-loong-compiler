#include "ast.hpp"
using namespace std;

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
