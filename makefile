CXX=g++
CWARN=-Wno-unused-parameter
CERROR=

COMPILEFLAGS=-O3 -pipe -Wall -Wextra -pedantic -std=c++17 -DNDEBUG $(CWARN) $(CERROR)
ifneq ($OS(OS), Windows_NT)
     UNAME_S := $(shell uname -s)
     ifeq ($(UNAME_S),Darwin)
		BISON ?= /opt/homebrew/opt/bison/bin/bison
		FLEX ?= /opt/homebrew/opt/flex/bin/flex
	    LINKERFLAG=-O3 -lm -flto -DNDEBUG
     else
		BISON ?= bison
		FLEX ?= flex
		LINKERFLAG=-O3 -lm -flto -static -static-libgcc -DNDEBUG
     endif
else # guessing what will work on Windows
	BISON ?= bison
	FLEX ?= flex
	LINKERFLAG=-O3 -lm -flto -static -static-libgcc -DNDEBUG
endif


.PHONY = all clean debug setdebug

all: pandaPIparser

pandaPIparser: src/hddl-token.o src/hddl.o src/main.o src/sortexpansion.o src/parsetree.o src/util.o src/domain.o src/output.o src/parametersplitting.o src/cwa.o src/typeof.o src/shopWriter.o src/hpdlWriter.o src/hddlWriter.o src/htn2stripsWriter.o src/orderingDecomposition.o src/plan.o src/verify.o src/properties.o src/verification_encoding.o src/cmdline.o
	${CXX} ${LINKERFLAG} $^ -o pandaPIparser 

%.o: %.cpp %.hpp src/hddl.hpp
	${CXX} ${COMPILEFLAGS} -o $@ -c $<

%.o: %.cpp src/hddl.hpp
	${CXX} ${COMPILEFLAGS} -o $@ -c $<

src/hddl-token.cpp: src/hddl.cpp src/hddl-token.l
	$(FLEX) --yylineno -o src/hddl-token.cpp src/hddl-token.l


src/hddl.cpp: src/hddl.y
	$(BISON) -v -d -o src/hddl.cpp src/hddl.y

src/hddl.hpp: src/hddl.cpp

src/main.cpp: src/cmdline.h

src/cmdline.c src/cmdline.h: src/options.ggo
	gengetopt --include-getopt --default-optional --unamed-opts --output-dir=src -i src/options.ggo


setdebug:
	$(eval COMPILEFLAGS=-O0 -ggdb -pipe -Wall -Wextra -pedantic -std=c++17 $(CWARN) $(CERROR))
	$(eval LINKERFLAG=-O0 -ggdb)

debug: setdebug all


clean:
	rm -f src/hddl-token.cpp
	rm -f src/hddl.cpp
	rm -f src/hddl.hpp
	rm -f src/*.o

install: pandaPIparser
	install pandaPIparser ${DESTDIR}/bin
