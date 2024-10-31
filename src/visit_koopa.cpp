#include <cassert>
#include <unordered_map>
#include "visit_koopa.hpp"

using namespace std;

static bool is_zero = false;
static int reg = 0;
static unordered_map<void *, string> reg_map;

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
            visit(out, reinterpret_cast<koopa_raw_value_t>(ptr));
            break;
        case KOOPA_RSIK_BASIC_BLOCK:
            visit(out, reinterpret_cast<koopa_raw_basic_block_t>(ptr));
            break;
        default:
            assert(false);
        }
    }
}

void visit(ofstream &out, const koopa_raw_function_t &func) {
    out << &(func->name[1]) << ":\n";
    
    visit(out, func->bbs);
}

void visit(ofstream &out, const koopa_raw_basic_block_t &bb) {
    visit(out, bb->insts);
}

void visit(ofstream &out, const koopa_raw_value_t &value) {
    const auto &kind = value->kind;
    switch(kind.tag) {
    case KOOPA_RVT_RETURN:
        visit(out, kind.data.ret);
        break;
    case KOOPA_RVT_INTEGER:
        visit(out, kind.data.integer);
        break;
    case KOOPA_RVT_BINARY:
        visit(out, kind.data.binary);
        break;
    default:
        assert(false);
    }
}

void visit(ofstream &out, const koopa_raw_return_t &ret) {
    
    if(ret.value->kind.tag == KOOPA_RVT_INTEGER) {
        out << "    li a0, ";
        out << ret.value->kind.data.integer.value;
    }
    else if(ret.value->kind.tag == KOOPA_RVT_BINARY) {
        out << "    mv a0, ";
        auto j = &ret.value->kind.data.binary;
        out << reg_map[(void *)j];
    }
    out << "\n    ret\n";
}

void visit(ofstream &out, const koopa_raw_integer_t &integer) {
    if(integer.value == 0) {
        is_zero = true;
    }
    else {
        out << "    li t" << reg++ << ", " << integer.value << "\n";
    }
}

void visit(ofstream &out, const koopa_raw_binary_t &binary) {
    if(reg_map.find((void *)&binary) != reg_map.end()) {
        return;
    }
    string regl, regr;
    int shrink = 2;
    auto lhs = binary.lhs;
    auto rhs = binary.rhs;

    if(lhs->kind.tag == KOOPA_RVT_INTEGER) {
        visit(out, lhs);
        if(is_zero) {
            is_zero = false;
            shrink--;
            regl = "x0";
        }
        else {
            regl = "t" + to_string(reg - 1);
        }
    }
    else if(lhs->kind.tag == KOOPA_RVT_BINARY) {
        auto i = &lhs->kind.data.binary;
        regl = reg_map[(void *)i];
    }

    if(rhs->kind.tag == KOOPA_RVT_INTEGER) {
        visit(out, rhs);
        if(is_zero) {
            is_zero = false;
            shrink--;
            regr = "x0";
        }
        else {
            regr = "t" + to_string(reg - 1);
        }
    }
    else if(rhs->kind.tag == KOOPA_RVT_BINARY) {
        auto i = &rhs->kind.data.binary;
        regr = reg_map[(void *)i];
    }   

    reg -= shrink;
    string reg_dest = "t" + to_string(reg++);
    reg_map.insert(make_pair((void *)&binary,reg_dest));
    

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