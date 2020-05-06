%define api.prefix {tt}

%{
    #include "TLexer.h"
    #include "TParser.h"
    void tterror(void*, NProgram**, const char *s) { printf("ERROR: %sn", s); }
%}

// this stores all of the types returned by the production rules
%union {
    NProgram *program;
    NInput *input;
    NOutput *output;
    NAssignment *assignment;
    NExpr *expr;
    NTensor *tensor;
    std::string *string;
    std::vector<std::string>* indexList;
};

%pure-parser
%lex-param {void *scanner}
%parse-param {void *scanner}
%parse-param {struct NProgram **program}

%token <string> IDENTIFIER
%token INPUT
%token OUTPUT
%token INTEGER
%token FLOAT
//%token ELLIPSIS // TODO include this guy

%type <program> Program       // Just a list of Input/Output/Assignments
%type <input> Input
%type <output> Output
%type <assignment> Assignment
%type <expr> Expr
%type <tensor> Tensor
%type <indexList> IndexList
%type <indexList> IndexListHidden

%left '+'
%left '*'

%start Program

%%

// alteneratively, define program as InputList | AssignmentList | OutputList
Program : Assignment ';' { $$ = new NProgram(); $$->addAssignment($1); *program = $$; }
        | Input      ';' { $$ = new NProgram(); $$->addInput($1);      *program = $$; }
        | Output     ';' { $$ = new NProgram(); $$->addOutput($1);     *program = $$; }
        | Program Assignment ';' { $$->addAssignment($2); }
        | Program Input      ';' { $$->addInput($2); }
        | Program Output     ';' { $$->addOutput($2); }
        ;

Output : OUTPUT '(' IDENTIFIER ')' { $$ = new NOutput(*$3); }
       ;
IndexListHidden : IDENTIFIER { $$ = new std::vector<std::string>();
                               $$->push_back(*$1); }
                | IndexListHidden ',' IDENTIFIER { $$->push_back(*$3); }
                ;
IndexList : '(' ')' { $$ = new std::vector<std::string>(); }
          | '(' IndexListHidden ')' { $$ = $2; }
          ;
Tensor : IDENTIFIER IndexList { $$ = new NTensor(*$1, *$2); delete $2; }
       ;
Input : Tensor '=' INPUT { $$ = new NInput($1); }
      ;
Expr : Tensor        { $$ = $1;                  }//$$->print(); std::cout << std::endl; }
     | '(' Expr ')'  { $$ = $2;                  }//$$->print(); std::cout << std::endl; }
     | Expr '+' Expr { $$ = new NPlusOp($1, $3); }//$$->print(); std::cout << std::endl; }
     | Expr '*' Expr { $$ = new NMultOp($1, $3); }//$$->print(); std::cout << std::endl; }
     ;
Assignment : Tensor '=' Expr { $$ = new NAssignment($1, $3); }
           ;
%%
