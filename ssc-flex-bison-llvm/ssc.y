%{
	#include <stdio.h>
	#include <stdlib.h>
	//contains our functions to be reused later by llvm.
	#include "IR.h"
	
	extern int yyparse();
	extern int yylex();
	extern FILE *yyin;
	void yyerror(const char *err);

	
	//#define DEBUGBISON
	//This code is for producing debug output.
	#ifdef DEBUGBISON
		#define debugBison(a) (printf("\n%d \n",a))
	#else
		#define debugBison(a)
	#endif
%}

%union {
	char *identifier;
	double double_literal;
	char *string_literal;
	int int_literal;
}

%token tok_printd
%token tok_prints
%token tok_if
%token tok_else
%token tok_for
%token tok_function
%token <identifier> tok_identifier
%token <double_literal> tok_double_literal
%token <string_literal> tok_string_literal
%token <int_literal> tok_int_literal

%type <double_literal> term expression

%left '+' '-' 
%left '*' '/'
%left '(' ')'

%start root

%%

root:   /* empty */                     {debugBison(1);}  
      | prints root                     {debugBison(2);}
      | printd root                      {debugBison(3);}
      | assignment root                   {debugBison(4);}
      | if root                           {debugBison(5);}
	  | else root                           {debugBison(6);}
      | for root                          {debugBison(7);}
      | function root                      {debugBison(8);}
      ;


prints:	tok_prints '(' tok_string_literal ')' ';'   {debugBison(9); print("%s\n", $3); } 
	;

printd:	tok_printd '(' term ')' ';'		{debugBison(10); print("%lf\n", $3); }
	;

if: tok_if '(' expression ')' '{' statement '}'		{debugBison(11); print("%s\n", "if($3) {$6}");}

else: tok_else '{' statement '}'	{debugBison(12); print("%s\n", "else{$3}");}

for: tok_for tok_identifier '=' tok_int_literal '{' statement '}' 	{debugBison(13); print("%s\n", "for(int $2 = 0; $2 < $4; $2++){$6}");}

function: tok_function  '{' statement '}'	{debugBison(14); print("%s\n", "void function() {$3}");} //we dont like parameters

term:	tok_identifier				{debugBison(15); $$ = getValueFromSymbolTable($1); } 
	| tok_double_literal			{debugBison(16); $$ = $1; }
	;

assignment:  tok_identifier '=' expression ';'	{debugBison(17); setValueInSymbolTable($1, $3); } 
	;

expression: term				{debugBison(18); $$= $1;}
	   | expression '+' expression		{debugBison(19); $$ = performBinaryOperation ($1, $3, '+');}
	   | expression '-' expression		{debugBison(20); $$ = performBinaryOperation ($1, $3, '-');}
	   | expression '/' expression		{debugBison(21); $$ = performBinaryOperation ($1, $3, '/');}
	   | expression '*' expression		{debugBison(22); $$ = performBinaryOperation ($1, $3, '*');}
	   | '(' expression ')'			{debugBison(23); $$= $2;}
	   ;	   
	      
statement: expression statement  
         | if statement  
         | for statement  
         | function statement  
         | /* empty */  // Allow empty statements
         ;

%%

void yyerror(const char *err) {
	fprintf(stderr, "\n%s\n", err);
}

int main(int argc, char** argv) {
	if (argc > 1) {
		FILE *fp = fopen(argv[1], "r");
		yyin = fp; //read from file when its name is provided.
	} 
	if (yyin == NULL) { 
		yyin = stdin; //otherwise read from terminal
	}
	
	//yyparse will call internally yylex
	//It will get a token and insert it into AST
	int parserResult = yyparse();
	
	return EXIT_SUCCESS;
}

