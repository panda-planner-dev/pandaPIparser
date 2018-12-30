#include "domain.hpp"
#include "parsetree.hpp"
#include "util.hpp"
#include <iostream>
#include <cassert>

using namespace std;

void flatten_tasks(){
	for(parsed_task a : parsed_primitive){
		// expand the effects first, as they will contain only one element
		vector<pair<vector<literal>, additional_variables> > elist = a.eff->expand();
		assert(elist.size() == 1); // can't handle disjuctive effects
			
		vector<pair<vector<literal>, additional_variables> > plist = a.prec->expand();
		int i = 0;
		for(auto p : plist){
			task t; i++;
			t.name = a.name;
			// sort out the constraints
			for(literal pl : p.first)
				if (pl.predicate == dummy_equal_literal)
					t.constraints.push_back(pl);
				else
					t.prec.push_back(pl);
			t.eff = elist[0].first;
			
			// add declared vars
			t.vars = a.arguments->vars;
			// gather the additional variables
			additional_variables addVars = p.second;
			for (auto elem : elist[0].second) addVars.insert(elem);
			for (auto v : addVars) t.vars.push_back(v);
			
			if (plist.size() > 1) {
				t.name += "_instance_" + to_string(i);
				// we have to create a new decomposition method at this point
				method m;
				m.name = "method_for_multiple_expansions_of_" + t.name;
				m.at = a.name;
				for(auto v : a.arguments->vars) m.atargs.push_back(v.first);
				m.vars = t.vars;
				plan_step ps;
				ps.task = t.name;
				ps.id = "id0";
				for(auto v : m.vars) ps.args.push_back(v.first);
				m.ps.push_back(ps);

				methods.push_back(m);
				// for this to be ok, we have to create an abstract task in the first round
				if (i == 1){
					task at;
					at.name = a.name;
					at.vars = a.arguments->vars;
					abstract_tasks.push_back(at);
				}
			}
			
			// add to list
			t.check_integrity();
			primitive_tasks.push_back(t);	
		}
		for(parsed_task a : parsed_abstract){
			task at;
			at.name = a.name;
			at.vars = a.arguments->vars;
			abstract_tasks.push_back(at);
		}
	}
	for(task t : primitive_tasks) task_name_map[t.name] = t;
	for(task t : abstract_tasks) task_name_map[t.name] = t;
}

