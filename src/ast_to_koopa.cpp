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
    SymbolTable::addTable();
    auto raw = new koopa_raw_program_t;

    raw->values = createSlice(KOOPA_RSIK_VALUE);
    raw->funcs = createSlice(KOOPA_RSIK_FUNCTION);

    initLibFuncs();
    
    for(auto &i : *defs) {
        if(!dynamic_cast<FuncDefAST*>(i.get()))
            i->toKoopa();
        else
            continue;
    }
    for(auto &i : *defs) {
        if(dynamic_cast<FuncDefAST*>(i.get()))
            i->toKoopa();
        else
            continue;
    }
    addItemToSlice(raw->funcs, bufferFuncs);
    addItemToSlice(raw->values, bufferGlobalValues);
    SymbolTable::removeTable();
    return raw;
}
void *FuncDefAST::toKoopa() const {
    
    insideFunc = true;
    auto ty = createTypeKind(KOOPA_RTT_FUNCTION);
    ty->data.function.params = createSlice(KOOPA_RSIK_TYPE);
    ty->data.function.ret = (koopa_raw_type_t)func_type->toKoopa();

    auto rawfunc = createFuncData(("@" + ident).c_str(), ty, KOOPA_RSIK_VALUE, KOOPA_RSIK_BASIC_BLOCK);
    SymbolTable::addItem(ident, rawfunc);
    SymbolTable::addTable();
    if(fparams != nullptr) {
        vector<const void *> bufferParams;
        vector<const void *> bufferTypeParmas;
        for(int i = 0; i < fparams->size(); i++) {
            auto ptr = (koopa_raw_value_data_t *)(*fparams)[i]->toKoopa();
            auto ptr_ty = ptr->ty;
            ptr->kind.data.func_arg_ref.index = i;
            bufferParams.push_back(ptr);
            bufferTypeParmas.push_back(ptr_ty);

            auto ty_pointer = createTypeKind(KOOPA_RTT_POINTER);
            ty_pointer->data.pointer.base = ptr_ty;
            string name(&(ptr->name[1]));
            auto raw_alloc = createValueData(KOOPA_RVT_ALLOC, ("@" + name).c_str(), ty_pointer, KOOPA_RSIK_VALUE);
            auto raw_store = createValueData(KOOPA_RVT_STORE, nullptr, createTypeKind(KOOPA_RTT_UNIT), KOOPA_RSIK_VALUE);
            raw_store->kind.data.store.dest = raw_alloc;
            raw_store->kind.data.store.value = ptr;
            bufferInsts.push_back(raw_alloc);
            bufferInsts.push_back(raw_store);
            SymbolTable::addItem(name, raw_alloc);
        }
        addItemToSlice(rawfunc->params, bufferParams);
        addItemToSlice(ty->data.function.params, bufferTypeParmas);
    }

    auto rawentry = createBasicBlockData("%entry", KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE);
    bufferBlocks.push_back(rawentry);
    block->toKoopa();

    if(ty->data.function.ret->tag == KOOPA_RTT_INT32)
        checkBlock(0);
    else
        checkBlock();
    
    endBlock();

    addItemToSlice(rawfunc->bbs, bufferBlocks);
    bufferBlocks.clear();

    bufferFuncs.push_back(rawfunc);

    insideFunc = false;
    SymbolTable::removeTable();
    return rawfunc;
}
void *FuncFParamAST::toKoopa() const {
    auto ty = (koopa_raw_type_kind_t *)bType->toKoopa();
    auto raw = createValueData(KOOPA_RVT_FUNC_ARG_REF, ("%" + ident).c_str(), ty, KOOPA_RSIK_VALUE);
    return raw;
}
void *TypeAST::toKoopa() const {
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
        if(!bufferInsts.empty()) {
            auto tag = ((koopa_raw_value_data_t*)bufferInsts.back())->kind.tag;
            if(tag == KOOPA_RVT_RETURN || tag == KOOPA_RVT_JUMP)
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
        value = nullptr;
    }
    else {
        value = (koopa_raw_value_data_t *)exp->toKoopa();
    }
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
void *StmtAST5::toKoopa() const {
    if(stackLoop.empty())
        assert(false);
    
    auto raw = createValueData(KOOPA_RVT_JUMP, nullptr, createTypeKind(KOOPA_RTT_UNIT), KOOPA_RSIK_VALUE);
    raw->kind.data.jump.target = (koopa_raw_basic_block_data_t *)stackLoop.back();
    raw->kind.data.jump.args = createSlice(KOOPA_RSIK_VALUE);

    bufferInsts.push_back(raw);
    return raw;
}
void *StmtAST6::toKoopa() const {
    if(stackLoop.empty())
        assert(false);
    
    auto raw = createValueData(KOOPA_RVT_JUMP, nullptr, createTypeKind(KOOPA_RTT_UNIT), KOOPA_RSIK_VALUE);
    raw->kind.data.jump.target = (koopa_raw_basic_block_data_t *)stackLoop.front();
    raw->kind.data.jump.args = createSlice(KOOPA_RSIK_VALUE);

    bufferInsts.push_back(raw);
    return raw;
}
void *IfAST1::toKoopa() const {
    auto ty = createTypeKind(KOOPA_RTT_UNIT);
    auto raw = createValueData(KOOPA_RVT_BRANCH, nullptr, ty, KOOPA_RSIK_VALUE);

    auto cond = (koopa_raw_value_data_t *)exp->toKoopa();
    addItemToSlice(cond->used_by, raw);
    bufferInsts.push_back(raw);
    endBlock();

    raw->kind.data.branch.cond = cond;

    auto true_bb = createBasicBlockData("%then", KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE);
    addItemToSlice(true_bb->used_by, raw);
    auto false_bb = createBasicBlockData("%end", KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE,KOOPA_RSIK_VALUE);
    addItemToSlice(false_bb->used_by, raw);
    
    bufferBlocks.push_back(true_bb);
    stmtThen->toKoopa();
    raw->kind.data.branch.true_bb = true_bb;
    raw->kind.data.branch.true_args = createSlice(KOOPA_RSIK_VALUE);
    checkBlock(false_bb);
    endBlock();

    bufferBlocks.push_back(false_bb);
    raw->kind.data.branch.false_bb = false_bb;
    raw->kind.data.branch.false_args = createSlice(KOOPA_RSIK_VALUE);

    return raw;
}
void *IfAST2::toKoopa() const {
    auto ty = createTypeKind(KOOPA_RTT_UNIT);
    auto raw = createValueData(KOOPA_RVT_BRANCH, nullptr, ty, KOOPA_RSIK_VALUE);

    auto cond = (koopa_raw_value_data_t *)exp->toKoopa();
    addItemToSlice(cond->used_by, raw);
    bufferInsts.push_back(raw);
    endBlock();

    raw->kind.data.branch.cond = cond;
    auto true_bb = createBasicBlockData("%then", KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE);
    addItemToSlice(true_bb->used_by, raw);
    auto false_bb = createBasicBlockData("%else", KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE,KOOPA_RSIK_VALUE);
    addItemToSlice(false_bb->used_by, raw);
    auto end_bb = createBasicBlockData("%end", KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE,KOOPA_RSIK_VALUE);

    bufferBlocks.push_back(true_bb);
    stmtThen->toKoopa();
    raw->kind.data.branch.true_bb = true_bb;
    raw->kind.data.branch.true_args = createSlice(KOOPA_RSIK_VALUE);
    bool flag_then = checkBlock(end_bb);
    endBlock();

    bufferBlocks.push_back(false_bb);
    stmtElse->toKoopa();
    raw->kind.data.branch.false_bb = false_bb;
    raw->kind.data.branch.false_args = createSlice(KOOPA_RSIK_VALUE);
    bool flag_else = checkBlock(end_bb);

    if(flag_then || flag_else) {
        endBlock();
        bufferBlocks.push_back(end_bb);
    }
    return raw;
}
void *WhileAST::toKoopa() const {
    auto ty_unit = createTypeKind(KOOPA_RTT_UNIT);

    auto block_while_entry = createBasicBlockData("%while_entry", KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE);
    auto block_while_body = createBasicBlockData("%while_body", KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE);
    auto block_while_end = createBasicBlockData("%while_end", KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE);

    stackLoop.push_back(block_while_end);
    stackLoop.push_front(block_while_entry);

    auto raw_jump = createValueData(KOOPA_RVT_JUMP, nullptr, ty_unit, KOOPA_RSIK_VALUE);
    raw_jump->kind.data.jump.target = block_while_entry;
    raw_jump->kind.data.jump.args = createSlice(KOOPA_RSIK_VALUE);
    bufferInsts.push_back(raw_jump);

    endBlock();
    bufferBlocks.push_back(block_while_entry);

    auto raw_branch = createValueData(KOOPA_RVT_BRANCH, nullptr, ty_unit, KOOPA_RSIK_VALUE);
    raw_branch->kind.data.branch.cond = (koopa_raw_value_data_t *)exp->toKoopa();
    raw_branch->kind.data.branch.true_bb = block_while_body;
    raw_branch->kind.data.branch.true_args = createSlice(KOOPA_RSIK_VALUE);
    raw_branch->kind.data.branch.false_bb = block_while_end;
    raw_branch->kind.data.branch.false_args = createSlice(KOOPA_RSIK_VALUE);
    bufferInsts.push_back(raw_branch);

    endBlock();
    bufferBlocks.push_back(block_while_body);

    stmt->toKoopa();
    checkBlock(block_while_entry);

    endBlock();
    bufferBlocks.push_back(block_while_end);

    stackLoop.pop_back();
    stackLoop.pop_front();

    return nullptr;
}
void *PrimaryExpAST::toKoopa() const {
    return createIntegerValueData(num);
}
void *UnaryExpAST1::toKoopa() const {
    if(unaryOp == "+")
        return unaryExp->toKoopa();

    auto rhs = (koopa_raw_value_data_t *)unaryExp->toKoopa();
    auto lhs = createIntegerValueData(0);
    auto raw = createBinaryValueData(op_map[unaryOp], lhs, rhs);
    bufferInsts.push_back(raw);

    return raw;
}
void *UnaryExpAST2::toKoopa() const {
    auto callee = SymbolTable::getItem(ident, SYMBOLTABLE_ITEM_FUNC).data.f;
    auto raw = createValueData(KOOPA_RVT_CALL, nullptr, callee->ty->data.function.ret, KOOPA_RSIK_VALUE);
    raw->kind.data.call.callee = callee;
    raw->kind.data.call.args = createSlice(KOOPA_RSIK_VALUE);
    if(rparams != nullptr) {
        vector<void *> bufferParams;
        for(int i = 0; i < rparams->size(); i++) {
            bufferParams.push_back((*rparams)[i]->toKoopa());
        }
        addItemToSlice(raw->kind.data.call.args, bufferParams);
    }
    bufferInsts.push_back(raw);
    return raw;
}
void *MulExpAST::toKoopa() const {
    auto lhs = (koopa_raw_value_data_t *)mulExp->toKoopa();
    auto rhs = (koopa_raw_value_data_t *)unaryExp->toKoopa();
    auto raw = createBinaryValueData(op_map[mulOp], lhs, rhs);
    bufferInsts.push_back(raw);

    return raw;
}
void *AddExpAST::toKoopa() const {
    auto lhs = (koopa_raw_value_data_t *)addExp->toKoopa();
    auto rhs = (koopa_raw_value_data_t *)mulExp->toKoopa();
    auto raw = createBinaryValueData(op_map[addOp], lhs, rhs);
    bufferInsts.push_back(raw);

    return raw;
}
void *RelExpAST::toKoopa() const {
    auto lhs = (koopa_raw_value_data_t *)relExp->toKoopa();
    auto rhs = (koopa_raw_value_data_t *)addExp->toKoopa();
    auto raw = createBinaryValueData(op_map[relOp], lhs, rhs);
    bufferInsts.push_back(raw);

    return raw;
}
void *EqExpAST::toKoopa() const {
    auto lhs = (koopa_raw_value_data_t *)eqExp->toKoopa();
    auto rhs = (koopa_raw_value_data_t *)relExp->toKoopa();
    auto raw = createBinaryValueData(op_map[eqOp], lhs, rhs);
    bufferInsts.push_back(raw);

    return raw;
}
void *LAndExpAST::toKoopa() const {
    auto ty_unit = createTypeKind(KOOPA_RTT_UNIT);
    auto ty_int32 = createTypeKind(KOOPA_RTT_INT32);
    auto raw_branch = createValueData(KOOPA_RVT_BRANCH, nullptr, ty_unit, KOOPA_RSIK_VALUE);

    auto cond = (koopa_raw_value_data_t *)lAndExp->toKoopa();
    raw_branch->kind.data.branch.cond = cond;
    auto block_no_short = createBasicBlockData("%no_short", KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE);
    addItemToSlice(block_no_short->used_by, raw_branch);
    auto block_short = createBasicBlockData("%short", KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE);
    addItemToSlice(block_short->used_by, raw_branch);
    auto param_short = createValueData(KOOPA_RVT_BLOCK_ARG_REF, "%result", ty_int32, KOOPA_RSIK_VALUE);
    addItemToSlice(block_short->params, param_short);


    bufferInsts.push_back(raw_branch);
    endBlock();

    // start block no short
    bufferBlocks.push_back(block_no_short);
    auto raw_rhs = (koopa_raw_value_data_t *)eqExp->toKoopa();

    auto raw_binary = createBinaryValueData(op_map["!="], createIntegerValueData(0), raw_rhs);
    bufferInsts.push_back(raw_binary);

    auto raw_jump = createValueData(KOOPA_RVT_JUMP, nullptr, ty_unit, KOOPA_RSIK_VALUE);
    raw_jump->kind.data.jump.target = block_short;
    raw_jump->kind.data.jump.args = createSlice(KOOPA_RSIK_VALUE);
    addItemToSlice(raw_jump->kind.data.jump.args, raw_binary);
    bufferInsts.push_back(raw_jump);
    endBlock();

    raw_branch->kind.data.branch.true_bb = block_no_short;
    raw_branch->kind.data.branch.true_args = createSlice(KOOPA_RSIK_VALUE);
    // end block no short

    //start block short
    bufferBlocks.push_back(block_short);
    raw_branch->kind.data.branch.false_bb = block_short;
    raw_branch->kind.data.branch.false_args = createSlice(KOOPA_RSIK_VALUE);
    addItemToSlice(raw_branch->kind.data.branch.false_args, createIntegerValueData(0)); 
    return param_short;
}
void *LOrExpAST::toKoopa() const {
    auto ty_unit = createTypeKind(KOOPA_RTT_UNIT);
    auto ty_int32 = createTypeKind(KOOPA_RTT_INT32);
    auto raw_branch = createValueData(KOOPA_RVT_BRANCH, nullptr, ty_unit, KOOPA_RSIK_VALUE);

    auto cond = (koopa_raw_value_data_t *)lOrExp->toKoopa();
    raw_branch->kind.data.branch.cond = cond;
    auto block_no_short = createBasicBlockData("%no_short", KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE);
    addItemToSlice(block_no_short->used_by, raw_branch);
    auto block_short = createBasicBlockData("%short", KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE, KOOPA_RSIK_VALUE);
    addItemToSlice(block_short->used_by, raw_branch);
    auto param_short = createValueData(KOOPA_RVT_BLOCK_ARG_REF, "%result", ty_int32, KOOPA_RSIK_VALUE);
    addItemToSlice(block_short->params, param_short);


    bufferInsts.push_back(raw_branch);
    endBlock();

    // start block no short
    bufferBlocks.push_back(block_no_short);
    auto raw_rhs = (koopa_raw_value_data_t *)lAndExp->toKoopa();

    auto raw_binary = createBinaryValueData(op_map["!="], createIntegerValueData(0), raw_rhs);
    bufferInsts.push_back(raw_binary);

    auto raw_jump = createValueData(KOOPA_RVT_JUMP, nullptr, ty_unit, KOOPA_RSIK_VALUE);
    raw_jump->kind.data.jump.target = block_short;
    raw_jump->kind.data.jump.args = createSlice(KOOPA_RSIK_VALUE);
    addItemToSlice(raw_jump->kind.data.jump.args, raw_binary);
    bufferInsts.push_back(raw_jump);
    endBlock();

    raw_branch->kind.data.branch.false_bb = block_no_short;
    raw_branch->kind.data.branch.false_args = createSlice(KOOPA_RSIK_VALUE);
    // end block no short

    //start block short
    bufferBlocks.push_back(block_short);
    raw_branch->kind.data.branch.true_bb = block_short;
    raw_branch->kind.data.branch.true_args = createSlice(KOOPA_RSIK_VALUE);
    addItemToSlice(raw_branch->kind.data.branch.true_args, createIntegerValueData(1)); 
    return param_short;
}
void *ConstDeclAST::toKoopa() const {
    for(int i = 0; i < constDef->size(); i++) {
        (*constDef)[i]->toKoopa();
    }
    return nullptr;
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
    if(insideFunc) {
        // ty暂时设成nullptr，之后在VarDecl中赋值
        auto raw = createValueData(KOOPA_RVT_ALLOC, ("@" + ident).c_str(), nullptr, KOOPA_RSIK_VALUE);
        bufferInsts.push_back(raw);
        SymbolTable::addItem(ident, raw);
        return raw;
    }
    else {
        auto ty_pint = createTypeKind(KOOPA_RTT_POINTER);
        auto ty_int = createTypeKind(KOOPA_RTT_INT32);
        ty_pint->data.pointer.base = ty_int;
        auto raw = createValueData(KOOPA_RVT_GLOBAL_ALLOC, ("@" + ident).c_str(), ty_pint, KOOPA_RSIK_VALUE);
        auto zeroinit = createValueData(KOOPA_RVT_ZERO_INIT, nullptr, ty_int, KOOPA_RSIK_VALUE);
        raw->kind.data.global_alloc.init = zeroinit;
        bufferGlobalValues.push_back(raw);
        SymbolTable::addItem(ident, raw);
        return raw;
    }
}
void *VarDefAST2::toKoopa() const {
    if(insideFunc) {
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
    else {
        auto ty_pint = createTypeKind(KOOPA_RTT_POINTER);
        auto ty_int = createTypeKind(KOOPA_RTT_INT32);
        ty_pint->data.pointer.base = ty_int;
        auto raw = createValueData(KOOPA_RVT_GLOBAL_ALLOC, ("@" + ident).c_str(), ty_pint, KOOPA_RSIK_VALUE);
        raw->kind.data.global_alloc.init = createIntegerValueData(initVal->calculateExp());
        bufferGlobalValues.push_back(raw);
        SymbolTable::addItem(ident, raw);
        return raw;
    }
}
void *LValAST::toKoopa() const {
    //作为右值引用一个符号（如果是变量，必须先Load）
    auto i = SymbolTable::getItem(ident);
    auto ty = createTypeKind(KOOPA_RTT_INT32);

    if(i.type == SYMBOLTABLE_ITEM_CONST) {
        return createIntegerValueData(i.data.c);
    }
    else if (i.type == SYMBOLTABLE_ITEM_VAR) {
        auto src = i.data.v;

        auto raw = createValueData(KOOPA_RVT_LOAD, nullptr, ty, KOOPA_RSIK_VALUE);
        raw->kind.data.load.src = src;

        addItemToSlice(src->used_by, raw);

        bufferInsts.push_back(raw);

        return raw;
    }
    else {
        cerr << "WTF How did u get here" << endl;
        exit(1);
    }
    return nullptr;
}
