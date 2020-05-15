#include <cstdio>
#include <iostream>
#include <vector>
#include <map>
#include <cassert>
#include "hddlWriter.hpp"
#include "parsetree.hpp"
#include "hddl.hpp"
#include "domain.hpp"
#include "sortexpansion.hpp"
#include "cwa.hpp"
#include "util.hpp"

using namespace std;

void hddl_output(ostream & dout, ostream & pout, bool internalHDDLOutput){

	auto sanitise = [&](string s){
		if (internalHDDLOutput) {
			if (s[0] == '_') return "t" + s;
			return s;
		}
		string result = "";
		for (size_t i = 0; i < s.size(); i++){
			char c = s[i];

			if (c == '_' && !i) result += "US";

			if (c == '<') result += "LA_";
			else if (c == '>') result += "RA_";
			else if (c == '[') result += "LB_";
			else if (c == ']') result += "RB_";
			else if (c == '|') result += "BAR_";
			else if (c == ';') result += "SEM_";
			else if (c == ',') result += "COM_";
			else if (c == '+') result += "PLUS_";
			else if (c == '-') result += "MINUS_";
			else result += c;
		}
		return result;
	};


	set<string> neg_pred;
	for (task t : primitive_tasks) for (literal l : t.prec) if (!l.positive) neg_pred.insert(l.predicate);
	for (task t : primitive_tasks) for (conditional_effect ceff : t.ceff) for (literal l : ceff.condition) if (!l.positive) neg_pred.insert(l.predicate);
	for (auto l : goal) if (!l.positive) neg_pred.insert(l.predicate);


	dout << "(define (domain d)" << endl;
	dout << "  (:requirements :typing :hierarchy";
	if (!internalHDDLOutput) dout << " :negative-preconditions";
	dout << ")" << endl;
	
	dout << endl;

	// sorts
	dout << "  (:types" << endl;
	for (auto x : sorts) dout << "    " << sanitise(x.first) << endl;
	dout << "  )" << endl;
	
	dout << endl;

	// predicate definitions
	dout << "  (:predicates" << endl;
	for (auto p : predicate_definitions) for (int negative = 0; negative < 2; negative++){
		if (negative && !internalHDDLOutput) continue; // compile only for internal output
		if (negative && !neg_pred.count(p.name)) continue;
		dout << "    (";
		if (negative) dout << "not_";
		dout << sanitise(p.name);

		// arguments
		for (size_t arg = 0; arg < p.argument_sorts.size(); arg++)
			dout << " ?var" << arg << " - " << sanitise(p.argument_sorts[arg]);
		
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
	}
	dout << endl;

	// abstract tasks
	for (task t : abstract_tasks){
		dout << "  (:task ";
		dout << sanitise(t.name);
		dout << " :parameters (";
		bool first = true;
		for (auto v : t.vars){
			if (!first) dout << " ";
			first = false;
			dout << sanitise(v.first) << " - " << sanitise(v.second);
		}
		dout << "))" << endl;
	}
	
	dout << endl;

	// decomposition methods
	for (method m : methods){
		dout << "  (:method ";
	   	dout << sanitise(m.name) << endl;
		dout << "    :parameters (";
		bool first = true;
		for (auto v : m.vars){
			if (!first) dout << " ";
			first = false;
			dout << sanitise(v.first) << " - " << sanitise(v.second);
		}
		dout << ")" << endl;

		// AT
		dout << "    :task (";
		dout << sanitise(m.at);
		for (string v : m.atargs) dout << " " << sanitise(v);
		dout << ")" << endl;

		// constraints
		if (m.constraints.size()){
			dout << "    :precondition (and" << endl;
			
			for (literal l : m.constraints){
				dout << "      ";
				if (!l.positive) dout << "(not ";
				dout << "(= " << sanitise(l.arguments[0]) << " " << sanitise(l.arguments[1]) << ")";
				if (!l.positive) dout << ")";
				dout << endl;
			}
			
			dout << "    )" << endl;
		}

		// subtasks
		dout << "    :subtasks (and" << endl;
		for (plan_step ps : m.ps){
			dout << "      (x" << sanitise(ps.id) << " (";
			dout << sanitise(ps.task);
			for (string v : ps.args) dout << " " << sanitise(v);
			dout << "))" << endl;
		}
		dout << "    )" << endl;

		
		if (m.ordering.size()){
			// ordering of subtasks
			dout << "    :ordering (and" << endl;
			for (auto o : m.ordering)
				dout << "      (x" << sanitise(o.first) << " < x" << sanitise(o.second) << ")" << endl;
			dout << "    )" << endl;
		}

		dout << "  )" << endl << endl;
	}


	// actions
	for (task t : primitive_tasks){
		dout << "  (:action ";
		dout << sanitise(t.name) << endl;
		dout << "    :parameters (";
		bool first = true;
		for (auto v : t.vars){
			if (!first) dout << " ";
			first = false;
			dout << sanitise(v.first) << " - " << sanitise(v.second);
		}
		dout << ")" << endl;
		
	
		if (t.prec.size() || t.constraints.size()){
			// precondition
			dout << "    :precondition (and" << endl;
			
			for (literal l : t.constraints){
				dout << "      ";
				if (!l.positive) dout << "(not ";
				dout << "(= " << sanitise(l.arguments[0]) << " " << sanitise(l.arguments[1]) << ")";
				if (!l.positive) dout << ")";
				dout << endl;
			}
			
			for (literal l : t.prec){
				string p;
				if (internalHDDLOutput)
					p = (l.positive ? "" : "not_")  + sanitise(l.predicate);
				else
					p = (l.positive ? "" : "not (") + sanitise(l.predicate);

				dout << "      (" << p;
				for (string v : l.arguments) dout << " " << sanitise(v);
				if (!internalHDDLOutput && !l.positive) dout << ")";
				dout << ")" << endl;
			}
			dout << "    )" << endl;
		}

		
		if (t.eff.size() || t.ceff.size() || 
				(hasActionCosts && t.costExpression.size())){
			// effect
			dout << "    :effect (and" << endl;

			for (literal l : t.eff){
				for (int positive = 0; positive < 2; positive ++){
					if ((neg_pred.count(l.predicate) && internalHDDLOutput) || (l.positive == positive)){
						dout << "      (";
						if (!positive) dout << "not (";
						dout << ((l.positive == positive) ? "" : "not_") << sanitise(l.predicate);
						for (string v : l.arguments) dout << " " << sanitise(v);
						if (!positive) dout << ")";
						dout << ")" << endl;
					}
				}
			}

			for (conditional_effect ceff : t.ceff) {
				for (int positive = 0; positive < 2; positive ++){
					if ((neg_pred.count(ceff.effect.predicate) && internalHDDLOutput) || (ceff.effect.positive == positive)){

						dout << "      (when (and";
					
						for (literal l : ceff.condition){
							dout << " (";
							if (!l.positive){
								dout << "not";
								if (internalHDDLOutput) dout << "_";
								else dout << " (";
							}
							dout << sanitise(l.predicate);
							for (string v : l.arguments) dout << " " << sanitise(v);
							if (!l.positive && !internalHDDLOutput) dout << ")";
							dout << ")";
						}

						dout << ") (";

						// actual effect
						if (!positive) dout << "not (";
						dout << ((ceff.effect.positive == positive) ? "" : "not_") << sanitise(ceff.effect.predicate);
						for (string v : ceff.effect.arguments) dout << " " << sanitise(v);
						if (!positive) dout << ")";

						dout << "))" << endl;
					}
				}
			}
			
			if (hasActionCosts){
				for (auto c : t.costExpression){
					dout << "      (increase (" << sanitise(metric_target) << ") ";
					if (c.isConstantCostExpression)
						 dout << c.costValue << ")" << endl;
					else {
						dout << "(" << sanitise(c.predicate);
						for (string v : c.arguments) dout << " " << sanitise(v);
						dout << ")";
					}
					dout << ")" << endl;
				}
			}
	
			dout << "    )" << endl;
		}
	
		dout << "  )" << endl << endl;
	}

	dout << ")" << endl;


	pout << "(define" << endl;
	pout << "  (problem p)" << endl;
	pout << "  (:domain d)" << endl;

	pout << "  (:objects" << endl;
		for (auto x : sorts) for (string s : x.second)
			pout << "    " << sanitise(s) << " - " << sanitise(x.first) << endl;
	pout << "  )" << endl;


	pout << "  (:htn" << endl;
	pout << "    :parameters ()" << endl;
	pout << "    :subtasks (and (";
	if (internalHDDLOutput) pout << "t";
	else pout << "US";
	pout << "__top))" << endl;
	pout << "  )" << endl;

	pout << "  (:init" << endl;
	for (auto gl : init){
		pout << "    (";
	   	if (!gl.positive){
		   pout << "not";
		   if (internalHDDLOutput) pout << "_";
		   else pout << " (";
		}
		pout << sanitise(gl.predicate);
		for (string c : gl.args) pout << " " << c;
		if (!gl.positive && !internalHDDLOutput) pout << ")";
		pout << ")" << endl;
	}

	// metric 
	if (hasActionCosts)
		for (auto f : init_functions){
			pout << "    (= (" << sanitise(f.first.predicate);
			for (auto c : f.first.args) pout << " " << sanitise(c);
			pout << ") " << f.second << ")" << endl;
		}

	pout << "  )" << endl;

	if (goal.size()){
		pout << "  (:goal (and" << endl;
		for (auto gl : goal){
			pout << "    (";
			if (!gl.positive) {
				pout << "not";
				if (internalHDDLOutput) pout << "_";
				else pout << " (";
			}
			pout << sanitise(gl.predicate);
			for (string c : gl.args) pout << " " << c;
			if (!gl.positive && !internalHDDLOutput) pout << ")";
			pout << ")" << endl;
		}
		pout << "  ))" << endl;
	}

	if (hasActionCosts)
		pout << "  (:metric minimize (" << sanitise(metric_target) << "))" << endl;


	pout << ")" << endl;
}
