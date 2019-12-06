#include "hpdlWriter.hpp"
#include "parsetree.hpp"
#include "domain.hpp" // for sorts of constants
#include "cwa.hpp"
#include <iostream>
#include <algorithm>
#include <cassert>
#include <variant>
#include <bitset>
#include <functional>


function<string(string)> variable_output_closure(map<string,string> var2const){
    return [var2const](string varOrConst) mutable { 
		// a variable that is not a parameter results from a constant being compiled away
		if (var2const.count(varOrConst)) return var2const[varOrConst];
        
		return varOrConst;
    };
}

function<string(string)> variable_declaration_closure(map<string,string> method2task, map<string,string> var2const, parsed_method & m, set<string> & declared){
    return [method2task,var2const,declared,m](string varOrConst) mutable {
		if (varOrConst[0] != '?') return varOrConst;
		// check if this variable is bound to a AT argument
		if (method2task.find(varOrConst) != method2task.end())
		   return method2task[varOrConst];
		if (declared.count(varOrConst)) return varOrConst;
		// write declaration
		for (auto varDecl :  m.vars->vars)
			if (varDecl.first == varOrConst){
				declared.insert(varOrConst);
		        return varOrConst + " - " + varDecl.second;
			}
		// a variable that is not a parameter results from a constant being compiled away
		if (var2const.count(varOrConst)) return var2const[varOrConst];

		cout << "FAIL !! for " << varOrConst << endl;
		exit(1);
		return varOrConst;
    };
}

void write_HPDL_parameters(ostream & out, parsed_task & task){
	out << "    :parameters (";
	bool first = true;
	for (pair<string,string> var : task.arguments->vars){
		if (! first) out << " ";
	    first = false;	
		out << var.first << " - " << var.second;
	}
	out << ")" << endl;
}


inline void write_HPDL_indent(ostream & out, int indent){
	for (int i = 0; i < indent; i++) out << "  ";
}

void write_HPDL_general_formula(ostream & out, general_formula * f, function<string(string)> & var, int indent){
	if (!f) return;
	if (f->type == EMPTY) return;

	if (f->type == ATOM || f->type == NOTATOM){
		write_HPDL_indent(out,indent);
		if (f->type == NOTATOM) out << "(not ";
		out << "(" << f->predicate;
		for (string & v : f->arguments.vars) out << " " << var(v);
		if (f->type == NOTATOM) out << ")";
		out << ")" << endl;
	}
	
	if (f->type == AND || f->type == OR){
		write_HPDL_indent(out,indent);
		if (f->type == AND) out << "(and" << endl;
		if (f->type == OR)  out << "(or"  << endl;
		if (f->type == FORALL) out << "(forall";
		if (f->type == EXISTS)  out << "(exists";
		
		if (f->type == FORALL || f->type == EXISTS){
			out << " (";
			int first = 0;
			for(pair<string,string> varDecl : f->qvariables.vars){
				if (first++) out << " ";
				out << varDecl.first << " - " << varDecl.second;
			}
			out << ")" << endl;
		}
		
		
		// write subformulae
		for (general_formula* s : f->subformulae) write_HPDL_general_formula(out,s,var,indent+1);

		write_HPDL_indent(out,indent);
		out << ")" << endl;
	}
}

void write_HPDL_general_formula_outer_and(ostream & out, general_formula * f, function<string(string)> & var, int indent=1){
	if (!f) return;
	if (f->type == EMPTY) return;

	if (f->type == AND){
		for (general_formula* s : f->subformulae) write_HPDL_general_formula(out,s,var, indent);
	} else {
		write_HPDL_general_formula(out,f,var,indent);
	}
}

void add_var_for_const_to_map(additional_variables additionalVars, map<string,string> & var2const){
	for(pair<string,string> varDecl : additionalVars){
		// determine const of this sort
		assert(sorts[varDecl.second].size() == 1);
		var2const[varDecl.first] = *(sorts[varDecl.second].begin());
	}
}

