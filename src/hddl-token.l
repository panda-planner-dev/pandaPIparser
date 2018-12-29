%{
	#include <iostream>
	#include <vector>
	#include "hddl.hpp"
	using namespace std;
%}
%option noyywrap
%option yylineno
%%
\(			{return '('; }
\)			{return ')'; }
-			{return '-'; }
:types		{return KEY_TYPES;}

[ \t\n\r]         ; // whitespace
;.*[\n\r]         ; // comments
[a-zA-Z][a-zA-Z0-9\-_]*		{yylval.sval = strdup(yytext); return NAME;}
[0-9]+\.[0-9]+  			{yylval.fval = atof(yytext); return FLOAT;}
[0-9]+          			{yylval.fval = atoi(yytext); return INT;}
%%
/*

NUMBER : [0-9][0-9]* '.'? [0-9]* | '.' [0-9]*;
*/
