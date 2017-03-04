CC=clang
CFLAGS=-g `llvm-config --cflags` -Wno-sign-compare -Wno-unused-function
LD=clang++
LDFLAGS=-lLLVM-3.8 `llvm-config --cxxflags --ldflags --libs core executionengine interpreter analysis native bitwriter --system-libs`

#sum.ll: sum.bc
#	llvm-dis $<


# to run the code use
#make clean; make; ./uai tests/simple_test.uai; llvm-dis tests/simple_test.uai.bc; cat tests/simple_test.uai.ll; lli tests/simple_test.uai.ll;

all: lex-gen yacc-gen compile


compile: lexer.o yacc.o assembler_core.o
	$(LD) *.o $(LDFLAGS) -o uai

test: compile
	./run_test.sh

lex-gen:
	lex --outfile=lexer/lexer.c lexer/tokens.lex

lexer.o: lexer/lexer.c
	$(CC) $(CFLAGS) -c $<

yacc-gen:
	yacc -dtv grammar/grammar.yacc -o grammar/grammar.c

yacc.o: grammar/grammar.c hash.o str.o list.o
	$(CC) $(CFLAGS) -c $<

hash.o: collections/hash.c util.o vector.o
	$(CC) $(CFLAGS) -c $<

util.o: collections/util.c
	$(CC) $(CFLAGS) -c $<

vector.o: collections/vector.c
	$(CC) $(CFLAGS) -c $<

list.o: collections/linkedList.c
	$(CC) $(CFLAGS) -c $<

str.o: collections/str.c
	$(CC) $(CFLAGS) -c $<

assembler_core.o: assembler/core.c
	$(CC) $(CFLAGS) -c $<

clean:
	-rm -f *.o *.bc *.ll

install_dependencies:
	sudo apt-get install flex bison clang-3.8

sanit_test:
	gcc experiments/sanit_test.c $(LDFLAGS) -o test

gen_code:
	clang -S -emit-llvm tests/code_snipets.c
	llc -march=cpp code_snipets.ll
