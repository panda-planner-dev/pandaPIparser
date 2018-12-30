#ifndef __CWA
#define __CWA

#include <vector>
#include <string>

using namespace std;

struct ground_literal{
	string predicate;
	bool positive;
	vector<string> args;
};


vector<ground_literal> compute_cwa();

extern vector<ground_literal> init;

#endif
