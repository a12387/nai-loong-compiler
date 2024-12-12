#pragma once
#include "koopa.h"
#include "stack_frame.hpp"
#include <fstream>

using namespace std;

void visit(ofstream &out, const koopa_raw_program_t raw);
void visit(ofstream &out, const koopa_raw_slice_t &slice);
void visit(ofstream &out, const koopa_raw_slice_t &slice, StackFrame &stackFrame);
void visit(ofstream &out, const koopa_raw_function_t &func);
void visit(ofstream &out, const koopa_raw_basic_block_t &bb, StackFrame &stackFrame);
void visit(ofstream &out, const koopa_raw_value_t &value, StackFrame &stackFrame);
void visit(ofstream &out, const koopa_raw_return_t &ret, StackFrame &stackFrame);
void visit(ofstream &out, const koopa_raw_integer_t &integer);
void visit(ofstream &out, const koopa_raw_binary_t &integer, StackFrame &stackFrame);
void visit(ofstream &out, const koopa_raw_load_t &load, StackFrame &stackFrame);
void visit(ofstream &out, const koopa_raw_store_t &store, StackFrame &stackFrame);
void visit(ofstream &out, const koopa_raw_branch_t &branch, StackFrame &stackFrame);
void visit(ofstream &out, const koopa_raw_jump_t &jump, StackFrame &stackFrame);
int getStackLength(const koopa_raw_function_t &func);
int getStackLength(const koopa_raw_slice_t &slice);
int getStackLength(const koopa_raw_value_t& value);
int getStackLength(const koopa_raw_basic_block_t &bb);
void prologue(ofstream &out, StackFrame &stackFrame);
void epilogue(ofstream &out, StackFrame &stackFrame);
string getReg(ofstream &out, const koopa_raw_value_t &value, StackFrame &stackFrame, int &shrink);
string getReg(ofstream &out, const koopa_raw_value_t &value, StackFrame &stackFrame);
void visitParams(ofstream &out, const koopa_raw_slice_t &params, StackFrame &stackFrame);