# pandaPIparser

This is the parsing utility of the pandaPI planning system. It is designed to parse HTN planning problems. Its main (and currently only) input language is HDDL (see the following [paper](http://www.uni-ulm.de/fileadmin/website_uni_ulm/iui.inst.090/Publikationen/2019/Hoeller2019HDDL.pdf)).

If you use pandaPIparser in any of your published work, we would kindly ask you to cite us (see Reference below).

## Capabilities 
The parser can currently produce two different output formats:

1. pandaPI's internal numeric format for lifted HTN planning problems
2. (J)SHOP2's input language.

Note that the translation into (J)SHOP2 is necessarily incomplete as (J)SHOP2 cannot express arbitrary partial orders in its ordering constraints. For example a method with the five subtasks (a,b,c,d,e) and the ordering constraints a < c, a < d, b < d, and b < e cannot be formulated in (J)SHOP2.


## Compilation
To compile pandaPIparser you need g++, make, flex, and bison. No libraries are required.

To create the executable, simply run `make -j` in the root folder, which will create an executable called `pandaPIparser`

## Usage
The parser is called with at least two arguments: the domain and the problem file. Both must be written in HDDL.
By default, the parser will output the given instance in pandaPI's internal format on standard our.
If you pass a third file name, pandaPIparser will instead output the internal representation of the instance to that file.


### Usage for Compilation to (J)SHOP2
pandaPIparser also offers to option to write the output to (J)SHOP2's input format. In order to do so add `-shop` as one of the command line arguments (the position does not matter).
With `-shop` you may specify up to four files as command line arguments: the input domain, the input problem, the output domain, and the output problem.
As an example consider

```
./pandaPIParser -shop transport.hddl pfile01.hddl shop-transport.lisp shop-pfile01.lisp
```

Note that
* pandaPIparser will shift some of the contents of the HDDL problem file to the (J)SHOP2 domain file. Most notably, pandaPIparser moves the initial task network of the HDDL problem into the (J)SHOP2 domain by compiling it into a method for a new abstract task `__top`.
* any propositional goal will be ignored in the translation.
* action costs will (for the time being) be ignored.
* any action named `call` will be renamed to `_call`, as `call` is a keyword for (J)SHOP2.
* current `forall` statements in preconditions are fully instantiated.

## Contact
If you have any issues with pandaPIparser -- or have any question relating to its use, please contact [Gregor Behnke](mailto:gregor.behnke@uni-ulm.de).

## Reference
If you would like to cite pandaPIparser, you may do so my referring to the following paper:

```
@inproceedings { Behnke2019GroundingWS,
		Title = {More Succinct Grounding of HTN Planning Problems -- {P}reliminary Results},
		Year = {2019},
		Pages = {40--48},
		Booktitle = {Proceedings of the Second {ICAPS} Workshop on Hierarchical Planning},
		Author = {Behnke, Gregor and H{\"o}ller, Daniel and Bercher, Pascal and Biundo, Susanne}
}
```
