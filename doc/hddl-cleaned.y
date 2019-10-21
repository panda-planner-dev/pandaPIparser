%{
	// Declare stuff from Flex that Bison needs to know about:
	extern int yylex();
	extern int yyparse();
	extern FILE *yyin;
	
	void yyerror(const char *s);
%}

%locations

%token KEY_TYPES KEY_DEFINE KEY_DOMAIN KEY_PROBLEM KEY_REQUIREMENTS KEY_PREDICATES 
%token KEY_TASK KEY_CONSTANTS KEY_ACTION KEY_PARAMETERS KEY_PRECONDITION KEY_EFFECT KEY_METHOD
%token KEY_GOAL KEY_INIT KEY_OBJECTS KEY_HTN 
%token KEY_AND KEY_OR KEY_NOT KEY_IMPLY KEY_FORALL KEY_EXISTS KEY_WHEN KEY_INCREASE KEY_TYPEOF
%token KEY_CONSTRAINTS KEY_ORDER KEY_ORDER_TASKS KEY_TASKS 
%token <sval> NAME REQUIRE_NAME VAR_NAME 
%token <ival> INT


%%
document: domain | problem


domain: '(' KEY_DEFINE '(' KEY_DOMAIN domain_symbol ')'
        domain_defs 
		')' 
domain_defs:	domain_defs require_def |
				domain_defs type_def |
				domain_defs const_def |
				domain_defs predicates_def |
				domain_defs task_def |
				domain_defs method_def |

problem: '(' KEY_DEFINE '(' KEY_PROBLEM NAME ')'
              '(' KEY_DOMAIN NAME ')'
			problem_defs
		')'

problem_defs: problem_defs require_def |
              problem_defs p_object_declaration |
              problem_defs p_htn | 
              problem_defs p_init | 
              problem_defs p_goal | 
              problem_defs p_constraint | 

p_object_declaration : '(' KEY_OBJECTS constant_declaration_list')';
p_init : '(' KEY_INIT init_el ')';
init_el : init_el literal |
p_goal : '(' KEY_GOAL gd ')' 

htn_type: KEY_HTN | KEY_TIHTN 
parameters-option: KEY_PARAMETERS '(' typed_var_list ')'  | 
p_htn : '(' htn_type
        parameters-option
        tasknetwork_def
		')' 

p_constraint : '(' KEY_CONSTRAINTS gd ')'

/////////////////////////////////////////////////////////////////////////////////////////////////////
// @PDDL
domain_symbol : NAME

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Requirement Statement
// @PDDL
require_def : '(' KEY_REQUIREMENTS require_defs ')'
require_defs : require_defs REQUIRE_NAME  | 



/////////////////////////////////////////////////////////////////////////////////////////////////////
// Type Definition
// @PDDL
type_def : '(' KEY_TYPES type_def_list ')';
type_def_list : NAME-list 
			  | NAME-list-non-empty '-' NAME type_def_list 



/////////////////////////////////////////////////////////////////////////////////////////////////////
// Constant Definition
const_def : '(' KEY_CONSTANTS constant_declaration_list ')'
// this is used in both the domain and the initial state
constant_declaration_list : constant_declaration_list constant_declarations |
constant_declarations : NAME-list-non-empty '-' NAME 

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Predicate Definition
// @PDDL
predicates_def : '(' KEY_PREDICATES atomic_predicate_def-list ')'

atomic_predicate_def-list : atomic_predicate_def-list atomic_predicate_def  | 
atomic_predicate_def : '(' NAME typed_var_list ')' 


/////////////////////////////////////////////////////////////////////////////////////////////////////
// Task Definition
// @HDDL
// @LABEL Abstract tasks are defined similar to actions. To use preconditions and effects in the definition,
//         please add the requirement definition :htn-abstract-actions
// @NOTE  tasks, i.e. abstract tasks, will not have preconditions and effects. This rule is just for ease of parsing

task_or_action: KEY_TASK  | KEY_ACTION 

task_def : '(' task_or_action NAME
			KEY_PARAMETERS '(' typed_var_list ')'
			precondition_option
			effect_option ')'

precondition_option: KEY_PRECONDITION gd  | 
effect_option: KEY_EFFECT effect  | 

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Method Definition
// @HDDL
// @LABEL In a pure HTN setting, methods consist of the definition of the abstract task they may decompose as well as the
//         resulting task network. The parameters of a method are supposed to include all parameters of the abstract task
//         that it decomposes as well as those of the tasks in its network of subtasks. By setting the :htn-method-pre-eff
//         requirement, one might use method preconditions and effects similar to the ones used in SHOP.
method_def :
   '(' KEY_METHOD NAME
      KEY_PARAMETERS '(' typed_var_list ')'
      KEY_TASK '(' NAME var_or_const-list ')'
      precondition_option
      tasknetwork_def
	')'

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Task Definition
// @HDDL
// @LABEL  The following definition of a task network is used in method definitions as well as in the problem definition
//         to define the intial task network. It contains the definition of a number of tasks as well sets of ordering
//         constraints, variable constraints between any method parameters. Please use the requirement :htn-causal-links
//         to include causal links into the model. When the keys :ordered-subtasks or :ordered-tasks are used, the
//         network is regarded to be totally ordered. In the other cases, ordering relations may be defined in the
//         respective section. To do so, the task definition includes an id for every task that can be referenced here.
//         They are also used to define causal links. Two dedicated ids "init" and "goal" can be used in causal link
//         definition to reference the initial state and the goal definition.
tasknetwork_def :
	subtasks_option
	ordering_option
	constraints_option	

