#include "ast.hpp"
using namespace std;

void *CompUnitAST::toKoopa() const {
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
void *FuncDefAST::toKoopa() const {
    auto raw = new koopa_raw_function_data_t;

    char *name = new char[ident.size() + 2];
    strcpy(name, ("@"+ident).c_str());
    raw->name = name;
    raw->params = {
        nullptr,
        0,
        KOOPA_RSIK_VALUE
    };
    
    auto ty = createTypeKind(KOOPA_RTT_FUNCTION);
    ty->data.function.params = {
        nullptr,
        0,
        KOOPA_RSIK_TYPE
    };
    ty->data.function.ret = (koopa_raw_type_t)func_type->toKoopa();
    raw->ty = ty;

    vector<const void *> buffer;
    buffer.push_back(block->toKoopa());
    raw->bbs.buffer = new const void *[buffer.size()];
    copy(buffer.begin(),buffer.end(),raw->bbs.buffer);
    raw->bbs.kind = KOOPA_RSIK_BASIC_BLOCK;
    raw->bbs.len = buffer.size();

    return raw;
}
void *FuncTypeAST::toKoopa() const {
    if(type == "int")
        return createTypeKind(KOOPA_RTT_INT32);
    else
        return createTypeKind(KOOPA_RTT_UNIT);
}
void *BlockAST::toKoopa() const {
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
    for(int i = 0; i < blockItem->size(); i++) {
        (*blockItem)[i]->toKoopa(buffer);
        if(!buffer.empty() && ((koopa_raw_value_data_t*)buffer.back())->kind.tag == KOOPA_RVT_RETURN) {
            break;
        }
    }
    if(buffer.empty() || ((koopa_raw_value_data_t*)buffer.back())->kind.tag != KOOPA_RVT_RETURN) {
        auto ty_value = createTypeKind(KOOPA_RTT_INT32);
        auto value = createValueData(KOOPA_RVT_INTEGER, nullptr, ty_value, KOOPA_RSIK_VALUE);

        auto ty_ret = createTypeKind(KOOPA_RTT_UNIT);
        auto ret = createValueData(KOOPA_RVT_RETURN, nullptr, ty_ret, KOOPA_RSIK_VALUE);
        ret->kind.data.ret.value = value;

        addItemToSlice(value->used_by, ret);

        buffer.push_back(ret);
    }
    
    raw->insts.buffer = new const void *[buffer.size()];
    copy(buffer.begin(), buffer.end(), raw->insts.buffer);
    raw->insts.kind = KOOPA_RSIK_VALUE;
    raw->insts.len = buffer.size();

    return raw;
}
void *StmtAST1::toKoopa(vector<const void *> & buffer) const {
    auto ty = createTypeKind(KOOPA_RTT_UNIT);
    auto raw = createValueData(KOOPA_RVT_RETURN, nullptr, ty, KOOPA_RSIK_VALUE);
    
    auto value = (koopa_raw_value_data_t *)exp->toKoopa(buffer);
    addItemToSlice(value->used_by, raw);
    raw->kind.data.ret.value = value;

    buffer.push_back(raw);
    return raw;
}
void *StmtAST2::toKoopa(vector<const void *> &buffer) const {
    auto ty = createTypeKind(KOOPA_RTT_UNIT);
    auto raw = createValueData(KOOPA_RVT_STORE, nullptr, ty, KOOPA_RSIK_VALUE);

    auto dest = (koopa_raw_value_data_t  *)lVal->toKoopa();
    raw->kind.data.store.dest = dest;
    addItemToSlice(dest->used_by, raw);
    auto value = (koopa_raw_value_data_t *)exp->toKoopa(buffer);
    raw->kind.data.store.value = value;
    addItemToSlice(value->used_by, raw);

    buffer.push_back(raw);
    return raw;
}
void *ExpAST::toKoopa(vector<const void *> &buffer) const {
    return lOrExp->toKoopa(buffer);
}
void *PrimaryExpAST1::toKoopa(vector<const void *> &buffer) const {
    return exp->toKoopa(buffer);
}
void *PrimaryExpAST2::toKoopa(vector<const void *> &buffer) const {
    auto ty = createTypeKind(KOOPA_RTT_INT32);
    auto raw = createValueData(KOOPA_RVT_INTEGER, nullptr, ty, KOOPA_RSIK_VALUE);

    raw->kind.data.integer.value = num;

    return raw;
}
void *PrimaryExpAST3::toKoopa(vector<const void *> &buffer) const {
    return lVal->toKoopa(buffer);
}
void *UnaryExpAST1::toKoopa(vector<const void *> &buffer) const {
    return primaryExp->toKoopa(buffer);
}
void *UnaryExpAST2::toKoopa(vector<const void *> &buffer) const {
    if(unaryOp == "+")
        return unaryExp->toKoopa(buffer);

    auto rhs = (koopa_raw_value_data_t *)unaryExp->toKoopa(buffer);

    auto lhs = createValueData(KOOPA_RVT_INTEGER, nullptr, rhs->ty, KOOPA_RSIK_VALUE);
    lhs->kind.data.integer.value = 0;

    auto raw = createValueData(KOOPA_RVT_BINARY, nullptr, rhs->ty, KOOPA_RSIK_VALUE);
    raw->kind.data.binary.rhs = rhs;
    raw->kind.data.binary.lhs = lhs;
    raw->kind.data.binary.op = op_map[unaryOp];

    addItemToSlice(rhs->used_by, raw);
    addItemToSlice(lhs->used_by, raw);

    buffer.push_back(raw);

    return raw;
}
void *MulExpAST1::toKoopa(vector<const void *> &buffer) const {
    return unaryExp->toKoopa(buffer);
}
void *MulExpAST2::toKoopa(vector<const void *> &buffer) const {
    auto lhs = (koopa_raw_value_data_t *)mulExp->toKoopa(buffer);
    auto rhs = (koopa_raw_value_data_t *)unaryExp->toKoopa(buffer);

    auto raw = createValueData(KOOPA_RVT_BINARY, nullptr, lhs->ty, KOOPA_RSIK_VALUE);
    raw->kind.data.binary.lhs = lhs;
    raw->kind.data.binary.rhs = rhs;
    raw->kind.data.binary.op = op_map[mulOp];

    addItemToSlice(lhs->used_by, raw);
    addItemToSlice(rhs->used_by, raw);

    buffer.push_back(raw);

    return raw;
}
void *AddExpAST1::toKoopa(vector<const void*> &buffer) const {
    return mulExp->toKoopa(buffer);
}
void *AddExpAST2::toKoopa(vector<const void*> &buffer) const {
    auto lhs = (koopa_raw_value_data_t *)addExp->toKoopa(buffer);
    auto rhs = (koopa_raw_value_data_t *)mulExp->toKoopa(buffer);

    auto raw = createValueData(KOOPA_RVT_BINARY, nullptr, lhs->ty, KOOPA_RSIK_VALUE);
    raw->kind.data.binary.lhs = lhs;
    raw->kind.data.binary.rhs = rhs;
    raw->kind.data.binary.op = op_map[addOp];
    
    addItemToSlice(lhs->used_by, raw);
    addItemToSlice(rhs->used_by, raw);

    buffer.push_back(raw);

    return raw;
}
void *RelExpAST1::toKoopa(vector<const void *> &buffer) const {
    return addExp->toKoopa(buffer);
}
void *RelExpAST2::toKoopa(vector<const void *> &buffer) const {
    auto lhs = (koopa_raw_value_data_t *)relExp->toKoopa(buffer);
    auto rhs = (koopa_raw_value_data_t *)addExp->toKoopa(buffer);

    auto raw = createValueData(KOOPA_RVT_BINARY, nullptr, lhs->ty, KOOPA_RSIK_VALUE);
    raw->kind.data.binary.lhs = lhs;
    raw->kind.data.binary.rhs = rhs;
    raw->kind.data.binary.op = op_map[relOp];

    addItemToSlice(rhs->used_by, raw);
    addItemToSlice(lhs->used_by, raw);

    buffer.push_back(raw);

    return raw;
}
void *EqExpAST1::toKoopa(vector<const void *> &buffer) const {
    return relExp->toKoopa(buffer);
}
void *EqExpAST2::toKoopa(vector<const void *> &buffer) const {
    auto lhs = (koopa_raw_value_data_t *)eqExp->toKoopa(buffer);
    auto rhs = (koopa_raw_value_data_t *)relExp->toKoopa(buffer);

    auto raw = createValueData(KOOPA_RVT_BINARY, nullptr, lhs->ty, KOOPA_RSIK_VALUE);
    raw->kind.data.binary.lhs = lhs;
    raw->kind.data.binary.rhs = rhs;
    raw->kind.data.binary.op = op_map[eqOp];

    addItemToSlice(lhs->used_by, raw);
    addItemToSlice(rhs->used_by, raw);

    buffer.push_back(raw);

    return raw;
}
void *LAndExpAST1::toKoopa(vector<const void *> &buffer) const {
    return eqExp->toKoopa(buffer);
}
void *LAndExpAST2::toKoopa(vector<const void *> &buffer) const {
    auto rhs1 = (koopa_raw_value_data_t *)lAndExp->toKoopa(buffer);

    auto lhs1= createValueData(KOOPA_RVT_INTEGER, nullptr, rhs1->ty, KOOPA_RSIK_VALUE);
    lhs1->kind.data.integer.value = 0;
    
    auto raw1 = createValueData(KOOPA_RVT_BINARY, nullptr, rhs1->ty, KOOPA_RSIK_VALUE);
    raw1->kind.data.binary.rhs = rhs1;
    raw1->kind.data.binary.lhs = lhs1;
    raw1->kind.data.binary.op = op_map["!="];
    addItemToSlice(rhs1->used_by, raw1);
    addItemToSlice(lhs1->used_by, raw1);

    buffer.push_back(raw1);

    auto rhs2 = (koopa_raw_value_data_t *)lAndExp->toKoopa(buffer);

    auto lhs2= createValueData(KOOPA_RVT_INTEGER, nullptr, rhs2->ty, KOOPA_RSIK_VALUE);
    lhs2->kind.data.integer.value = 0;
    
    auto raw2 = createValueData(KOOPA_RVT_BINARY, nullptr, rhs2->ty, KOOPA_RSIK_VALUE);
    raw2->kind.data.binary.rhs = rhs2;
    raw2->kind.data.binary.lhs = lhs2;
    raw2->kind.data.binary.op = op_map["!="];
    addItemToSlice(rhs2->used_by, raw2);
    addItemToSlice(lhs2->used_by, raw2);

    buffer.push_back(raw2);

    auto raw = createValueData(KOOPA_RVT_BINARY, nullptr, raw1->ty, KOOPA_RSIK_VALUE);
    raw->kind.data.binary.lhs = raw1;
    raw->kind.data.binary.rhs = raw2;
    raw->kind.data.binary.op = op_map["&&"];

    addItemToSlice(raw1->used_by, raw);
    addItemToSlice(raw2->used_by, raw);

    buffer.push_back(raw);

    return raw;
}
void *LOrExpAST1::toKoopa(vector<const void *> &buffer) const {
    return lAndExp->toKoopa(buffer);
}
void *LOrExpAST2::toKoopa(vector<const void *> &buffer) const {
    auto lhs0 = (koopa_raw_value_data_t *)lOrExp->toKoopa(buffer);
    auto rhs0 = (koopa_raw_value_data_t *)lAndExp->toKoopa(buffer);

    auto raw0 = createValueData(KOOPA_RVT_BINARY, nullptr, lhs0->ty, KOOPA_RSIK_VALUE);
    raw0->kind.data.binary.lhs = lhs0;
    raw0->kind.data.binary.rhs = rhs0;
    raw0->kind.data.binary.op = op_map["||"];

    addItemToSlice(lhs0->used_by, raw0);
    addItemToSlice(rhs0->used_by, raw0);

    buffer.push_back(raw0);

    auto lhs= createValueData(KOOPA_RVT_INTEGER, nullptr, raw0->ty, KOOPA_RSIK_VALUE);
    lhs->kind.data.integer.value = 0;

    auto raw = createValueData(KOOPA_RVT_BINARY, nullptr, raw0->ty, KOOPA_RSIK_VALUE);
    raw->kind.data.binary.lhs = lhs;
    raw->kind.data.binary.rhs = raw0;
    raw->kind.data.binary.op = op_map["!="];

    addItemToSlice(lhs->used_by, raw);
    addItemToSlice(raw0->used_by, raw);

    buffer.push_back(raw);

    return raw;
}
void *DeclAST1::toKoopa(vector<const void*> &buffer) const {
    return constDecl->toKoopa();;
}
void *DeclAST2::toKoopa(vector<const void*> &buffer) const {
    return varDecl->toKoopa(buffer);
}
void *ConstDeclAST::toKoopa() const {
    for(int i = 0; i < constDef->size(); i++) {
        (*constDef)[i]->toKoopa();
    }
    return nullptr;
}
void *BTypeAST::toKoopa() const {
    if(type == "int")
        return createTypeKind(KOOPA_RTT_INT32);
    else
        return createTypeKind(KOOPA_RTT_UNIT);
}
void *ConstDefAST::toKoopa() const{
    SymbolTable::addItem(ident, constInitVal->calculateExp());
    return nullptr;
}
void *VarDeclAST::toKoopa(vector<const void *> &buffer) const {
    auto ty = createTypeKind(KOOPA_RTT_POINTER);
    ty->data.pointer.base = (koopa_raw_type_t)bType->toKoopa();
    for(int i = 0; i < varDef->size(); i++) {
        auto ptr = (koopa_raw_value_data_t *)(*varDef)[i]->toKoopa(buffer);
        ptr->ty = ty;
    }
    return nullptr;
}
void *VarDefAST1::toKoopa(vector<const void *> &buffer) const {
    // ty暂时设成nullptr，之后在VarDecl中赋值
    auto raw = createValueData(KOOPA_RVT_ALLOC, ("@" + ident).c_str(), nullptr, KOOPA_RSIK_VALUE);

    buffer.push_back(raw);

    SymbolTable::addItem(ident, raw);

    return raw;
}
void *VarDefAST2::toKoopa(vector<const void *> &buffer) const {
    // ty暂时设成nullptr，之后在VarDecl中赋值
    auto raw1 = createValueData(KOOPA_RVT_ALLOC, ("@" + ident).c_str(), nullptr, KOOPA_RSIK_VALUE);

    buffer.push_back(raw1);

    SymbolTable::addItem(ident, raw1);

    auto ty = createTypeKind(KOOPA_RTT_UNIT);

    auto value = (koopa_raw_value_data_t *)initVal->toKoopa(buffer);

    auto raw2 = createValueData(KOOPA_RVT_STORE, nullptr, ty, KOOPA_RSIK_VALUE);
    raw2->kind.data.store.dest = raw1;
    raw2->kind.data.store.value = value;

    addItemToSlice(raw1->used_by, raw2);
    addItemToSlice(value->used_by, raw2);

    buffer.push_back(raw2);

    return raw1;
    
}
void *InitValAST::toKoopa(vector<const void *> &buffer) const {
    return exp->toKoopa(buffer);
}
void *LValAST::toKoopa() const {
    //作为左值引用一个符号
    auto i = SymbolTable::getItem(ident);
    if(i.type == SYMBOLTABLE_ITEM_CONST) {
        cerr << "lvalue required as left operand of assignment" << endl;
        exit(1);
    }
    else if(i.type == SYMBOLTABLE_ITEM_VAR) {
        return i.data.v;
    }
    return nullptr;
}
void *LValAST::toKoopa(vector<const void *> &buffer) const {
    //作为右值引用一个符号（如果是变量，必须先Load）
    auto i = SymbolTable::getItem(ident);
    auto ty = createTypeKind(KOOPA_RTT_INT32);

    if(i.type == SYMBOLTABLE_ITEM_CONST) {
        auto raw = createValueData(KOOPA_RVT_INTEGER, nullptr, ty, KOOPA_RSIK_VALUE);
        raw->kind.data.integer.value = i.data.c;

        return raw;
    }
    else if (i.type == SYMBOLTABLE_ITEM_VAR) {
        auto src = i.data.v;

        auto raw = createValueData(KOOPA_RVT_LOAD, nullptr, ty, KOOPA_RSIK_VALUE);
        raw->kind.data.load.src = src;

        addItemToSlice(src->used_by, raw);

        buffer.push_back(raw);

        return raw;
    }
    return nullptr;
}
