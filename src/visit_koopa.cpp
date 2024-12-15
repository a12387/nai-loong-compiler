#include <cassert>
#include "visit_koopa.hpp"
#include <cstring>

using namespace std;

static bool isZero = false;
static int reg = 0;
static unordered_map<void *, string> registers;
static set<koopa_raw_value_t> spill;
static string current_func;
void visit(ofstream &out, const koopa_raw_program_t raw) {
    visit(out, raw.values);
    visit(out, raw.funcs);
}

void visit(ofstream &out, const koopa_raw_slice_t &slice) {
    for(int i = 0; i < slice.len; i++) {
        auto ptr = slice.buffer[i];
        switch(slice.kind) {
        case KOOPA_RSIK_FUNCTION:
            visit(out, reinterpret_cast<koopa_raw_function_t>(ptr));
            break;
        case KOOPA_RSIK_VALUE:
            visit(out, reinterpret_cast<koopa_raw_value_t>(ptr));
            break;
        default:
            assert(false);
        }
    }
}

void visit(ofstream &out, const koopa_raw_slice_t &slice, StackFrame &stackFrame) {
    for(int i = 0; i < slice.len; i++) {
        auto ptr = slice.buffer[i];
        switch(slice.kind) {
        case KOOPA_RSIK_FUNCTION:
            visit(out, reinterpret_cast<koopa_raw_function_t>(ptr));
            break;
        case KOOPA_RSIK_VALUE:
            visit(out, reinterpret_cast<koopa_raw_value_t>(ptr), stackFrame);
            break;
        case KOOPA_RSIK_BASIC_BLOCK:
            visit(out, reinterpret_cast<koopa_raw_basic_block_t>(ptr), stackFrame);
            break;
        default:
            assert(false);
        }
    }
}

void visit(ofstream &out, const koopa_raw_value_t &value) {
    out << "    .data\n";
    out << "    .globl " << &(value->name[1]) << endl;
    out << &(value->name[1]) << ":\n";

    auto v = value->kind.data.global_alloc.init;
    if(v->kind.tag == KOOPA_RVT_ZERO_INIT) {
        out << "    .zero 4\n"; 
    }
    else {
        out << "    .word " << v->kind.data.integer.value << endl;
    }
}

void visit(ofstream &out, const koopa_raw_function_t &func) {
    if(func->bbs.len == 0)
        return;
    current_func = &(func->name[1]);
    out << "    .text\n";
    out << "    .globl " << current_func << "\n";
    out << current_func << ":\n";
    
    StackFrame stackFrame = StackFrame();
    preprocess(func->bbs, stackFrame);
    getStackLength(func, stackFrame);
    

    prologue(out, stackFrame);
    visit(out, func->params, stackFrame);
    visit(out, func->bbs, stackFrame);
    out << endl;
}

void visit(ofstream &out, const koopa_raw_basic_block_t &bb, StackFrame &stackFrame) {
    if(strcmp(bb->name, "%entry") != 0 ) {
        out << current_func << "_" << &(bb->name[1]) << ":\n"; 
    }
    visit(out, bb->params, stackFrame);
    visit(out, bb->insts, stackFrame);
}

