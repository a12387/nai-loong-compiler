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

void addItemToSlice(koopa_raw_slice_t &slice, void *item) {
    auto newbuf = new const void *[slice.len + 1];
    if(slice.buffer != nullptr) {
        memcpy(newbuf, slice.buffer, sizeof(const void *) * slice.len);
        delete[] slice.buffer;
    }
        
    newbuf[slice.len++] = item;
    slice.buffer = newbuf;
}
koopa_raw_type_kind_t *createTypeKind(koopa_raw_type_tag_t tag)  {
    auto ty = new koopa_raw_type_kind_t;
    ty->tag = tag;
    return ty;
}
koopa_raw_value_data_t *createValueData(koopa_raw_value_tag_t tag, const char *name, koopa_raw_type_t ty, koopa_raw_slice_item_kind_t used_by_kind) {
    auto raw = new koopa_raw_value_data_t;
    raw->kind.tag = tag;
    raw->ty = ty;

    if(name != nullptr) {
        auto n = new char[strlen(name) + 1];
        strcpy(n, name);
        raw->name = n;
    }
    else {
        raw->name = nullptr;
    }

    raw->used_by = {
        nullptr,
        0,
        used_by_kind
    };
    return raw;
}