#include "cwa.hpp"
#include "domain.hpp"
#include <cassert>
#include <iostream>

vector<ground_literal> init;

vector<ground_literal> compute_cwa(){
	// find predicates occuring negatively in preconditions and their types
	map<string,set<vector<string>>> neg_predicates_with_arg_sorts;
	
	for (task t : primitive_tasks) for (literal l : t.prec) if (!l.positive) {
		vector<string> argSorts;
		for (string v : l.arguments) for (auto x : t.vars) if (x.first == v) argSorts.push_back(x.second);
		assert(argSorts.size() == l.arguments.size());
		neg_predicates_with_arg_sorts[l.predicate].insert(argSorts);
	}

	map<string,set<vector<string>>> init_check;
	for(auto l : init)
		init_check[l.predicate].insert(l.args);
	
	vector<ground_literal> ret = init;
	for (auto np : neg_predicates_with_arg_sorts){
		set<vector<string>> instantiations;
		for (vector<string> arg_sorts : np.second){
			vector<vector<string>> inst;
			vector<string> __empty; inst.push_back(__empty);

			for (string s : arg_sorts) {
				vector<vector<string>> prev = inst; inst.clear();
				for (string c : sorts[s]) for (vector<string> p : prev) {
					p.push_back(c); inst.push_back(p);
				}
			}
			
			for (vector<string> p : inst) instantiations.insert(p);
		}
		for (vector<string> p : instantiations){
			if (init_check[np.first].count(p)) continue;
			ground_literal lit; lit.predicate = np.first;
			lit.args = p;
			lit.positive = false;
			ret.push_back(lit);
		}
	}

	return ret;
}