void visit(ofstream &out, const koopa_raw_value_t &value, StackFrame &stackFrame) {
    const auto &kind = value->kind;
    void *ptr = (void *)&kind;
    switch(kind.tag) {
    case KOOPA_RVT_RETURN: // unit
        visit(out, kind.data.ret, stackFrame);
        break;
    case KOOPA_RVT_INTEGER:// int32(好像这个函数不会访问到这里？)
        visit(out, kind.data.integer);
        break;
    case KOOPA_RVT_BINARY:// int32
        visit(out, kind.data.binary, stackFrame);
        if(value->used_by.len > 0)
            registers[ptr] = "t" + to_string(reg - 1);
        else 
            reg--;
        break;
    case KOOPA_RVT_ALLOC:// pointer int32
        if(value->ty->data.pointer.base->tag == KOOPA_RTT_INT32) {
            stackFrame.add(ptr, 4);
            break;
        }
        else
            assert(false);
    case KOOPA_RVT_LOAD:// int32
        visit(out, kind.data.load, stackFrame);
        registers[ptr] = "t" + to_string(reg - 1);
        break;
    case KOOPA_RVT_STORE:// unit
        visit(out, kind.data.store, stackFrame);
        break;
    case KOOPA_RVT_BLOCK_ARG_REF:
        if(kind.data.block_arg_ref.index >= 8)
            stackFrame.add(ptr, 4);
        else {
            registers[ptr] = "a" + to_string(kind.data.block_arg_ref.index);
            if(spill.find(value) != spill.end()) {
                stackFrame.add(ptr, 4);
                out << "    sw a" << kind.data.block_arg_ref.index << ", " << stackFrame.find(ptr) << "(sp)\n";
                registers.erase(ptr);
            }

        }

        break;
    case KOOPA_RVT_FUNC_ARG_REF:
        if(kind.data.func_arg_ref.index >= 8) {
            stackFrame.addToLastFrame(ptr, 4);
        }
        else
            registers[ptr] = "a" + to_string(kind.data.func_arg_ref.index);
        break;
    case KOOPA_RVT_BRANCH:
        visit(out, kind.data.branch, stackFrame);
        break;
    case KOOPA_RVT_JUMP:
        visit(out, kind.data.jump, stackFrame);
        break;
    case KOOPA_RVT_CALL:
        visit(out, kind.data.call, stackFrame);
        if(value->ty->tag == KOOPA_RTT_INT32) {
            out << "    mv t" << reg++ << ", a0\n";
            registers[ptr] = "t" + to_string(reg - 1);
        }
        break;
    default:
        assert(false);
    }

    if(value->ty->tag == KOOPA_RTT_INT32 && kind.tag != KOOPA_RVT_BLOCK_ARG_REF && kind.tag != KOOPA_RVT_FUNC_ARG_REF) {
        if(spill.find(value) != spill.end()) {
            stackFrame.add(ptr, 4);
            out << "    sw t" << reg - 1<< ", " << stackFrame.find(ptr) << "(sp)\n";
            reg--;
            registers.erase(ptr);
        }
    }
}

void visit(ofstream &out, const koopa_raw_return_t &ret, StackFrame &stackFrame) {
    if(ret.value != nullptr) {
        string regv = getReg(out, ret.value, stackFrame);
        out << "    mv a0, " << regv << endl;
    }
    epilogue(out, stackFrame);
    out << "    ret\n";
}

void visit(ofstream &out, const koopa_raw_integer_t &integer) {
    if(integer.value == 0) {
        isZero = true;
    }
    else {
        out << "    li t" << reg++ << ", " << integer.value << "\n";
    }
}

void visit(ofstream &out, const koopa_raw_binary_t &binary, StackFrame &stackFrame) {
    
    auto lhs = binary.lhs;
    auto rhs = binary.rhs;

    int shrink = 2;
    string regl = getReg(out, lhs, stackFrame, shrink);
    string regr = getReg(out, rhs, stackFrame, shrink);
    reg -= shrink;
    string reg_dest = "t" + to_string(reg++);
    
    switch(binary.op) {
    case KOOPA_RBO_ADD:
        out << "    add " << reg_dest << ", " << regl << ", " << regr << "\n";
        break;
    case KOOPA_RBO_SUB:
        out << "    sub " << reg_dest << ", " << regl << ", " << regr << "\n";
        break;
    case KOOPA_RBO_MUL:
        out << "    mul " << reg_dest << ", " << regl << ", " << regr << "\n";
        break;
    case KOOPA_RBO_DIV:
        out << "    div " << reg_dest << ", " << regl << ", " << regr << "\n";
        break;
    case KOOPA_RBO_MOD:
        out << "    rem " << reg_dest << ", " << regl << ", " << regr << "\n";
        break;
    case KOOPA_RBO_LT:
        out << "    slt " << reg_dest << ", " << regl << ", " << regr << "\n";
        break;
    case KOOPA_RBO_GT:
        out << "    sgt " << reg_dest << ", " << regl << ", " << regr << "\n";
        break;
    case KOOPA_RBO_LE:
        out << "    sgt " << reg_dest << ", " << regl << ", " << regr << "\n";
        out << "    xori "<< reg_dest << ", " << reg_dest << ", " << 1 << "\n";
        break;
    case KOOPA_RBO_GE:
        out << "    slt " << reg_dest << ", " << regl << ", " << regr << "\n";
        out << "    xori "<< reg_dest << ", " << reg_dest << ", " << 1 << "\n";
        break;
    case KOOPA_RBO_EQ:
        out << "    xor " << reg_dest << ", " << regl << ", " << regr << "\n";
        out << "    seqz "<< reg_dest << ", " << reg_dest << "\n";
        break;
    case KOOPA_RBO_NOT_EQ:
        out << "    xor " << reg_dest << ", " << regl << ", " << regr << "\n";
        out << "    snez "<< reg_dest << ", " << reg_dest << "\n";
        break;
    case KOOPA_RBO_AND:
        out << "    and " << reg_dest << ", " << regl << ", " << regr << "\n";
        break;
    case KOOPA_RBO_OR:
        out << "    or "  << reg_dest << ", " << regl << ", " << regr << "\n";
        break;
    }
}

