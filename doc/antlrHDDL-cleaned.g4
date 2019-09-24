grammar antlrHDDL;

/**
 * Created by Daniel Höller, Ulm University (daniel.hoeller@uni-ulm.de)
 * and Mario Schmautz, Ulm University (mario.schmautz@uni-ulm.de) [some refactoring & additions]
 *
 * - This file contains the grammar for the ANTLR parser generator
 * - It describes an extension of the Planning Domain Definition Language to describe hierarchical
 *   planning problems like Hybrid Planning or Hierarchical Task Network planning.
 * - It is based on the PDDL 2.1 definition by Maria Fox & Derek Long (so far, only the non-temporal part)
**/

// @IGNORE
hddl_file : domain | problem;

//
// General Structure of a Domain Definition
//
// @MODIFIED
// @LABEL {The domain definition has been extended by definitions for compound tasks and methods.}
domain : '(' 'define' '(' 'domain' domain_symbol ')'
             require_def?
             type_def?
             const_def?
             predicates_def?
             comp_task_def* // @HIGHLIGHT
             method_def* // @HIGHLIGHT
             action_def* ')';

// @PDDL
domain_symbol : NAME;

//
// Requirement Statement
//
// @PDDL
require_def : '(' ':requirements' require_defs ')';
require_defs : REQUIRE_NAME+;

//
// Type Definition
//
// @PDDL
type_def : '(' ':types' type_def_list ')';
type_def_list : NAME* | (new_types '-' var_type type_def_list) ;
new_types : NAME+;

//
// Domain Constant Definition
//
// @PDDL
const_def : '(' ':constants' typed_obj_list ')';

//
// Predicate Definition
//
// @PDDL
predicates_def : '(' ':predicates' atomic_formula_skeleton+ ')';
atomic_formula_skeleton : '(' predicate typed_var_list ')';

//
// Task Definition
//
// @HDDL
// @LABEL {Abstract tasks are defined similar to actions. To use preconditions and effects in the definition,
//         please add the requirement definition :htn-abstract-actions}
comp_task_def :
   '(' ':task' task_def;

task_def : task_symbol
      ':parameters' '(' typed_var_list ')' ')';

task_symbol : NAME;

//
// Method Definition
//
// @HDDL
// @LABEL {In a pure HTN setting, methods consist of the definition of the abstract task they may decompose as well as the
//         resulting task network. The parameters of a method are supposed to include all parameters of the abstract task
//         that it decomposes as well as those of the tasks in its network of subtasks. By setting the :htn-method-pre-eff
//         requirement, one might use method preconditions and effects similar to the ones used in SHOP.}
method_def :
   '(' ':method' method_symbol
      ':parameters' '(' typed_var_list ')'
      ':task' '(' task_symbol var_or_const* ')'
      (':precondition' gd)?
      tasknetwork_def;

//
// Task Definition
//
// @HDDL
// @LABEL {The following definition of a task network is used in method definitions as well as in the problem definition
//         to define the intial task network. It contains the definition of a number of tasks as well sets of ordering
//         constraints, variable constraints between any method parameters. Please use the requirement :htn-causal-links
//         to include causal links into the model. When the keys :ordered-subtasks or :ordered-tasks are used, the
//         network is regarded to be totally ordered. In the other cases, ordering relations may be defined in the
//         respective section. To do so, the task definition includes an id for every task that can be referenced here.
//         They are also used to define causal links. Two dedicated ids "init" and "goal" can be used in causal link
//         definition to reference the initial state and the goal definition.}
tasknetwork_def :
      ((':subtasks' | ':tasks' | ':ordered-subtasks' | ':ordered-tasks') subtask_defs)?
      ((':ordering' | ':order') ordering_defs)?
      (':constraints' constraint_defs)?
      ')';

method_symbol : NAME;

//
// Subtasks
//
// @HDDL
// @LABEL {The subtask definition may contain one or more subtasks. The tasks consist of a task symbol as well as a
//         list of parameters. In case of a method's subnetwork, these parameters have to be included in the method's
//         parameters, in case of the initial task network, they have to be defined as constants in s0 or in a dedicated
//         parameter list (see definition of the initial task network). The tasks may start with an id that can
//         be used to define ordering constraints and causal links.}
subtask_defs : '(' ')' | subtask_def | '(' 'and' subtask_def+ ')';
subtask_def : ('(' task_symbol var_or_const* ')' | '(' subtask_id '(' task_symbol var_or_const* ')' ')');
subtask_id : NAME;

