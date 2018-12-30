#include <cstdio>
#include <iostream>
#include <vector>
#include <map>
#include <cassert>
#include "output.hpp"
#include "parsetree.hpp"
#include "hddl.hpp"
#include "domain.hpp"
#include "sortexpansion.hpp"
#include "util.hpp"

using namespace std;

void verbose_output(int verbosity){
	cout << "number of sorts: " << sorts.size() << endl;
	if (verbosity > 0) for(auto s : sorts){
		cout << "\t" << color(COLOR_RED,s.first) << ":";
		for (string e : s.second) cout << " " << e;
		cout << endl;
	}
	
	cout << "number of predicates: " << predicate_definitions.size() << endl;
	if (verbosity > 0) for (auto def : predicate_definitions){
		cout << "\t" << color(COLOR_RED,def.name);
		for (string arg : def.argument_sorts) cout << " " << arg;
		cout << endl;
	}

	cout << "number of primitives: " << primitive_tasks.size() << endl;
   	if (verbosity > 0) for(task t : primitive_tasks){
		cout << "\t" << color(COLOR_RED, t.name) << endl;
		if (verbosity == 1) continue;
		cout << "\t\tvars:" << endl;
		for(auto v : t.vars) cout << "\t\t     " << v.first << " - " << v.second << endl;
		if (verbosity == 2) continue;
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

	cout << "number of abstracts: " << abstract_tasks.size() << endl;
   	if (verbosity > 0) for(task t : abstract_tasks){
		cout << "\t" << color(COLOR_RED, t.name) << endl;
		if (verbosity == 1) continue;
		cout << "\t\tvars:" << endl;
		for(auto v : t.vars) cout << "\t\t     " << v.first << " - " << v.second << endl;
	}

	cout << "number of methods: " << methods.size() << endl;
	if (verbosity > 0) for (method m : methods) {
		cout << "\t" << color(COLOR_RED, m.name) << endl;
		cout << "\t\tabstract task: " << color(COLOR_BLUE, m.at);
	    for (string v : m.atargs) cout << " " << v;
		cout << endl;	
		if (verbosity == 1) continue;
		cout << "\t\tvars:" << endl;
		for(auto v : m.vars) cout << "\t\t     " << v.first << " - " << v.second << endl;
		if (verbosity == 2) continue;
		cout << "\t\tsubtasks:" << endl;
		for(plan_step ps : m.ps){
			cout << "\t\t     " << ps.id << " " << color(COLOR_GREEN, ps.task);
			for (string v : ps.args) cout << " " << v;
			cout << endl;
		}
		if (verbosity == 3) continue;
		cout << "\t\tordering:" << endl;
		for(auto o : m.ordering)
			cout << "\t\t     " << color(COLOR_YELLOW,o.first) << " < " << color(COLOR_YELLOW,o.second) << endl;
		cout << "\t\tconstraints:" << endl;
		for(literal l : m.constraints){
			cout << "\t\t     " << (l.positive?"+":"-") << " " << color(COLOR_CYAN,l.predicate);
			for(string v : l.arguments) cout << " " << v;
			cout << endl;
		}
	}



}

void simple_hddl_output(){

}

