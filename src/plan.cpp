#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include "parsetree.hpp"
#include "util.hpp"
#include "plan.hpp"

using namespace std;


vector<int> parse_list_of_integers(istringstream & ss, int debugMode){
	vector<int> ints;
	while (true){
		if (ss.eof()) break;
		int x; ss >> x;
		ints.push_back(x);
	}

	return ints;
}

vector<int> parse_list_of_integers(string & line, int debugMode){
	if (debugMode) cout << "Reading list of integers from \"" << line << "\"" << endl;
	istringstream ss (line);
	return parse_list_of_integers(ss,debugMode);
}

instantiated_plan_step parse_plan_step_from_string(string input, int debugMode){
	if (debugMode) cout << "Parse instantiated task from \"" << input << "\"" << endl;
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


parsed_plan parse_plan(istream & plan, int debugMode){
	// parse everything until marker
	string s = "";
	while (s != "==>") plan >> s;
	// then read the primitive plan

	parsed_plan pplan;

	pplan.tasks.clear();
	pplan.primitive_plan.clear();
	pplan.pos_in_primitive_plan.clear();
	pplan.appliedMethod.clear();
	pplan.subtasksForTask.clear();
	pplan.root_tasks.clear();
	

	cout << "Reading plan given as input" << endl;
	while (1){
		string head; plan >> head;
		if (head == "root") break;
		int id = atoi(head.c_str());
		if (id < 0){
			cout << color(COLOR_RED,"Negative id: ") << color(COLOR_RED,to_string(id)) << endl;
			exit(1);
		}
		string rest_of_line; getline(plan,rest_of_line);
		instantiated_plan_step ps = parse_plan_step_from_string(rest_of_line, debugMode);		
		ps.declaredPrimitive = true;
		pplan.primitive_plan.push_back(id);
		pplan.pos_in_primitive_plan[id] = pplan.primitive_plan.size() - 1;
		if (pplan.tasks.count(id)){
			cout << color(COLOR_RED,"Two primitive task have the same id: ") <<	color(COLOR_RED,to_string(id)) << endl;
			exit(1);
		}
		pplan.tasks[id] = ps;
		cout << "Parsed action id=" << id << " " << ps.name;
		for(string arg : ps.arguments) cout << " " << arg;
		cout << endl;
	}


	cout << "Size of primitive plan: " << pplan.primitive_plan.size() << endl;
	string root_line; getline(plan,root_line);
	pplan.root_tasks = parse_list_of_integers(root_line, debugMode);
	cout << "Root tasks (" << pplan.root_tasks .size() << "):";
	for (int & rt : pplan.root_tasks) cout << " " << rt;
	cout << endl;


	cout << "Reading plan given as input" << endl;
	while (1){
		string line;
		getline(plan,line);
		if (plan.eof()) break;
	    size_t first = line.find_first_not_of(' ');
    	size_t last = line.find_last_not_of(' ');
	    line = line.substr(first, (last-first+1));
		
		istringstream ss (line);
		int id; ss >> id; 
		if (id < 0){
			cout << color(COLOR_RED,"Negative id: ") << color(COLOR_RED,to_string(id)) << endl;
			exit(1);
		}
		string task = "";
		s = "";
		do {
			task += " " + s;
			ss >> s;
		} while (s != "->");
		instantiated_plan_step at = parse_plan_step_from_string(task, debugMode);
		at.declaredPrimitive = false;
		// add this task to the map
		if (pplan.tasks.count(id)){
			cout << color(COLOR_RED,"Two task have the same id: ") << color(COLOR_RED,to_string(id)) << endl;
			exit(1);
		}
		pplan.tasks[id] = at;
		cout << "Parsed abstract task id=" << id << " " << at.name;
		for(string arg : at.arguments) cout << " " << arg;
		

		// read the actual content of the method
		string methodName; ss >> methodName;
		// read subtask IDs
		vector<int> subtasks = parse_list_of_integers(ss, debugMode);
		
		// id cannot be contained in the maps as it was possible to insert the id into the tasks map
		pplan.appliedMethod[id] = methodName;
		pplan.subtasksForTask[id] = subtasks;

		cout << " and is decomposed into";
		for(int st : subtasks) cout << " " << st;
		cout << endl;
	}

	return pplan;
}
