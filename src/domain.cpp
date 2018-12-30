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
			t.check_integrety();
			primitive_tasks.push_back(t);	
		}
		for(parsed_task a : parsed_abstract){
			task at;
			at.name = a.name;
			at.vars = a.arguments->vars;
			abstract_tasks.push_back(at);
		}
	}
}

void parsed_method_to_data_structures(){
	for (auto e : parsed_methods) for (parsed_method pm : e.second){
		method m;
		m.name = pm.name;
		m.at = e.first;

		methods.push_back(m);
	}

}


void task::check_integrety(){
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

