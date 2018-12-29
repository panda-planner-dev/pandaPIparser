#include<vector>
#include<map>
#include<set>
#include<string>
#include "domain.hpp"
using namespace std;


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
				   EQUAL, NOTEQUAL  
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
};


// global places to put data structures
extern vector<sort_definition> sort_definitions;
extern vector<predicate_definition> predicate_definitions;
extern vector<parsed_task> parsed_primitive;
extern vector<parsed_task> parsed_abstract;


