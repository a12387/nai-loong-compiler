#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include "ast.hpp"
#include "koopa.h"
#include "visit_koopa.hpp"

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

    

    unique_ptr<BaseAST> ast;
    auto ret = yyparse(ast);
    assert(!ret);

    string s = "";
    
    ast->dump();
    ast->toKoopa(s);

    if(mode[1] == 'k') {
        ofstream out(output, ios::out | ios::trunc);
        assert(out.is_open());
        out << s;
        out.close();
    }
    else {
        koopa_program_t program;
        koopa_error_code_t ret = koopa_parse_from_string(s.c_str(), &program);
        assert(ret == KOOPA_EC_SUCCESS);
        koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
        koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
        koopa_delete_program(program);

        koopa_generate_raw_to_koopa(&raw, &program);
        koopa_dump_to_stdout(program);

        ofstream out(output, ios::out | ios::trunc);
        assert(out.is_open());
        visit(out, raw);
        out.close();
        koopa_delete_raw_program_builder(builder);
    }

    return 0;
}
