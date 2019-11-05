#include "verify.hpp"
#include <iostream>
#include <sstream>

using namespace std;

struct instantiated_plan_step{
	string name;
	vector<string> arguments;
};

instantiated_plan_step parse_plan_step_from_string(string input){
	istringstream ss (input);
	bool first = true;
	instantiated_plan_step ps;
	while (1){
		string s; ss >> s;
		if (ss.eof()) break;
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
	while (1){
		string head; plan >> head;
		if (head == "root") break;
		int id = atoi(head.c_str());
		string rest_of_line; getline(plan,rest_of_line);
		cerr << "plan step id \"" << id << "\": \"" << rest_of_line << "\"" << endl;
		instantiated_plan_step ps = parse_plan_step_from_string(rest_of_line);		
		primitive_plan.push_back(id);
		tasks[id] = ps;
	}
	cerr << "Size of primitive plan: " << primitive_plan.size() << endl;
	int root_task; plan >> root_task;
	cerr << "Root task: " << root_task << endl;

	while (1){
		int id; plan >> id; if (plan.eof()) break;
		string task = "";
		s = "";
		do {
			task += " " + s;
			plan >> s;
		} while (s != "->");

		
	}
	return true;
}

