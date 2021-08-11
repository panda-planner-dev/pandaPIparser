#ifndef __VERIFY_ENCODING
#define __VERIFY_ENCODING

#include <string>
#include <vector>
using namespace std;

struct solution_step{
    string action_name;
    vector<string> arguments;
};

void parseSolution(const string &planFile, vector<solution_step> &plan);
void encode_plan_verification(string & planFile);

#endif
