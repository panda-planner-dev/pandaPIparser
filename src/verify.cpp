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

instantiated_plan_step parse_plan_step_from_string(string input){
	istringstream ss (input);
	bool first = true;
	instantiated_plan_step ps;
	while (1){
		if (ss.eof()) break;
		string s; ss >> s;
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

	
	//=========================================
	// check whether the individual tasks exist, and comply with their argument restrictions
	bool wrongTaskDeclarations = false;
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
		}
	
		if (foundInPrimitive != ps.declaredPrimitive){
			string inPlanAs = ps.declaredPrimitive ? "primitive" : "abstract";
			string inDomainAs = foundInPrimitive ?   "primitive" : "abstract";
			cout << color(COLOR_RED,"Task with id="+to_string(entry.first)+" is a " + inPlanAs + " in the plan, but is declared as an " + inDomainAs + " in the domain.") << endl;
			wrongTaskDeclarations = true;
		}
	}
	cout << "Tasks declared in plan actually exist and can be instantiated as given: ";
	if (!wrongTaskDeclarations) cout << color(COLOR_GREEN,"true",MODE_BOLD) << endl;
	else cout << color(COLOR_RED,"false",MODE_BOLD) << endl;



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
	

	cout << "Plan is executable: " << color(COLOR_GREEN,"true",MODE_BOLD) << endl;


	return true;
}

