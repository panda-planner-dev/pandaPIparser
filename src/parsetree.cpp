#include "parsetree.hpp"
#include <iostream>
#include <cassert>

void general_formula::negate(){
	if (this->type == EMPTY) return;
	else if (this->type == AND) this->type = OR;
	else if (this->type == OR) this->type = AND;
	else if (this->type == FORALL) this->type = EXISTS;
	else if (this->type == EXISTS) this->type = FORALL;
	else if (this->type == EQUAL) this->type = NOTEQUAL;
	else if (this->type == NOTEQUAL) this->type = EQUAL;
	else if (this->type == ATOM) this->type = NOTATOM;
	else if (this->type == NOTATOM) this->type = ATOM;

	for(auto sub : this->subformulae) sub->negate();
}

string sort_for_const(string c){
	string s = "sort_for_" + c;
	sorts[s].insert(c);
	return s;
}


// hard expansion of formulae. This can grow up to exponentially, but is currently the only thing we can do about disjunctions.
// this will also handle forall and exists quantors by expansion
// sorts must have been parsed and expanded prior to this call
vector<pair<vector<literal>, additional_variables> > general_formula::expand(){
	vector<pair<vector<literal>, additional_variables> > ret;

	if (this->type == EMPTY || (this-subformulae.size() == 0 &&
				(this->type == AND || this->type == OR || this->type == FORALL || this->type == EXISTS))){
		vector<literal> empty; additional_variables none;
		ret.push_back(make_pair(empty,none));
	}
	
	vector<vector<pair<vector<literal>, additional_variables> > > subresults;
	for(auto sub : this->subformulae) subresults.push_back(sub->expand());	
	
	// just add all disjuncts to set of literals
	if (this->type == OR) for(auto subres : subresults) for (auto res: subres) ret.push_back(res);

	//
	if (this->type == AND){
		vector<pair<vector<literal>, additional_variables> > cur = subresults[0];
		for(unsigned int i = 1; i < subresults.size(); i++){
			vector<pair<vector<literal>, additional_variables> > prev = cur;
			cur.clear();
			for(auto next : subresults[i]) for(auto old : prev){
				pair<vector<literal>, additional_variables>	combined = old;
				for(literal l : next.first) combined.first.push_back(l);
				for(auto v : next.second) combined.second.insert(v);
				cur.push_back(combined);
			}
		}
		ret = cur;
	}

	// add additional variables for every quantified variable. We have to do this for every possible instance of the precondition below	
	if (this->type == EXISTS){
		vector<pair<vector<literal>, additional_variables> > cur = subresults[0];	
		for(pair<string,string> var : this->qvariables.vars){
			vector<pair<vector<literal>, additional_variables> > prev = cur;
			cur.clear();
			for(auto old : prev){
				pair<vector<literal>, additional_variables>	combined = old;
				combined.second.insert(var);
				cur.push_back(combined);
			}
		}
		ret = cur;
		// we cannot generate more possible expansions.
		assert(ret.size() == subresults[0].size());
	}

	// generate a big conjunction for all
	if (this->type == FORALL){
		vector<pair<vector<literal>, additional_variables> > bef = subresults[0];
		for(auto old : bef){
			int counter = 0;
			vector<literal> nl = old.first;
			additional_variables avs = old.second;
			for(pair<string,string> var : this->qvariables.vars) {
				vector<literal> oldL = nl;
				nl.clear();
				for(string c : sorts[var.second]){
					string newSort = sort_for_const(c);
					string newVar = var.first + "_" + to_string(counter); counter++;
					avs.insert(make_pair(newVar,newSort));
					for(literal l : oldL){
						for(unsigned int i = 0; i < l.arguments.size(); i++)
							if (l.arguments[i] == var.first) l.arguments[i] = newVar;
						nl.push_back(l);
					}
				}
			}
			ret.push_back(make_pair(nl,avs));
		}
		// we cannot generate more possible expansions.
		assert(ret.size() == subresults[0].size());
	}


	if (this->type == ATOM || this->type == NOTATOM) {
		vector<literal> ls;
		literal l;
		l.positive = this->type == ATOM;
		l.predicate = this->predicate;
		l.arguments = this->arguments.vars;
		ls.push_back(l);

		additional_variables vars = this->arguments.newVar;
		ret.push_back(make_pair(ls,vars));	
	}
	// add dummy literal for equal and not equal constraints
	if (this->type == EQUAL || this->type == NOTEQUAL){
		vector<literal> ls;
		literal l;
		l.positive = this->type == EQUAL;
		l.predicate = dummy_equal_literal;
		l.arguments.push_back(this->arg1);
		l.arguments.push_back(this->arg2);
		ls.push_back(l);

		additional_variables vars; // no new vars. Never
		ret.push_back(make_pair(ls,vars));	
		
	}

	return ret;
}