//
// Ordering
//
// @HDDL
// @LABEL {The ordering constraints are defined via the task ids. They have to induce a partial order.}
ordering_defs : '(' ')' | ordering_def | '(' 'and' ordering_def+ ')';
ordering_def : '(' subtask_id '<' subtask_id ')';

//
// Variable Constraits
//
// @HDDL
// @LABEL {The variable constraints enable to codesignate or non-codesignate variables; or to enforce (or forbid) a
//         variable to have a certain type.}
// @EXAMPLE {(= ?v1 ?v2)), (not (= ?v3 ?v4))
constraint_defs : '(' ')' | constraint_def | '(' 'and' constraint_def+ ')';
constraint_def : '(' ')' | '(' 'not' equallity var_or_const var_or_const')' ')' | equallity var_or_const var_or_const ')' ;


//
// Action Definition
//
// @MODIFIED
// @LABEL {The original action definition of PDDL has been split up to reuse its body in the task definition.}
action_def :
   '(' ':action' task_def
      (':precondition' gd)?
      (':effect' effect)? ')';

//
// Goal Description
// @LABEL {gd means "goal description". It is used to define goals and preconditions.}
//
// @MODIFIED
gd : gd_empty | atomic_formula | gd_negation | gd_implication | gd_conjuction | gd_disjuction | gd_existential | gd_universal | gd_equality_constraint;
              

gd_empty : '(' ')';
gd_conjuction : '(' 'and' gd* ')';
gd_disjuction : '(' 'or' gd* ')';
gd_negation : '(' 'not' gd ')';
gd_implication : '(' 'imply' gd gd ')'; // new
gd_existential : '(' 'exists' '(' typed_var_list ')' gd ')';
gd_universal : '(' 'forall' '(' typed_var_list ')' gd ')';

gd_equality_constraint : equallity var_or_const var_or_const ')';


//
// Effects
//
// @LABEL {In contrast to earlier versions of this grammar, nested conditional effects are now permitted.
//         This is not allowed in PDDL 2.1}
effect : eff_empty | eff_conjunction | eff_universal | eff_conditional | literal;

eff_empty : '(' ')';
eff_conjunction : '(' 'and' effect* ')';
eff_universal : '(' 'forall' '(' typed_var_list ')' effect ')';
eff_conditional : '(' 'when' gd effect ')';

literal : neg_atomic_formula | atomic_formula;
neg_atomic_formula : '(' 'not' atomic_formula ')';

//
// Basic Definitions
//

// @LABEL {Predicate and atom definition:}
atomic_formula : '('predicate var_or_const*')';
predicate : NAME;

// @LABEL {Special "predicate" for equallity:}
equallity : '(' '=' | '(=';

// @LABEL {Lists of typed variables and objects:}
typed_var_list : typed_vars*;
typed_obj_list : typed_objs*;

// @LABEL {One or more variable names, followed by a type:}
typed_vars : VAR_NAME+ '-' var_type;
typed_var : VAR_NAME '-' var_type;
typed_objs : new_consts+ '-' var_type;
new_consts : NAME;
var_type : NAME | '(' 'either' var_type+ ')';

// @LABEL {"require"-statements start with a ":"-symbol:}
REQUIRE_NAME : ':' NAME;

// @LABEL {Names of variables start with a "?":}
var_or_const : NAME | VAR_NAME;
VAR_NAME : '?'NAME;

// @LABEL {Basic name definition:}
term : NAME | VAR_NAME | functionterm;
functionterm : '(' func_symbol term* ')';
func_symbol : NAME;
NAME : [a-zA-Z][a-zA-Z0-9\-_]* ;
COMMENT : (';' ~[\r\n]* ('\r'|'\n') ('\r'|'\n')? ) -> skip ;
WS : [ \t\r\n]+ -> skip ;
NUMBER : [0-9][0-9]* '.'? [0-9]* | '.' [0-9]*;

//
/*********************************************************************************/
//
// Problem Definition
//

problem : '(' 'define' '(' 'problem' NAME ')'
              '(' ':domain' NAME ')'
              require_def?
              p_object_declaration?
              p_htn?
              p_init
              p_goal?
              ')';

p_object_declaration : '(' ':objects' typed_obj_list')';
p_init : '(' ':init' init_el*')';
init_el : literal | num_init;
num_init : equallity f_head NUMBER ')';
p_goal : '(' ':goal' gd ')';

p_htn : '(' ':htn'
        (':parameters' '(' typed_var_list ')')?
        tasknetwork_def;
