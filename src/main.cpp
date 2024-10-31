#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

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
    
    Json::FastWriter json_writer;
    Json::Value root;
    root["CompUnit"] = ast->dump();
    Json::Value symtab;
    auto it = symbolTable.begin();
    int i = 0;
    while(it != symbolTable.end()) {
        Json::Value sym;
        sym["Name"] = it->first;
        sym["Value"] = it->second;
        it++;
        symtab[i++] = sym;
    }
    root["Symbol Table"] = symtab;

    ofstream out("ast.json", ios::out | ios::trunc);
    out << json_writer.write(root);
    auto raw = ast->toKoopa();

    if(mode[1] == 'k') {
        koopa_program_t program;
        auto ret = koopa_generate_raw_to_koopa((koopa_raw_program_t *)raw, &program);
        assert(ret == KOOPA_EC_SUCCESS);
        koopa_dump_to_file(program, output);
        
    }
    // else {
    //     koopa_program_t program;
    //     koopa_error_code_t ret;
    //     ret = koopa_generate_raw_to_koopa((koopa_raw_program_t *)raw, &program);
    //     assert(ret == KOOPA_EC_SUCCESS);
    //     size_t len;
    //     ret = koopa_dump_to_string(program, nullptr, &len);
    //     assert(ret == KOOPA_EC_SUCCESS);
    //     char *s = new char[2 * len + 1];
    //     len *= 2;
    //     ret = koopa_dump_to_string(program, s, &len);
        
    //     if(ret != KOOPA_EC_SUCCESS) {
    //         cout << "ERROR ! -" << ret << "\n" << len << endl;
    //     }
    //     ret = koopa_parse_from_string(s, &program);
    //     assert(ret == KOOPA_EC_SUCCESS);
    //     koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    //     koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    //     koopa_delete_program(program);

    //     koopa_generate_raw_to_koopa(&raw, &program);
    //     koopa_dump_to_stdout(program);

    //     ofstream out(output, ios::out | ios::trunc);
    //     assert(out.is_open());
    //     visit(out, raw);
    //     out.close();
    //     koopa_delete_raw_program_builder(builder);
    // }

    return 0;
}
