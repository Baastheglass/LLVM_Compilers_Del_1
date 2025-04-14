%{
	//Will be created by Bison later on but we will be creating it at the moment for testing.
	#include "ssc.tab.h"
	#include <stdlib.h>
	#include <stdio.h>
	#include <string>
	extern void yyerror(const char *msg);
	
	//#define DEBUGSSC
	//This code is for producing debug output.
	#ifdef DEBUGSSC
		#define debugFlex(a) (printf("\n%d: Token=<ID, lexeme>=<%d, %s>\n",yylineno, a, yytext))
	#else
		#define debugFlex(a)
	#endif
%}

%option yylineno 
%option noyywrap

%%

"printd"				{debugFlex(tok_printd); return tok_printd;}
"prints"				{debugFlex(tok_prints); return tok_prints;}
"if"					{debugFlex(tok_if); return tok_if;}
"else"					{debugFlex(tok_else); return tok_else;}
"for"					{debugFlex(tok_for); return tok_for;}
"function" 				{debugFlex(tok_function); return tok_function;}
"=="                    {debugFlex(tok_eq); return tok_eq;}
"!="                    {debugFlex(tok_neq); return tok_neq;}
"<="                    {debugFlex(tok_leq); return tok_leq;}
">="                    {debugFlex(tok_geq); return tok_geq;}
"<"                     {debugFlex(tok_lt); return tok_lt;}
">"                     {debugFlex(tok_gt); return tok_gt;}
[a-zA-Z][a-zA-Z0-9_]*			{debugFlex(tok_identifier); yylval.identifier=strdup(yytext); return tok_identifier;}
[0-9]+\.[0-9]+      { debugFlex(tok_double_literal); yylval.double_literal = atof(yytext); return tok_double_literal; }
[0-9]+                  {debugFlex(tok_int_literal); yylval.int_literal=atoi(yytext); return tok_int_literal;}
\"[^"]*\"				{
                            debugFlex(tok_string_literal); 
                            // Strip the quotes from the string
                            yytext[yyleng-1] = '\0';
                            yylval.string_literal=strdup(yytext+1); 
                            return tok_string_literal;
                        }
\/\/.*					{;}
[ \n\r\t]				{;}
"+"|"-"|"*"|"\/"|"="|";"|"("|")"|"{"|"}"	{debugFlex(yytext[0]); return yytext[0];} 
.					{debugFlex(-1); yyerror("this is a illegal lexeme"); exit(EXIT_FAILURE);}
%%

