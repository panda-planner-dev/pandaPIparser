#include "shopWriter.hpp"
#include "cwa.hpp"
#include <iostream>
#include <cassert>
#include <variant>
#include <strings.h> // for ffs
#include <bitset>


const string shop_type_predicate_prefix = "(type_";

string sanitise(string in){
	if (in == "call") return "_call";
	return in;
}

void write_literal_list_SHOP(ostream & dout, vector<literal> & literals){
	bool first = true;
	for (literal & l : literals){
		if (!first) dout << " "; else first=false;
		dout << "(";
		if (!l.positive) dout << "not (";
		dout << sanitise(l.predicate);
		for (string arg : l.arguments) dout << " " << arg;
		if (!l.positive) dout << ")";
		dout << ")";
	}
}


struct shop_order {
	bool isParallel; // if not it is sequential
	vector<variant<string,shop_order*>> elements;
};


bool shop_order_matrix[64][64];


shop_order* extract_shop_order_dfs(vector<string> & ids, int bitmask){
	//cout << "Split " << bitset<5>(bitmask) << endl;	
	if (__builtin_popcount(bitmask) == 1) return 0;
	
	// try to find a subset to split on
	for (int i = 1; i < bitmask; i++){
		if (i & ~bitmask) continue; // if there is any bit set in i that is not set in bitmask
		// check
		bool anyOrder = false;
		bool allAfter = true;

		for (size_t ax = 0; ax < ids.size(); ax++){
			int a = 1 << ax;
			if (!(a & bitmask)) continue;
			if (a & ~i) continue;
			for (size_t bx = 0; bx < ids.size(); bx++){
				int b = 1 << bx; 
				if (!(b & bitmask)) continue;
				if (i & b) continue;
				if (shop_order_matrix[ax][bx] || shop_order_matrix[bx][ax]) anyOrder = true;
				if (!shop_order_matrix[ax][bx]) allAfter = false;
			}
		}


		if (anyOrder && (!allAfter)) continue;
		//cout << "Try " << bitset<5>(i) << " anyorder " << anyOrder << " allorder " << allAfter << endl;	


		shop_order* first = extract_shop_order_dfs(ids, i);
		shop_order* second = extract_shop_order_dfs(ids, bitmask & (~i));

		shop_order* ret = new shop_order;
		if (first) ret->elements.push_back(first);
		else ret->elements.push_back(ids[ffs(i)-1]);
		
		if (second) ret->elements.push_back(second);
		else ret->elements.push_back(ids[ffs(bitmask & (~i))-1]);


		if (!anyOrder) ret->isParallel = true;
		else ret->isParallel = false;

		return ret;
	}
	assert(false);
	return NULL;
}

shop_order* extract_shop_order(vector<pair<string,string>> ordering, vector<string> ids){
	if (!ids.size()) return NULL;
	if (ids.size() == 1) {
		shop_order* ret = new shop_order;
		ret->isParallel = false;
		ret->elements.push_back(ids[0]);
		return ret;
	}
	
	map<string,int> ids_to_int;
	for (size_t i = 0; i < ids.size(); i++) ids_to_int[ids[i]] = i;
	for (size_t i = 0; i < ids.size(); i++)
		for (size_t j = 0; j < ids.size(); j++)
			shop_order_matrix[i][j] = false;

	for (pair<string,string> & o : ordering)
		shop_order_matrix[ids_to_int[o.first]][ids_to_int[o.second]] = true;
	
	for (size_t k = 0; k < ids.size(); k++)
		for (size_t i = 0; i < ids.size(); i++)
			for (size_t j = 0; j < ids.size(); j++)
				if (shop_order_matrix[i][k] && shop_order_matrix[k][j])
					shop_order_matrix[i][j] = true;

	/*for (size_t i = 0; i < ids.size(); i++){
		for (size_t j = 0; j < ids.size(); j++)
			cout << shop_order_matrix[i][j] << " "; 
		cout << endl;
	}*/

	return extract_shop_order_dfs(ids,(1<<ids.size()) - 1);
}

shop_order* simplify_shop_order(shop_order* ord){
	vector<variant<string,shop_order*>> sub;
	for (variant<string,shop_order*> s : ord->elements)
		if (holds_alternative<string>(s))
			sub.push_back(s);
		else
			sub.push_back(simplify_shop_order(get<shop_order*>(s)));

	ord->elements.clear();
	for (variant<string,shop_order*> s : sub)
		if (holds_alternative<string>(s))
			ord->elements.push_back(s);
		else if (ord->isParallel != get<shop_order*>(s)->isParallel)
			ord->elements.push_back(s);
		else {
			// expand
			for (variant<string,shop_order*> ss : get<shop_order*>(s)->elements)
				ord->elements.push_back(ss);
		}
	return ord;
}


void write_shop_order(ostream & dout, shop_order* order, map<string,plan_step> & idmap, set<string> names_of_primitives){
	if (!order) return;
	dout << "(";
	if (order->isParallel) dout << ":unordered ";

	bool first = true;
	for (variant<string,shop_order*> elem : order->elements){
		if (first) first = false; else dout << " ";
		if (holds_alternative<string>(elem)){
			plan_step ps = idmap[get<string>(elem)];
			dout << "(";
			if (names_of_primitives.count(ps.task)) dout << "!";
		   	dout << sanitise(ps.task);
			for (string & arg : ps.args) dout << " " << arg;
			dout << ")";
		} else
			write_shop_order(dout, get<shop_order*>(elem),idmap, names_of_primitives);
	}
	
	dout << ")";
}


