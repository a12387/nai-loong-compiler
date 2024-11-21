#pragma once

#include <unordered_map>
#include <vector>
#include <koopa.h>

using namespace std;

enum ItemType{
    SYMBOLTABLE_ITEM_CONST,
    SYMBOLTABLE_ITEM_VAR
};

struct Item {
public:
    ItemType type;
    union {
        int c;
        koopa_raw_value_data_t *v;
    } data;
    Item(int c) { type = SYMBOLTABLE_ITEM_CONST; data.c = c; }
    Item(koopa_raw_value_data_t *v) { type = SYMBOLTABLE_ITEM_VAR; data.v = v; }
};

class SymbolTable {
public:
    static Item getItem(string ident);
    static void addItem(string ident, int c);
    static void addItem(string ident, koopa_raw_value_data_t *v);
    static void addTable();
    static void removeTable();
private:
    inline static vector<unordered_map<string, Item> > tables = {};
};