#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <map>
#include <vector>

#include "cwa.hpp"
#include "domain.hpp"
#include "hddl.hpp"
#include "hddlWriter.hpp"
#include "hpdlWriter.hpp"
#include "htn2stripsWriter.hpp"
#include "output.hpp"
#include "parametersplitting.hpp"
#include "parsetree.hpp"
#include "plan.hpp"
#include "shopWriter.hpp"
#include "sortexpansion.hpp"
#include "typeof.hpp"
#include "util.hpp"
#include "verify.hpp"
#include "properties.hpp"

#include "cmdline.h"

using namespace std;

// declare parser function manually
void run_parser_on_file(FILE* f, char* filename);

// parsed domain data structures
bool has_typeof_predicate = false;
vector<sort_definition> sort_definitions;
vector<predicate_definition> predicate_definitions;
vector<parsed_task> parsed_primitive;
vector<parsed_task> parsed_abstract;
map<string,vector<parsed_method> > parsed_methods;
vector<pair<predicate_definition,string>> parsed_functions;
string metric_target = dummy_function_type;


map<string,set<string> > sorts;
vector<method> methods;
vector<task> primitive_tasks;
vector<task> abstract_tasks;

map<string, task> task_name_map;


int main(int argc, char** argv) {
	cin.sync_with_stdio(false);
	cout.sync_with_stdio(false);
	int dfile = -1;
	int pfile = -1;
	int doutfile = -1;
	int poutfile = -1;
	bool splitParameters = true;
	bool compileConditionalEffects = true;
	bool linearConditionalEffectExpansion = false;
	bool encodeDisjunctivePreconditionsInMethods = false;
	bool compileGoalIntoAction = false;
	
	bool shopOutput = false;
	bool hpdlOutput = false;
	bool htn2stripsOutput = false;
	bool pureHddlOutput = false;
	bool hddlOutput = false;
	bool internalHDDLOutput = false;
	bool lenientVerify = false;
	bool verboseOutput = false;
	bool verifyPlan = false;
	bool useOrderInPlanVerification = true;
	bool convertPlan = false;
	bool showProperties = false;
	bool removeMethodPreconditions = false;
	int verbosity = 0;
	
	gengetopt_args_info args_info;
	if (cmdline_parser(argc, argv, &args_info) != 0) return 1;

	// set debug mode
	if (args_info.debug_given) {
		verboseOutput = true;
		verbosity = args_info.debug_arg;
	}
	if (args_info.no_colour_given) no_colors_in_output = true;
	if (args_info.no_split_parameters_given) splitParameters = false;
	if (args_info.keep_conditional_effects_given) compileConditionalEffects = false;
	if (args_info.linear_conditional_effect_given) {
		compileConditionalEffects = false; linearConditionalEffectExpansion = true;
	}
	if (args_info.encode_disjunctive_preconditions_in_htn_given) encodeDisjunctivePreconditionsInMethods = true;
	if (args_info.goal_action_given) compileGoalIntoAction = true;
	if (args_info.remove_method_preconditions_given) removeMethodPreconditions = true;

	if (args_info.shop_given) shopOutput = true;
	if (args_info.shop1_given) shopOutput = shop_1_compatability_mode = true;
	if (args_info.hpdl_given) hpdlOutput = true;
	if (args_info.hppdl_given) htn2stripsOutput = true;
	if (args_info.hddl_given) pureHddlOutput = true;
	if (args_info.processed_hddl_given) hddlOutput = true;
	if (args_info.internal_hddl_given) hddlOutput = internalHDDLOutput = true;

	if (args_info.verify_given){
		verifyPlan = true;
		verbosity = args_info.verify_arg;
	}
	if (args_info.vverify_given){
		verifyPlan = true;
		verbosity = 1;
	}
	if (args_info.vvverify_given){
		verifyPlan = true;
		verbosity = 2;
	}
	if (args_info.lenient_given) verifyPlan = lenientVerify = true;
	if (args_info.verify_no_order_given) {
		verifyPlan = true;
		useOrderInPlanVerification = false;
	}

	if (args_info.panda_converter_given) convertPlan = true;
	if (args_info.properties_given) showProperties = true;

	
	cout << "pandaPIparser is configured as follows" << endl;
	cout << "  Colors in output: " << boolalpha << !no_colors_in_output << endl;
	if (showProperties){
		cout << "  Mode: show instance properties" << endl;
	} else if (convertPlan){
		cout << "  Mode: convert pandaPI plan" << endl;
	} else if (verifyPlan){
		cout << "  Mode: plan verification" << endl;
		cout << "  Verbosity: " << verbosity << endl;
		cout << "  Lenient mode: " << boolalpha << lenientVerify << endl;
		cout << "  Ignore given order: " << !useOrderInPlanVerification << endl;
	} else {
		cout << "  Mode: parsing mode" << endl;
		cout << "  Parameter splitting: " << boolalpha << splitParameters << endl;
		cout << "  Conditional effects: ";
		if (compileConditionalEffects){
			if (linearConditionalEffectExpansion) cout << "linear encoding";
			else cout << "exponential encoding";
		} else cout << "keep";
		cout << endl;	
		cout << "  Disjunctive preconditions as HTN: " << boolalpha << encodeDisjunctivePreconditionsInMethods << endl;
		cout << "  Replace goal with action: " << boolalpha << compileGoalIntoAction << endl;
	
		cout << "  Output: ";
		if (shopOutput) cout << "SHOP2";
		else if (shop_1_compatability_mode) cout << "SHOP1";
		else if (hpdlOutput) cout << "HPDL";
		else if (htn2stripsOutput) cout << "HPPDL";
		else if (pureHddlOutput) cout << "HDDL (no transformations)";
		else if (hddlOutput && internalHDDLOutput) cout << "HDDL (internal)";
		else if (hddlOutput && !internalHDDLOutput) cout << "HDDL (with transformations)";
		else cout << "pandaPI format";
		cout << endl;
	}


	vector<string> inputFiles;
	for (unsigned i = 0 ; i < args_info.inputs_num; i++)
    	inputFiles.push_back(args_info.inputs[i]);

	if (inputFiles.size() > 0) dfile = 0;
	if (inputFiles.size() > 1) pfile = 1;
	if (inputFiles.size() > 2) doutfile = 2;
	if (inputFiles.size() > 3) poutfile = 3;

	if (dfile == -1){
		if (convertPlan)
			cout << "You need to provide a plan as input." << endl;
		else
			cout << "You need to provide a domain and problem file as input." << endl;
		return 1;
	}

	// if we want to simplify a plan, just parse nothing
	if (convertPlan){
		ifstream * plan   = new ifstream(inputFiles[dfile]);
		ostream * outplan = &cout;
		if (pfile != -1){
			ofstream * of  = new ofstream(inputFiles[pfile]);
			if (!of->is_open()){
				cout << "I can't open " << inputFiles[pfile] << "!" << endl;
				return 2;
			}
			outplan = of;
		}
		
		
		convert_plan(*plan, *outplan);
		return 0;
	}

	if (pfile == -1 && !convertPlan){
		cout << "You need to provide a domain and problem file as input." << endl;
		return 1;
	}

	// open c-style file handle 
	FILE *domain_file = fopen(inputFiles[dfile].c_str(), "r");
	FILE *problem_file = fopen(inputFiles[pfile].c_str(), "r");

	if (!domain_file) {
		cout << "I can't open " << inputFiles[dfile] << "!" << endl;
		return 2;
	}
	if (!problem_file) {
		cout << "I can't open " << inputFiles[pfile] << "!" << endl;
		return 2;
	}
	if (!shopOutput && !hpdlOutput && !hddlOutput && !pureHddlOutput && !htn2stripsOutput && poutfile != -1){
		cout << "For ordinary pandaPI output, you may only specify one output file, but you specified two: " << inputFiles[doutfile] << " and " << inputFiles[poutfile] << endl;
	}
	
	// parsing of command line arguments has been completed	
		
		
	// parse the domain file
	run_parser_on_file(domain_file, (char*) inputFiles[dfile].c_str());
	run_parser_on_file(problem_file, (char*) inputFiles[pfile].c_str());

	if (showProperties){
		printProperties();
		return 0;
	}

	if (pureHddlOutput || htn2stripsOutput) {
		// produce streams for output
		ostream * dout = &cout;
		ostream * pout = &cout;
		if (doutfile != -1){
			ofstream * df  = new ofstream(inputFiles[doutfile]);
			if (!df->is_open()){
				cout << "I can't open " << inputFiles[doutfile] << "!" << endl;
				return 2;
			}
			dout = df;
		}
		if (poutfile != -1){
			ofstream * pf  = new ofstream(inputFiles[poutfile]);
			if (!pf->is_open()){
				cout << "I can't open " << inputFiles[poutfile] << "!" << endl;
				return 2;
			}
			pout = pf;
		}
		if (pureHddlOutput)
			hddl_output(*dout,*pout, false, true);
		else if (htn2stripsOutput)
			htn2strips_output(*dout,*pout);
		return 0;
	}

	
	if (!hpdlOutput) expand_sorts(); // add constants to all sorts
	
	// handle typeof-predicate
	if (!hpdlOutput && has_typeof_predicate) create_typeof();

	if (compileGoalIntoAction) compile_goal_into_action();
	if (removeMethodPreconditions) remove_method_preconditions();

	// do not preprocess the instance at all if we are validating a solution
	if (verifyPlan){
		ifstream * plan  = new ifstream(inputFiles[doutfile]);
		bool result = verify_plan(*plan, useOrderInPlanVerification, lenientVerify, verbosity);
		cout << "Plan verification result: ";
		if (result) cout << color(COLOR_GREEN,"true",MODE_BOLD);
		else cout << color(COLOR_RED,"false",MODE_BOLD);
		cout << endl;
		return result ? 0 : 1;
	}

	if (!hpdlOutput) {
		// flatten all primitive tasks
		flatten_tasks(compileConditionalEffects, linearConditionalEffectExpansion, encodeDisjunctivePreconditionsInMethods);
		// .. and the goal
		flatten_goal();
		// create appropriate methods and expand method preconditions
		parsed_method_to_data_structures(compileConditionalEffects, linearConditionalEffectExpansion, encodeDisjunctivePreconditionsInMethods);
	}

	if (shopOutput || hpdlOutput){
		// produce streams for output
		ostream * dout = &cout;
		ostream * pout = &cout;
		if (doutfile != -1){
			ofstream * df  = new ofstream(inputFiles[doutfile]);
			if (!df->is_open()){
				cout << "I can't open " << inputFiles[doutfile] << "!" << endl;
				return 2;
			}
			dout = df;
		}
		if (poutfile != -1){
			ofstream * pf  = new ofstream(inputFiles[poutfile]);
			if (!pf->is_open()){
				cout << "I can't open " << inputFiles[poutfile] << "!" << endl;
				return 2;
			}
			pout = pf;
		}
		if (shopOutput)	write_instance_as_SHOP(*dout,*pout);
		if (hpdlOutput)	write_instance_as_HPDL(*dout,*pout);
		return 0;
	}


	// split methods with independent parameters to reduce size of grounding
	if (splitParameters) split_independent_parameters();
	// cwa, but only if we actually want to compile negative preconditions
	if (!hpdlOutput || internalHDDLOutput) compute_cwa();
	// simplify constraints as far as possible
	reduce_constraints();
	clean_up_sorts();
	remove_unnecessary_predicates();

	// write to output
	if (verboseOutput) verbose_output(verbosity);
	else if (hddlOutput) {
		// produce streams for output
		ostream * dout = &cout;
		ostream * pout = &cout;
		if (doutfile != -1){
			ofstream * df  = new ofstream(inputFiles[doutfile]);
			if (!df->is_open()){
				cout << "I can't open " << inputFiles[doutfile] << "!" << endl;
				return 2;
			}
			dout = df;
		}
		if (poutfile != -1){
			ofstream * pf  = new ofstream(inputFiles[poutfile]);
			if (!pf->is_open()){
				cout << "I can't open " << inputFiles[poutfile] << "!" << endl;
				return 2;
			}
			pout = pf;
		}
		hddl_output(*dout,*pout, internalHDDLOutput, false);
	} else {
		ostream * dout = &cout;
		if (doutfile != -1){
			ofstream * df  = new ofstream(inputFiles[doutfile]);
			if (!df->is_open()){
				cout << "I can't open " << inputFiles[doutfile] << "!" << endl;
				return 2;
			}
			dout = df;
		}
		simple_hddl_output(*dout);
	}
}
