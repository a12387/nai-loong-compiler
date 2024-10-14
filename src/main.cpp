#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <ast.hpp>

using namespace std;

extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);


int main(int argc, const char *argv[])
{

    assert(argc == 5);
    auto mode = argv[1];
    auto input = argv[2];
    auto output = argv[4];

    yyin = fopen(input, "r");
    assert(yyin);

    ofstream out(output, ios::out | ios::trunc);
    assert(out.is_open());

    unique_ptr<BaseAST> ast;
    auto ret = yyparse(ast);
    assert(!ret);

    ast->toKoopa(out);

    return 0;
}
