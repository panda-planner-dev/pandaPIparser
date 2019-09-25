CC=g++

CWARN=-Wno-unused-parameter
CERROR=-Werror=implicit-function-declaration

COMPILEFLAGS=-O3 -pipe -Wall -Wextra -pedantic -std=c++11 $(CWARN) $(CERROR)
LINKERFLAG=-O3 -lm -flto
#COMPILEFLAGS=-O0 -ggdb -pipe -Wall -Wextra -pedantic -std=c++11 $(CWARN) $(CERROR)
#LINKERFLAG=-O0 -ggdb

.PHONY = parser clean

parser: src/hddl-token.o src/hddl.o src/main.o src/sortexpansion.o src/parsetree.o src/util.o src/domain.o src/output.o src/parametersplitting.o src/cwa.o src/typeof.cpp
	${CC} ${LINKERFLAG} $^ -o parser

%.o: %.cpp %.hpp
	${CC} ${COMPILEFLAGS} -o $@ -c $<

%.o: %.cpp
	${CC} ${COMPILEFLAGS} -o $@ -c $<

src/hddl-token.cpp: src/hddl.cpp src/hddl-token.l
	flex --yylineno -o src/hddl-token.cpp src/hddl-token.l


src/hddl.cpp: src/hddl.y
	bison -v -d -o src/hddl.cpp src/hddl.y

src/hddl.hpp: src/hddl.cpp

clear:
	rm src/hddl-token.cpp
	rm src/hddl.cpp
	rm src/hddl.hpp
	rm src/*o
