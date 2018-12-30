#ifndef __PARSETREE
#define __PARSETREE

#include<vector>
#include<map>
#include<set>
#include<string>
#include "domain.hpp"
using namespace std;

extern int task_id_counter;

struct sort_definition{
	vector<string> declared_sorts;
	bool has_parent_sort;
	string parent_sort;
};

struct var_declaration{
	vector<pair<string, string> > vars;
};

struct predicate_definition{
	string name;
	vector<string> argument_sorts;
};

typedef set<pair<string, string> > additional_variables ;
struct var_and_const{
	vector<string> vars;
	additional_variables newVar; // varname & sort
};

enum formula_type {EMPTY, AND, OR, FORALL, EXISTS, ATOM, NOTATOM,  // formulae
				   EQUAL, NOTEQUAL, OFSORT, NOTOFSORT,
				   WHEN   // conditional effect
				  };

class general_formula{
	public:
		formula_type type;
		vector<general_formula*> subformulae;
		string predicate;
		var_and_const arguments;
		var_declaration qvariables;

		string arg1;
		string arg2;

		void negate();
		vector<pair<vector<literal>, additional_variables> > expand();
};


struct parsed_task{
	string name;
	var_declaration* arguments;
	general_formula* prec;
	general_formula* eff;
};


struct sub_task{
	string id;
	string task;
	var_and_const* arguments;
};

struct parsed_task_network{
	vector<sub_task*> tasks;
	vector<pair<string,string>*> ordering;
	general_formula* constraint;
};

struct parsed_method{
	string name;
	vector<string> atArguments;
	var_declaration* vars; 	
	general_formula* prec;
	general_formula* eff;
	parsed_task_network* tn;
};


// global places to put data structures
extern bool has_typeof_predicate;
extern vector<sort_definition> sort_definitions;
extern vector<predicate_definition> predicate_definitions;
extern vector<parsed_task> parsed_primitive;
extern vector<parsed_task> parsed_abstract;
extern map<string,vector<parsed_method> > parsed_methods;


#endif
