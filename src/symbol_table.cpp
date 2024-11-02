#include "symbol_table.hpp"
#include <iostream>

Item SymbolTable::getItem(string ident) {
    auto i = table.find(ident);
    if(i == table.end()) {
        cerr << "Use of undeclared identifier!" << endl;
        exit(1);
    }
    return i->second;
}
void SymbolTable::addItem(string ident, int c) {
    if(table.find(ident) != table.end()) {
        cerr << "Multi definition of identifier: " << ident << endl;
        exit(1);
    }
    Item i = Item(c);
    table.insert(make_pair(ident, i));
}
void SymbolTable::addItem(string ident, koopa_raw_value_data_t *v) {
    if(table.find(ident) != table.end()) {
        cerr << "Multi definition of identifier: " << ident << endl;
        exit(1);
    }
    Item i = Item(v);
    table.insert(make_pair(ident, i));
}