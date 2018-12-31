#include <cstdio>
#include <iostream>
#include <vector>
#include <map>
#include <cassert>
#include "parsetree.hpp"
#include "hddl.hpp"
#include "domain.hpp"
#include "sortexpansion.hpp"
#include "parametersplitting.hpp"
#include "cwa.hpp"
#include "typeof.hpp"
#include "util.hpp"
#include "output.hpp"

using namespace std;

// declare parser function manually
void run_parser_on_file(FILE* f);

// parsed domain data structures
bool has_typeof_predicate = false;
vector<sort_definition> sort_definitions;
vector<predicate_definition> predicate_definitions;
vector<parsed_task> parsed_primitive;
vector<parsed_task> parsed_abstract;
map<string,vector<parsed_method> > parsed_methods;


map<string,set<string> > sorts;
vector<method> methods;
vector<task> primitive_tasks;
vector<task> abstract_tasks;

map<string, task> task_name_map;


int main(int argc, char** argv) {
	cin.sync_with_stdio(false);
	cout.sync_with_stdio(false);
	if (argc < 2){
		cout << "You need to provide a domain and problem file as input." << endl;
		return 1;
	}
	// open c-style file handle 
	FILE *domain_file = fopen(argv[1], "r");
	FILE *problem_file = fopen(argv[2], "r");
	bool verboseOutput = false;
	int level = 0;
	if (argc > 3){
		string s(argv[3]);
		verboseOutput = s == "-debug";
		if (argc > 4) level = atoi(argv[4]);
	}

	if (!domain_file) {
		cout << "I can't open " << argv[1] << "!" << endl;
		return 2;
	}
	if (!problem_file) {
		cout << "I can't open " << argv[2] << "!" << endl;
		return 2;
	}
	// parse the domain file
	run_parser_on_file(domain_file);
	run_parser_on_file(problem_file);

	expand_sorts(); // add constants to all sorts
	
	// handle typeof-predicate
	if (has_typeof_predicate) create_typeof();
	// flatten all primitive tasks
	flatten_tasks();
	// .. and the goal
	flatten_goal();
	// create appropriate methods and expand method preconditions
	parsed_method_to_data_structures();
	// split methods with independent parameters to reduce size of grounding
	split_independent_parameters();
	// cwa
	compute_cwa();
	// simplify constraints as far as possible
	reduce_constraints();
	clean_up_sorts();
	remove_unnecessary_predicates();

	// write to output
	if (verboseOutput) verbose_output(level);
	else simple_hddl_output();
}
