#include "ast.hpp"
#include "helper.hpp"
using namespace std;

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
    auto ty = createTypeKind(KOOPA_RTT_FUNCTION);
    ty->data.function.params = {
        nullptr,
        0,
        KOOPA_RSIK_TYPE
    };
    ty->data.function.ret = (koopa_raw_type_t)func_type->toKoopa();

    auto rawfunc = createFuncData(("@" + ident).c_str(), ty, KOOPA_RSIK_VALUE, KOOPA_RSIK_BASIC_BLOCK);

    auto rawentry = createBasicBlockData("%entry", KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE);
    bufferBlocks.push_back(rawentry);
    block->toKoopa();
    auto ptr = (koopa_raw_basic_block_data_t *)bufferBlocks.back();
    addItemToSlice(ptr->insts, bufferInsts);

    addItemToSlice(rawfunc->bbs, bufferBlocks);
    bufferBlocks.clear();

    return rawfunc;
}
void *FuncTypeAST::toKoopa() const {
    if(type == "int")
        return createTypeKind(KOOPA_RTT_INT32);
    else
        return createTypeKind(KOOPA_RTT_UNIT);
}
void *BlockAST::toKoopa() const {
    SymbolTable::addTable();
    if(blockItem == nullptr) {
        SymbolTable::removeTable();
        return nullptr;
    }
    for(int i = 0; i < blockItem->size(); i++) {
        (*blockItem)[i]->toKoopa();
        if(!bufferInsts.empty() && ((koopa_raw_value_data_t*)bufferInsts.back())->kind.tag == KOOPA_RVT_RETURN) {
            break;
        }
    }
    SymbolTable::removeTable();
    return nullptr;
}
void *StmtAST1::toKoopa() const {
    auto ty = createTypeKind(KOOPA_RTT_UNIT);
    auto raw = createValueData(KOOPA_RVT_RETURN, nullptr, ty, KOOPA_RSIK_VALUE);

    koopa_raw_value_data_t *value;
    if(exp == nullptr) {
        auto ty_zero = createTypeKind(KOOPA_RTT_INT32);
        auto zero = createValueData(KOOPA_RVT_INTEGER, nullptr, ty_zero, KOOPA_RSIK_VALUE);
        zero->kind.data.integer.value = 0;
        value = zero;
    }
    else {
        value = (koopa_raw_value_data_t *)exp->toKoopa();
    }
    addItemToSlice(value->used_by, raw);
    raw->kind.data.ret.value = value;

    bufferInsts.push_back(raw);
    return raw;
}
void *StmtAST2::toKoopa() const {
    auto ty = createTypeKind(KOOPA_RTT_UNIT);
    auto raw = createValueData(KOOPA_RVT_STORE, nullptr, ty, KOOPA_RSIK_VALUE);

    auto dest = (koopa_raw_value_data_t  *)getLVal(lVal.get());
    raw->kind.data.store.dest = dest;
    addItemToSlice(dest->used_by, raw);
    auto value = (koopa_raw_value_data_t *)exp->toKoopa();
    raw->kind.data.store.value = value;
    addItemToSlice(value->used_by, raw);

    bufferInsts.push_back(raw);
    return raw;
}
void *StmtAST3::toKoopa() const {
    if(exp == nullptr)
        return nullptr;
    else
        return exp->toKoopa();
}
void *StmtAST4::toKoopa() const {
    return block->toKoopa();
}
void *IfAST1::toKoopa() const {
    auto ty = createTypeKind(KOOPA_RTT_UNIT);
    auto raw = createValueData(KOOPA_RVT_BRANCH, nullptr, ty, KOOPA_RSIK_VALUE);

    auto cond = (koopa_raw_value_data_t *)exp->toKoopa();
    addItemToSlice(cond->used_by, raw);
    bufferInsts.push_back(raw);
    auto ptr = (koopa_raw_basic_block_data_t *)bufferBlocks.back();
    addItemToSlice(ptr->insts, bufferInsts);
    bufferInsts.clear();

    raw->kind.data.branch.cond = cond;

    auto true_bb = createBasicBlockData("%then", KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE);
    addItemToSlice(true_bb->used_by, raw);
    auto false_bb = createBasicBlockData("%end", KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE,KOOPA_RSIK_VALUE);
    addItemToSlice(false_bb->used_by, raw);
    
    bufferBlocks.push_back(true_bb);
    stmtThen->toKoopa();
    raw->kind.data.branch.true_bb = true_bb;
    raw->kind.data.branch.true_args = {
        nullptr,
        0,
        KOOPA_RSIK_VALUE
    };
    auto last = bufferInsts.empty() ? 255 : ((koopa_raw_value_data_t*)bufferInsts.back())->kind.tag;
    if(last != KOOPA_RVT_RETURN && last != KOOPA_RVT_BRANCH) {
        auto rawjmp = createValueData(KOOPA_RVT_JUMP, nullptr, ty, KOOPA_RSIK_VALUE);
        rawjmp->kind.data.jump.target = false_bb;
        rawjmp->kind.data.jump.args = {
            nullptr,
            0,
            KOOPA_RSIK_VALUE
        };
        addItemToSlice(false_bb->used_by, rawjmp);
        bufferInsts.push_back(rawjmp);
    }
    auto ptr1 = (koopa_raw_basic_block_data_t *)bufferBlocks.back();
    addItemToSlice(ptr1->insts, bufferInsts);
    bufferInsts.clear();

    bufferBlocks.push_back(false_bb);
    raw->kind.data.branch.false_bb = false_bb;
    raw->kind.data.branch.false_args = {
        nullptr,
        0,
        KOOPA_RSIK_VALUE
    };

    return raw;
}
void *IfAST2::toKoopa() const {
    auto ty = createTypeKind(KOOPA_RTT_UNIT);
    auto raw = createValueData(KOOPA_RVT_BRANCH, nullptr, ty, KOOPA_RSIK_VALUE);

    auto cond = (koopa_raw_value_data_t *)exp->toKoopa();
    addItemToSlice(cond->used_by, raw);
    bufferInsts.push_back(raw);
    auto ptr = (koopa_raw_basic_block_data_t *)bufferBlocks.back();
    addItemToSlice(ptr->insts, bufferInsts);
    bufferInsts.clear();

    raw->kind.data.branch.cond = cond;
    auto true_bb = createBasicBlockData("%then", KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE);
    addItemToSlice(true_bb->used_by, raw);
    auto false_bb = createBasicBlockData("%else", KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE,KOOPA_RSIK_VALUE);
    addItemToSlice(false_bb->used_by, raw);
    auto end_bb = createBasicBlockData("%end", KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE,KOOPA_RSIK_VALUE);

    bufferBlocks.push_back(true_bb);
    stmtThen->toKoopa();
    raw->kind.data.branch.true_bb = true_bb;
    raw->kind.data.branch.true_args = {
        nullptr,
        0,
        KOOPA_RSIK_VALUE
    };
    auto last1 = bufferInsts.empty() ? 255 : ((koopa_raw_value_data_t*)bufferInsts.back())->kind.tag;
    if(last1 != KOOPA_RVT_RETURN && last1 != KOOPA_RVT_BRANCH) {
        auto rawjmp1 = createValueData(KOOPA_RVT_JUMP, nullptr, ty, KOOPA_RSIK_VALUE);
        rawjmp1->kind.data.jump.target = end_bb;
        rawjmp1->kind.data.jump.args = {
            nullptr,
            0,
            KOOPA_RSIK_VALUE
        };
        addItemToSlice(end_bb->used_by, rawjmp1);
        bufferInsts.push_back(rawjmp1);
    }
    auto ptr1 = (koopa_raw_basic_block_data_t *)bufferBlocks.back();
    addItemToSlice(ptr1->insts, bufferInsts);
    bufferInsts.clear();

    bufferBlocks.push_back(false_bb);
    stmtElse->toKoopa();
    raw->kind.data.branch.false_bb = false_bb;
    raw->kind.data.branch.false_args = {
        nullptr,
        0,
        KOOPA_RSIK_VALUE
    };
    auto last2 = bufferInsts.empty() ? 255 : ((koopa_raw_value_data_t*)bufferInsts.back())->kind.tag;
    if(last2 != KOOPA_RVT_RETURN && last2 != KOOPA_RVT_BRANCH) {
        auto rawjmp2 = createValueData(KOOPA_RVT_JUMP, nullptr, ty, KOOPA_RSIK_VALUE);
        rawjmp2->kind.data.jump.target = end_bb;
        rawjmp2->kind.data.jump.args = {
            nullptr,
            0,
            KOOPA_RSIK_VALUE
        };
        bufferInsts.push_back(rawjmp2);
        addItemToSlice(end_bb->used_by, rawjmp2);
    }
    auto ptr2 = (koopa_raw_basic_block_data_t *)bufferBlocks.back();
    addItemToSlice(ptr2->insts, bufferInsts);
    bufferInsts.clear();

    bufferBlocks.push_back(end_bb);
    return raw;
}
void *PrimaryExpAST::toKoopa() const {
    auto ty = createTypeKind(KOOPA_RTT_INT32);
    auto raw = createValueData(KOOPA_RVT_INTEGER, nullptr, ty, KOOPA_RSIK_VALUE);

    raw->kind.data.integer.value = num;

    return raw;
}
void *UnaryExpAST::toKoopa() const {
    if(unaryOp == "+")
        return unaryExp->toKoopa();

    auto rhs = (koopa_raw_value_data_t *)unaryExp->toKoopa();

    auto lhs = createValueData(KOOPA_RVT_INTEGER, nullptr, rhs->ty, KOOPA_RSIK_VALUE);
    lhs->kind.data.integer.value = 0;

    auto raw = createValueData(KOOPA_RVT_BINARY, nullptr, rhs->ty, KOOPA_RSIK_VALUE);
    raw->kind.data.binary.rhs = rhs;
    raw->kind.data.binary.lhs = lhs;
    raw->kind.data.binary.op = op_map[unaryOp];

    addItemToSlice(rhs->used_by, raw);
    addItemToSlice(lhs->used_by, raw);

    bufferInsts.push_back(raw);

    return raw;
}
void *MulExpAST::toKoopa() const {
    auto lhs = (koopa_raw_value_data_t *)mulExp->toKoopa();
    auto rhs = (koopa_raw_value_data_t *)unaryExp->toKoopa();

    auto raw = createValueData(KOOPA_RVT_BINARY, nullptr, lhs->ty, KOOPA_RSIK_VALUE);
    raw->kind.data.binary.lhs = lhs;
    raw->kind.data.binary.rhs = rhs;
    raw->kind.data.binary.op = op_map[mulOp];

    addItemToSlice(lhs->used_by, raw);
    addItemToSlice(rhs->used_by, raw);

    bufferInsts.push_back(raw);

    return raw;
}
void *AddExpAST::toKoopa() const {
    auto lhs = (koopa_raw_value_data_t *)addExp->toKoopa();
    auto rhs = (koopa_raw_value_data_t *)mulExp->toKoopa();

    auto raw = createValueData(KOOPA_RVT_BINARY, nullptr, lhs->ty, KOOPA_RSIK_VALUE);
    raw->kind.data.binary.lhs = lhs;
    raw->kind.data.binary.rhs = rhs;
    raw->kind.data.binary.op = op_map[addOp];
    
    addItemToSlice(lhs->used_by, raw);
    addItemToSlice(rhs->used_by, raw);

    bufferInsts.push_back(raw);

    return raw;
}
void *RelExpAST::toKoopa() const {
    auto lhs = (koopa_raw_value_data_t *)relExp->toKoopa();
    auto rhs = (koopa_raw_value_data_t *)addExp->toKoopa();

    auto raw = createValueData(KOOPA_RVT_BINARY, nullptr, lhs->ty, KOOPA_RSIK_VALUE);
    raw->kind.data.binary.lhs = lhs;
    raw->kind.data.binary.rhs = rhs;
    raw->kind.data.binary.op = op_map[relOp];

    addItemToSlice(rhs->used_by, raw);
    addItemToSlice(lhs->used_by, raw);

    bufferInsts.push_back(raw);

    return raw;
}
void *EqExpAST::toKoopa() const {
    auto lhs = (koopa_raw_value_data_t *)eqExp->toKoopa();
    auto rhs = (koopa_raw_value_data_t *)relExp->toKoopa();

    auto raw = createValueData(KOOPA_RVT_BINARY, nullptr, lhs->ty, KOOPA_RSIK_VALUE);
    raw->kind.data.binary.lhs = lhs;
    raw->kind.data.binary.rhs = rhs;
    raw->kind.data.binary.op = op_map[eqOp];

    addItemToSlice(lhs->used_by, raw);
    addItemToSlice(rhs->used_by, raw);

    bufferInsts.push_back(raw);

    return raw;
}
void *LAndExpAST::toKoopa() const {
    auto rhs1 = (koopa_raw_value_data_t *)lAndExp->toKoopa();

    auto lhs1= createValueData(KOOPA_RVT_INTEGER, nullptr, rhs1->ty, KOOPA_RSIK_VALUE);
    lhs1->kind.data.integer.value = 0;
    
    auto raw1 = createValueData(KOOPA_RVT_BINARY, nullptr, rhs1->ty, KOOPA_RSIK_VALUE);
    raw1->kind.data.binary.rhs = rhs1;
    raw1->kind.data.binary.lhs = lhs1;
    raw1->kind.data.binary.op = op_map["!="];
    addItemToSlice(rhs1->used_by, raw1);
    addItemToSlice(lhs1->used_by, raw1);

    bufferInsts.push_back(raw1);

    auto rhs2 = (koopa_raw_value_data_t *)lAndExp->toKoopa();

    auto lhs2= createValueData(KOOPA_RVT_INTEGER, nullptr, rhs2->ty, KOOPA_RSIK_VALUE);
    lhs2->kind.data.integer.value = 0;
    
    auto raw2 = createValueData(KOOPA_RVT_BINARY, nullptr, rhs2->ty, KOOPA_RSIK_VALUE);
    raw2->kind.data.binary.rhs = rhs2;
    raw2->kind.data.binary.lhs = lhs2;
    raw2->kind.data.binary.op = op_map["!="];
    addItemToSlice(rhs2->used_by, raw2);
    addItemToSlice(lhs2->used_by, raw2);

    bufferInsts.push_back(raw2);

    auto raw = createValueData(KOOPA_RVT_BINARY, nullptr, raw1->ty, KOOPA_RSIK_VALUE);
    raw->kind.data.binary.lhs = raw1;
    raw->kind.data.binary.rhs = raw2;
    raw->kind.data.binary.op = op_map["&&"];

    addItemToSlice(raw1->used_by, raw);
    addItemToSlice(raw2->used_by, raw);

    bufferInsts.push_back(raw);

    return raw;
}
void *LOrExpAST::toKoopa() const {
    auto lhs0 = (koopa_raw_value_data_t *)lOrExp->toKoopa();
    auto rhs0 = (koopa_raw_value_data_t *)lAndExp->toKoopa();

    auto raw0 = createValueData(KOOPA_RVT_BINARY, nullptr, lhs0->ty, KOOPA_RSIK_VALUE);
    raw0->kind.data.binary.lhs = lhs0;
    raw0->kind.data.binary.rhs = rhs0;
    raw0->kind.data.binary.op = op_map["||"];

    addItemToSlice(lhs0->used_by, raw0);
    addItemToSlice(rhs0->used_by, raw0);

    bufferInsts.push_back(raw0);

    auto lhs= createValueData(KOOPA_RVT_INTEGER, nullptr, raw0->ty, KOOPA_RSIK_VALUE);
    lhs->kind.data.integer.value = 0;

    auto raw = createValueData(KOOPA_RVT_BINARY, nullptr, raw0->ty, KOOPA_RSIK_VALUE);
    raw->kind.data.binary.lhs = lhs;
    raw->kind.data.binary.rhs = raw0;
    raw->kind.data.binary.op = op_map["!="];

    addItemToSlice(lhs->used_by, raw);
    addItemToSlice(raw0->used_by, raw);

    bufferInsts.push_back(raw);

    return raw;
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
void *VarDeclAST::toKoopa() const {
    auto ty = createTypeKind(KOOPA_RTT_POINTER);
    ty->data.pointer.base = (koopa_raw_type_t)bType->toKoopa();
    for(int i = 0; i < varDef->size(); i++) {
        auto ptr = (koopa_raw_value_data_t *)(*varDef)[i]->toKoopa();
        ptr->ty = ty;
    }
    return nullptr;
}
void *VarDefAST1::toKoopa() const {
    // ty暂时设成nullptr，之后在VarDecl中赋值
    auto raw = createValueData(KOOPA_RVT_ALLOC, ("@" + ident).c_str(), nullptr, KOOPA_RSIK_VALUE);

    bufferInsts.push_back(raw);

    SymbolTable::addItem(ident, raw);

    return raw;
}
void *VarDefAST2::toKoopa() const {
    // ty暂时设成nullptr，之后在VarDecl中赋值
    auto raw1 = createValueData(KOOPA_RVT_ALLOC, ("@" + ident).c_str(), nullptr, KOOPA_RSIK_VALUE);

    bufferInsts.push_back(raw1);

    SymbolTable::addItem(ident, raw1);

    auto ty = createTypeKind(KOOPA_RTT_UNIT);

    auto value = (koopa_raw_value_data_t *)initVal->toKoopa();

    auto raw2 = createValueData(KOOPA_RVT_STORE, nullptr, ty, KOOPA_RSIK_VALUE);
    raw2->kind.data.store.dest = raw1;
    raw2->kind.data.store.value = value;

    addItemToSlice(raw1->used_by, raw2);
    addItemToSlice(value->used_by, raw2);

    bufferInsts.push_back(raw2);

    return raw1;
    
}
void *LValAST::toKoopa() const {
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

        bufferInsts.push_back(raw);

        return raw;
    }
    return nullptr;
}