void write_instance_as_SHOP(ostream & dout, ostream & pout){
	dout << "(defdomain domain (" << endl;

	set<string> names_of_primitives; // this is needed for writing the !'s in methods
	// output all actions, in shop they are named operators
	for (task & prim : primitive_tasks){
		names_of_primitives.insert(prim.name);
		dout << "  (:operator (!" << sanitise(prim.name);
		// arguments
		for (pair<string,string> var : prim.vars)
			dout << " " << var.first;
		dout << ")" << endl;
		
		// precondition
		dout << "    ;; preconditions" << endl;
		dout << "    (" << endl;
		dout << "      ";
		for (size_t i = 0; i < prim.vars.size(); i++){
			if (i) dout << " ";
			dout << shop_type_predicate_prefix << prim.vars[i].second << " " << prim.vars[i].first << ")";
		}
		dout << endl << "      ";
		write_literal_list_SHOP(dout, prim.prec);
		// write constraints
		for (literal & constraint : prim.constraints){
			dout << " (";
			if (!constraint.positive) dout << "not (";
			dout << "call =";
			for (string arg : constraint.arguments) dout << " " << arg;
			if (!constraint.positive) dout << ")";
			dout << ")";
		}
		dout << endl << "    )" << endl;

		vector<literal> add, del;
		for (literal & l : prim.eff)
			if (l.positive) add.push_back(l); else {
				del.push_back(l);
				del[del.size()-1].positive = true;
			}


		// delete effects
		dout << "    ;; delete effects" << endl;
		dout << "    (";
		write_literal_list_SHOP(dout, del);
		dout << ")" << endl;

		// add effects
		dout << "    ;; add effects" << endl;
		dout << "    (";
		write_literal_list_SHOP(dout, add);
		dout << ")" << endl;


		dout << "  )" << endl;
	}


	for (method & m : methods){
		dout << "  ;; method named " << m.name << endl;
		dout << "  (:method (" << sanitise(m.at);
		for (string & atarg : m.atargs)
			dout << " " << atarg;
		dout << ")" << endl;

		// find the corresponding at
		task at; bool found = false;
		for (task & a : abstract_tasks)
			if (a.name == m.at){
				at = a;
				found = true;
				break;
			}

		if (! found) {
			cerr << "method " << m.name << " decomposes unknown task " << m.at << endl;
			_Exit(1);
		}


		// method precondition
		dout << "    ("  << endl << "      ";
		// typing constraints of the AT
		for (size_t i = 0; i < at.vars.size(); i++){
			if (i) dout << " ";
			dout << shop_type_predicate_prefix << at.vars[i].second << " " << m.atargs[i] << ")";
		}
		dout << endl << "      ";
		// typing of the method
		bool first = true;
		for (pair<string,string> & v : m.vars){
			if (first) first = false; else dout << " ";
			dout << shop_type_predicate_prefix << v.second << " " << v.first << ")";
		}
		dout << endl << "      ";
		// method precondition in the input
		for (plan_step & ps : m.ps){
			if (ps.task.rfind("method_precondition_", 0)) continue;
			found = false;
			for (task & p : primitive_tasks)
				if (p.name == ps.task){
					found = true;
					assert(p.eff.size() == 0);
					write_literal_list_SHOP(dout,p.prec);
					break;
				}
			assert(found);
		}

		dout << endl << "    )" << endl;

		// subtasks
		
		vector<string> ids;
		map<string,plan_step> idmap;
		for (plan_step & ps : m.ps){
			if (ps.task.rfind("method_precondition_", 0) == 0) continue;
			ids.push_back(ps.id);
			idmap[ps.id] = ps;
		}

		vector<pair<string,string>> filtered_ordering;
		for (pair<string,string> & ord : m.ordering)
			if (idmap.count(ord.first) && idmap.count(ord.second))
				filtered_ordering.push_back(ord);
		// else the ordering pertains to the method precondition. At this point, there can be only one so just ignoring these constraints is ok -- they cannot introduce transitive constraints, that are relevant
		
		if (ids.size()){	
			shop_order* order = extract_shop_order(filtered_ordering,ids);
			order = simplify_shop_order(order);
			dout << "    ";
			write_shop_order(dout,order,idmap,names_of_primitives);
			dout << endl;	
		} else {
			// empty method
			dout << "    ()" << endl;
		}
		


		dout << "  )" << endl;
	}
	
	
	dout << "))" << endl;


	//-------------------------------------------
	// write the problem instance
	pout << "(defproblem problem domain " << endl;
	pout << "  (" << endl;
	// initial state
	for (ground_literal & gl : init){
		if (!gl.positive) continue;
		pout << "    (" << sanitise(gl.predicate);
		for (string & arg : gl.args)
			pout << " " << arg;
		pout << ")" << endl;
	}
	
	// type assertions
	for (auto & entry : sorts){
		for (const string & constant : entry.second){
			pout << "    " << shop_type_predicate_prefix << entry.first << " " << constant << ")" << endl;
		}	
	}

	pout << "  )" << endl;
	pout << "  ((__top))" << endl;
	pout << ")" << endl;

}

