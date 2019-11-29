#include <cstdio>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <map>
#include <cassert>
#include <cstring>
#include "parsetree.hpp"
#include "hddl.hpp"
#include "domain.hpp"
#include "sortexpansion.hpp"
#include "parametersplitting.hpp"
#include "cwa.hpp"
#include "typeof.hpp"
#include "util.hpp"
#include "output.hpp"
#include "shopWriter.hpp"
#include "hpdlWriter.hpp"
#include "verify.hpp"

using namespace std;

// declare parser function manually
void run_parser_on_file(FILE* f, char* filename);

// parsed domain data structures
bool has_typeof_predicate = false;
vector<sort_definition> sort_definitions;
vector<predicate_definition> predicate_definitions;
vector<parsed_task> parsed_primitive;
vector<parsed_task> parsed_abstract;
map<string,vector<parsed_method> > parsed_methods;
vector<pair<predicate_definition,string>> parsed_functions;
string metric_target = dummy_function_type;


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
	int dfile = -1;
	int pfile = -1;
	int doutfile = -1;
	int poutfile = -1;
	bool splitParameters = true;
	bool shopOutput = false;
	bool hpdlOutput = false;
	bool verboseOutput = false;
	bool verifyPlan = false;
	int level = 0;
	for (int i = 1; i < argc; i++){
		if (strcmp(argv[i], "-no-split-parameters") == 0) splitParameters = false;
		else if (strcmp(argv[i], "-shop") == 0 || strcmp(argv[i], "-shop2") == 0 || strcmp(argv[i], "-shop1") == 0){
		   	shopOutput = true;
			if (strcmp(argv[i], "-shop1") == 0)
			shop_1_compatability_mode = true;
		}
		else if (strcmp(argv[i], "-hpdl") == 0) hpdlOutput = true;
		else if (strcmp(argv[i], "-verify") == 0) verifyPlan = true;
		else if (strcmp(argv[i], "-debug") == 0){
		   	verboseOutput = true;
			if (i+1 == argc) continue;
			string s(argv[i+1]);
			if (all_of(s.begin(), s.end(), ::isdigit)){
				i++;
				level = atoi(argv[i]);
			}
		}
		else if (dfile == -1) dfile = i;
		else if (pfile == -1) pfile = i;
		else if (doutfile == -1) doutfile = i;
		else if (poutfile == -1) poutfile = i;
		else {
			cout << "Don't know what you meant with \"" << argv[i] <<"\"." << endl;
			return 1;
		}
	}


	// open c-style file handle 
	FILE *domain_file = fopen(argv[dfile], "r");
	FILE *problem_file = fopen(argv[pfile], "r");

	if (!domain_file) {
		cout << "I can't open " << argv[dfile] << "!" << endl;
		return 2;
	}
	if (!problem_file) {
		cout << "I can't open " << argv[pfile] << "!" << endl;
		return 2;
	}
	if (!shopOutput && poutfile != -1){
		cout << "For ordinary pandaPI output, you may only specify one output file, but you specified two: " << argv[doutfile] << " and " << argv[poutfile] << endl;
	}
	
	// parsing of command line arguments has been completed	
		
		
	// parse the domain file
	run_parser_on_file(domain_file, argv[dfile]);
	run_parser_on_file(problem_file, argv[pfile]);

	expand_sorts(); // add constants to all sorts
	
	// handle typeof-predicate
	if (has_typeof_predicate) create_typeof();

	// do not preprocess the instance at all if we are validating a solution
	if (verifyPlan){
		ifstream * plan  = new ifstream(argv[doutfile]);
		bool result = verify_plan(*plan);
		if (result) cout << "correct" << endl;
		else cout << "wrong" << endl;
		return 0;
	}


	// flatten all primitive tasks
	flatten_tasks();
	// .. and the goal
	flatten_goal();
	// create appropriate methods and expand method preconditions
	parsed_method_to_data_structures();

	if (shopOutput || hpdlOutput){
		// produce streams for output
		ostream * dout = &cout;
		ostream * pout = &cout;
		if (doutfile != -1){
			ofstream * df  = new ofstream(argv[doutfile]);
			if (!df->is_open()){
				cout << "I can't open " << argv[doutfile] << "!" << endl;
				return 2;
			}
			dout = df;
		}
		if (poutfile != -1){
			ofstream * pf  = new ofstream(argv[poutfile]);
			if (!pf->is_open()){
				cout << "I can't open " << argv[poutfile] << "!" << endl;
				return 2;
			}
			pout = pf;
		}
		if (shopOutput)	write_instance_as_SHOP(*dout,*pout);
		if (hpdlOutput)	write_instance_as_HPDL(*dout,*pout);
		return 0;
	}


	// split methods with independent parameters to reduce size of grounding
	if (splitParameters) split_independent_parameters();
	// cwa
	compute_cwa();
	// simplify constraints as far as possible
	reduce_constraints();
	clean_up_sorts();
	remove_unnecessary_predicates();

	// write to output
	if (verboseOutput) verbose_output(level);
	else {
		ostream * dout = &cout;
		if (doutfile != -1){
			ofstream * df  = new ofstream(argv[doutfile]);
			if (!df->is_open()){
				cout << "I can't open " << argv[doutfile] << "!" << endl;
				return 2;
			}
			dout = df;
		}
		simple_hddl_output(*dout);
	}
}
