#pragma once
#include "ast.hpp"
#include "helper.hpp"
#include <map>

class Optimizer {
public:
    static void optimize(koopa_raw_program_t *raw) {
        while(true) {
            bool t = false;
            t = constantFolding(raw) || t;
            t = constantPropagation(raw) || t;
            if(!t)
                break; 
        }
    }
private:
    static bool constantFolding(koopa_raw_program_t *raw) {
        bool flag = false;
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
                            flag = true;
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
        return flag;
    }
    static bool constantPropagation(koopa_raw_program_t *raw) {
        bool flag = false;
        for(int _i = 0; _i < raw->funcs.len; _i++) {
            auto func = (koopa_raw_function_data_t *)raw->funcs.buffer[_i];
            for(int _j = 0; _j < func->bbs.len; _j++) {
                auto bb = (koopa_raw_basic_block_data_t *)func->bbs.buffer[_j];
                bool flag_change = false;
                vector<const void *> buffer;
                map<pair<koopa_raw_value_t, int>, int> consts;
                for(int _k = 0; _k < bb->insts.len; _k++) {
                    auto inst = (koopa_raw_value_data_t *)bb->insts.buffer[_k];
                    bool flag_join = true;
                    if(inst->kind.tag == KOOPA_RVT_STORE) {
                        auto store = inst->kind.data.store;
                        if(store.value->kind.tag == KOOPA_RVT_INTEGER) {
                            if(store.dest->kind.tag == KOOPA_RVT_ALLOC) {
                                consts[make_pair(store.dest, -1)] = store.value->kind.data.integer.value;
                            }
                            else {
                                auto d = store.dest;
                                int bias = 0;
                                bool flag_unknown = false;
                                while(d->kind.tag == KOOPA_RVT_GET_ELEM_PTR) {
                                    if(d->kind.data.get_elem_ptr.index->kind.tag != KOOPA_RVT_INTEGER) {
                                        flag_unknown = true;
                                        break;
                                    }
                                    bias += getArraySize(d->ty->data.pointer.base) * d->kind.data.get_elem_ptr.index->kind.data.integer.value;
                                    d = d->kind.data.get_elem_ptr.src;
                                }
                                if(!flag_unknown)
                                    consts[make_pair(d, bias)] = store.value->kind.data.integer.value;
                            }
                        }
                    }
                    else if(inst->kind.tag == KOOPA_RVT_LOAD) {
                        auto load = inst->kind.data.load;
                        if(load.src->kind.tag == KOOPA_RVT_ALLOC) {
                            auto iter = consts.find(make_pair(inst->kind.data.load.src, -1));
                            if(iter != consts.end()) {
                                flag = true;
                                inst->kind.tag = KOOPA_RVT_INTEGER;
                                inst->kind.data.integer.value = iter->second;
                                flag_join = false;
                                flag_change = true;
                            }
                        }
                        else if(load.src->kind.tag != KOOPA_RVT_GLOBAL_ALLOC) {
                            auto last = (koopa_raw_value_t)buffer.back();
                            int bias = 0;
                            bool flag_unknown = false;
                            vector<const void *> tmp;
                            while(last->kind.tag == KOOPA_RVT_GET_ELEM_PTR) {
                                buffer.pop_back();
                                tmp.push_back(last);
                                if(last->kind.data.get_elem_ptr.index->kind.tag != KOOPA_RVT_INTEGER) {
                                    buffer.insert(buffer.end(), tmp.rbegin(), tmp.rend());
                                    flag_unknown = true;
                                    break;
                                }
                                bias += getArraySize(last->ty->data.pointer.base) * last->kind.data.get_elem_ptr.index->kind.data.integer.value;
                                if(last->kind.data.get_elem_ptr.src->kind.tag == KOOPA_RVT_ALLOC || last->kind.data.get_elem_ptr.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
                                    last = last->kind.data.get_elem_ptr.src;
                                    break;
                                }
                                last = (koopa_raw_value_data_t *)buffer.back();
                            }
                            if(!flag_unknown) {
                                auto iter = consts.find(make_pair(last, bias));
                                if(iter != consts.end()) {
                                    flag = true;
                                    inst->kind.tag = KOOPA_RVT_INTEGER;
                                    inst->kind.data.integer.value = iter->second;
                                    flag_join = false;
                                    flag_change = true;
                                }
                                else {
                                    buffer.insert(buffer.end(), tmp.rbegin(), tmp.rend());
                                }
                            }
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
        return flag;
    }
};