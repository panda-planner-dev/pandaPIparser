#include <cstdio>
#include <iostream>
#include <vector>
#include <map>
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


map<string,set<string> > sorts;

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
		set<string> allSorts;
		for(auto s : sorts) allSorts.insert(s.first);
		sorts["Type"] = allSorts;
		// TODO predicate
	}


	cout << "number of sorts: " << sorts.size() << endl;
	/*for(auto s : sorts){
		cout << s.first << ":";
		for (string e : s.second) cout << " " << e;
		cout << endl;
	}*/

	cout << endl << "number of primitive: " << parsed_primitive.size() << endl;
	for(parsed_task a : parsed_primitive){
		cout << a.name << endl;
		vector<pair<vector<literal>, additional_variables> > ex = a.prec->expand();
		cout << "\tnumber of expansions: " << ex.size() << endl;
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
	}
	
		
	//for (auto def : predicate_definitions){
	//	cout << "Predicate: " << def.name;
	//	for (string arg : def.argument_sorts) cout << " " << arg;
	//	cout << endl;
	//}
}
