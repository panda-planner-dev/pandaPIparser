%{
	#include <cstdio>
	#include <iostream>
	#include <vector>
	#include "parsetree.hpp"
	
	using namespace std;
	
	// Declare stuff from Flex that Bison needs to know about:
	extern int yylex();
	extern int yyparse();
	extern FILE *yyin;
	
	void yyerror(const char *s);
%}

%locations

%union {
	int ival;
	float fval;
	char *sval;
	std::vector<std::string>* vstring;
}

%token KEY_TYPES
%token <sval> NAME FLOAT INT STRING

%type <vstring> NAME-list NAME-list-non-empty
%%

type_def : '(' KEY_TYPES type_def_list ')';
type_def_list : NAME-list {	sort_definition s; s.has_parent_sort = false; s.declared_sorts = *($1); delete $1;
			  				sort_definitions.push_back(s);}
			  | NAME-list-non-empty '-' NAME type_def_list {
							sort_definition s; s.has_parent_sort = true; s.parent_sort = $3; free($3);
							s.declared_sorts = *($1); delete $1;
			  				sort_definitions.push_back(s);}



NAME-list-non-empty: NAME-list NAME {string s($2); free($2); $$->push_back(s);}
NAME-list: NAME-list NAME {string s($2); free($2); $$->push_back(s);}
			|  {$$ = new vector<string>();} 
;

%%
void run_parser_on_file(FILE* f){
	yyin = f;
	yyparse();
}

void yyerror(const char *s) {
  cout << "Parse error in line " << yylloc.first_line << endl << "Message: " << s << endl;
  // might as well halt now:
  exit(-1);
}
