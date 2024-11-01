%code requires {
  #include <memory>
  #include <string>
  #include <ast.hpp>
}

%{

#include <iostream>
#include <memory>
#include <string>
#include <ast.hpp>

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
  vector<unique_ptr<BaseAST> >  *vec_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN CONST
%token <str_val> IDENT
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef FuncType Block Stmt
%type <ast_val> Exp PrimaryExp UnaryExp AddExp MulExp RelExp EqExp LAndExp LOrExp
%type <ast_val> Decl ConstDecl BType ConstDef ConstInitVal BlockItem LVal ConstExp VarDecl VarDef InitVal
%type <vec_val> ConstMultiDef BlockMultiItem VarMultiDef
%type <int_val> Number
%type <str_val> UnaryOp MulOp AddOp RelOp EqOp

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
CompUnit
  : FuncDef {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->func_def = unique_ptr<BaseAST>($1);
    ast = move(comp_unit);
  }
  ;

// FuncDef ::= FuncType IDENT '(' ')' Block;
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值, 我们可以通过给这个符号赋值的方法来返回结果
// 你可能会问, FuncType, IDENT 之类的结果已经是字符串指针了
// 为什么还要用 unique_ptr 接住它们, 然后再解引用, 把它们拼成另一个字符串指针呢
// 因为所有的字符串指针都是我们 new 出来的, new 出来的内存一定要 delete
// 否则会发生内存泄漏, 而 unique_ptr 这种智能指针可以自动帮我们 delete
// 虽然此处你看不出用 unique_ptr 和手动 delete 的区别, 但当我们定义了 AST 之后
// 这种写法会省下很多内存管理的负担
FuncDef
  : FuncType IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

// 同上, 不再解释
FuncType
  : INT {
    auto ast = new FuncTypeAST();
    ast->type = "int";
    $$ = ast;
  }
  ;

Block
  : '{' BlockMultiItem '}' {
    auto ast = new BlockAST();
    ast->blockItem = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    $$ = ast;
  }
  ;

BlockMultiItem
  : BlockMultiItem BlockItem {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  | BlockItem {
    auto vec = new vector<unique_ptr<BaseAST> >;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  ;

BlockItem // 没有必要为它定义AST
  : Decl {
    $$ = $1;
  }
  | Stmt {
    $$ = $1;
  }
  ;

Stmt
  : RETURN Exp ';' {
    auto ast = new StmtAST1();
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | LVal '=' Exp ';' {
    auto ast = new StmtAST2();
    ast->lVal = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast->lOrExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto ast = new PrimaryExpAST1();
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | Number {
    auto ast = new PrimaryExpAST2();
    ast->num = $1;
    $$ = ast;
  }
  | LVal {
    auto ast = new PrimaryExpAST3();
    ast->lVal = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Number
  : INT_CONST {
    $$ = $1;
  }
  ;

UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST1();
    ast->primaryExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | UnaryOp UnaryExp {
    auto ast = new UnaryExpAST2();
    ast->unaryOp = *unique_ptr<string>($1);
    ast->unaryExp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

UnaryOp
  : '+' {
    $$ = new string("+");
  }
  | '-' {
    $$ = new string("-");
  }
  | '!' {
    $$ = new string("!");
  }
  ;

MulExp
  : UnaryExp {
    auto ast = new MulExpAST1();
    ast->unaryExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | MulExp MulOp UnaryExp {
    auto ast = new MulExpAST2();
    ast->mulExp = unique_ptr<BaseAST>($1);
    ast->mulOp = *unique_ptr<string>($2);
    ast->unaryExp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

MulOp
  : '*' {
    $$ = new string("*");
  }
  | '/' {
    $$ = new string("/");
  }
  | '%' {
    $$ = new string("%");
  }
  ;

AddExp 
  : MulExp {
    auto ast = new AddExpAST1();
    ast->mulExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | AddExp AddOp MulExp {
    auto ast = new AddExpAST2();
    ast->addExp = unique_ptr<BaseAST>($1);
    ast->addOp = *unique_ptr<string>($2);
    ast->mulExp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

AddOp
  : '+' {
    $$ = new string("+");
  }
  | '-' {
    $$ = new string("-");
  }
  ;

RelExp
  : AddExp {
    auto ast = new RelExpAST1();
    ast->addExp = unique_ptr<BaseAST>($1);
    $$ = $1;
  }
  | RelExp RelOp AddExp {
    auto ast = new RelExpAST2();
    ast->relExp = unique_ptr<BaseAST>($1);
    ast->relOp = *unique_ptr<string>($2);
    ast->addExp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

RelOp
  : '<' {
    $$ = new string("<");
  }
  | '>' {
    $$ = new string(">");
  }
  | '<' '=' {
    $$ = new string("<=");
  }
  | '>' '=' {
    $$ = new string(">=");
  }
  ;

EqExp
  : RelExp {
    auto ast = new EqExpAST1();
    ast->relExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | EqExp EqOp RelExp {
    auto ast = new EqExpAST2();
    ast->eqExp = unique_ptr<BaseAST>($1);
    ast->eqOp = *unique_ptr<string>($2);
    ast->relExp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

EqOp
  : '=' '=' {
    $$ = new string("==");
  }
  | '!' '=' {
    $$ = new string("!=");
  }
  ;

LAndExp 
  : EqExp {
    auto ast = new LAndExpAST1();
    ast->eqExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LAndExp '&' '&' EqExp {
    auto ast = new LAndExpAST2();
    ast->lAndExp = unique_ptr<BaseAST>($1);
    ast->eqExp = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

LOrExp
  : LAndExp {
    auto ast = new LOrExpAST1();
    ast->lAndExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LOrExp '|' '|' LAndExp {
    auto ast = new LOrExpAST2();
    ast->lOrExp = unique_ptr<BaseAST>($1);
    ast->lAndExp = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

Decl
  : ConstDecl {
    auto ast = new DeclAST1();
    ast->constDecl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | VarDecl {
    auto ast = new DeclAST2();
    ast->varDecl = unique_ptr<BaseAST>($1);
  }
  ;

ConstDecl
  : CONST BType ConstMultiDef ';' {
    auto ast = new ConstDeclAST();
    ast->bType = unique_ptr<BaseAST>($2);
    ast->constDef = unique_ptr<vector<unique_ptr<BaseAST> > >($3);
    $$ = ast;
  }
  ;

ConstMultiDef
  : ConstMultiDef ',' ConstDef {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  | ConstDef {
    auto vec = new vector<unique_ptr<BaseAST> >;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  ;

BType
  : INT {
    auto ast = new BTypeAST();
    ast->type = "int";
    $$ = ast;
  }
  ;

ConstDef
  : IDENT '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->constInitVal = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

ConstInitVal
  : ConstExp {
    auto ast = new ConstInitValAST();
    ast->constExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

VarDecl
  : BType VarMultiDef ';' {
    auto ast = new VarDeclAST();
    ast->bType = unique_ptr<BaseAST>($1);
    ast->varDef = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    $$ = ast;
  }
  ;

VarMultiDef
  : VarMultiDef ',' VarDef {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  | VarDef {
    auto vec = new vector<unique_ptr<BaseAST> >;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  ;

VarDef 
  : IDENT {
    auto ast = new VarDefAST1();
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  | IDENT '=' InitVal {
    auto ast = new VarDefAST2();
    ast->ident = *unique_ptr<string>($1);
    ast->initVal = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

InitVal
  : Exp {

  }

LVal
  : IDENT {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  ;

ConstExp 
  : Exp {
    auto ast = new ConstExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;


%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
