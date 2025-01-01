#include "ast.hpp"
using namespace Json;

Value CompUnitAST::dump() const {
    Value comp_unit;
    for(auto &i : *defs)
        comp_unit["Def"] = i->dump();
    return comp_unit;
}
Value FuncDefAST::dump() const {
    Value v;

    v["FuncType"] = func_type->dump();
    v["Ident"] = ident;
    if(fparams == nullptr) {
        v["Params"] = "Empty";
    }
    else {
        Value params;
        for(int i = 0; i < fparams->size(); i++) {
            params[i] = (*fparams)[i]->dump();
        }
        v["Params"] = params;
    }
    v["Block"] = block->dump();

    return v;
}
Value FuncFParamAST::dump() const {
    Value v;
    v["BType"] = bType->dump();
    v["Ident"] = ident;
    return v;
}
Value TypeAST::dump() const {
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
    if(exp != nullptr) {
        Value stmt;
        stmt["Exp"] = exp->dump();
        Value v;
        v["Return"] = stmt;
        return v;
    }
    else {
        Value v;
        v["Return"] = "Void";
        return v;
    }
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
Value StmtAST5::dump() const {
    Value v;
    v["Break"] = "Break";
    return v;
}
Value StmtAST6::dump() const {
    Value v;
    v["Continue"] = "Continue";
    return v;
}
Value IfAST1::dump() const {
    Value ifstmt;
    ifstmt["Exp"] = exp->dump();
    ifstmt["Then"] = stmtThen->dump();
    Value v;
    v["If"] = ifstmt;
    return v;
}
Value IfAST2::dump() const {
    Value ifstmt;
    ifstmt["Exp"] = exp->dump();
    ifstmt["Then"] = stmtThen->dump();
    ifstmt["Else"] = stmtElse->dump();
    Value v;
    v["If"] = ifstmt;
    return v;
}
Value WhileAST::dump() const {
    Value whilestmt;
    whilestmt["Exp"] = exp->dump();
    whilestmt["Stmt"] = stmt->dump();
    Value v;
    v["While"] = whilestmt;
    return v;
}
Value PrimaryExpAST::dump() const {
    Value v;
    v["Number"] = num;
    return v;
}
Value UnaryExpAST1::dump() const {
    Value v;
    v["Operator"] = unaryOp;
    v["UnaryExp"] = unaryExp->dump();
    return v;
}
Value UnaryExpAST2::dump() const {
    Value v;
    v["Func"] = ident;
    if(rparams == nullptr) {
        v["Params"] = "Empty";
    }
    else {
        Value params;
        for(int i = 0; i < rparams->size(); i++) {
            params[i] = (*rparams)[i]->dump();
        }
        v["Params"] = params;
    }
    return v;
}
Value MulExpAST::dump() const {
    Value v;
    v["MulExp"] = mulExp->dump();
    v["Operator"] = mulOp;
    v["UnaryExp"] = unaryExp->dump();
    return v;
}
Value AddExpAST::dump() const {
    Value v;
    v["AddExp"] = addExp->dump();
    v["Operator"] = addOp;
    v["MulExp"] = mulExp->dump();
    return v;
}
Value RelExpAST::dump() const {
    Value v;
    v["RelExp"] = relExp->dump();
    v["Operator"] = relOp;
    v["AddExp"] = addExp->dump();
    return v;
}
Value EqExpAST::dump() const {
    Value v;
    v["EqExp"] = eqExp->dump();
    v["Operator"] = eqOp;
    v["RelExp"] = relExp->dump();
    return v;
}
Value LAndExpAST::dump() const {
    Value v;
    v["LAndExp"] = lAndExp->dump();
    v["Operator"] = "&&";
    v["EqExp"] = eqExp->dump();
    return v;
}
Value LOrExpAST::dump() const {
    Value v;
    v["LOrExp"] = lOrExp->dump();
    v["Operator"] = "||";
    v["LAndExp"] = lAndExp->dump();
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
Value ConstDefAST::dump() const {
    Value v;
    v["Ident"] = ident;
    v["ConstInitVal"] = constInitVal->dump();
    return v;
}
Value ConstArrayDefAST::dump() const {
    Value v;
    v["Ident"] = ident;
    //v["Length"] = exp_length->dump();
    v["InitList"] = initVal->dump();
    return v;
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
Value VarArrayDefAST1::dump() const {
    Value v;
    v["Ident"] = ident;
    //v["Length"] = exp_length->dump();
    return v;
}
Value VarArrayDefAST2::dump() const {
    Value v;
    v["Ident"] = ident;
    //v["Length"] = exp_length->dump();
    v["InitList"] = initVal->dump();
    return v;
}
Value VarDefAST2::dump() const {
    Value v;
    v["Ident"] = ident;
    v["InitVal"] = initVal->dump();
    return v;
}
Value LValAST1::dump() const {
    Value v;
    v["Ident"] = ident;
    return v;
}
Value LValAST2::dump() const {
    Value v;
    v["Ident"] = ident;
    Value items;
    for(int i = 0; i < indexes->size(); i++) {
        items[i] = (*indexes)[i]->dump();
    }
    v["Index"] = items;
    return v;
}
Value ConstInitValAST::dump() const {
    Value v;
    if(values == nullptr) {
        v["Array"] = "Empty";
        return v;
    }

    Value items;
    for(int i = 0; i < values->size(); i++) {
        items[i] = (*values)[i]->dump();
    }
    v["Array"] = items;
    return v;
}
Value InitValAST::dump() const {
    Value v;
    if(values == nullptr) {
        v["Array"] = "Empty";
        return v;
    }

    Value items;
    for(int i = 0; i < values->size(); i++) {
        items[i] = (*values)[i]->dump();
    }
    v["Array"] = items;
    return v;
}
