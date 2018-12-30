#include <cstdio>
#include <iostream>
#include <vector>
#include <map>
#include <cassert>
#include "parsetree.hpp"
#include "hddl.hpp"
#include "domain.hpp"
#include "sortexpansion.hpp"
#include "util.hpp"

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

int main(int argc, char** argv) {
	if (argc < 2){
		cout << "You need to provide a domain and problem file as input." << endl;
		return 1;
	}
	// open c-style file handle 
	FILE *domain_file = fopen(argv[1], "r");
	FILE *problem_file = fopen(argv[2], "r");
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
	cout << "DOMAIN has typeof preciate " << has_typeof_predicate << endl;
	if (has_typeof_predicate){
		// create a sort containing all objects
		for (auto s : sorts) for (string e : s.second) sorts["__object"].insert(e);
		
		set<string> allSorts;
		for(auto s : sorts) allSorts.insert(s.first);
		sorts["Type"] = allSorts;

		predicate_definition typePred;
		typePred.name = "typeOf";
		typePred.argument_sorts.push_back("__object");
		typePred.argument_sorts.push_back("Type");
		predicate_definitions.push_back(typePred);
	}
	
	// flatten all primitive tasks
	flatten_tasks();
	// create appropriate
	parsed_method_to_data_structures();

	cout << "number of sorts: " << sorts.size() << endl;
	/*for(auto s : sorts){
		cout << s.first << ":";
		for (string e : s.second) cout << " " << e;
		cout << endl;
	}*/

	/*cout << endl << "number of primitive: " << parsed_primitive.size() << endl;
	for(parsed_task a : parsed_primitive){
		cout << a.name << endl;
		vector<pair<vector<literal>, additional_variables> > ex = a.prec->expand();
		cout << "\tnumber of precondition expansions: " << ex.size() << endl;
		for(auto e : ex){
			cout << "\texpansion: " << endl;
			for (literal l : e.first){
				cout << "\t\t" << (l.positive ? "+" : "-") << " " << color(COLOR_BLUE,l.predicate);
				for(string v : l.arguments) cout << " " << v;
				cout << endl;
			}
			for(pair<string,string> nv : e.second)
				cout << "\t\t" << nv.first << " - " << nv.second << endl;
		}
		vector<pair<vector<literal>, additional_variables> > effex = a.eff->expand();
		cout << "\tnumber of effect expansions: " << effex.size() << endl;
		assert(effex.size() == 1);
		for(auto e : effex){
			cout << "\texpansion: " << endl;
			for (literal l : e.first){
				cout << "\t\t" << (l.positive ? "+" : "-") << " " << color(COLOR_RED,l.predicate);
				for(string v : l.arguments) cout << " " << v;
				cout << endl;
			}
			for(pair<string,string> nv : e.second)
				cout << "\t\t" << nv.first << " - " << nv.second << endl;
		}
	}*/

	cout << "number of primitives: " << primitive_tasks.size() << endl;
   	for(task t : primitive_tasks){
		cout << "\t" << color(COLOR_RED, t.name) << endl;
		cout << "\t\tvars:" << endl;
		for(auto v : t.vars) cout << "\t\t     " << v.first << " - " << v.second << endl;
		cout << "\t\tprec:" << endl;
		for(literal l : t.prec){
			cout << "\t\t     " << (l.positive?"+":"-") << " " << color(COLOR_BLUE,l.predicate);
			for(string v : l.arguments) cout << " " << v;
			cout << endl;
		}
		cout << "\t\teff:" << endl;
		for(literal l : t.eff){
			cout << "\t\t     " << (l.positive?"+":"-") << " " << color(COLOR_GREEN,l.predicate);
			for(string v : l.arguments) cout << " " << v;
			cout << endl;
		}
		cout << "\t\tconstraints:" << endl;
		for(literal l : t.constraints){
			cout << "\t\t     " << (l.positive?"+":"-") << " " << color(COLOR_CYAN,l.predicate);
			for(string v : l.arguments) cout << " " << v;
			cout << endl;
		}
	}	

	cout << "number of methods: " << methods.size() << endl;
	




	//for (auto def : predicate_definitions){
	//	cout << "Predicate: " << def.name;
	//	for (string arg : def.argument_sorts) cout << " " << arg;
	//	cout << endl;
	//}
}
