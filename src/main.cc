#include <iostream>
#include "dumpdot.hh"
#include "node.hh"
#include "util.hh"
#include "global.hh"

using namespace std;
using namespace llvm;

extern char* filename;
extern FILE *yyin;	// flex uses yyin as input file's pointer
extern int ERROR;
extern int MODEL;
extern int yylex();	// lexer.cc provides yylex()
extern int yyparse();	// parser.cc provides yyparse()
extern void Delete();
extern CompUnitNode *root;	// AST's root, shared with yyparse()
extern Module *TheModule;
int main(int argc,char **argv)
{
	filename=argv[1];
	//output the name of file
	//printf("\033[35m\nFile src ==> %s\n",filename);
	if (handle_opt(argc, argv) == false)
        	return 0;
	yyin = infp;
	yyparse();
	if(dumpfp!=NULL && (MODEL||!ERROR)){//Graph
		DumpDOT *dumper=new DumpDOT(dumpfp);
		root->dumpdot(dumper);  
		delete dumper;
		fclose(dumpfp);
	}

	if(root!=NULL && !ERROR){  //IR
		LLVMContext &Context = getGlobalContext();
		TheModule = new Module("LLVM IR for C1 language", Context);
		root->Codegen();   
		TheModule->dump();   //codegen
		Delete();     //delete all the nodes
	}

	return 0;
}