void parsed_method_to_data_structures(){
	int i = 0;
	for (auto e : parsed_methods) for (parsed_method pm : e.second) {
		// compute flattening of method precondition
		vector<pair<vector<literal>, additional_variables> > precs = pm.prec->expand();
		for (auto prec : precs){
			method m; i++;
			m.name = pm.name; if (precs.size() > 1) m.name += "_" + to_string(i);
			m.at = e.first;
			m.atargs = pm.atArguments;
			
			// collect all the variable
			m.vars = pm.vars->vars;
			// variables from precondition
			map<string,string> mprec_additional_vars;
			for (pair<string, string> av : prec.second){
				mprec_additional_vars[av.first] = av.first + "_" + to_string(i++);
				m.vars.push_back(make_pair(mprec_additional_vars[av.first],av.second));
			}

			// subtasks
			for(sub_task* st : pm.tn->tasks){
				plan_step ps;
				ps.id = st->id;
				ps.task = st->task;
				map<string,string> arg_replace;
				for (auto av : st->arguments->newVar){
					arg_replace[av.first] = av.first + "_" + to_string(i++);
					m.vars.push_back(make_pair(arg_replace[av.first],av.second));
				}
				for (string v : st->arguments->vars)
					if (arg_replace.count(v))
						ps.args.push_back(arg_replace[v]);
					else
						ps.args.push_back(v);

				// we might have added more parameters to these tasks to account for constants in them. We have to add them here
				task psTask = task_name_map[ps.task];
				assert(psTask.name == ps.task); // ensure that we have found one
				for (unsigned int j = st->arguments->vars.size(); j < psTask.vars.size(); j++){
					string v = psTask.vars[j].first + "_method_" + m.name + "_instance_" + to_string(i++);
					m.vars.push_back(make_pair(v,psTask.vars[j].second)); // add var to set of vars
					ps.args.push_back(v);
				}

				m.ps.push_back(ps);
			}

			// add a subtask for the method precondition
			vector<literal> mPrec;
			for (literal l : prec.first)
				if (l.predicate == dummy_equal_literal || l.predicate == dummy_ofsort_literal)
					m.constraints.push_back(l);
				else mPrec.push_back(l);

			map<string,string> sorts_of_vars; for(auto pp : m.vars) sorts_of_vars[pp.first] = pp.second;

			// actually have method precondition
			if (mPrec.size()){
				task t;
				t.name = "method_precondition_" + m.name;
				t.prec = mPrec;
				set<string> args;
				for (literal l : t.prec) for (string a : l.arguments) args.insert(a);
				// get types of vars
				for (string v : args) {
					string accessV = v;
					if (mprec_additional_vars.count(v)) accessV = mprec_additional_vars[v];
					t.vars.push_back(make_pair(v,sorts_of_vars[accessV]));
				}
				// add t as a new primitive task
				t.check_integrity();
				primitive_tasks.push_back(t);
				task_name_map[t.name] = t;
				
				plan_step ps;
				ps.task = t.name;
				ps.id = "__m-prec-id";
				for(auto v : t.vars) {
					string arg = v.first;
					if (mprec_additional_vars.count(arg)) mprec_additional_vars[arg];
					ps.args.push_back(arg);
				}

				// precondition ps precedes all other tasks
				for (plan_step other_ps : m.ps)
					m.ordering.push_back(make_pair(ps.id,other_ps.id));
				m.ps.push_back(ps);
			}

			// ordering
			for(auto o : pm.tn->ordering) m.ordering.push_back(*o);

			// constraints
			vector<pair<vector<literal>, additional_variables> > exconstraints = pm.tn->constraint->expand();
			assert(exconstraints.size() == 1);
			assert(exconstraints[0].second.size() == 0); // no additional vars due to constraints
			for(literal l : exconstraints[0].first) m.constraints.push_back(l);

			// remove sort constraints
			vector<literal> oldC = m.constraints;
			m.constraints.clear();
			for(literal l : oldC) if (l.predicate == dummy_equal_literal) m.constraints.push_back(l); else {
				// determine the now forbidden variables
				set<string> currentVals = sorts[sorts_of_vars[l.arguments[0]]];
				set<string> sortElems = sorts[l.arguments[1]];
				vector<string> forbidden;
				if (l.positive) {
					for (string s : currentVals) if (sortElems.count(s) == 0) forbidden.push_back(s);
				} else
					for (string s : sortElems) forbidden.push_back(s);
				for(string f : forbidden){
					literal nl;
					nl.positive = false;
					nl.predicate = dummy_equal_literal;
					nl.arguments.push_back(l.arguments[0]);
					nl.arguments.push_back(f);
					m.constraints.push_back(nl);
				}
			}


			m.check_integrity();
			methods.push_back(m);
		}
	}

}


void task::check_integrity(){
	for(auto v : this->vars)
		assert(v.second.size() != 0); // variables must have a sort
	
	for(literal l : this->prec) {
		bool hasPred = false;
		for(auto p : predicate_definitions) if (p.name == l.predicate) hasPred = true;
		assert(hasPred);
		
		for(string v : l.arguments) {
			bool hasVar = false;
			for(auto mv : this->vars) if (mv.first == v) hasVar = true;
			assert(hasVar);
		}
	}

	for(literal l : this->eff) {
		bool hasPred = false;
		for(auto p : predicate_definitions) if (p.name == l.predicate) hasPred = true;
		assert(hasPred);
		
		for(string v : l.arguments) {
			bool hasVar = false;
			for(auto mv : this->vars) if (mv.first == v) hasVar = true;
			assert(hasVar);
		}
	}

}

void method::check_integrity(){
	for (plan_step ps : this->ps){
		task t = task_name_map[ps.task];
		assert(ps.args.size() == t.vars.size());

		for (string v : ps.args) {
			bool found = false;
			for (auto vd : this->vars) found |= vd.first == v;
			assert(found);
		}
	}
}
