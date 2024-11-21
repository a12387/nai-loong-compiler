#include "symbol_table.hpp"
#include <iostream>

Item SymbolTable::getItem(string ident) {
    for(int i = tables.size() - 1; i >= 0; i--) {
        auto &table = tables[i];
        auto iter = table.find(ident);
        if(iter != table.end()) {
            return iter->second;
        }
    }
    cerr << "Use of undeclared identifier: " << ident << endl;
    exit(1);
}
void SymbolTable::addItem(string ident, int c) {
    auto &table = tables.back();
    if(table.find(ident) != table.end()) {
        cerr << "Multi definition of identifier: " << ident << endl;
        exit(1);
    }
    Item i = Item(c);
    table.insert(make_pair(ident, i));
}
void SymbolTable::addItem(string ident, koopa_raw_value_data_t *v) {
    auto &table = tables.back();
    if(table.find(ident) != table.end()) {
        cerr << "Multi definition of identifier: " << ident << endl;
        exit(1);
    }
    Item i = Item(v);
    table.insert(make_pair(ident, i));
}
void SymbolTable::addTable() {
    tables.push_back({});
}
void SymbolTable::removeTable() {
    tables.pop_back();
}