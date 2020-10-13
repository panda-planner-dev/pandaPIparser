#include "domain.hpp"
#include "parsetree.hpp"
#include "parametersplitting.hpp"
#include "util.hpp"
#include <iostream>
using namespace std;

void split_independent_parameters(){
	// find static predicates

	set<string> staticPredicates;
	for (auto p : predicate_definitions) staticPredicates.insert(p.name);
	for (task & prim : primitive_tasks) {
		for (literal & l : prim.eff) staticPredicates.erase(l.predicate);
		for (conditional_effect & ce : prim.ceff) staticPredicates.erase(ce.effect.predicate);
	}


	//cout << "Static predicates are: " << endl;
	//for (string s : staticPredicates)
	//	cout << s << endl;
	

	bool splittedSomeMethod = true;
	int i = 0;
	while (splittedSomeMethod){
		splittedSomeMethod = false;
	
		set<string> artificialTasks;
		for (task & prim : primitive_tasks)
			if (prim.artificial && prim.eff.size() == 0 && prim.ceff.size() == 0) // no effects ...
				artificialTasks.insert(prim.name);
		
		
		vector<method> old = methods;
		methods.clear();
		for(method m : old){
			// find variables that occur only in one of the plan steps
			map<string,set<pair<string,int>>> variables_ps_id;
			for(plan_step ps : m.ps) {
				if (!artificialTasks.count(ps.task))
					for(string v : ps.args) variables_ps_id[v].insert({ps.id,-1});
				else {
					set<string> vars_in_non_static;
					int pCount = 0;
					for (literal & l : task_name_map[ps.task].prec){
						if (staticPredicates.count(l.predicate)){
							for (string & a : l.arguments)
								variables_ps_id[a].insert({ps.id,pCount});
						} else {
							for (string & a : l.arguments)
								vars_in_non_static.insert(a);
						}
						pCount++;
					}
					
					for(string v : vars_in_non_static)
						variables_ps_id[v].insert({ps.id,-1});
				}
			}
	
			map<string,string> variableSorts;
			for(auto v : m.vars) variableSorts[v.first] = v.second;
			// don't consider variables that have at most one instantiation
			for(auto v : m.vars) if (sorts[v.second].size() < 2) variables_ps_id.erase(v.first); 
			// can't split on arguments of abstract task
			for (string v : m.atargs) variables_ps_id.erase(v);
			// don't consider variables that are part of constraints
			for (literal l : m.constraints) variables_ps_id.erase(l.arguments[0]), variables_ps_id.erase(l.arguments[1]);
	
			// no variables can be splitted
			if (variables_ps_id.size() == 0) { methods.push_back(m); continue; }
	
			/*cout << "====================" << endl << "Method: " << m.name << endl;
			for (auto ps : m.ps)
				cout << "\t\tPS: " << ps.id << " " << ps.task << endl;
			for (auto [v,ss] : variables_ps_id){
				cout << "\tvar: " << v << endl;
				for (auto [ps,prec] : ss)
					cout << "\t\t" << ps << " @ " << prec << endl;
			}*/	
			
			/*set<string> allElems;
			for (auto [vps,ss] : variables_ps_id) 
				for (auto [ps,prec] : ss) allElems.insert(ps);
*/
	
			string largestSplittable = "";
			// check for every splittable variable whether we can split it away.
			set<string> beforeID, afterID;
			for (auto [vps,ss] : variables_ps_id) {
				set<string> ordered_ps;
				set<string> all_ps;
				// we can ignore order for static predicates on artificial tasks
				for (auto [ps,prec] : ss) {
					all_ps.insert(ps);
					if (prec == -1) ordered_ps.insert(ps);
				}

// splitting on everything does not bring anything ...
				if (all_ps.size() == m.ps.size()) continue; 
				
				//cout << "Var: " << vps << endl;
				//for (string ps : ordered_ps) cout << "\t" << ps << endl; 
	
				set<string> newbeforeID, newafterID;
				if (m.is_sub_group(ordered_ps, newbeforeID, newafterID)){
					if (largestSplittable.size() == 0)
						largestSplittable = vps, beforeID = newbeforeID, afterID = newafterID;
					else if (sorts[variableSorts[vps]].size() > sorts[variableSorts[largestSplittable]].size())
						largestSplittable = vps, beforeID = newbeforeID, afterID = newafterID;
				}	
			}
	
			if (largestSplittable == "") { methods.push_back(m); continue; }

			splittedSomeMethod = true;

			//cout << "Largest Splittable: " << largestSplittable << endl;
			// gather all variables that occur only in this group (might be more just this one)
	
			set<pair<string,int>> subgroup = variables_ps_id[largestSplittable];
			set<string> subgroupPSIDs;
			for (auto [id,_x] : subgroup) subgroupPSIDs.insert(id);

			// which variables can we add
			set<string> variables_in_sub_group;
			for (auto [v,occ] : variables_ps_id){
				bool ok = true;
				for (auto p : occ) ok &= subgroupPSIDs.count(p.first);
				if (ok) {
					variables_in_sub_group.insert(v);
					subgroup.insert(occ.begin(), occ.end());
				}
			}
			//cout << "Variables in splitting subgroup:";
			//for (string v : variables_in_sub_group) cout << " " << v;
			//cout << endl;
	
			map<string,string> variables_to_remove;
			for (auto v : variables_in_sub_group)
				variables_to_remove[v].size(); // insert into map
	
			method base;
			base.name = m.name;
			base.at = m.at;
			base.atargs = m.atargs;
			base.constraints = m.constraints;
	
			// create new abstract task
			task at;
			at.name = m.name + "_splitted_" + to_string(++i);
			// create a new method for the splitted task
			method sm;
			sm.name = "_splitting_method_" + at.name; // must start with an underscore s.t. this method will be removed by the solution compiler
			sm.at = at.name;
			
			// ordering
			base.ordering.clear();
			for (auto [b,a] : m.ordering){
				int c = 0;
				if (subgroup.count({b,-1})) c++;
				if (subgroup.count({a,-1})) c++;
				if (c == 0)	base.ordering.push_back({b,a});
				if (c == 2)	sm.ordering.push_back({b,a});
			}
			//base.ordering = m.ordering;
			// only take variables that are not to be removed
			map<string,string> sorts_of_remaining;
			for(auto v : m.vars)
				if (variables_to_remove.count(v.first))
					variables_to_remove[v.first] = v.second;
				else
					base.vars.push_back(v), sorts_of_remaining[v.first] = v.second;
		
			set<string> smVars; // for duplicate check
			
			set<string> subSplittedIDs;
			// put the plan steps in their respective methods
			for (plan_step ops : m.ps){
				// check if this an artificial task which we may have to split
				set<int> splitPrecs;
				bool isSub = false;
				for (auto [id,prec] : subgroup){
					if (id != ops.id) continue;
					if (prec == -1){
						isSub = true;
					} else {
						splitPrecs.insert(prec);
					}
				}
	
				if (splitPrecs.size() == 0)	{
					if (!isSub) base.ps.push_back(ops);
					else sm.ps.push_back(ops);
				} else {
					// this is an artificial task that needs splitting
					task tMain = task_name_map[ops.task];
					task tBase; plan_step pBase;
					task tSub;  plan_step pSub;
					pBase.id = ops.id;
					pSub.id = ops.id;
					subSplittedIDs.insert(pSub.id);
					tBase.artificial = true;
					tSub.artificial = true;
	
					// create names
					tBase.name = tMain.name + "_base"; pBase.task = tBase.name;
					tSub.name = tMain.name + "_split"; pSub.task = tSub.name;
	
					// contains only preconditions ...
					for (size_t i = 0; i < tMain.prec.size(); i++)
						if (splitPrecs.count(i))
							tSub.prec.push_back(tMain.prec[i]);
						else
							tBase.prec.push_back(tMain.prec[i]);
	
					set<string> varsSub, varsBase;
					for (literal l : tSub.prec) for (string v : l.arguments) varsSub.insert(v);
					for (literal l : tBase.prec) for (string v : l.arguments) varsBase.insert(v);
	
					// create variables
					for (size_t vnum = 0; vnum < tMain.vars.size(); vnum++){
						auto vdecl = tMain.vars[vnum];
						if (varsSub.count(vdecl.first)) {
							tSub.vars.push_back(vdecl);
							pSub.args.push_back(ops.args[vnum]);
						}
						if (varsBase.count(vdecl.first)) {
							tBase.vars.push_back(vdecl);
							pBase.args.push_back(ops.args[vnum]);
						}
					}
					tBase.number_of_original_vars = tBase.vars.size();
					tMain.number_of_original_vars = tMain.vars.size();
	
					primitive_tasks.push_back(tBase); task_name_map[tBase.name] = tBase;
					primitive_tasks.push_back(tSub);  task_name_map[tSub.name] = tSub;
					
					base.ps.push_back(pBase);
					sm.ps.push_back(pSub);
				}
			}
	
			for (plan_step ps : sm.ps){
				if (subSplittedIDs.count(ps.id)) continue;
				for (string id : subSplittedIDs)
					sm.ordering.push_back({id,ps.id});
			}
	
			// nps is the plan steps that is inserted into the base method
			plan_step nps;
			set<string> npsVars; // duplicate check
			for (plan_step ps : sm.ps){
				// variables
				for (string v : ps.args){	
					// if we don't remove it, it is an argument of the abstract task
					string sort; 
					if (!variables_to_remove.count(v)){
						sort = sorts_of_remaining[v];
						if (!npsVars.count(v)){
							npsVars.insert(v);
							at.vars.push_back(make_pair(v,sort));
							nps.args.push_back(v);
						}
					} else 
						sort = variables_to_remove[v];
	
					if (!smVars.count(v)){
						smVars.insert(v);
						sm.vars.push_back(make_pair(v,sort));
					}
				}
			}
	
			// the new abstract task is done so push it
			at.number_of_original_vars = at.vars.size(); // does not matter as it will get pruned
			abstract_tasks.push_back(at);
			task_name_map[at.name] = at;
	
			// add the new plan step to the base method	
			nps.task = at.name;
			nps.id = "x_split_" + to_string(i);
			for (string b : beforeID) base.ordering.push_back({b,nps.id});
			for (string a : afterID) base.ordering.push_back({nps.id,a});
			
			base.ps.push_back(nps);
	
			sm.atargs = nps.args;
			//cout << "SM" << endl;
			sm.check_integrity();
			methods.push_back(sm);
	
			//cout << "BASE" << endl;
			base.check_integrity();
			methods.push_back(base);
		}
		//break;
	}	
}

