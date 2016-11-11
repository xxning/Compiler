CC=clang++
LEX=lex
YACC=bison
CFLAGS=-I include -Wno-deprecated-register
YFLAGS=
LFLAGS=

all: bin/C1 bin/mylib.o bin/mylib.a

bin/C1: bin/lexer.o bin/parser.o bin/main.o bin/util.o bin/global.o bin/node.o bin/dumpdot.o bin/codegen.o
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $^ `llvm-config-3.6 --cxxflags --system-libs --libs core --ldflags mcjit native`

src/parser.cc include/tok.hh: config/parser.y
	$(YACC) -v $(YFLAGS) --defines=include/tok.hh -o src/parser.cc $<

bin/parser.o: src/parser.cc include/node.hh include/util.hh include/global.hh
	@mkdir -p bin
	$(CC) $(CFLAGS) -c -o $@ $< `llvm-config-3.6 --cxxflags --ldflags`

src/lexer.cc: config/lexer.l
	$(LEX) $(LFLAGS) -o $@ $< 

bin/lexer.o: src/lexer.cc include/tok.hh include/node.hh
	@mkdir -p bin
	$(CC) $(CFLAGS) -c -o $@ $< `llvm-config-3.6 --cxxflags --ldflags`

bin/node.o: src/node.cc include/node.hh
	@mkdir -p bin
	$(CC) $(CFLAGS) -c -o $@ $< `llvm-config-3.6 --cxxflags --ldflags`

bin/main.o: src/main.cc include/tok.hh include/util.hh include/global.hh include/node.hh
	@mkdir -p bin
	$(CC) $(CFLAGS) -c -o $@ $< `llvm-config-3.6 --cxxflags --ldflags`

bin/util.o: src/util.cc include/util.hh include/global.hh
	@mkdir -p bin
	$(CC) $(CFLAGS) -c -o $@ $<

bin/global.o: src/global.cc include/global.hh
	@mkdir -p bin
	$(CC) $(CFLAGS) -c -o $@ $<

bin/dumpdot.o: src/dumpdot.cc include/dumpdot.hh include/node.hh
	@mkdir -p bin
	$(CC) $(CFLAGS) -std=c++11 -c -o $@ $< `llvm-config-3.6 --cxxflags --ldflags`

bin/codegen.o: src/codegen.cc include/node.hh
	@mkdir -p bin
	$(CC) $(CFLAGS) -c -o $@ $< `llvm-config-3.6 --cxxflags --ldflags`
bin/mylib.o:src/mylib.c
	@mkdir -p bin
	clang -c -o $@ $<
bin/mylib.a:bin/mylib.o
	@mkdir -p bin
	ar rcs $@ $<
.PHONY: clean
clean:
	rm -f bin/*.o src/lexer.cc src/parser.cc include/tok.hh
	rm -f bin/*.a bin/Test bin/Test_for
	rm -f *.o *.ll
	rm -f *.png *.dot
	rm -f src/*.output
	rm -f bin/*.o bin/C1
	rm -f */*~ *~
	rm -f *.dot *.png