subtasks_option: 	  KEY_TASKS subtask_defs 
			   		| KEY_ORDER_TASKS subtask_defs 
					| 

ordering_option: KEY_ORDER ordering_defs | 
constraints_option: KEY_CONSTRAINTS constraint_def  | 

 /////////////////////////////////////////////////////////////////////////////////////////////////////
// Subtasks
// @HDDL
// @LABEL The subtask definition may contain one or more subtasks. The tasks consist of a task symbol as well as a
//         list of parameters. In case of a method's subnetwork, these parameters have to be included in the method's
//         parameters, in case of the initial task network, they have to be defined as constants in s0 or in a dedicated
//         parameter list (see definition of the initial task network). The tasks may start with an id that can
//         be used to define ordering constraints and causal links.
subtask_defs : '(' ')' 
			  | subtask_def 
			  | '(' KEY_AND subtask_def-list ')' 
subtask_def-list : subtask_def-list subtask_def 
				 | 
subtask_def :	'(' NAME var_or_const-list ')' 
			  | '(' NAME '(' NAME var_or_const-list ')' ')' 

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Ordering
//
// @HDDL
// @LABEL The ordering constraints are defined via the task ids. They have to induce a partial order.
ordering_defs : '(' ')'
			   | ordering_def 
			   | '(' KEY_AND ordering_def-list ')' 
ordering_def-list: ordering_def-list ordering_def 
				 | 
ordering_def : '(' NAME '<' NAME ')' 

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Variable Constraits
// @HDDL
// @LABEL The variable constraints enable to codesignate or non-codesignate variables, or to enforce (or forbid) a
//         variable to have a certain type.
// @EXAMPLE (= ?v1 ?v2)), (not (= ?v3 ?v4)), (sort ?v - type), (not (sort ?v - type))
constraint_def-list: constraint_def-list constraint_def 
				    | 
constraint_def : '(' ')' 
				| '(' KEY_AND constraint_def-list ')' 
				| '(' '=' var_or_const var_or_const ')' 
				| '(' KEY_NOT '(' '=' var_or_const var_or_const ')' ')' 

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Goal Description
// @LABEL gd means "goal description". It is used to define goals and preconditions. The PDDL 2.1 definition has been extended by the LTL defintions given by Gerevini andLon "Plan Constraints and Preferences in PDDL3"
gd :  gd_empty 
	| atomic_formula 
	| gd_negation 
	| gd_implication 
	| gd_conjuction 
	| gd_disjuction 
	| gd_existential 
	| gd_universal 
	| gd_equality_constraint 

gd-list : gd-list gd  
		| 

gd_empty : '(' ')' 
gd_conjuction : '(' KEY_AND gd-list ')' 
gd_disjuction : '(' KEY_OR gd-list ')' 
gd_negation : '(' KEY_NOT gd ')' 
gd_implication : '(' KEY_IMPLY gd gd ')' 
gd_existential : '(' KEY_EXISTS '(' typed_var_list ')' gd ')'  
gd_universal : '(' KEY_FORALL '(' typed_var_list ')' gd ')'  
gd_equality_constraint : '(' '=' var_or_const var_or_const ')' 

var_or_const-list :   var_or_const-list NAME 
					| var_or_const-list VAR_NAME 
					| 

var_or_const : NAME | VAR_NAME 
atomic_formula : '('NAME var_or_const-list')' 



/////////////////////////////////////////////////////////////////////////////////////////////////////
// Effects
// @LABEL In contrast to earlier versions of this grammar, nested conditional effects are now permitted.
//         This is not allowed in PDDL 2.1

effect-list: effect-list effect  
		| 

effect : eff_empty 
	   | eff_conjunction 
	   | eff_universal 
	   | eff_conditional 
	   | literal 

eff_empty : '(' ')' 
eff_conjunction : '(' KEY_AND effect-list ')' 
eff_universal : '(' KEY_FORALL '(' typed_var_list ')' effect ')'
eff_conditional : '(' KEY_WHEN gd effect ')' 


literal : neg_atomic_formula  | atomic_formula 
neg_atomic_formula : '(' KEY_NOT atomic_formula ')' 


/////////////////////////////////////////////////////////////////////////////////////////////////////
// elementary list of names
NAME-list-non-empty: NAME-list NAME 
NAME-list: NAME-list NAME 
			|   


/////////////////////////////////////////////////////////////////////////////////////////////////////
// elementary list of variable names
VAR_NAME-list-non-empty: VAR_NAME-list VAR_NAME 
VAR_NAME-list: VAR_NAME-list VAR_NAME 
			|   

/////////////////////////////////////////////////////////////////////////////////////////////////////
// lists of variables
typed_vars : VAR_NAME-list-non-empty '-' NAME 
typed_var : VAR_NAME '-' NAME 
typed_var_list : typed_var_list typed_vars 
			   |  

%%
void run_parser_on_file(FILE* f){
	yyin = f;
	yyparse();
}

void yyerror(const char *s) {
  cout << "Parse error in line " << yylloc.first_line << endl << "Message: " << s << endl;
  // might as well halt now:
  exit(-1);
}
