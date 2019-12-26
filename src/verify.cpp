#include <iostream>
#include <sstream>
#include <algorithm>
#include "verify.hpp"
#include "parsetree.hpp"
#include "util.hpp"

using namespace std;

struct instantiated_plan_step{
	string name;
	vector<string> arguments;
	bool declaredPrimitive;
};

vector<int> parse_list_of_integers(istringstream & ss){
	vector<int> ints;
	while (true){
		if (ss.eof()) break;
		int x; ss >> x;
		ints.push_back(x);
	}

	return ints;
}

vector<int> parse_list_of_integers(string & line){
	istringstream ss (line);
	return parse_list_of_integers(ss);
}



vector<pair<map<string,int>,map<string,string>>> generateAssignmentsDFS(parsed_method & m, map<int,instantiated_plan_step> & tasks,
							set<string> doneIDs, vector<int> & subtasks, int curpos,
							map<string,string> variableAssignment,
							map<string,int> matching){
	vector<pair<map<string,int>,map<string,string>>> ret;
	if (doneIDs.size() == subtasks.size()){
		// check whether the variable assignment is ok
		for (pair<string,string> varDecl : m.vars->vars){
			string sort = varDecl.second;
			if (!variableAssignment.count(varDecl.first)){
				cout << "unassigned variable " << varDecl.first << endl;
				continue;
			}

			string param = variableAssignment[varDecl.first];
			if (!sorts[sort].count(param)) return ret; // parameter is not consistent with delcared sort
		}
		
		
		// found a full matching
		ret.push_back(make_pair(matching, variableAssignment));
		return ret;
	}
	
	
	// find the remaining sources
	set<string> allIDs;
	map<string,sub_task*> subtasksAccess;
	for (sub_task* st : m.tn->tasks) allIDs.insert(st->id), subtasksAccess[st->id] = st;
	for (string id : doneIDs) allIDs.erase(id);
	for (auto & order : m.tn->ordering){
		// this order is completely dealt with
		if (doneIDs.count(order->first)) continue;
		if (doneIDs.count(order->second)) continue;
		allIDs.erase(order->second); // has a predecessor
	}

	// allIDs now contains sources
	for (string source : allIDs){
		// now we try to map source to subtasks[curpos]
		instantiated_plan_step & ps = tasks[subtasks[curpos]];
		// check whether this task is ok

		//1. has to be the same task type as declared
		if (subtasksAccess[source]->task != ps.name) continue;

		// check that the variable assignment is compatible
		map<string,string> newVariableAssignment = variableAssignment;
		bool assignmentOk = true;
		for (unsigned int i = 0; i < ps.arguments.size(); i++){
			string methodVar = subtasksAccess[source]->arguments->vars[i];
			string taskParam = ps.arguments[i];
			if (newVariableAssignment.count(methodVar)){
				if (newVariableAssignment[methodVar] != taskParam)
					assignmentOk = false; 
			}
			newVariableAssignment[methodVar] = taskParam;
		}
		if (!assignmentOk) continue; // if the assignment is not ok then don't continue on it
		set<string> newDone = doneIDs; newDone.insert(source);
		map<string,int> newMatching = matching;
		newMatching[source] = subtasks[curpos];
		
		auto recursive = generateAssignmentsDFS(m, tasks, newDone,
							   subtasks, curpos + 1,
							   newVariableAssignment, newMatching);

		for (auto r : recursive) ret.push_back(r);
	}
	
	return ret;
}


void getRecursive(int source, vector<int> & allSub,map<int,vector<int>> & subtasksForTask){
	if (!subtasksForTask.count(source)) {
		allSub.push_back(source);
		return;
	}
	for (int sub : subtasksForTask[source])
		getRecursive(sub,allSub,subtasksForTask);	
}


