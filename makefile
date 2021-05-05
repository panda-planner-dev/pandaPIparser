CXX=g++

CWARN=-Wno-unused-parameter
CERROR=


COMPILEFLAGS=-O3 -pipe -Wall -Wextra -pedantic -std=c++17 -DNDEBUG $(CWARN) $(CERROR)
ifneq ($OS(OS), Windows_NT)
     UNAME_S := $(shell uname -s)
     ifeq ($(UNAME_S),Darwin)
	     LINKERFLAG=-O3 -lm -flto -DNDEBUG
     else
	     LINKERFLAG=-O3 -lm -flto -static -static-libgcc -DNDEBUG
     endif
else # guessing what will work on Windows
     LINKERFLAG=-O3 -lm -flto -static -static-libgcc -DNDEBUG
endif

#COMPILEFLAGS=-O0 -ggdb -pipe -Wall -Wextra -pedantic -std=c++17 $(CWARN) $(CERROR)
#LINKERFLAG=-O0 -ggdb

.PHONY = all clean

all: src/hddl-token.o src/hddl.o src/main.o src/sortexpansion.o src/parsetree.o src/util.o src/domain.o src/output.o src/parametersplitting.o src/cwa.o src/typeof.o src/shopWriter.o src/hpdlWriter.o src/hddlWriter.o src/htn2stripsWriter.o src/orderingDecomposition.o src/plan.o src/verify.o src/properties.o src/cmdline.o
	${CXX} ${LINKERFLAG} $^ -o pandaPIparser 

%.o: %.cpp %.hpp src/hddl.hpp
	${CXX} ${COMPILEFLAGS} -o $@ -c $<

%.o: %.cpp src/hddl.hpp
	${CXX} ${COMPILEFLAGS} -o $@ -c $<

src/hddl-token.cpp: src/hddl.cpp src/hddl-token.l
	flex --yylineno -o src/hddl-token.cpp src/hddl-token.l


src/hddl.cpp: src/hddl.y
	bison -v -d -o src/hddl.cpp src/hddl.y

src/hddl.hpp: src/hddl.cpp

src/main.cpp: src/cmdline.h

src/cmdline.c src/cmdline.h: src/options.ggo
	gengetopt --include-getopt --default-optional --unamed-opts --output-dir=src -i src/options.ggo


clean:
	rm src/hddl-token.cpp
	rm src/hddl.cpp
	rm src/hddl.hpp
	rm src/*o
