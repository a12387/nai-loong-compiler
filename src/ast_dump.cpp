#include "ast.hpp"
using namespace Json;

Value CompUnitAST::dump() const {
    Value comp_unit;
    comp_unit["FuncDef"] = func_def->dump();
    return comp_unit;
}
Value FuncDefAST::dump() const {
    Value v;

    v["FuncType"] = func_type->dump();
    v["Ident"] = ident;
    v["Block"] = block->dump();

    return v;
}
Value FuncTypeAST::dump() const {
    Value v;
    v["Type"] = type;
    return v;
}
Value BlockAST::dump() const {
    Value v;
    Value items;
    if(blockItem == nullptr) {
        v["BlockItem"] = "Empty";
    }
    else {
        for(int i = 0; i < blockItem->size(); i++) {
            items[i] = (*blockItem)[i]->dump();
        }
        v["BlockItem"] = items;
    }
    return v;
}
Value StmtAST1::dump() const {
    Value stmt;
    stmt["Exp"] = exp->dump();
    Value v;
    v["Return"] = stmt;
    return v;
}
Value StmtAST2::dump() const {
    Value stmt;
    stmt["LVal"] = lVal->dump();
    stmt["Exp"] = exp->dump();
    Value v;
    v["Assign"] = stmt;
    return v;
}
Value StmtAST3::dump() const {
    Value v;
    if(exp == nullptr) {
        v["Non-Assign Exp"] = "Empty";
    }
    else {
        v["Non-Assign Exp"] = exp->dump();
    }
    return v;
}
Value StmtAST4::dump() const {
    Value stmt;
    if(block != nullptr)
        stmt["Block"] = block->dump();
    else 
        stmt["Block"] = "Empty";
    Value v;
    v["New Block"] = stmt;
    return v;
}
Value ExpAST::dump() const {
    return lOrExp->dump();
}
Value PrimaryExpAST1::dump() const {
    return exp->dump();
}
Value PrimaryExpAST2::dump() const {
    Value v;
    v["Number"] = num;
    return v;
}
Value PrimaryExpAST3::dump() const {
    Value v;
    v["LVal"] = lVal->dump();
    return v;
}
Value UnaryExpAST1::dump() const {
    return primaryExp->dump();
}
Value UnaryExpAST2::dump() const {
    Value v;
    v["Operator"] = unaryOp;
    v["UnaryExp"] = unaryExp->dump();
    return v;
}
Value MulExpAST1::dump() const {
    return unaryExp->dump();
}
Value MulExpAST2::dump() const {
    Value v;
    v["MulExp"] = mulExp->dump();
    v["Operator"] = mulOp;
    v["UnaryExp"] = unaryExp->dump();
    return v;
}
Value AddExpAST1::dump() const {
    return mulExp->dump();
}
Value AddExpAST2::dump() const {
    Value v;
    v["AddExp"] = addExp->dump();
    v["Operator"] = addOp;
    v["MulExp"] = mulExp->dump();
    return v;
}
Value RelExpAST1::dump() const {
    return addExp->dump();
}
Value RelExpAST2::dump() const {
    Value v;
    v["RelExp"] = relExp->dump();
    v["Operator"] = relOp;
    v["AddExp"] = addExp->dump();
    return v;
}
Value EqExpAST1::dump() const {
    return relExp->dump();
}
Value EqExpAST2::dump() const {
    Value v;
    v["EqExp"] = eqExp->dump();
    v["Operator"] = eqOp;
    v["RelExp"] = relExp->dump();
    return v;
}
Value LAndExpAST1::dump() const {
    return eqExp->dump();
}
Value LAndExpAST2::dump() const {
    Value v;
    v["LAndExp"] = lAndExp->dump();
    v["Operator"] = "&&";
    v["EqExp"] = eqExp->dump();
    return v;
}
Value LOrExpAST1::dump() const {
    return lAndExp->dump();
}
Value LOrExpAST2::dump() const {
    Value v;
    v["LOrExp"] = lOrExp->dump();
    v["Operator"] = "||";
    v["LAndExp"] = lAndExp->dump();
    return v;
}
Value DeclAST1::dump() const {
    Value v;
    v["ConstDecl"] = constDecl->dump();
    return v;
}
Value DeclAST2::dump() const {
    Value v;
    v["VarDecl"] = varDecl->dump();
    return v;
}
Value ConstDeclAST::dump() const {
    Value v;
    v["BType"] = bType->dump();
    Value defs;
    for(int i = 0; i < constDef->size(); i++) {
        defs[i] = (*constDef)[i]->dump();
    }
    v["ConstDef"] = defs;
    return v;
}
Value BTypeAST::dump() const {
    Value v;
    v["Type"] = type;
    return v;
}
Value ConstDefAST::dump() const {
    Value v;
    v["Ident"] = ident;
    v["ConstInitVal"] = constInitVal->dump();
    return v;
}
Value ConstInitValAST::dump() const {
    return constExp->dump();
}
Value VarDeclAST::dump() const {
    Value v;
    v["BType"] = bType->dump(); 
    Value defs;
    for(int i = 0; i < varDef->size(); i++) {
        defs[i] = (*varDef)[i]->dump();
    }
    v["VarDef"] = defs;
    return v;
}
Value VarDefAST1::dump() const {
    Value v;
    v["Ident"] = ident;
    return v;
}
Value VarDefAST2::dump() const {
    Value v;
    v["Ident"] = ident;
    v["InitVal"] = initVal->dump();
    return v;
}
Value InitValAST::dump() const {
    return exp->dump();
}
Value LValAST::dump() const {
    Value v;
    v["Ident"] = ident;
    return v;
}
Value ConstExpAST::dump() const {
    return exp->dump();
}
