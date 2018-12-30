#ifndef __DOMAIN
#define __DOMAIN

#include<vector>
#include<map>
#include<set>
#include<string>

using namespace std;

// sort name and set of elements
extern map<string,set<string> > sorts;


const string dummy_equal_literal = "__equal";
struct literal{
	bool positive;
	string predicate;
	vector<string> arguments;
};

#endif
