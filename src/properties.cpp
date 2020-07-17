#include <iostream>
#include <cassert>
#include <algorithm>

#include "properties.hpp"
#include "parsetree.hpp"

using namespace std;

vector<string> liftedTopSort;

void liftedPropertyTopSortDFS(string cur, map<string,vector<string>> & adj, map<string, int> & colour){
	assert (colour[cur] != 1);
	if (colour[cur]) return;

	colour[cur] = 1;
	for (string & nei : adj[cur]) liftedPropertyTopSortDFS(nei,adj,colour);
	colour[cur] = 2;

	liftedTopSort.push_back(cur);
}

void liftedPropertyTopSort(parsed_task_network* tn){
	liftedTopSort.clear();
	map<string,vector<string>> adj;
	for (pair<string,string> * nei : tn->ordering)
		adj[nei->first].push_back(nei->second);

	map<string,int> colour;

	for (sub_task* t : tn->tasks)
		if (!colour[t->id]) liftedPropertyTopSortDFS(t->id, adj, colour);

	reverse(liftedTopSort.begin(), liftedTopSort.end());
}

bool recursionFindingDFS(string cur, map<string,int> & colour){
	if (colour[cur] == 1) return true;
	if (colour[cur]) return false;

	colour[cur] = 1;
	for (parsed_method & m : parsed_methods[cur])
		for (sub_task* sub : m.tn->tasks)
			if (recursionFindingDFS(sub->task,colour)) return true;
	colour[cur] = 2;

	return false;
}


bool isRecursiveParentSort(string current, string target){
	if (current == target) return true;
	for (sort_definition & sd : sort_definitions){
		for (string & ss : sd.declared_sorts)
			if (current == ss){
				if (!sd.has_parent_sort) continue;
				if (isRecursiveParentSort(sd.parent_sort, target)) return true;
			}
	}
	return false;
}

void printProperties(){
	// determine lifted instance properties and print them
	
	// 1. Total order
	bool totalOrder = true;
	for (auto & [_,ms] : parsed_methods)
		for (auto & m : ms){
			if (m.tn->tasks.size() < 2) continue;
			// do topsort
			liftedPropertyTopSort(m.tn);

			// check whether it is a total order
			for (size_t i = 1; i < liftedTopSort.size(); i++){
				bool orderEnforced = false;
				for (pair<string,string> * nei : m.tn->ordering)
					if (nei->first == liftedTopSort[i-1] && nei->second == liftedTopSort[i]){
						orderEnforced = true;
						break;
					}

				if (! orderEnforced){
					totalOrder = false;
					break;
				}
			}
			if (! totalOrder) break;
		}


	cout << "Instance is totally-ordered: ";
	if (totalOrder) cout << "yes" << endl; else cout << "no" << endl;


	// 2. recursion
	map<string,int> colour;
	bool hasLiftedRecursion = recursionFindingDFS("__top",colour);
	
	cout << "Instance is acyclic:         ";
	if (!hasLiftedRecursion) cout << "yes" << endl; else cout << "no" << endl;

	// requirements
	cout << "Requirements:" << endl;
	cout << "\t:hierarchy" << endl;
	if (sort_definitions.size()) cout << "\t:typing" << endl;	
	bool hasEquals = false;
	for (auto & [_,ms] : parsed_methods)
		for (auto & m : ms)
			hasEquals |= m.prec->hasEquals() || m.eff->hasEquals();
	for (auto & p : parsed_primitive)
		hasEquals |= p.prec->hasEquals() || p.eff->hasEquals();
	if (hasEquals) cout << "\t:equality" << endl;


	bool exists_prec = false;
	for (auto & [_,ms] : parsed_methods)
		for (auto & m : ms)
			exists_prec |= m.prec->hasExists();
	for (auto & p : parsed_primitive)
		exists_prec |= p.prec->hasExists();
	if (exists_prec) cout << "\t:existential-precondition" << endl;

	bool forall_prec = false;
	for (auto & [_,ms] : parsed_methods)
		for (auto & m : ms)
			forall_prec |= m.prec->hasForall();
	for (auto & p : parsed_primitive)
		forall_prec |= p.prec->hasForall();
	if (forall_prec) cout << "\t:universal-precondition" << endl;

	bool forall_eff = false;
	for (auto & [_,ms] : parsed_methods)
		for (auto & m : ms)
			forall_eff |= m.eff->hasForall();
	for (auto & p : parsed_primitive)
		forall_eff |= p.eff->hasForall();
	if (forall_eff) cout << "\t:universal-effect" << endl;
	
	
	bool method_preconditon = false;
	for (auto & [_,ms] : parsed_methods)
		for (auto & m : ms)
			method_preconditon |= !m.prec->isEmpty();
	if (method_preconditon) cout << "\t:method-precondition" << endl;


	for (auto & [_,ms] : parsed_methods)
		for (auto & m : ms){
			for (sub_task * sub : m.tn->tasks){
				// quadratic
				for (parsed_task prim : parsed_primitive){
					if (sub->task != prim.name) continue;
					for (size_t i = 0; i < prim.arguments->vars.size(); i++){
						string primArgType = prim.arguments->vars[i].second;
						string method_var = sub->arguments->vars[i];
					
						for (pair<string,string> a : m.vars->vars){
							if (method_var == a.first){
								if (primArgType != a.second){
									if (!isRecursiveParentSort(a.second, primArgType)){
										cout << "Method: " << m.name << endl;
										cout << "\tVariable: " << method_var << endl;
										cout << "\tSubtask: " << sub->id << endl;
										cout << "\t\tTask argument: " << primArgType << endl;
										cout << "\t\tvariable: " << a.second << endl;
									}
								}
							}
						}
					}
				}
				for (parsed_task prim : parsed_abstract){
					if (sub->task != prim.name) continue;
					for (size_t i = 0; i < prim.arguments->vars.size(); i++){
						string primArgType = prim.arguments->vars[i].second;
						string method_var = sub->arguments->vars[i];
						
						for (pair<string,string> a : m.vars->vars){
							if (method_var == a.first){
								if (primArgType != a.second){
									if (!isRecursiveParentSort(a.second, primArgType)){
										cout << "Method: " << m.name << endl;
										cout << "\tVariable: " << method_var << endl;
										cout << "\tSubtask: " << sub->id << endl;
										cout << "\t\tTask argument: " << primArgType << endl;
										cout << "\t\tvariable: " << a.second << endl;
									}
									
								}
								break;
							}
						}
					}
				}
			
			}
		}


}

