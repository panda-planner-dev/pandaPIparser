#ifndef __HDDLWRITER
#define __HDDLWRITER

#include "parsetree.hpp"

void hddl_output(ostream & dout, ostream & pout, bool internalHDDLOutput, bool usedParsed);


tuple<vector<string>,
	  vector<int>,
	  map<string,string>,
	  map<int,int> > compute_local_type_hierarchy();

void print_indent(ostream & out, int indent, bool end = false);
void print_var_and_const(ostream & out, var_and_const & vars);
void print_formula(ostream & out, general_formula * f, int indent);
void print_formula_for(ostream & out, general_formula * f, string topic);

#endif
