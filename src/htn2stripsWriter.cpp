#include <cstdio>
#include <iostream>
#include <vector>
#include <map>
#include <cassert>
#include <algorithm>
#include <unordered_set>
#include "hddlWriter.hpp"
#include "htn2stripsWriter.hpp"
#include "parsetree.hpp"
#include "hddl.hpp"
#include "domain.hpp"
#include "sortexpansion.hpp"
#include "cwa.hpp"
#include "properties.hpp"
#include "util.hpp"

using namespace std;

void htn2strips_output(ostream & dout, ostream & pout){

	auto sanitise = [&](string s){
		if (s[0] == '_') return "t" + s;
		return s;
	};


	set<string> neg_pred;
	for (task t : primitive_tasks) for (literal l : t.prec) if (!l.positive) neg_pred.insert(l.predicate);
	for (task t : primitive_tasks) for (conditional_effect ceff : t.ceff) for (literal l : ceff.condition) if (!l.positive) neg_pred.insert(l.predicate);
	for (auto l : goal) if (!l.positive) neg_pred.insert(l.predicate);


	// TODO do this more intelligently
	dout << "(define (domain d)" << endl;
	dout << "  (:requirements :strips :disjunctive-preconditions :negative-preconditions";
	dout << ")" << endl;
	
	dout << endl;

	
	
	// TODO identical types are not recognised and are treated as a non-dag structure, which is not necessary
	dout << "  (:types" << endl;
	vector<string> sortsInOrder;
	map<int,int> replacedSorts;
	map<string,string> sortOfElement;
	set<string> declaredSorts;
	for (sort_definition s : sort_definitions){
		bool first = true;
		for (string ss : s.declared_sorts){
			if (s.has_parent_sort){
				if (first) dout << "   ";
				first = false;
				dout << " " << ss;
			} else
				dout << "    " << ss << endl;
			
			declaredSorts.insert(ss);
		}
		if (s.has_parent_sort){
			dout << " - " << s.parent_sort, declaredSorts.insert(s.parent_sort);
			dout << endl;
		}
	}	
	
	dout << "  )" << endl;
	
	dout << endl;



	pout << "(define" << endl;
	pout << "  (problem p)" << endl;
	pout << "  (:domain d)" << endl;

	
	// determine which constants need to be declared in the domain
	set<string> constants_in_domain = compute_constants_in_domain();

	if (constants_in_domain.size()) dout << "  (:constants" << endl;
	pout << "  (:objects" << endl;
	for (auto x : sorts) {
		if (! declaredSorts.count(x.first)) continue;
		for (string s : x.second)
			if (!constants_in_domain.count(s))
				pout << "    " << sanitise(s) << " - " << sanitise(x.first) << endl;
			else
				dout << "    " << sanitise(s) << " - " << sanitise(x.first) << endl;
	}

	pout << "  )" << endl;

	if (constants_in_domain.size()) dout << "  )" << endl << endl;




	// predicate definitions
	dout << "  (:predicates" << endl;
	map<string,string> sortReplace;
	map<string,string> typeConstraint;
	for (auto & [s,_] : sorts) sortReplace[s] = s;

	for (auto [originalSort,replacement] : replacedSorts){
		string oldSort = sortsInOrder[originalSort];
		string newSort = sortsInOrder[replacement];
		sortReplace[oldSort] = newSort;
		string predicateName = "p_sort_member_" + sanitise(oldSort);
		typeConstraint[oldSort] = predicateName;
		dout << "    (" << predicateName << " ?x - " << sanitise(newSort) << ")" << endl;
	}

	for (auto p : predicate_definitions) for (int negative = 0; negative < 2; negative++){
		if (negative) continue; // compile only for internal output
		dout << "    (";
		if (negative) dout << "not_";
		dout << sanitise(p.name);

		// arguments
		for (size_t arg = 0; arg < p.argument_sorts.size(); arg++)
			dout << " ?var" << arg << " - " << sanitise(sortReplace[p.argument_sorts[arg]]);
		
		dout << ")" << endl;
	}

	dout << "  )" << endl;
	dout << endl;
	

	bool hasActionCosts = metric_target != dummy_function_type;

	// functions (for cost expressions)
	if (parsed_functions.size() && hasActionCosts){
		dout << "  (:functions" << endl;
		for(auto f : parsed_functions){
			dout << "    (" << sanitise(f.first.name);
			for (size_t arg = 0; arg < f.first.argument_sorts.size(); arg++)
				dout << " ?var" << arg << " - " << sanitise(f.first.argument_sorts[arg]);
			dout << ") - " << f.second << endl;
		}
		dout << "  )" << endl;
        dout << endl;
    }

	// abstract tasks
    dout << "  (:tasks " << endl;
    for (parsed_task a : parsed_abstract){
        dout << "    (";
        dout << sanitise(a.name);
        for (auto [v,s] : a.arguments->vars){
            dout << " ";
            dout << sanitise(v) << " - " << sanitise(sortReplace[s]);
        }
        dout << ")" << endl;
    }
    for (parsed_task a : parsed_primitive){
        dout << "    (";
        dout << sanitise(a.name);
        for (auto [v,s] : a.arguments->vars){
            dout << " ";
            dout << sanitise(v) << " - " << sanitise(sortReplace[s]);
        }
        dout << ")" << endl;
    }
    dout << "  )" << endl;
    dout << endl;

	// decomposition methods
	for (auto [atname,ms] : parsed_methods) for (parsed_method m : ms){
		dout << "  (:method ";
	   	dout << sanitise(m.name) << endl;
		dout << "    :parameters (";
		bool first = true;
		for (auto [v,s] : m.vars->vars){
			if (!first) dout << " ";
			first = false;
			dout << sanitise(v) << " - " << sanitise(sortReplace[s]);
		}
		dout << ")" << endl;

		// AT
		dout << "    :task (";
		dout << sanitise(atname);
		map<string,string> atConstants;
		for (auto [v,s] : m.newVarForAT)
			atConstants[v] = *sorts[s].begin();

		for (string v : m.atArguments) {
			if (atConstants.count(v))
				v = atConstants[v];
			dout << " " << sanitise(v);
		}
		dout << ")" << endl;
		

		if (!m.prec->isEmpty() || !m.tn->constraint->isEmpty()){
			general_formula * precFormula = m.prec;
			if (m.prec->isEmpty())
				precFormula = m.tn->constraint;
			else if (!m.tn->constraint->isEmpty()){
				precFormula = new general_formula();
				precFormula->type = AND;
				precFormula->subformulae.push_back(m.prec);
				precFormula->subformulae.push_back(m.tn->constraint);
			} 
			
			print_formula_for(dout,precFormula,":precondition");
		}

		if (!m.eff->isEmpty())
			print_formula_for(dout,m.eff,":effect");

		// subtasks
		vector<string> liftedTopSort = liftedPropertyTopSort(m.tn);
		if (isTopSortTotalOrder(liftedTopSort,m.tn)){
			dout << "    :tasks (" << endl;
			map<string, sub_task* > idMap;
			for (sub_task* t : m.tn->tasks) idMap[t->id] = t;
			for (string id : liftedTopSort){
				dout << "      (" << sanitise(idMap[id]->task);
				print_var_and_const(dout,*idMap[id]->arguments);
				dout << ")" << endl;
			}
			dout << "    )" << endl;
		} else {
			for (sub_task * task : m.tn->tasks){
				dout << "    :tasks (" << sanitise(task->id) << " (" << sanitise(task->task);
				print_var_and_const(dout,*task->arguments);
				dout << "))" << endl;
			}
			//dout << "    )" << endl;
			if (m.tn->ordering.size()){
				// ordering of subtasks
				dout << "    :ordering (" << endl;
				for (auto p : m.tn->ordering)
					dout << " (" << sanitise(p->first) << " " << sanitise(p->second) << ")" << endl;
				dout << "     )" << endl;
			}
		} 
		
		
		dout << "  )" << endl << endl;
	}
	

	// actions
	for (parsed_task p : parsed_primitive){
		dout << "  (:action " << sanitise(p.name) << endl;
		dout << "    :parameters (";
		bool first = true;
		for (auto [v,s] : p.arguments->vars){
			if (!first) dout << " ";
			first = false;
			dout << sanitise(v) << " - " << sanitise(sortReplace[s]);
		}
		dout << ")" << endl;

        dout << "    :task (";
        dout << sanitise(p.name);
        dout << " ";
        first = true;
        for (auto [v,s] : p.arguments->vars){
            if (!first) dout << " ";
            first = false;
            dout << sanitise(v);
        }
        dout << ")" << endl;


        if (!p.prec->isEmpty())
			print_formula_for(dout,p.prec,":precondition");
		else
			dout << "    :precondition ()" << endl;
		if (!p.eff->isEmpty())
			print_formula_for(dout,p.eff,":effect");
		else	
			dout << "    :effect ()" << endl;

		dout << "  )" << endl;
	}

	dout << ")" << endl;


	bool instance_is_classical = true;
	for (task t : abstract_tasks)
		if (t.name == "__top") instance_is_classical = false;
	
	for (parsed_task t : parsed_abstract)
		if (t.name == "__top") instance_is_classical = false;

	/*
	if (! instance_is_classical){
		pout << "  (:htn" << endl;
		pout << "    :parameters ()" << endl;
		pout << "    :subtasks (and (";
		pout << "t";
		pout << "__top))" << endl;
		pout << "  )" << endl;
	}
	*/

	pout << "  (:init" << endl;
	for (auto gl : init){
		if (!gl.positive) continue; // don't output negatives in normal mode
		pout << "    (";
	   	if (!gl.positive){
		   pout << "not";
		   pout << " (";
		}
		pout << sanitise(gl.predicate);
		for (string c : gl.args) pout << " " << sanitise(c);
		if (!gl.positive) pout << ")";
		pout << ")" << endl;
	}

	for (auto [oldType,predicate] : typeConstraint){
		for (string c : sorts[oldType])
			pout << "    (" << sanitise(predicate) << " " << sanitise(c) << ")" << endl;
	}

	// metric 
	if (hasActionCosts)
		for (auto f : init_functions){
			pout << "    (= (" << sanitise(f.first.predicate);
			for (auto c : f.first.args) pout << " " << sanitise(c);
			pout << ") " << f.second << ")" << endl;
		}

	pout << "  )" << endl;

    if (! instance_is_classical) {
        pout << endl << "  (:tasks ((t__top)))" << endl << endl;
    }

	if (goal_formula != nullptr && !goal_formula->isEmpty()){
		print_formula_for(pout,goal_formula,"(:goal");
		pout << "  )" << endl;
	}

	if (hasActionCosts)
		pout << "  (:metric minimize (" << sanitise(metric_target) << "))" << endl;


	pout << ")" << endl;
}
