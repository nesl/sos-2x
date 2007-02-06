
all: sos_jumpgen.exe  

sos_jumpgen.exe: sos_jumpgen.lex sos_jumpgen.y 
	lex  sos_jumpgen.lex
	yacc -d sos_jumpgen.y
	gcc lex.yy.c y.tab.c -o sos_jumpgen.exe
	rm -f lex.yy.c y.tab.c y.tab.h

test:
	./sos_jumpgen.exe test_i.h
	./sos_jumpgen.exe -k $(SOSROOT)/kernel/include/module_in.h

clean:
	rm -f lex.yy.c y.tab.c y.tab.h sos_jumpgen.exe
