%{

#include "NParser.h"
#include "NLexer.h"
#define SAVE_TOKEN yylval->string = new std::string(yytext, yyleng)

%}

%option noinput
%option nounput

%option bison-bridge
%option extra-type="struct TTLexerExtra *"
%option prefix="tt"

%option reentrant

%%

[Ii][Nn][Pp][Uu][Tt]                          return INPUT;
[Oo][Uu][Tt][Pp][Uu][Tt]                      return OUTPUT;
[Mm][Ii][Nn]                                  return MIN;
[Mm][Aa][Xx]                                  return MAX;
[Aa][Bb][Ss]                                  return ABS;
"="                                           return ('=');
"+"                                           return ('+');
"*"                                           return ('*');
"-"                                           return ('-');
")"                                           return (')');
"("                                           return ('(');
","                                           return (',');
";"                                           return (';');
[0-9]+                                        SAVE_TOKEN; return INTEGER;
[+-]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?        SAVE_TOKEN; return FLOAT;
[A-Za-z][A-Za-z0-9_]*                         SAVE_TOKEN; return IDENTIFIER;
[ \t]                                         ;
.                                             printf("Unknown token!\n"); yyterminate();

%%

#ifdef __cplusplus
extern "C" {
#endif
int ttwrap(yyscan_t scanner){
  return 1;
}
#ifdef __cplusplus
}
#endif

