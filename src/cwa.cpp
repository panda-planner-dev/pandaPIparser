#include "cwa.hpp"
#include "domain.hpp"
#include <cassert>
#include <iostream>

vector<ground_literal> init;
vector<pair<ground_literal,int>> init_functions;
vector<ground_literal> goal;
general_formula* goal_formula = NULL;
general_formula* constraint_formula = NULL;
vector<pair<string,vector<ground_literal>>> preferences;

bool operator< (const ground_literal& lhs, const ground_literal& rhs){
	if (lhs.predicate < rhs.predicate) return true;
	if (lhs.predicate > rhs.predicate) return false;
	
	if (lhs.positive < rhs.positive) return true;
	if (lhs.positive > rhs.positive) return false;
	
	if (lhs.args < rhs.args) return true;
	if (lhs.args > rhs.args) return false;

	return false; // equal
}

void flatten_goal(){
	if (goal_formula != NULL) {
		vector<pair<pair<vector<variant<literal,conditional_effect>>,vector<literal> >, additional_variables> > ex = goal_formula->expand(false);
		assert(ex.size() == 1);
		assert(ex[0].first.second.size() == 0);
		map<string,string> access;
		for (auto x : ex[0].second){
			string sort = x.second;
			assert(sorts[sort].size() == 1); // must be an actual constant
			access[x.first] = *sorts[sort].begin();
		}

		for (variant<literal,conditional_effect> l : ex[0].first.first){
			if (holds_alternative<conditional_effect>(l))
				assert(false); // goal may not contain conditional effects

			ground_literal gl;
			gl.predicate = get<literal>(l).predicate;
			gl.positive = get<literal>(l).positive;
			for (string v : get<literal>(l).arguments) gl.args.push_back(access[v]);
			goal.push_back(gl);
		}
	}


	// process the problem constraints. This can contain a cost bound, preferences, and further goals
	// TODO here we would have to process LTL formulae in the future
	if (constraint_formula != NULL){
		additional_variables vars;
		vector<general_formula*> subformulae = constraint_formula->expandQualifiedCondGD(vars);

		for (general_formula * cf : subformulae){
			if (cf->type == PREFERENCE){
				string name = cf->arg1;

				assert(cf->subformulae[0]->type == ATEND);

				general_formula * endformula = cf->subformulae[0]->subformulae[0];

				vector<pair<pair<vector<variant<literal,conditional_effect>>,vector<literal> >, additional_variables> > ex = endformula->expand(false);
				// disjunctions are ok in utilities
				for (auto expansion : ex){
					assert(expansion.first.second.size() == 0);
					map<string,string> access;
					for (auto x : expansion.second){
						string sort = x.second;
						assert(sorts[sort].size() == 1); // must be an actual constant
						access[x.first] = *sorts[sort].begin();
					}
					for (auto x : vars){
						string sort = x.second;
						assert(sorts[sort].size() == 1); // must be an actual constant
						access[x.first] = *sorts[sort].begin();
					}
			
					vector<ground_literal> conditions;
			
					for (variant<literal,conditional_effect> l : expansion.first.first){
						if (holds_alternative<conditional_effect>(l))
							assert(false); // goal may not contain conditional effects
			
						ground_literal gl;
						gl.predicate = get<literal>(l).predicate;
						gl.positive = get<literal>(l).positive;
						for (string v : get<literal>(l).arguments) gl.args.push_back(access[v]);
						conditions.push_back(gl);
					}
					preferences.push_back({name, conditions});
				}
			} else {
				assert(cf->type == ATEND);
				general_formula* end_formula = cf->subformulae[0];
				
				vector<pair<pair<vector<variant<literal,conditional_effect>>,vector<literal> >, additional_variables> > ex = end_formula->expand(false);
				// disjunctions are ok in utilities
				for (auto expansion : ex){
					map<string,string> access;
					for (auto x : expansion.second){
						string sort = x.second;
						assert(sorts[sort].size() == 1); // must be an actual constant
						access[x.first] = *sorts[sort].begin();
					}
					for (auto x : vars){
						string sort = x.second;
						assert(sorts[sort].size() == 1); // must be an actual constant
						access[x.first] = *sorts[sort].begin();
					}

					for (variant<literal,conditional_effect> l : expansion.first.first){
						if (holds_alternative<conditional_effect>(l))
							assert(false); // goal may not contain conditional effects
					
						if (get<literal>(l).isCostCompareExpression){
							cost_bound = get<literal>(l).costValue;
							metric_target = get<literal>(l).predicate;
						} else {
							// this is a true state-based goal so write it as such
							ground_literal gl;
							gl.predicate = get<literal>(l).predicate;
							gl.positive = get<literal>(l).positive;
							for (string v : get<literal>(l).arguments) gl.args.push_back(access[v]);
							goal.push_back(gl);
						}
					}
				}
			}
		}
	}
}


map<string,vector<int> > int_sorts;
map<string,int> const_int;
vector<string> int_const;


// fully instantiate predicate via recursion
void fully_instantiate(int * inst, unsigned int pos, vector<string> & arg_sorts, vector<vector<int>> & result){
	if (pos == arg_sorts.size()){
		vector<int> instance(inst,inst + arg_sorts.size());
		result.push_back(instance);
		return;
	}

	for (int c : int_sorts[arg_sorts[pos]]) {
		inst[pos] = c;
		fully_instantiate(inst,pos+1,arg_sorts,result);
	}
}


void compute_cwa(){
	// transform sort representation to ints
	for (auto & s : sorts){
		for (string c : s.second){
			auto it = const_int.find(c);
			int id;
			if (it == const_int.end()) {
				id = const_int.size();
				const_int[c] = id;
				int_const.push_back(c);
			} else id = it->second;
			int_sorts[s.first].push_back(id);
		}
	}


	// find predicates occurring negatively in preconditions and their types
	map<string,set<vector<string>>> neg_predicates_with_arg_sorts;
	
	for (task t : primitive_tasks) {
		vector<literal> literals = t.prec;
		for (conditional_effect ceff : t.ceff)
			for (literal l : ceff.condition)
				literals.push_back(l);
		
		for (literal l : literals) if (!l.positive) {
			vector<string> argSorts;
			for (string v : l.arguments) for (auto x : t.vars) if (x.first == v) argSorts.push_back(x.second);
			assert(argSorts.size() == l.arguments.size());
			neg_predicates_with_arg_sorts[l.predicate].insert(argSorts);
		}
	}
	
	// predicates negative in goal
	for (auto l : goal) if (!l.positive) {
		vector<string> args;
		for (string c : l.args) args.push_back(sort_for_const(c));
		neg_predicates_with_arg_sorts[l.predicate].insert(args);
	} 

	map<string,set<vector<int>>> init_check;
	for(auto l : init){
		vector<int> args;
		for (string & arg : l.args) args.push_back(const_int[arg]);
		init_check[l.predicate].insert(args);
	}

	for (auto np : neg_predicates_with_arg_sorts){
		set<vector<int>> instantiations;

		for (vector<string> arg_sorts : np.second){
			vector<vector<int>> inst;
			int * partial_instance = new int[arg_sorts.size()];
			fully_instantiate(partial_instance, 0, arg_sorts, inst);
			delete[] partial_instance;

			for (vector<int> & p : inst) if (instantiations.count(p) == 0){
				instantiations.insert(p);
				if (init_check[np.first].count(p)) continue;
				ground_literal lit; lit.predicate = np.first;
				for (int arg : p) lit.args.push_back(int_const[arg]);
				lit.positive = false;
				init.push_back(lit);
			}
		}
	}
}
