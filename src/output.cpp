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
#include "cwa.hpp"
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

	cout << "Initial state: " << init.size() << endl;
	if (verbosity > 0) for(ground_literal l : init){
		cout << "\t" << (l.positive?"+":"-")<< color(COLOR_BLUE,l.predicate);
		for(string c : l.args) cout << " " << c;
		cout << endl;
	}
	
	cout << "Goal state: " << goal.size() << endl;
	if (verbosity > 0) for(ground_literal l : goal){
		cout << "\t" << (l.positive?"+":"-")<< color(COLOR_BLUE,l.predicate);
		for(string c : l.args) cout << " " << c;
		cout << endl;
	}
}

void simple_hddl_output(){
	// prep indices
	map<string,int> constants;
	vector<string> constants_out;
	for (auto x : sorts) for (string s : x.second) {
		if (constants.count(s) == 0) constants[s] = constants.size(), constants_out.push_back(s);
	}

	map <string,int> sort_id;
	vector<pair<string,set<string>>> sort_out;
	for (auto x : sorts) if (!sort_id.count(x.first))
		sort_id[x.first] = sort_id.size(), sort_out.push_back(x);

	set<string> neg_pred;
	for (task t : primitive_tasks) for (literal l : t.prec) if (!l.positive) neg_pred.insert(l.predicate);
	for (auto l : goal) if (!l.positive) neg_pred.insert(l.predicate);

	map<string,int> predicates;
	vector<pair<string,predicate_definition>> predicate_out;
	for (auto p : predicate_definitions){
		predicates["+" + p.name] = predicates.size();
		predicate_out.push_back(make_pair("+" + p.name, p));

		if (neg_pred.count(p.name)){
			predicates["-" + p.name] = predicates.size();
			predicate_out.push_back(make_pair("-" + p.name, p));
		}
	}

	map<string,int> task_id;
	vector<pair<task,bool>> task_out;
	for (task t : primitive_tasks){
		task_id[t.name] = task_id.size();
		task_out.push_back(make_pair(t,true));
	}
	for (task t : abstract_tasks){
		task_id[t.name] = task_id.size();
		task_out.push_back(make_pair(t,false));
	}

	// write domain to std out
	cout << "#number_constants_number_sorts" << endl;
	cout << constants.size() << " " << sorts.size() << endl;
	cout << "#constants" << endl;
	for (string c : constants_out) cout << c << endl;
	cout << "#end_constants" << endl;
	cout << "#sorts_each_with_number_of_members_and_members" << endl;
	for(auto s : sort_out) {
		cout << s.first << " " << s.second.size();
		for (auto c : s.second) cout << " " << constants[c];
		cout << endl;	
	}
	cout << "#end_sorts" << endl;
	cout << "#number_of_predicates" << endl;
	cout << predicate_out.size() << endl;
	cout << "#predicates_each_with_number_of_arguments_and_argument_sorts" << endl;
	for(auto p : predicate_out){
		cout << p.first << " " << p.second.argument_sorts.size();
		for(string s : p.second.argument_sorts) assert(sort_id.count(s)), cout << " " << sort_id[s];
		cout << endl;
	}
	cout << "#end_predicates" << endl;
	cout << "#number_primitiv_tasks_and_number_abstract_tasks" << endl;
	cout << primitive_tasks.size() << " " << abstract_tasks.size() << endl;

	for (auto tt : task_out){
		task t = tt.first;
		cout << "#begin_task_name_costs_number_of_variables" << endl;
		cout << t.name << " " << 1 << " " << t.vars.size() << endl;
		cout << "#sorts_of_variables" << endl;
		map<string,int> v_id;
		for (auto v : t.vars) assert(sort_id.count(v.second)), cout << sort_id[v.second] << " ", v_id[v.first] = v_id.size();
		cout << endl;
		cout << "#end_variables" << endl;
		
		if (tt.second){
			cout << "#preconditions_each_predicate_and_argument_variables" << endl;
			cout << t.prec.size() << endl;
			for (literal l : t.prec){
				string p = (l.positive ? "+" : "-") + l.predicate;
				cout << predicates[p];
				for (string v : l.arguments) cout << " " << v_id[v];
				cout << endl;
			}
	
			// determine number of add and delete effects
			int add = 0, del = 0;
			for (literal l : t.eff){
				if (neg_pred.count(l.predicate)) add++,del++;
				else if (l.positive) add++;
				else del++;
			}
			cout << "#add_each_predicate_and_argument_variables" << endl;
			cout << add << endl;
			for (literal l : t.eff){
				if (!neg_pred.count(l.predicate) && !l.positive) continue;
				string p = (l.positive ? "+" : "-") + l.predicate;
				cout << predicates[p];
				for (string v : l.arguments) cout << " " << v_id[v];
				cout << endl;
			}
			
			cout << "#del_each_predicate_and_argument_variables" << endl;
			cout << del << endl;
			for (literal l : t.eff){
				if (!neg_pred.count(l.predicate) && l.positive) continue;
				string p = (l.positive ? "-" : "+") + l.predicate;
				cout << predicates[p];
				for (string v : l.arguments) cout << " " << v_id[v];
				cout << endl;
			}
	
			cout << "#variable_constaints_first_number_then_individual_constraints" << endl;
			cout << t.constraints.size() << endl;
			for (literal l : t.constraints){
				if (!l.positive) cout << "!";
				cout << "= " << v_id[l.arguments[0]] << " " << v_id[l.arguments[1]] << endl;
				assert(l.arguments[1][0] == '?'); // cannot be a constant
			}
		}
		cout << "#end_of_task" << endl;
	}
	
	cout << "#number_of_methods" << endl;
	cout << methods.size() << endl;

	for (method m : methods){
		cout << "#begin_method_name_abstract_task_number_of_variables" << endl;
		cout << m.name << " " << task_id[m.at] << " " << m.vars.size() << endl;
		cout << "#variable_sorts" << endl;
		map<string,int> v_id;
		for (auto v : m.vars) assert(sort_id.count(v.second)), cout << sort_id[v.second] << " ", v_id[v.first] = v_id.size();
		cout << endl;
		cout << "#parameter_of_abstract_task" << endl;
		for (string v : m.atargs) cout << v_id[v] << " ";
		cout << endl;
		cout << "#number_of_subtasks" << endl;
		cout << m.ps.size() << endl;
		cout << "#subtasks_each_with_task_id_and_parameter_variables" << endl;
		map<string,int> ps_id;
		for (plan_step ps : m.ps){
			ps_id[ps.id] = ps_id.size();
			cout << task_id[ps.task];
			for (string v : ps.args) cout << " " << v_id[v];
			cout << endl;
		}
		cout << "#number_of_ordering_constraints_and_ordering" << endl;
		cout << m.ordering.size() << endl;
		for (auto o : m.ordering) cout << ps_id[o.first] << " " << ps_id[o.second] << endl;

		cout << "#variable_constraints" << endl;
		cout << m.constraints.size() << endl;
		for (literal l : m.constraints){
			if (!l.positive) cout << "!";
			cout << "= " << v_id[l.arguments[0]] << " " << v_id[l.arguments[1]] << endl;
			assert(l.arguments[1][0] == '?'); // cannot be a constant
		}
		cout << "#end_of_method" << endl;
	}

	cout << "#init_and_goal_facts" << endl;
	cout << init.size() << " " << goal.size() << endl;
	for (auto gl : init){
		string pn = (gl.positive ? "+" : "-") + gl.predicate;
		assert(predicates.count(pn) != 0);
		cout << predicates[pn];
		for (string c : gl.args) cout << " " << constants[c];
		cout << endl;
	}
	cout << "#end_init" << endl;
	for (auto gl : goal){
		string pn = (gl.positive ? "+" : "-") + gl.predicate;
		assert(predicates.count(pn) != 0);
		cout << predicates[pn];
		for (string c : gl.args) cout << " " << constants[c];
		cout << endl;
	}
	cout << "#end_goal" << endl;
	cout << "#initial_task" << endl;
	cout << task_id["__top"] << endl;
}

