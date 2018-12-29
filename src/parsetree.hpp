#include<vector>
#include<string>

using namespace std;


struct sort_definition{
	vector<string> declared_sorts;
	bool has_parent_sort;
	string parent_sort;
};





// global places to put data structures
extern vector<sort_definition> sort_definitions;
