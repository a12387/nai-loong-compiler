#include <cassert>
#include "visit_koopa.hpp"
#include <cstring>

using namespace std;

static bool isZero = false;
static int reg = 0;
static unordered_map<void *, string> registers;

void visit(ofstream &out, const koopa_raw_program_t raw) {
    out << "    .text\n";
    out << "    .globl main\n";
    
    visit(out, raw.funcs);
    visit(out, raw.values);
}

void visit(ofstream &out, const koopa_raw_slice_t &slice) {
    for(int i = 0; i < slice.len; i++) {
        auto ptr = slice.buffer[i];
        switch(slice.kind) {
        case KOOPA_RSIK_FUNCTION:
            visit(out, reinterpret_cast<koopa_raw_function_t>(ptr));
            break;
        case KOOPA_RSIK_VALUE:
            //visit(out, reinterpret_cast<koopa_raw_value_t>(ptr));
            // Not Implemented
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

void visit(ofstream &out, const koopa_raw_function_t &func) {
    out << &(func->name[1]) << ":\n";
    
    int stackBytes = getStackLength(func);
    int stackLength = (stackBytes / 16 + (int)(stackBytes % 16 > 0)) * 16;
    StackFrame stackFrame(stackLength);

    prologue(out, stackFrame);
    visit(out, func->bbs, stackFrame);
}

void visit(ofstream &out, const koopa_raw_basic_block_t &bb, StackFrame &stackFrame) {
    if(strcmp(bb->name, "%entry") != 0 ) {
        out << &(bb->name[1]) << ":\n"; 
    }
    visit(out, bb->params, stackFrame);
    visit(out, bb->insts, stackFrame);
}

void visit(ofstream &out, const koopa_raw_value_t &value, StackFrame &stackFrame) {
    const auto &kind = value->kind;
    void *ptr = (void *)&kind;
    if(registers.find(ptr) != registers.end()) {
        return;
    }
    switch(kind.tag) {
    case KOOPA_RVT_RETURN:
        visit(out, kind.data.ret, stackFrame);
        break;
    case KOOPA_RVT_INTEGER:
        visit(out, kind.data.integer);
        break;
    case KOOPA_RVT_BINARY:
        visit(out, kind.data.binary, stackFrame);
        if(value->used_by.len > 0)
            registers[ptr] = "t" + to_string(reg - 1);
        else 
            reg--;
        break;
    case KOOPA_RVT_ALLOC:
        if(value->ty->data.pointer.base->tag == KOOPA_RTT_INT32) {
            stackFrame.add(ptr, 4);
            break;
        }
        else
            assert(false);
    case KOOPA_RVT_LOAD:
        visit(out, kind.data.load, stackFrame);
        registers[ptr] = "t" + to_string(reg - 1);
        break;
    case KOOPA_RVT_STORE:
        visit(out, kind.data.store, stackFrame);
        break;
    case KOOPA_RVT_BLOCK_ARG_REF:
        if(kind.data.block_arg_ref.index > 8)
            stackFrame.add(ptr, 4);
        else
            registers[ptr] = "a" + to_string(kind.data.block_arg_ref.index);
        break;
    case KOOPA_RVT_BRANCH:
        visit(out, kind.data.branch, stackFrame);
        break;
    case KOOPA_RVT_JUMP:
        visit(out, kind.data.jump, stackFrame);
        break;
    default:
        assert(false);
    }
}

void visit(ofstream &out, const koopa_raw_return_t &ret, StackFrame &stackFrame) {
    
    if(ret.value->kind.tag == KOOPA_RVT_INTEGER) {
        out << "    li a0, ";
        out << ret.value->kind.data.integer.value << '\n';
    }
    else {
        out << "    mv a0, ";
        out << registers[(void *)&ret.value->kind] << '\n';
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
    out << "    lw t" << reg++ << ", " << stackFrame.find(ptr) << "(sp)\n";
}

void visit(ofstream &out, const koopa_raw_store_t &store, StackFrame &stackFrame) {
    auto dest = store.dest;
    auto value = store.value;

    string regv = getReg(out, value, stackFrame);
    out << "    sw " << regv << ", " << stackFrame.find((void *)&dest->kind) << "(sp)\n";
}

void visit(ofstream &out, const koopa_raw_branch_t &branch, StackFrame &stackFrame) {
    auto cond = branch.cond;
    auto true_bb = branch.true_bb;
    auto true_args = branch.true_args;
    auto false_bb = branch.false_bb;
    auto false_args = branch.false_args;

    string regc = getReg(out, cond, stackFrame);

    visitParams(out, true_args, stackFrame);
    out << "    bnez " << regc << ", " << &(true_bb->name[1]) << endl;
    visitParams(out, false_args, stackFrame);
    out << "    j " << &(false_bb->name[1]) << endl;
}

void visit(ofstream &out, const koopa_raw_jump_t &jump, StackFrame &stackFrame) {
    auto target = jump.target;
    auto args = jump.args;

    visitParams(out, args, stackFrame);
    out << "    j " << &(target->name[1]) << endl;
}

int getStackLength(const koopa_raw_function_t &func) {
    return getStackLength(func->bbs);
}

int getStackLength(const koopa_raw_slice_t &slice) {
    int ret = 0;
    for(int i = 0; i < slice.len; i++) {
        auto ptr = slice.buffer[i];
        switch(slice.kind) {
        case KOOPA_RSIK_VALUE:
            ret += getStackLength(reinterpret_cast<koopa_raw_value_t>(ptr));
            break;
        case KOOPA_RSIK_BASIC_BLOCK:
            ret += getStackLength(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
            break;
        default:
            assert(false);
        }
    }
    return ret;
}

int getStackLength(const koopa_raw_value_t& value) {
    switch(value->ty->tag) {
    case KOOPA_RTT_INT32:
    case KOOPA_RTT_UNIT:
        return 0;
    case KOOPA_RTT_POINTER:
        if(value->ty->data.pointer.base->tag == KOOPA_RTT_INT32) {
            return 4;
        }
    default:
        return 0;
    }
}

int getStackLength(const koopa_raw_basic_block_t &bb) {
    return getStackLength(bb->insts);
}

void prologue(ofstream &out, StackFrame &stackFrame) {
    out << "    addi sp, sp, " << -stackFrame.length << '\n';
}

void epilogue(ofstream &out, StackFrame &stackFrame) {
    out << "    addi sp, sp, " << stackFrame.length << '\n';
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
    else if (value->kind.tag == KOOPA_RVT_BLOCK_ARG_REF && value->kind.data.block_arg_ref.index >= 8){
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
        auto ptr = (koopa_raw_value_data_t *)params.buffer[i];
        if(i < 8) {
            switch(ptr->kind.tag) {
            case KOOPA_RVT_INTEGER:
                out << "    li a" << i << ", " << ptr->kind.data.integer.value << endl;
                break;
            default:
                out << "    mv a" << i << ", " << getReg(out, ptr, stackFrame) << endl;
                break;
            }
        }
        else {
            stackFrame.add((void *)&ptr->kind, 4);
            out << "    sw" << getReg(out, ptr, stackFrame) << ", " << stackFrame.find((void *)&ptr->kind) << "(sp)\n";
        }
    }
}