vector<sub_task*> get_tasks_in_total_order(vector<sub_task*> tasks, vector<pair<string,string>*> & ordering){
	// we can do this inefficiently, as methods are usually small
	vector<sub_task*> ordered_subtasks;

	map<string,int> pre; // number of predecessors
	for (pair<string,string>* p : ordering) pre[p->second]++;

	for (unsigned int r = 0; r < tasks.size(); r++){
		for (unsigned int i = 0; i < tasks.size(); i++){
			if (pre[tasks[i]->id] == -1) continue; // already added
			if (pre[tasks[i]->id]) continue; // still has predecessors
			ordered_subtasks.push_back(tasks[i]);
			for (pair<string,string>*  p : ordering)
				if (pre[p->first] == pre[tasks[i]->id])
					pre[p->second]--;
			pre[tasks[i]->id] = -1;

			break;
		}
	}

	return ordered_subtasks;
}

void write_instance_as_HPDL(ostream & dout, ostream & pout){
	dout << "(define (domain dom)" << endl;
	dout << "  (:requirements " << endl << "    :typing" << endl << "  )" << endl;
	dout << "  (:types " << endl;
	for (sort_definition sort_def : sort_definitions){
		dout << "   ";
		for (string sort : sort_def.declared_sorts)
			dout << " " << sort;
		if (sort_def.has_parent_sort)
			dout << " - " << sort_def.parent_sort;
		dout << endl;
	}
	dout << "  )" << endl;

	dout << endl << endl;
	dout << "  (:predicates" << endl;
	for (predicate_definition pred_def : predicate_definitions){
		dout << "    (" << pred_def.name;
		for(unsigned int i = 0; i < pred_def.argument_sorts.size(); i++)
			dout << " ?var" << i << " - " << pred_def.argument_sorts[i];
		dout << ")" << endl;
	}
	dout << "  )" << endl;

	dout << endl << endl;
	
	// write abstract tasks
	for (parsed_task & at : parsed_abstract){
		dout << "  (:task " << at.name << endl;
		write_HPDL_parameters(dout,at);
		
		// HPDL puts methods into the abstract tasks, so output them
		for (parsed_method & method : parsed_methods[at.name]){
			dout << "    (:method "  << method.name << endl;

			map<string,string> method2Task;
			map<string,string> task2Method;
			for (unsigned int i = 0; i < method.atArguments.size(); i++){
				method2Task[method.atArguments[i]] = at.arguments->vars[i].first;
				task2Method[at.arguments->vars[i].first] = method.atArguments[i];
			}

			map<string,string> varsForConst;
			add_var_for_const_to_map(method.newVarForAT,varsForConst);
			for (sub_task* st : method.tn->tasks)
				add_var_for_const_to_map(st->arguments->newVar,varsForConst);


			// preconditions
			dout << "      :precondition (and " << endl;
			set<string> state_declared_variables;
			auto variable_declaration = variable_declaration_closure(method2Task,varsForConst,method,state_declared_variables);
			write_HPDL_general_formula_outer_and(dout,method.prec,variable_declaration,4);
			dout << "      )" << endl;

			// subtasks
			dout << "      :tasks (" << endl;
			for (sub_task* t : get_tasks_in_total_order(method.tn->tasks,method.tn->ordering)){
				dout << "        (" << t->task;
				for (string & var : t->arguments->vars)
					dout << " " << variable_declaration(var);
				dout << ")" << endl;
			}
			dout << "      )" << endl;

			dout << "    )" << endl;
		}


		dout << "  )" << endl << endl;
	}
	
	return;
	for (parsed_task prim : parsed_primitive){
		map<string,string> varsForConst;
		auto simple_variable_output = variable_output_closure(varsForConst);
		add_var_for_const_to_map(prim.prec->variables_for_constants(),varsForConst);
		add_var_for_const_to_map(prim.eff->variables_for_constants(),varsForConst);

		dout << "  (:action " << prim.name << endl;
		write_HPDL_parameters(dout,prim);
		// preconditions
		dout << "    :precondition (and " << endl;
		write_HPDL_general_formula_outer_and(dout,prim.prec,simple_variable_output ,3);
		dout << "    )" << endl;
		// effects
		dout << "    :effect (and " << endl;
		write_HPDL_general_formula_outer_and(dout,prim.eff,simple_variable_output, 3);
		dout << "    )" << endl;


		dout << "  )" << endl << endl;
	}
	
	dout << ")" << endl;
}