void visit(ofstream &out, const koopa_raw_load_t &load, StackFrame &stackFrame) {
    void *ptr = (void *)&load.src->kind;
    if(load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        out << "    la t" << reg << ", " << &(load.src->name[1]) << endl;
        out << "    lw t" << reg << ", 0(t" << reg++ << ")\n"; 
    }
    else
        out << "    lw t" << reg++ << ", " << stackFrame.find(ptr) << "(sp)\n";
}

void visit(ofstream &out, const koopa_raw_store_t &store, StackFrame &stackFrame) {
    auto dest = store.dest;
    auto value = store.value;

    string regv = getReg(out, value, stackFrame);
    if(dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        out << "    la t" << reg + 1 << ", " << &(dest->name[1]) << endl;
        out << "    sw " << regv << ", 0(t" << reg + 1 << ")\n"; 
    }
    else
        out << "    sw " << regv << ", " << stackFrame.find((void *)&dest->kind) << "(sp)\n";
}

void visit(ofstream &out, const koopa_raw_branch_t &branch, StackFrame &stackFrame) {
    auto cond = branch.cond;
    auto true_bb = branch.true_bb;
    auto true_args = branch.true_args;
    auto false_bb = branch.false_bb;
    auto false_args = branch.false_args;

    string regc = getReg(out, cond, stackFrame);
    if(regc[0] == 'a') {
        out << "    mv t" << reg << ", " << regc << endl;
        regc = "t" + to_string(reg);
    }
    visitParams(out, true_args, stackFrame);
    out << "    bnez " << regc << ", " << current_func << "_" << &(true_bb->name[1]) << endl;
    visitParams(out, false_args, stackFrame);
    out << "    j " << current_func << "_" << &(false_bb->name[1]) << endl;
}

void visit(ofstream &out, const koopa_raw_jump_t &jump, StackFrame &stackFrame) {
    auto target = jump.target;
    auto args = jump.args;

    visitParams(out, args, stackFrame);
    out << "    j " << current_func << "_" << &(target->name[1]) << endl;
}

void visit(ofstream &out, const koopa_raw_call_t &call, StackFrame &stackFrame) {
    auto callee = call.callee;
    auto args = call.args;

    visitParams(out, args, stackFrame);
    // for(auto iter = registers.begin(); iter != registers.end(); iter++) {
    //     if(iter->second[0] == 't') {
    //         out << "    mv s" << iter->second[1] << ", " << iter->second << endl;
    //     }
    // }
    out << "    call " << &(callee->name[1]) << endl;
    // for(auto iter = registers.begin(); iter != registers.end(); iter++) {
    //     if(iter->second[0] == 't') {
    //         out << "    mv " << iter->second << ", s" << iter->second[1] << endl;
    //     }
    // }
}

void getStackLength(const koopa_raw_function_t &func, StackFrame &stackFrame) {
    bool called = false;
    int ret = 0;
    unsigned int max_args = 0; // to solve error of max(int, uint32_t)
    for(int i = 0; i < func->bbs.len; i++) {
        auto ptr_bb = (koopa_raw_basic_block_data_t *)func->bbs.buffer[i];
        for(int j = 0; j < ptr_bb->insts.len; j++) {
            auto value = (koopa_raw_value_data_t *)ptr_bb->insts.buffer[j];
            if(value->kind.tag == KOOPA_RVT_CALL) {
                called = true;
                if(value->kind.data.call.args.len > 8) {
                    max_args = max(max_args, value->kind.data.call.args.len - 8);
                }
            }
            switch(value->ty->tag) {
            case KOOPA_RTT_POINTER:
                if(value->ty->data.pointer.base->tag == KOOPA_RTT_INT32) {
                    ret += 4;
                }
            case KOOPA_RTT_INT32:
            case KOOPA_RTT_UNIT:
            default:
                break;
            }
        }
    }
    ret += max_args * 4;
    if(called)
        ret += 4;
    
    stackFrame.saved_ra = called;
    stackFrame.paramsLength = max_args * 4;
    stackFrame.length += ret;
    stackFrame.align();
}

