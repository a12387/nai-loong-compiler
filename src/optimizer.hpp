#pragma once
#include "ast.hpp"
#include "helper.hpp"
class Optimizer {
public:
    static void optimize(koopa_raw_program_t *raw) {
        constantFolding(raw);
    }
private:
    static void constantFolding(koopa_raw_program_t *raw) {
        for(int _i = 0; _i < raw->funcs.len; _i++) {
            auto func = (koopa_raw_function_data_t *)raw->funcs.buffer[_i];
            for(int _j = 0; _j < func->bbs.len; _j++) {
                auto bb = (koopa_raw_basic_block_data_t *)func->bbs.buffer[_j];
                bool flag_change = false;
                vector<const void *> buffer;
                for(int _k = 0; _k < bb->insts.len; _k++) {
                    auto inst = (koopa_raw_value_data_t *)bb->insts.buffer[_k];
                    bool flag_join = true;
                    if(inst->kind.tag == KOOPA_RVT_BINARY) {
                        auto binary = inst->kind.data.binary;
                        if(binary.lhs->kind.tag == KOOPA_RVT_INTEGER
                        && binary.rhs->kind.tag == KOOPA_RVT_INTEGER) {
                            int l = binary.lhs->kind.data.integer.value;
                            int r = binary.rhs->kind.data.integer.value;
                            int result = 0;
                            switch (binary.op) {
                            case KOOPA_RBO_AND:
                                result = l & r;
                                break;
                            case KOOPA_RBO_ADD:
                                result = l + r;
                                break;
                            case KOOPA_RBO_DIV:
                                result = l / r;
                                break;
                            case KOOPA_RBO_EQ:
                                result = l == r;
                                break;
                            case KOOPA_RBO_GE:
                                result = l >= r;
                                break;
                            case KOOPA_RBO_GT:
                                result = l > r;
                                break;
                            case KOOPA_RBO_LE:
                                result = l <= r;
                                break;
                            case KOOPA_RBO_LT:
                                result = l < r;
                                break;
                            case KOOPA_RBO_MOD:
                                result = l % r;
                                break;
                            case KOOPA_RBO_MUL:
                                result = l * r;
                                break;
                            case KOOPA_RBO_NOT_EQ:
                                result = l != r;
                                break;
                            case KOOPA_RBO_OR:
                                result = l | r;
                                break;
                            case KOOPA_RBO_SUB:
                                result = l - r;
                                break;
                            case KOOPA_RBO_XOR:
                                result = l ^ r;
                                break;
                            case KOOPA_RBO_SAR:
                                result = l >> r;
                                break;
                            case KOOPA_RBO_SHR:
                                result = (unsigned)l >> (unsigned)r;
                                break;
                            case KOOPA_RBO_SHL:
                                result = l << r;
                                break;
                            default:
                                break;
                            }
                            inst->kind.tag = KOOPA_RVT_INTEGER;
                            inst->kind.data.integer.value = result;
                            inst->name = nullptr;
                            flag_join = false;
                            flag_change = true;
                        }
                    }
                    if(flag_join) {
                        buffer.push_back(inst);
                    }
                }
                if(flag_change) {
                    clearAddItemToSlice(bb->insts, buffer);
                }
            }
        }
    }
};