#include <cassert>
#include "visit_koopa.hpp"

using namespace std;

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
    default:
        assert(false);
    }
}

void visit(ofstream &out, const koopa_raw_return_t &ret) {
    out << "    li a0, ";
    visit(out, ret.value);
    out << "\n    ret\n";
}

void visit(ofstream &out, const koopa_raw_integer_t &integer) {
    out << integer.value;
}