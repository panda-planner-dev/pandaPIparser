#include <unordered_map>
#include <fstream>

#include "verification_encoding.hpp"
#include "cwa.hpp"
#include "domain.hpp"

void parseSolution(const string &planFile, vector<solution_step> &plan) {
    ifstream file(planFile);
    string line;
    while (getline(file, line)) {
        solution_step s;
        int endActionName = line.find("[");
        s.action_name = line.substr(0, endActionName);
        do {
            line = line.substr(endActionName + 1);
            endActionName = line.find(",");
            if (endActionName >= 0) {
                s.arguments.push_back(line.substr(0, endActionName));
            } else {
                s.arguments.push_back(line.substr(0, line.length() - 1));
            }
        } while (endActionName >= 0);
        plan.push_back(s);
    }
}



void encode_plan_verification(string & planFile){
	vector<solution_step> plan;
	parseSolution(planFile, plan);
	
	// add new state features
	int first_pos_pred = predicate_definitions.size();
	for (size_t i = 0; i < plan.size() + 1; i++) {
	    predicate_definition pi;
	    pi.name = "pref_pos_" + to_string(i);
	    predicate_definitions.push_back(pi);
	}
	// unreachable state feature
	predicate_definition pUnreachable;
	pUnreachable.name = "f_unreachable";
	predicate_definitions.push_back(pUnreachable);
	
	int objNr = 0;
	map<string, string> argToType;
	unordered_map<string, set<task>> newMethods;
	vector<task> newAbstract;
	int orgActions = primitive_tasks.size();
	
	for (size_t ai = 0; ai < plan.size(); ai++) {
	    // find action from the plan
	    task t = task_name_map.find(plan[ai].action_name)->second;
	    // remove it from actions and store it so that it is added to abstract tasks later on
	    // it is still in the mapping used before -> needed when an action is contained more than once
	    auto iter = primitive_tasks.begin();
	    while (iter != primitive_tasks.end()){
	        if ((*iter).name == t.name) {
	            break;
	        } else {
	            iter++;
	        }
	    }
	    if (iter != primitive_tasks.end()) { // need to move from primitive to abstract tasks
	        primitive_tasks.erase(iter);
	        newAbstract.push_back(t);
	    }
	
	    // create copy of action
	    task actionCopy;
	    actionCopy.name = t.name + "|_plan_position_" + to_string(ai);
	    actionCopy.number_of_original_vars = t.number_of_original_vars;
	
	    for (size_t pi = 0; pi < plan[ai].arguments.size(); pi++) {
	        string argName = plan[ai].arguments[pi];
	        if (argToType.find(argName) == argToType.end()) {
	            // create subtype only containing that particular object
	            string orgType = t.vars[pi].second;
	            string newType = orgType + "_" + to_string(objNr++);
	            argToType[argName] = newType;
	            sorts[newType].insert(argName);
	        }
	        string var = t.vars[pi].first;
	        actionCopy.vars.push_back(make_pair(var, argToType[argName])); // add argument with the newly created type
	    }
	    // no deep copy needed
	    for (size_t i = 0; i < t.prec.size(); i++) {
	        actionCopy.prec.push_back(t.prec[i]);
	    }
	    for (size_t i = 0; i < t.eff.size(); i++) {
	        actionCopy.eff.push_back(t.eff[i]);
	    }
	    for (size_t i = 0; i < t.ceff.size(); i++) {
	        actionCopy.ceff.push_back(t.ceff[i]);
	    }
	    for (size_t i = 0; i < t.constraints.size(); i++) {
	        actionCopy.constraints.push_back(t.constraints[i]);
	    }
	    for (size_t i = 0; i < t.costExpression.size(); i++) {
	        actionCopy.costExpression.push_back(t.costExpression[i]);
	    }
	    actionCopy.artificial = true;
	
	    // add new preconditions and effects to position the new action in the plan
	    literal lPrec;
	    lPrec.positive = true;
	    lPrec.predicate = predicate_definitions[first_pos_pred + ai].name;
	    actionCopy.prec.push_back(lPrec);
	
	    literal lAdd;
	    lAdd.positive = true;
	    lAdd.predicate = predicate_definitions[first_pos_pred + ai + 1].name;
	    actionCopy.eff.push_back(lAdd);
	
	    literal lDel;
	    lDel.positive = false;
	    lDel.predicate = predicate_definitions[first_pos_pred + ai].name;
	    actionCopy.eff.push_back(lDel);
	
	    primitive_tasks.push_back(actionCopy);
	
	    // create a new method decomposing the new abstract task into the new action
	    method m;
	    m.name = "_!m_prefix_" + to_string(ai);
	    plan_step ps;
	    ps.id = "task1";
	    ps.task = actionCopy.name;
	    for (size_t i = 0; i < actionCopy.vars.size(); i++) {
	        m.vars.push_back(actionCopy.vars[i]);
	        m.atargs.push_back(actionCopy.vars[i].first);
	        ps.args.push_back(actionCopy.vars[i].first);
	    }
	    m.ps.push_back(ps);
	    m.at = t.name;
	    methods.push_back(m);
	}
	// now we move that tasks that had former been actions to the abstract tasks
	for (task t : newAbstract) {
	    t.prec.clear();
	    t.eff.clear();
	    t.ceff.clear();
	    abstract_tasks.push_back(t);
	}
	
	// all original actions not contained in the plan to verify get a new predicate that make them unreachable
	for (size_t i = 0; i < (orgActions - newAbstract.size()); i++) {
	    literal l;
	    l.predicate = pUnreachable.name;
	    l.positive = true;
	    primitive_tasks[i].prec.push_back(l);
	}
	
	// the name-to-task-mapping is invalid, we renew it
	task_name_map.clear();
	for(task t : abstract_tasks) {
	    task_name_map[t.name] = t;
	}
	for(task t : primitive_tasks) {
	    task_name_map[t.name] = t;
	}
	
	// add initial fact to make the first action applicable
	ground_literal lInit;
	lInit.positive = true;
	lInit.predicate = predicate_definitions[first_pos_pred].name;
	init.push_back(lInit);
	
	// add a new goal fact enforcing all actions from the verified solution to be contained in the new solution
	ground_literal lGoal;
	lGoal.positive = true;
	lGoal.predicate = predicate_definitions[predicate_definitions.size() - 2].name;
	goal.push_back(lGoal);
}
