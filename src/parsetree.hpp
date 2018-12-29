#include<vector>
#include<map>
#include<string>

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



// global places to put data structures
extern vector<sort_definition> sort_definitions;
extern vector<predicate_definition> predicate_definitions;



