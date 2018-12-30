CC=g++
COMPILEFLAGS=-O2 -Wall -std=c++11
LINKERFLAG=-lm

.PHONY = parser clean

parser: src/hddl-token.o src/hddl.o src/main.o src/sortexpansion.o src/parsetree.o src/util.o
	${CC} ${LINKERFLAG} $^ -o parser

%.o: %.cpp %.hpp
	${CC} ${COMPILEFLAGS} -o $@ -c $<

src/hddl-token.cpp: src/hddl.cpp src/hddl-token.l
	flex --yylineno -o src/hddl-token.cpp src/hddl-token.l

src/hddl.cpp: src/hddl.y
	bison -d -o src/hddl.cpp src/hddl.y

clear:
	rm src/hddl-token.cpp
	rm src/hddl.cpp
	rm src/hddl.hpp
	rm src/*o