void prologue(ofstream &out, StackFrame &stackFrame) {
    if(stackFrame.length > 0) {
        for(int i = 0; i < stackFrame.length / 2048; i++) {
            out << "    addi sp, sp, " << -2048 << '\n';
        }
        out << "    addi sp, sp, " << -stackFrame.length % 2048 << '\n';
    }

    if(stackFrame.saved_ra) 
        out << "    sw ra, " << stackFrame.length - 4 << "(sp)\n";
    
}

void epilogue(ofstream &out, StackFrame &stackFrame) {
    if(stackFrame.saved_ra) 
        out << "    lw ra, " << stackFrame.length - 4 << "(sp)\n";
    
    if(stackFrame.length > 0) {
        for(int i = 0; i < stackFrame.length / 2048; i++) {
            out << "    addi sp, sp, " << 2048 << '\n';
        }
        out << "    addi sp, sp, " << stackFrame.length % 2048 << '\n';
    }
}

string getReg(ofstream &out, const koopa_raw_value_t &value, StackFrame &stackFrame, int &shrink) {
    string regv;

    if(value->kind.tag == KOOPA_RVT_INTEGER) {
        visit(out, value->kind.data.integer);
        if(isZero) {
            isZero = false;
            shrink--;
            regv = "x0";
        }
        else {
            regv = "t" + to_string(reg - 1);
        }
    }
    else if (((
                value->kind.tag == KOOPA_RVT_BLOCK_ARG_REF 
                || value->kind.tag == KOOPA_RVT_FUNC_ARG_REF
            ) && value->kind.data.block_arg_ref.index >= 8
        ) || spill.find(value) != spill.end()){
        void *ptr = (void *)&value->kind;
        regv = "t" + to_string(reg++);
        out << "    lw " << regv << ", " << stackFrame.find(ptr) << "(sp)\n";
    }
    else {
        void *ptr = (void *)&value->kind;
        regv = registers[ptr];
        if(regv[0] != 'a')
            registers.erase(ptr);
        else
            shrink--;
    }

    return regv;
}

string getReg(ofstream &out, const koopa_raw_value_t &value, StackFrame &stackFrame) {
    int shrink = 1;
    string s = getReg(out, value, stackFrame, shrink);
    reg -= shrink;
    return s;
}

void visitParams(ofstream &out, const koopa_raw_slice_t &params, StackFrame &stackFrame) {
    if(params.kind != KOOPA_RSIK_VALUE)
        assert(false);
    for(int i = 0; i < params.len; i++) {
        auto param = (koopa_raw_value_data_t *)params.buffer[i];
        auto ptr = (void *)&param->kind;
        if(param->kind.tag != KOOPA_RVT_INTEGER) {
            auto r = registers.find(ptr);
            if(r != registers.end() && r->second[0] == 'a') {
                out << "    mv t" << reg++ << ", " << r->second << endl;
                registers[ptr] = "t" + to_string(reg - 1);
            }
        }
    }
    for(int i = 0; i < params.len; i++) {
        auto ptr = (koopa_raw_value_data_t *)params.buffer[i];
        if(i < 8) {
            switch(ptr->kind.tag) {
            case KOOPA_RVT_INTEGER:
                out << "    li a" << i << ", " << ptr->kind.data.integer.value << endl;
                break;
            default:
                string s = getReg(out, ptr, stackFrame);
                out << "    mv a" << i << ", " << s << endl;
                break;
            }
        }
        else {
            string s = getReg(out, ptr, stackFrame);
            out << "    sw " << s << ", " << (i - 8) * 4 << "(sp)\n";
        }
    }
}