bool findLinearisation(int currentTask,
		map<int,parsed_method> & parsedMethodForTask,
		map<int,instantiated_plan_step> & tasks,
		map<int,vector<int>> & subtasksForTask,
		map<int,vector<pair<map<string,int>,map<string,string>>>> & matchings,
		map<int,int> pos_in_primitive_plan,
		bool uniqueMatching){
	if (tasks[currentTask].declaredPrimitive) return true; // order is ok
	
	for (auto & matching : matchings[currentTask]){
		// check all orderings
		map<int,vector<int>> allSubs;
		for (int st : subtasksForTask[currentTask]){
			vector<int> subs;
			getRecursive(st, subs, subtasksForTask);
			allSubs[st] = subs;
		}
		
		bool badOrdering = false;
		for (auto ordering : parsedMethodForTask[currentTask].tn->ordering){
			vector<int> allBefore = allSubs[matching.first[ordering->first]];
			vector<int> allAfter = allSubs[matching.first[ordering->second]];

			// check whether this ordering is adhered to for all
			for (int before : allBefore) for (int after : allAfter){
				bool primitiveBad = pos_in_primitive_plan[before] > pos_in_primitive_plan[after];
				if (primitiveBad && uniqueMatching){
					cout << color(COLOR_RED,"Task with id="+to_string(currentTask)+" has two children: " +
					to_string(matching.first[ordering->first]) + " (" + ordering->first + ") and "  +
					to_string(matching.first[ordering->second]) + " (" + ordering->second + ") who will be decomposed into primitive tasks: " + to_string(before) + " and " + to_string(after) + ". The method enforces " + to_string(before) + " < " + to_string(after) + " but the plan is ordered the other way around.") << endl;
				}

				badOrdering |= primitiveBad;
			}
		}
		if (badOrdering) continue;
		// check recursively
		bool subtasksOk = true;
		for (int st : subtasksForTask[currentTask])
			subtasksOk &= findLinearisation(st,parsedMethodForTask,tasks,subtasksForTask,matchings,pos_in_primitive_plan,uniqueMatching && (matchings[currentTask].size() == 1));

		if (!subtasksOk) continue;

		return true;
	}
	return false; // no good matching found
}


instantiated_plan_step parse_plan_step_from_string(string input){
	istringstream ss (input);
	bool first = true;
	instantiated_plan_step ps;
	while (1){
		if (ss.eof()) break;
		string s; ss >> s;
		if (s == "") break;
		if (first) {
			first = false;
			ps.name = s;
		} else {
			ps.arguments.push_back(s);
		}
	}
	return ps;
}


