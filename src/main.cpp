#include <cstdio>
#include <iostream>
#include <vector>
#include "parsetree.hpp"
#include "hddl.hpp"

using namespace std;

// declare parser function manually
void run_parser_on_file(FILE* f);

// parsed domain data structures
vector<sort_definition> sort_definitions;
vector<predicate_definition> predicate_definitions;


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

	cout << "number of sort defs: " << sort_definitions.size() << endl;

	for (auto def : sort_definitions){
		for (string subsort : def.declared_sorts) cout << subsort << " ";
		if (def.has_parent_sort){
			cout << "- " << def.parent_sort << endl;
		} else {
			cout << " -- no parent sort" << endl;
		}
	}

	for (auto def : predicate_definitions){
		cout << "Predicate: " << def.name;
		for (string arg : def.argument_sorts) cout << " " << arg;
		cout << endl;
	}

}
