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

    // helpers to track control flow context
    void *currentIfBlock = nullptr;
    bool processingThen = false;
    bool processingElse = false;

    // Add this to track for loop context
    void *currentForBlock = nullptr;
%}

%union {
	char *identifier;
	double double_literal;
	char *string_literal;
	int int_literal;
	void *value;        // Use void* instead of llvm::Value*
	void *block;        // Use void* for all block types
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
%token tok_eq tok_neq tok_leq tok_geq tok_lt tok_gt

%type <value> term expression
%type <block> if_statement for_statement function_definition

%left tok_eq tok_neq
%left tok_lt tok_gt tok_leq tok_geq
%left '+' '-' 
%left '*' '/'
%left '(' ')'
%nonassoc THEN
%nonassoc tok_else

%start root

%%

root:   /* empty */                     {debugBison(1); initLLVM();}  
      | statement root                  {debugBison(2);}
      ;

statement:
        prints              {debugBison(3);}
      | printd              {debugBison(4);}
      | assignment          {debugBison(5);}
      | if_statement        {debugBison(6);}
      | for_statement       {debugBison(7);}
      | function_definition {debugBison(8);}
      | function_call       {debugBison(9);}
      ;

prints:	tok_prints '(' tok_string_literal ')' ';'   {debugBison(10); printString($3); } 
	;

printd:	tok_printd '(' expression ')' ';'		{debugBison(11); printDouble($3); }
	;

if_statement: 
        tok_if '(' expression ')' {
            // Save the entry block state
            void *ifBlock = createIfElseBlock($3);
            currentIfBlock = ifBlock;
            processingThen = true;
        } 
        '{' statements '}' {
            processingThen = false;
            completeIfThen(currentIfBlock);
        } 
        tok_else {
            processingElse = true;
        } 
        '{' statements '}' {
            processingElse = false;
            completeIfElse(currentIfBlock);
            currentIfBlock = nullptr;
            $$ = NULL;
        }
      | tok_if '(' expression ')' {
            // For if without else
            void *ifBlock = createIfBlock($3);
            currentIfBlock = ifBlock;
            processingThen = true;
        } 
        '{' statements '}' %prec THEN {
            processingThen = false;
            completeIfBlock(currentIfBlock);
            currentIfBlock = nullptr;
            $$ = NULL;
        }
      ;

for_statement: 
        tok_for tok_identifier '=' tok_int_literal '{' {
            // Create the for loop and set insertion point to loop body
            debugBison(14);
            currentForBlock = createForLoop($2, $4);
        } 
        statements '}' {
            // Complete the for loop body and set insertion point after the loop
            debugBison(15);
            completeForBody(currentForBlock);
            currentForBlock = nullptr;
            $$ = NULL;
        }
        ;

function_definition: 
        tok_function tok_identifier '{' {
            // Create function and set builder insertion point to function body
            debugBison(30);
            void *funcBlock = createFunction($2);
            $<block>$ = funcBlock;  // Save block for later
        } 
        statements '}' {
            // Complete function and restore builder insertion point
            debugBison(31);
            completeFunction($<block>4);  // Use saved block
        }
      ;

function_call:
        tok_identifier '(' ')' ';' {
            debugBison(32);
            callFunction($1);
        }
      ;

term:	tok_identifier				{debugBison(17); $$ = getValueFromSymbolTable($1); } 
	| tok_double_literal			{debugBison(18); $$ = createDoubleConstant($1); }
	;

assignment:  tok_identifier '=' expression ';'	{debugBison(19); setDouble($1, $3); } 
	;

expression: term				{debugBison(20); $$= $1;}
	   | expression '+' expression		{debugBison(21); $$ = performBinaryOperation($1, $3, '+');}
	   | expression '-' expression		{debugBison(22); $$ = performBinaryOperation($1, $3, '-');}
	   | expression '/' expression		{debugBison(23); $$ = performBinaryOperation($1, $3, '/');}
	   | expression '*' expression		{debugBison(24); $$ = performBinaryOperation($1, $3, '*');}
	   | '(' expression ')'			{debugBison(25); $$= $2;}
	   | expression tok_eq expression	{debugBison(26); $$ = performComparison($1, $3, 1);}
	   | expression tok_neq expression	{debugBison(27); $$ = performComparison($1, $3, 2);}
	   | expression tok_leq expression	{debugBison(28); $$ = performComparison($1, $3, 3);}
	   | expression tok_geq expression	{debugBison(29); $$ = performComparison($1, $3, 4);}
	   | expression tok_lt expression	{debugBison(30); $$ = performComparison($1, $3, 5);}
	   | expression tok_gt expression	{debugBison(31); $$ = performComparison($1, $3, 6);}
	   ;	   
	      
statements: 
         statement statements
       | /* empty */
       ;

%%

void yyerror(const char *err) {
	fprintf(stderr, "\n%s\n", err);
}

int main(int argc, char** argv) {
	// Initialize LLVM explicitly before parsing
	initLLVM();
	
	if (argc > 1) {
		FILE *fp = fopen(argv[1], "r");
		if (!fp) {
			fprintf(stderr, "Error: Cannot open file %s\n", argv[1]);
			return EXIT_FAILURE;
		}
		yyin = fp; //read from file when its name is provided.
	} 
	if (yyin == NULL) { 
		yyin = stdin; //otherwise read from terminal
	}
	
	//yyparse will call internally yylex
	//It will get a token and insert it into AST
	int parserResult = yyparse();
	if (parserResult != 0) {
		fprintf(stderr, "Parsing failed\n");
		return EXIT_FAILURE;
	}
	
	// Add return instruction and print LLVM IR
	addReturnInstr();
	printLLVMIR();
	
	return EXIT_SUCCESS;
}

