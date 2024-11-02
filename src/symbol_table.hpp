#pragma once

#include <unordered_map>
#include <koopa.h>

using namespace std;

enum ItemType{
    CONST,
    VAR
};

struct Item {
public:
    ItemType type;
    union {
        int c;
        koopa_raw_value_data_t *v;
    } data;
    Item(int c) { type = CONST; data.c = c; }
    Item(koopa_raw_value_data_t *v) { type = VAR; data.v = v; }
};

class SymbolTable {
public:
    static Item getItem(string ident);
    static void addItem(string ident, int c);
    static void addItem(string ident, koopa_raw_value_data_t *v);
private:
    static unordered_map<string, Item> table;
};