---
geometry: "left=1.5cm,right=1.5cm,top=1.5cm,bottom=1.5cm"
--- 

## pandaPIparser - Description of the output format

### General rules
Any line that starts with a # sign is a comment line.
It is to be ignored.
Comments only occur at the beginning of a line, i.e., if the # sign occurs at a position other than the first, it does not indicate a comment.

There are no linebreaks or empty lines except if mentioned in this description. Listed items are always separated by a single space between them. There leading spaces, but lists might contain trailing spaces.

All indices are 0-indexed, as are all lists.

### Sections
The output is roughly split into 4 sections

1. The logic section
2. The tasks section
3. The method section
4. The init/goal section.

There are no delimiters between these sections.
They simply follow one another without any marker or empty line.

### The Logic Section

The logic section consists of

1. One line containing two space-separated non-negative integers ``C S``, where ``C`` is the number of constants and ``S`` is the number of sorts.
2. C many lines, each containing a string (not containing a space), listing the constants. The i^th^ such line will be the constant numbered i (0-indexed).
3. S many lines. Each such line declares a sort and provides the full list of constants that are members of this sort. Note that one constant might occur in multiple sorts. Each contains
    1. One string (not containing a space), the sort's name, followed by a space and
    2. A space-separated list of non-negative integers all smaller than C, each indicating a constant.
4. One line containing a space-separated non-negative integer ``P`` -- the number of predicates.
5. P many lines. Each predicate has a name and a list of arguments. For each argument, the sort of that argument is given.
  Each line comprises
    1. One string (not containing a space), the predicate's name, followed by a space and
    2. The number of arguments and
    3. A space-separated list of non-negative integers all smaller than S, each indicating a sort.
6. One line containing a space-separated non-negative integer ``F`` -- the number of functions.
7. F many lines. Each function has a name and a list of arguments. For each argument, the sort of that argument is given.
  Each line comprises
    1. One string (not containing a space), the function's name, followed by a space and
    2. A space-separated list of non-negative integers all smaller than S, each indicating a sort.


### The Task Section

The task section consists of

1. One line containing two space-separated non-negative integers ``A C``, where ``A`` is the number of primitive tasks (or actions) and ``C`` is the number of abstract (or compound) tasks.
2. Then A many blocks, each describing one action follow. Each action block comprises
    1. One line containing
        1. A string (not containing a space), the action's name
        2. A non-negative number ``OV`` indicating the number of argument variables this action had in the input domain
        3. A non-negative number ``V`` indicating the total number of argument variables of this action. It has to be $OV \leq V$

    *Note:*
    The argument variables of this action are numbered 0 to V-1.
    The first variables, i.e., 0 to OV-1, are those that originally appeared in the input domain. The additional variables OV to V-1 are variables that have been added by the parser.
    2. One line containing V many integers from 0 to S-1. The i^th^ of these numbers $s_i$ indicates that the variable i is of the sort $s_i$.
    3. One line containing the number of cost statements for this action.
    4. One line for each cost statement. This line contains either
        1. The string ``const`` followed by a space and an integer indicating a cost, or
        2. The string ``var`` followed by a number ``f`` indicating a function, then followed by as many arguments as this function takes. The arguments are indices of argumments of this action. The cost of this action is then the result of the evaluation of the function ``f`` on the given arguments.
    5. One line containing the number of preconditions.
    6. One line for each precondition containing:
        1. An integer ``p`` indicating the predicate of this precondition.
        2. As many integers ``v`` as this predicate takes arguments. These integers refer to the arguments of the action that are to be the arguments of that predicate. I.e. a line ``2 1 5 0`` indicates that the precondition is on predicate No 2, and its arguments are the first, fifth, and zeroth argument of the action.
    7. One line containing the number of unconditional adding effects.
    8. One line for each adding effect, described in the same way as preconditions are.
    9. One line containing the number of conditional adding effects.
    10. One line for each conditional adding effect, containing
        1. A number ``c`` indicating the number of conditions.
        2. Then follow ``c`` many descriptions of preconditions, in the same format as general precondoitions all in the same line (attention: you need to extract the number of arguments per predicate to parse this list correctly).
            3. Then follows one description of an adding effect, in the same format as a precondition.
    11. One line containing the number of unconditional deleting effects.
    12. One line for each deleting effect, described in the same way as preconditions are.
    13. One line containing the number of conditional deleing effects.
    14. One line for each conditional deleing effect, describing it in the same way that conditional adding effects are.
    15. One line containing a number ``c``, the number of variable constraints.
    16. One line for each variable constraint, containing
        1. Either the string ``=`` for an equals constraint, or ``!=`` for a not-equals constraint
        2. A number ``v1`` referring to an argument of the action
        3. A number ``v2`` referring to an argument of the action  
        The implied constraint is that $v_1 = v_2$ or $v_1 \neq v_2$, respectively.
3. Then follow C many blocks, each descibing an abstract task. Each such block has the same format as an action block, but only contains the sub-elements 1-2.


*Note* the primitive and abstract tasks are implicitly numbered by their order in the input file.
The action occuring first has number 0, the second one number 1, and so on. The numbering does _not_ start anew for the abstract tasks. I.e. the first abstract task is always numbered A.


### The Method Section

The method section consists of

1. One line containing a single non-negative integer ``M``.
2. Then follow ``M`` many blocks, one for each method. Each such block consists of
    1. A single line containing
        1. A string (without spaces) indicating ther name of the method
        2. A number ``t`` with $A \leq t < A+C$ indicating the task that this method decomposes
        3. A number ``v`` indicating the number of variables for this method. The variables are numbered 0 to $v-1$.
    2. One line containing V many integers from 0 to S-1. The i^th^ of these numbers $s_i$ indicates that the variable i is of the sort $s_i$.
    3. One line containing as many integers as the abstract task ``t`` has parameters, each referring to a variable of this method, indicating which variables are those the arguments of the task to be decomposed.
    4. One line with a single non-negative integer ``s`` indicating the number of sub tasks.  
    5. Then for each subtask one line describing the subtask containing
        1. An integer ``ts`` indicating the number of the task of this subtask
        2. The method variables that are its arguments.
    *Note* The subtasks are implicitly numbered 0 to $s-1$ as given by the order they appear in this list.
    6. One line containing the number of ordering constraints ``o``.
    7. One line containing $2o$ numbers, split into $o$ many pairs (first and second, then third and fourth, and so so). Each such pair ``t1 t2`` indicates that the subtask ``t1`` is to be ordered before ``t2``
    8. One line for each variable constraint, containing
        1. Either the string ``=`` for an equals constraint, or ``!=`` for a not-equals constraint
        2. A number ``v1`` referring to an argument of the action
        3. A number ``v2`` referring to an argument of the action  
        The implied constraint is that $v_1 = v_2$ or $v_1 \neq v_2$, respectively.

### The Init/Goal Section

The init and goal section consists of

1. One line with two non-negative integers ``I G``, indicating the number of facts true in the initial state ``I`` and the number of goal facts ``G``.
2. Then follow ``I`` many lines, each describing a fact that is true in the initial state. Each such line contains
    1. A number ``p`` indicating the predicate of the fact.
    2. As many number ``c`` as this predicate takes arguments, each indicating a constant.
3. Then follow ``G`` many lines, each describing a fact that should be true in the goal, each with the same format as a fact in init.
4. One line containing a single integer ``T``. If this number is $-1$, this is a classical planning problem. Otherwise, this number is $A \leq T < A+C$ and indicates the initial abstract task of the problem.
