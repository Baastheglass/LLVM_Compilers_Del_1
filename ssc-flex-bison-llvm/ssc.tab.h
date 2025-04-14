/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     tok_printd = 258,
     tok_prints = 259,
     tok_if = 260,
     tok_else = 261,
     tok_for = 262,
     tok_function = 263,
     tok_identifier = 264,
     tok_double_literal = 265,
     tok_string_literal = 266,
     tok_int_literal = 267,
     tok_eq = 268,
     tok_neq = 269,
     tok_leq = 270,
     tok_geq = 271,
     tok_lt = 272,
     tok_gt = 273,
     THEN = 274
   };
#endif
/* Tokens.  */
#define tok_printd 258
#define tok_prints 259
#define tok_if 260
#define tok_else 261
#define tok_for 262
#define tok_function 263
#define tok_identifier 264
#define tok_double_literal 265
#define tok_string_literal 266
#define tok_int_literal 267
#define tok_eq 268
#define tok_neq 269
#define tok_leq 270
#define tok_geq 271
#define tok_lt 272
#define tok_gt 273
#define THEN 274




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 30 "ssc.y"
{
	char *identifier;
	double double_literal;
	char *string_literal;
	int int_literal;
	void *value;        // Use void* instead of llvm::Value*
	void *block;        // Use void* for all block types
}
/* Line 1529 of yacc.c.  */
#line 96 "ssc.tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

