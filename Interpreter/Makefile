CXX?=g++
CC=$(CXX)
CXXFLAGS?=-std=c++11 -Wall -Werror -pedantic-errors -g -O0
BISONFLAGS?=-v -Dparse.trace
PS=Parser/

%.tab.cpp %.tab.hpp: %.ypp
	bison $(BISONFLAGS) --file-prefix=$* $< 

%.flex.cpp %.flex.h: %.lpp
	flex --outfile=$*.flex.cpp --header-file=$*.flex.h $<

all: Intr IntrTests

test: IntrTests
	./IntrTests

release:
	make clean
	CXXFLAGS="-std=c++11 -O2 -DNDEBUG -march=native" \
	BISONFLAGS= make all 

Error.o: Error.cpp Error.h

VarCont.o: VarCont.cpp VarCont.h

Intr.o: Intr.cpp $(PS)ParsingDriver.h Error.h Table.h SyntaxTree.h \
TreeAnalyzer.h FuncCont.h Interpreter.h

Intr: Intr.o Error.o VarCont.o FuncCont.o TreeAnalyzer.o Interpreter.o \
$(PS)ParsingDriver.o $(PS)lexer.flex.o $(PS)parser.tab.o

TreeAnalyzer.o: TreeAnalyzer.cpp TreeAnalyzer.h Table.h VarCont.h \
Error.h SyntaxTree.h FuncCont.h Interpreter.h

IntrTests.o: IntrTests.cpp Error.h Table.h SyntaxTree.h \
TreeAnalyzer.h FuncCont.h Interpreter.h $(PS)ParsingDriver.h

IntrTests: IntrTests.o Error.o TreeAnalyzer.o VarCont.o \
FuncCont.o Interpreter.o $(PS)ParsingDriver.o $(PS)lexer.flex.o \
$(PS)parser.tab.o

FuncCont.o: FuncCont.cpp FuncCont.h

Interpreter.o: Interpreter.cpp Interpreter.h Error.h Table.h FuncCont.h

$(PS)lexer.flex.o: $(PS)lexer.lpp $(PS)parser.tab.hpp $(PS)ParsingDriver.h

$(PS)parser.tab.o: $(PS)parser.ypp $(PS)ParsingDriver.h SyntaxTree.h \
FuncCont.h Table.h

$(PS)ParsingDriver.o: $(PS)ParsingDriver.cpp $(PS)ParsingDriver.h \
$(PS)lexer.flex.h $(PS)parser.tab.hpp SyntaxTree.h Table.h \
FuncCont.h Error.h

$(PS)ParsingDriver.h: $(PS)parser.tab.hpp $(PS)lexer.flex.h

clean:
	rm -f *.o *~ Exprs/*~ $(PS)*~ $(PS)*.o \
	$(PS)*.tab.* $(PS)*.flex.* $(PS)stack.hh \
	$(PS)parser.output Intr IntrTests