%{
	// import header file generated for from the bison file s.t. KEYs are known here
	#include "hddl.hpp"
%}
%option noyywrap
%option yylineno
%%
\(				{return '('; }
\)				{return ')'; }
-				{return '-'; }
=				{return '='; }
\<				{return '<'; }
define			{return KEY_DEFINE;}
domain			{return KEY_DOMAIN;}
:domain			{return KEY_DOMAIN;}
problem			{return KEY_PROBLEM;}
:requirements	{return KEY_REQUIREMENTS;}
:types			{return KEY_TYPES;}
:constants		{return KEY_CONSTANTS;}
:predicates		{return KEY_PREDICATES;}
:task			{return KEY_TASK;}
:action			{return KEY_ACTION;}
:parameters		{return KEY_PARAMETERS;}
:precondition	{return KEY_PRECONDITION;}
:effect			{return KEY_EFFECT;}
:method			{return KEY_METHOD;}
:init			{return KEY_INIT;}
:goal			{return KEY_GOAL;}
:htn			{return KEY_HTN;}
:objects		{return KEY_OBJECTS;}
      
:subtasks			{return KEY_TASKS;}
:tasks				{return KEY_TASKS;}
:ordered-subtasks	{return KEY_ORDER_TASKS;}
:ordered-tasks		{return KEY_ORDER_TASKS;}
:ordering			{return KEY_ORDER;}
:order				{return KEY_ORDER;}

:constraints		{return KEY_CONSTRAINTS;}

and				{return KEY_AND;}
or				{return KEY_OR;}
not				{return KEY_NOT;}
imply			{return KEY_IMPLY;}
forall			{return KEY_FORALL;}
exists			{return KEY_EXISTS;}
when			{return KEY_WHEN;}

type			{return KEY_TYPEOF;}
typeof			{return KEY_TYPEOF;}
sort			{return KEY_TYPEOF;}
sortof			{return KEY_TYPEOF;}


[ \t\n\r]         ; // whitespace
;.*[\n\r]         ; // comments
\?[a-zA-Z][a-zA-Z0-9\-_]*	{yylval.sval = strdup(yytext); return VAR_NAME;}
:[a-zA-Z][a-zA-Z0-9\-_]*	{yylval.sval = strdup(yytext); return REQUIRE_NAME;}
[a-zA-Z][a-zA-Z0-9\-_]*		{yylval.sval = strdup(yytext); return NAME;}
[0-9]+          			{yylval.fval = atoi(yytext); return INT;}
%%