bool verify_plan(istream & plan){
	// parse everything until marker
	string s = "";
	while (s != "==>") plan >> s;
	// then read the primitive plan
	
	map<int,instantiated_plan_step> tasks;
	vector<int> primitive_plan;
	map<int,int> pos_in_primitive_plan;
	map<int,string> appliedMethod;
	map<int,vector<int>> subtasksForTask;
	

	cout << "Reading plan given as input" << endl;
	while (1){
		string head; plan >> head;
		if (head == "root") break;
		int id = atoi(head.c_str());
		string rest_of_line; getline(plan,rest_of_line);
		//cout << "plan step id \"" << id << "\": \"" << rest_of_line << "\"" << endl;
		instantiated_plan_step ps = parse_plan_step_from_string(rest_of_line);		
		ps.declaredPrimitive = true;
		primitive_plan.push_back(id);
		pos_in_primitive_plan[id] = primitive_plan.size() - 1;
		if (tasks.count(id)){
			cout << color(COLOR_RED,"Two primitive task have the same id: ") << 
				color(COLOR_RED,to_string(id)) << endl;
			exit(1);
		}
		tasks[id] = ps;
		cout << "Parsed action id=" << id << " " << ps.name;
		for(string arg : ps.arguments) cout << " " << arg;
		cout << endl;
	}


	cout << "Size of primitive plan: " << primitive_plan.size() << endl;
	string root_line; getline(plan,root_line);
	vector<int> root_tasks = parse_list_of_integers(root_line);
	cout << "Root task (" << root_tasks.size() << "):";
	for (int & rt : root_tasks) cout << " " << rt;
	cout << endl;



	cout << "Reading plan given as input" << endl;
	while (1){
		string line;
		getline(plan,line);
		if (plan.eof()) break;
		istringstream ss (line);
		int id; ss >> id; 
		string task = "";
		s = "";
		do {
			task += " " + s;
			ss >> s;
		} while (s != "->");
		instantiated_plan_step at = parse_plan_step_from_string(task);
		at.declaredPrimitive = false;
		// add this task to the map
		if (tasks.count(id)){
			cout << color(COLOR_RED,"Two task have the same id: ") << 
				color(COLOR_RED,to_string(id)) << endl;
			exit(1);
		}
		tasks[id] = at;
		cout << "Parsed abstract task id=" << id << " " << at.name;
		for(string arg : at.arguments) cout << " " << arg;
		

		// read the actual content of the method
		string methodName; ss >> methodName;
		// read subtask IDs
		vector<int> subtasks = parse_list_of_integers(ss);
		
		// id cannot be contained in the maps as it was possible to insert the id into the tasks map
		appliedMethod[id] = methodName;
		subtasksForTask[id] = subtasks;

		cout << " and is decomposed into";
		for(int st : subtasks) cout << " " << st;
		cout << endl;
	}

	// now that we successfully got the input, we can start to run the checker
	//=========================================================================================================================
	
	//=========================================
	// check whether the individual tasks exist, and comply with their argument restrictions
	bool wrongTaskDeclarations = false;
	map<int,map<string,string>> variableValues;
	for (auto & entry : tasks){
		instantiated_plan_step & ps = entry.second;
		// search for the name of the plan step
		bool foundInPrimitive = true;
		parsed_task domain_task; domain_task.name = "__none_found";
		for (parsed_task prim : parsed_primitive)
			if (prim.name == ps.name)
				domain_task = prim;
		
		for (parsed_task & abstr : parsed_abstract)
			if (abstr.name == ps.name)
				domain_task = abstr, foundInPrimitive = false;

		if (domain_task.name == "__none_found"){
			cout << color(COLOR_RED,"Task with id="+to_string(entry.first)+" and task name \"" + ps.name + "\" is not declared in the domain.") << endl;
			wrongTaskDeclarations = true;
			continue;
		}
	
		if (foundInPrimitive != ps.declaredPrimitive){
			string inPlanAs = ps.declaredPrimitive ? "primitive" : "abstract";
			string inDomainAs = foundInPrimitive ?   "primitive" : "abstract";
			cout << color(COLOR_RED,"Task with id="+to_string(entry.first)+" is a " + inPlanAs + " in the plan, but is declared as an " + inDomainAs + " in the domain.") << endl;
			wrongTaskDeclarations = true;
			continue;
		}

		if (domain_task.arguments->vars.size() != ps.arguments.size()){
			cout << color(COLOR_RED,"Task with id="+to_string(entry.first)+" has wrong number of arguments. "+ to_string(ps.arguments.size()) + " are given in the plan, but the domain required " + to_string(domain_task.arguments->vars.size()) + " parameters.") << endl;
			wrongTaskDeclarations = true;
			continue;
		}

		// check whether the individual arguments are ok
		for (unsigned int arg = 0; arg < ps.arguments.size(); arg++){
			string param = ps.arguments[arg];
			string argumentSort = domain_task.arguments->vars[arg].second;
			// check that the parameter is part of the variable's sort
			if (sorts[argumentSort].count(param) == 0){
				cout << color(COLOR_RED,"Task with id="+to_string(entry.first)+" has the parameter " + param + " assigned to variable " + domain_task.arguments->vars[arg].first + " of sort " + argumentSort + " - but the parameter is not a member of this sort.") << endl;
				wrongTaskDeclarations = true;
				continue;
			}
			variableValues[entry.first][domain_task.arguments->vars[arg].first] = param;
		}
	}
	cout << "Tasks declared in plan actually exist and can be instantiated as given: ";
	if (!wrongTaskDeclarations) cout << color(COLOR_GREEN,"true",MODE_BOLD) << endl;
	else cout << color(COLOR_RED,"false",MODE_BOLD) << endl;



	//=========================================
	// TODO: this is not properly tested yet
	// check the validity of variable constraints
	cout << "Plan is executable: " << color(COLOR_GREEN,"true",MODE_BOLD) << endl;

	
	set<int> causedTasks;
	causedTasks.insert(root_tasks.begin(), root_tasks.end());
	//=========================================
	bool methodsContainDuplicates = false;
	for (auto & entry : subtasksForTask){
		for (int st : entry.second){
			causedTasks.insert(st);
			int stCount = count(entry.second.begin(), entry.second.end(), st);
			if (stCount != 1){
				cout << color(COLOR_RED,"The method decomposing the task id="+to_string(entry.first)+" contains the subtask id=" + to_string(st) + " " + to_string(stCount) + " times") << endl;
				methodsContainDuplicates = true;
			}
		}
	}
	cout << "Methods don't contain duplicates: ";
	if (!methodsContainDuplicates) cout << color(COLOR_GREEN,"true",MODE_BOLD) << endl;
	else cout << color(COLOR_RED,"false",MODE_BOLD) << endl;
	
	//=========================================
	// check for orphanded tasks, i.e.\ those that do not occur in methods
	bool orphanedTasks = false;
	for (auto & entry : tasks){
		if (!causedTasks.count(entry.first)){
			cout << color(COLOR_RED,"The task id="+to_string(entry.first)+" is not contained in any method and is not a root task") << endl;
			orphanedTasks = true;
		}
	}
	cout << "Methods don't contain orphaned Tasks: ";
	if (!orphanedTasks) cout << color(COLOR_GREEN,"true",MODE_BOLD) << endl;
	else cout << color(COLOR_RED,"false",MODE_BOLD) << endl;
	

	//=========================================
	bool wrongMethodApplication = false;
	map<int,vector<pair<map<string,int>,map<string,string>>>> possibleMethodInstantiations;
	map<int,parsed_method> parsedMethodForTask;
	for (auto & entry : appliedMethod){
		int atID = entry.first;
		instantiated_plan_step & at = tasks[atID];
		string taskName = at.name;
		// look for the applied method
		parsed_method m; m.name == "__no_method";
		for (parsed_method & mm : parsed_methods[taskName])
			if (mm.name == entry.second) // if method's name is the name of the method that was given in in the input
				m = mm;

		if (m.name == "__no_method"){
			cout << color(COLOR_RED,"Task with id="+to_string(entry.first)+" is decomposed with method \"" + entry.second + "\", but there is no such method.") << endl;
			wrongMethodApplication  = true;
			continue;
		}
		parsedMethodForTask[atID] = m;

		// the __top_method is added by the parser as an artificial top method. We check the top tasks later (either those given in the problem or the __top task)

		map<string,string> methodParamers;

		// assert at args to variables
		for (unsigned int i = 0; i < m.atArguments.size(); i++){
			string atParam = at.arguments[i];
			string atVar = m.atArguments[i];
			if (methodParamers.count(atVar)){
				if (methodParamers[atVar] != atParam){
					cout << color(COLOR_RED,"Task with id="+to_string(entry.first)+" is instantiated with \"" + atParam + "\" as its " + to_string(i) + "th parameter, but the variable \"" + atVar + "\" is already assigned to \"" + methodParamers[atVar] + "\".") << endl;
					wrongMethodApplication = true;
				}
			}
			methodParamers[atVar] = atParam;
		}
		
		// generate all compatible assignment of given subtasks to subtasks declared in the method
		set<string> done;
		map<string,int> __matching;
		auto matchings = generateAssignmentsDFS(m, tasks, done,
							   subtasksForTask[atID], 0,
							   methodParamers, __matching);

		if (matchings.size() == 0){
			cout << color(COLOR_RED,"Task with id="+to_string(entry.first)+" has no matchings for its subtasks.") << endl;
			wrongMethodApplication = true;
		}
		
		possibleMethodInstantiations[atID] = matchings;
	}	
		
	cout << "Methods can be instantiated: ";
	if (!wrongMethodApplication) cout << color(COLOR_GREEN,"true",MODE_BOLD) << endl;
	else cout << color(COLOR_RED,"false",MODE_BOLD) << endl;

	//==============================================
	// check whether the applied methods leads to the ordering enforced by the methods
	bool orderingIsConsistent = true;

	for (int rt : root_tasks){
		if (!findLinearisation(rt,parsedMethodForTask,tasks,subtasksForTask,possibleMethodInstantiations,pos_in_primitive_plan,true)){
			cout << color(COLOR_RED,"Ordering below task with id="+to_string(rt)+" is under no matching compatible with primitive plan.") << endl;
			orderingIsConsistent = false;
		}
	}
	
	cout << "Order induced by methods is present in plan: ";
	if (orderingIsConsistent) cout << color(COLOR_GREEN,"true",MODE_BOLD) << endl;
	else cout << color(COLOR_RED,"false",MODE_BOLD) << endl;
	return true;
}

