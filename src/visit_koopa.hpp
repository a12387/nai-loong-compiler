#pragma once
#include "koopa.h"
#include <fstream>

using namespace std;

void visit(ofstream &out, const koopa_raw_program_t raw);
void visit(ofstream &out, const koopa_raw_slice_t &slice);
void visit(ofstream &out, const koopa_raw_function_t &func);
void visit(ofstream &out, const koopa_raw_basic_block_t &bb);
void visit(ofstream &out, const koopa_raw_value_t &value);
void visit(ofstream &out, const koopa_raw_return_t &ret);
void visit(ofstream &out, const koopa_raw_integer_t &integer);