void preprocess(const koopa_raw_slice_t &bbs, StackFrame &stackFrame) {
    int spill_bytes = 0;
    spill.clear();
    for(int i = 0; i < bbs.len; i++) {
        auto block = (koopa_raw_basic_block_data_t *)bbs.buffer[i];
        unordered_map<koopa_raw_value_t, int> start, end;
        unordered_map<int, set<koopa_raw_value_t> > use;
        unordered_map<int, koopa_raw_value_t> def;
        unordered_map<int, set<koopa_raw_value_t> > extra_def;
        set<koopa_raw_value_t> def_but_not_used;
        for(int j = 0; j < block->params.len; j++) {
            auto param = (koopa_raw_value_data_t *)block->params.buffer[j];
            def_but_not_used.insert(param);
        }
        for(int j = 0; j < block->insts.len; j++) {
            auto inst = (koopa_raw_value_data_t *)block->insts.buffer[j];
            switch(inst->kind.tag) {
            case KOOPA_RVT_RETURN: 
                {
                    auto v = inst->kind.data.ret.value;
                    if(v != nullptr) {
                        end[v] = j;
                        use[j].insert(v);
                        if(v->kind.tag == KOOPA_RVT_INTEGER && v->kind.data.integer.value != 0)
                            extra_def[j].insert(v);
                        else 
                            def_but_not_used.erase(v);
                    }
                }
                break;
            case KOOPA_RVT_BINARY:
                {
                    auto l = inst->kind.data.binary.lhs;
                    auto r = inst->kind.data.binary.rhs;
                    end[l] = j;
                    end[r] = j;
                    use[j].insert(l);
                    use[j].insert(r);
                    if(l->kind.tag == KOOPA_RVT_INTEGER && l->kind.data.integer.value != 0)
                        extra_def[j].insert(l);
                    else
                        def_but_not_used.erase(l);
                    if(r->kind.tag == KOOPA_RVT_INTEGER && r->kind.data.integer.value != 0)
                        extra_def[j].insert(r);
                    else
                        def_but_not_used.erase(r);
                }
                break;
            case KOOPA_RVT_STORE:
                {
                    auto v = inst->kind.data.store.value;
                    end[v] = j;
                    use[j].insert(v);
                    if(v->kind.tag == KOOPA_RVT_INTEGER && v->kind.data.integer.value != 0)
                        extra_def[j].insert(v);
                    else
                        def_but_not_used.erase(v);
                }
                break;
            case KOOPA_RVT_BRANCH:
                {
                    auto c = inst->kind.data.branch.cond;
                    end[c] = j;
                    use[j].insert(c);
                    if(c->kind.tag == KOOPA_RVT_INTEGER && c->kind.data.integer.value != 0)
                        extra_def[j].insert(c);
                    else
                        def_but_not_used.erase(c);
                }
                break;
            case KOOPA_RVT_CALL:
                for(int i = 0; i < inst->kind.data.call.args.len; i++) {
                    auto v = (koopa_raw_value_t)inst->kind.data.call.args.buffer[i];
                    end[v] = j;
                    use[j].insert(v);
                    if(v->kind.tag == KOOPA_RVT_INTEGER && v->kind.data.integer.value != 0)
                        extra_def[j].insert(v);
                    else
                        def_but_not_used.erase(v);
                }
                if(inst->ty->tag == KOOPA_RTT_INT32) {
                    for(auto iter = def_but_not_used.begin(); iter != def_but_not_used.end(); iter++) {
                        auto s = start.find(*iter);
                        if(s != start.end())
                            def.erase(s->second);
                        spill.insert(*iter);
                        spill_bytes += 4;
                    }
                }
            default:
                break;
            }
            if(inst->ty->tag == KOOPA_RTT_INT32) {
                def[j] = inst;
                start[inst] = j;
                def_but_not_used.insert(inst);
            }
        }
        for(auto iter = def_but_not_used.begin(); iter != def_but_not_used.end(); iter++) {
            spill.insert(*iter);
            spill_bytes += 4;
        }
        // params不参与t0-t5的分配

        set<koopa_raw_value_t> active;
        for(int j = 0; j < block->insts.len; j++) {
            //auto inst = (koopa_raw_value_data_t *)block->insts.buffer[j];
            active.insert(extra_def[j].begin(), extra_def[j].end());

            while(active.size() > 6) {
                auto spill_value = active.begin();
                for(auto k = active.begin(); k != active.end(); k++) {
                    if(end[*k] > end[*spill_value]) {
                        spill_value = k;
                    }
                    else if(end[*k] == end[*spill_value]) {
                        if(start[*k] < start[*spill_value]) {
                            spill_value = k;
                        }
                    }
                }
                spill.insert(*spill_value);
                active.erase(spill_value);
                if((*spill_value)->ty->tag == KOOPA_RTT_INT32)
                    spill_bytes += 4;
            }
            set<koopa_raw_value_t> temp;
            set_difference(active.begin(), active.end(), use[j].begin(), use[j].end(), inserter(temp, temp.end()));
            active = temp;
            auto d = def.find(j);
            if(d != def.end())
                active.insert(d->second);
        }
    }
    stackFrame.length += spill_bytes;
}