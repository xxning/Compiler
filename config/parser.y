%{
#include <stdio.h>
#include <string.h>
#include <vector>
#include "node.hh"
//#define DEBUG
//#define LINENO
#include "util.hh"

int yylex();
void yyerror(const char *msg);
char* filename=(char*)malloc(32*sizeof(char));
CompUnitNode *root=0;
std::vector<Node*> BUFF; //a vector for nodes
std::vector<std::string*> Empty;
std::vector<ExpNode*> EmptyL;
void printerrline(int line,int column);
void Delete();
int ERROR=0;//ERROR=1 表示有错误
int MODEL=1;//MODEL=0表示遇到错误后不建立语法树,MODEL=1表示错误恢复后建立语法树

%}

%union {
	int num;
	std::string *var;
	Node *node;
}

%token <num> NUMBER 
%type <var> RelOp
%token WHILE IF ELSE INT CONST VOID FOR
%token <var> EQ UE LE GE LT GT IDENTIFIER
%token <var> NOT AND OR 
%token <var> BREAK CONTINUE RETURN

%left '+' '-'
%left '*' '/' '%'
%right UNARY_OP

%precedence HIGHER
%precedence LOWER

//%nonassoc error

%type <node> CompUnit Decl FuncDef ConstDecl VarDecl
%type <node> ConstDef1 ConstDef Exp Exp1
%type <node> Var1 Var Block BlockItem1
%type <node> BlockItem Stmt Lval Cond 
%type <node> ParaD 

%%
CompUnit	:
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug ("CompUnit ::= empty\n"); 
			if(MODEL || !ERROR)
			{
				$$=new CompUnitNode;
				$$->setLoc((Loc*)&(@$));
				root=(CompUnitNode*)$$;
				BUFF.push_back($$);
			}
		}
		|CompUnit Decl
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("CompUnit ::= CompUnit Decl\n");
			if(MODEL || !ERROR)
			{
				((CompUnitNode*)$1)->append((ExpNode*)$2);
				$$=$1;
				root=(CompUnitNode*)$$;
			}
		}
		|CompUnit FuncDef
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug ("CompUnit ::= CompUnit FuncDef\n");
			if(MODEL || !ERROR)
			{
				((CompUnitNode*)$1)->append((FunNode*)$2);
				$$=$1;
				root=(CompUnitNode*)$$;
			}
		}
		;
	
Decl		:ConstDecl	{
				  lineno("%d to %d ",@$.first_line,@$.last_line);
				  debug("Decl ::= ConstDecl\n");
				}
		|VarDecl	{
				  lineno("%d to %d ",@$.first_line,@$.last_line);
				  debug("Decl ::= VarDecl\n");
				}
		;

ConstDecl	:CONST INT ConstDef1 ';'
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("ConstDecl ::= CONST INT ConstDef1\n");
			if(MODEL || !ERROR)
			{
				$$=new ConstDeclNode((ConstDefListNode*)$3);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		;


ConstDef1	:ConstDef
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("ConstDef1 ::= ConstDef\n");
			if(MODEL || !ERROR)
			{
				$$=new ConstDefListNode();
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
				((ConstDefListNode*)$$)->append((ExpNode*)$1);
			}
		}
		|ConstDef1 ',' ConstDef
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("ConstDef1 ::=  ConstDef1 ',' ConstDef\n");
			if(MODEL || !ERROR)
			{
				((ConstDefListNode*)$1)->append((ExpNode*)$3);
				$$=$1;
			}
		}
		;


ConstDef	:IDENTIFIER '=' Exp
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("ConstDef ::= IDENTIFIER = Exp\n");
			if(MODEL || !ERROR)
			{
				$$=new ConstDefS($1,(ExpNode*)$3);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|IDENTIFIER '[' ']' '=' '{' Exp1 '}'
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("ConstDef ::= IDENTIFIER[] = {Exp Exp1}\n");
			if(MODEL || !ERROR)
			{
				$$=new ConstDefM($1,NULL,(ExpListNode*)$6);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|IDENTIFIER '[' '=' '{' Exp1 '}'
		{
			printerrline(@1.first_line,@1.last_column);
	    		error("%4d(%2d)lack ']' after IDENTIFIER\n",@1.first_line,@1.last_column);
			if(MODEL){
				$$=new ConstDefM($1,NULL,(ExpListNode*)$5);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}
		}
		|IDENTIFIER ']' '=' '{' Exp1 '}'
		{
			printerrline(@1.first_line,@1.last_column);
	    		error("%4d(%2d)lack '[' after IDENTIFIER\n",@1.first_line,@1.last_column);
			if(MODEL){
				$$=new ConstDefM($1,NULL,(ExpListNode*)$5);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}
		}
		|IDENTIFIER '[' Exp ']' '=' '{' Exp1 '}'
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("ConstDef ::= IDENTIFIER[Exp] = {Exp Exp1}\n");
			if(MODEL || !ERROR)
			{
				$$=new ConstDefM($1,(ExpNode*)$3,(ExpListNode*)$7);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|IDENTIFIER '[' Exp '=' '{' Exp1 '}'
		{
			printerrline(@1.first_line,@1.last_column);
	    		error("%4d(%2d)lack ']' after IDENTIFIER\n",@1.first_line,@1.last_column);
			if(MODEL){
				$$=new ConstDefM($1,(ExpNode*)$3,(ExpListNode*)$6);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}
		}
		|IDENTIFIER Exp ']' '=' '{' Exp1 '}'
		{
			printerrline(@2.first_line,@2.last_column);
	    		error("%4d(%2d)lack '[' after IDENTIFIER\n",@2.first_line,@2.last_column);
			if(MODEL){
				$$=new ConstDefM($1,(ExpNode*)$2,(ExpListNode*)$6);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}
		}
		;
	

Exp1		:Exp
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Exp1 ::= Exp \n");
			if(MODEL || !ERROR)
			{
				$$=new ExpListNode();
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
				((ExpListNode*)$$)->append((ExpNode*)$1);
			}
		}
		|Exp1 ',' Exp
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Exp1 ::= Exp1 , Exp\n");	
			if(MODEL || !ERROR)
			{
				((ExpListNode*)$1)->append((ExpNode*)$3);
				$$=$1;
			}
		}
		;

ParaD		:INT Var
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("ParaD ::= INT Var\n");
			if(MODEL || !ERROR)
			{
				$$=new ParaListNode();
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
				((ParaListNode*)$$)->append(((VarNode*)$2)->Name);
				((ParaListNode*)$$)->appendn((VarNode*)$2);
			}
		}
		|ParaD ',' INT Var
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("ParaD ::= ParaD , INT Var\n");
			if(MODEL || !ERROR)
			{
				((ParaListNode*)$1)->append(((VarNode*)$4)->Name);
				((ParaListNode*)$$)->appendn((VarNode*)$4);
				$$=$1;
			}
		}
		;
/*
ParaC		:Var
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Parac ::= Var\n");
			if(MODEL || !ERROR)
			{
				$$=new ParaListNode();
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
				((ParaListNode*)$$)->append((VarNode*)$1->Name);
			}
		}
		|ParaC ',' Var
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("ParaC ::= ParaC , Var\n");
			if(MODEL || !ERROR)
			{
				((ParaListNode*)$1)->append((VarNode*)$3->name);
				$$=$1;
			}
		}
		;
*/

VarDecl		:INT Var1 ';'
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("VarDecl ::= INT var var1;\n");
			if(MODEL || !ERROR)
			{
				$$=new VarDeclNode((VarListNode*)$2);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		;


Var1		:Var
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Var1 ::= Var\n");
			if(MODEL || !ERROR)
			{
				$$=new VarListNode();
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
				((VarListNode*)$$)->append((VarNode*)$1);
				((VarListNode*)$$)->appendv(((VarNode*)$1)->Name);
			}
		}
		|Var1 ',' Var
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Var1 ::= Var1 Var\n");
			if(MODEL || !ERROR)
			{
				((VarListNode*)$1)->append((VarNode*)$3);
				((VarListNode*)$$)->appendv(((VarNode*)$3)->Name);
				$$=$1;
			}
		}
		;


Var		:IDENTIFIER
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Var ::= IDENTIFIER\n");
			if(MODEL || !ERROR)
			{
				$$=new VarNode($1,NULL);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|IDENTIFIER '[' Exp ']'
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Var ::= IDENTIFIER = Exp\n"); 
			if(MODEL || !ERROR)
			{
				$$=new ArrayNode($1,(ExpNode*)$3,NULL);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|IDENTIFIER '[' Exp error
		{
			printerrline(@1.first_line,@1.last_column);
	    		error("%4d(%2d)lack ']' after IDENTIFIER\n",@1.first_line,@1.last_column);
			if(MODEL){
				$$=new ArrayNode($1,(ExpNode*)$3,NULL);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}	
		}
		|IDENTIFIER error Exp ']' 
		{
			printerrline(@2.first_line,@2.last_column);
	    		error("%4d(%2d)lack '[' after IDENTIFIER\n",@2.first_line,@2.last_column);
			if(MODEL){
				$$=new ArrayNode($1,(ExpNode*)$3,NULL);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}	
		}
		|IDENTIFIER '=' Exp
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Var ::= IDENTIFIER = Exp\n"); 
			if(MODEL || !ERROR)
			{
				$$=new VarNode($1,(ExpNode*)$3);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}

		}
		|IDENTIFIER '[' Exp ']' '=' '{' Exp1 '}'
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Var ::= IDENTIFIER[Exp] = { Exp1 }\n");
			if(MODEL || !ERROR)
			{
				$$=new ArrayNode($1,(ExpNode*)$3,(ExpListNode*)$7);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|IDENTIFIER '[' Exp '=' '{' Exp1 '}'
		{
			printerrline(@1.first_line,@1.last_column);
	    		error("%4d(%2d)lack '[' after IDENTIFIER\n",@1.first_line,@1.last_column);
			if(MODEL){
				$$=new ArrayNode($1,(ExpNode*)$3,(ExpListNode*)$6);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}	
		}
		|IDENTIFIER Exp ']' '=' '{' Exp1 '}'
		{
			printerrline(@2.first_line,@2.last_column);
	    		error("%4d(%2d)lack '[' after IDENTIFIER\n",@2.first_line,@2.last_column);
			if(MODEL){
				$$=new ArrayNode($1,(ExpNode*)$2,(ExpListNode*)$6);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}	
		}
		;

FuncDef		:VOID IDENTIFIER '(' ')' Block
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("FunDef ::= VOID IDENTIFIER() Block\n");;
			if(MODEL || !ERROR)
			{
				$$=new FunNode($2,(BlockNode*)$5,0,Empty,EmptyL);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|VOID IDENTIFIER '(' ParaD ')' Block
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("FunDef ::= VOID IDENTIFIER(ParaD) Block\n");;
			if(MODEL || !ERROR)
			{
				$$=new FunNode($2,(BlockNode*)$6,0,((ParaListNode*)$4)->Var_names,((ParaListNode*)$4)->Arg_List);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|VOID IDENTIFIER '(' ParaD Block
		{
			printerrline(@2.first_line,@2.last_column);
	    		error("%4d(%2d)lack ')' after IDENTIFIER\n",@2.first_line,@2.last_column);
			if(MODEL){
				$$=new FunNode($2,(BlockNode*)$5,0,((ParaListNode*)$4)->Var_names,((ParaListNode*)$4)->Arg_List);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}	
		}
		|VOID IDENTIFIER ParaD ')' Block
		{
			printerrline(@2.first_line,@2.last_column);
	    		error("%4d(%2d)lack '(' after IDENTIFIER\n",@2.first_line,@2.last_column);
			if(MODEL){
				$$=new FunNode($2,(BlockNode*)$5,0,((ParaListNode*)$3)->Var_names,((ParaListNode*)$3)->Arg_List);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}	
		}
		|VOID IDENTIFIER '(' Block
		{
			printerrline(@2.first_line,@2.last_column);
	    		error("%4d(%2d)lack ')' after IDENTIFIER\n",@2.first_line,@2.last_column);
			if(MODEL){
				$$=new FunNode($2,(BlockNode*)$4,0,Empty,EmptyL);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}	
		}
		|VOID IDENTIFIER ')' Block
		{
			printerrline(@2.first_line,@2.last_column);
	    		error("%4d(%2d)lack '(' after IDENTIFIER\n",@2.first_line,@2.last_column);
	    		if(MODEL){
				$$=new FunNode($2,(BlockNode*)$4,0,Empty,EmptyL);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}
		}
		|INT IDENTIFIER '(' ')' Block
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("FunDef ::= INT IDENTIFIER() Block\n");;
			if(MODEL || !ERROR)
			{
				$$=new FunNode($2,(BlockNode*)$5,1,Empty,EmptyL);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|INT IDENTIFIER '(' ParaD ')' Block
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("FunDef ::= INT IDENTIFIER(ParaD) Block\n");;
			if(MODEL || !ERROR)
			{
				$$=new FunNode($2,(BlockNode*)$6,1,((ParaListNode*)$4)->Var_names,((ParaListNode*)$4)->Arg_List);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|INT IDENTIFIER '(' ParaD Block
		{
			printerrline(@2.first_line,@2.last_column);
	    		error("%4d(%2d)lack ')' after IDENTIFIER\n",@2.first_line,@2.last_column);
			if(MODEL){
				$$=new FunNode($2,(BlockNode*)$5,0,((ParaListNode*)$4)->Var_names,((ParaListNode*)$4)->Arg_List);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}	
		}
		|INT IDENTIFIER ParaD ')' Block
		{
			printerrline(@2.first_line,@2.last_column);
	    		error("%4d(%2d)lack '(' after IDENTIFIER\n",@2.first_line,@2.last_column);
			if(MODEL){
				$$=new FunNode($2,(BlockNode*)$5,0,((ParaListNode*)$3)->Var_names,((ParaListNode*)$3)->Arg_List);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}	
		}
		|INT IDENTIFIER '(' Block
		{
			printerrline(@2.first_line,@2.last_column);
	    		error("%4d(%2d)lack ')' after IDENTIFIER\n",@2.first_line,@2.last_column);
	    		if(MODEL){
				$$=new FunNode($2,(BlockNode*)$4,1,Empty,EmptyL);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}
		}
		|INT IDENTIFIER ')' Block
		{
			printerrline(@2.first_line,@2.last_column);
	    		error("%4d(%2d)lack '(' after IDENTIFIER\n",@2.first_line,@2.last_column);
	    		if(MODEL){
				$$=new FunNode($2,(BlockNode*)$4,1,Empty,EmptyL);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}
		}
		|VOID IDENTIFIER '(' ')' ';'
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("FuncDef ::= VOID IDENTIFIER() \n");
			if(MODEL || !ERROR)
			{
				$$=new FunNode($2,NULL,0,Empty,EmptyL);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|VOID IDENTIFIER '(' ParaD ')' ';'
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("FuncDef ::= VOID IDENTIFIER(ParaD) \n");
			if(MODEL || !ERROR)
			{
				$$=new FunNode($2,NULL,0,((ParaListNode*)$4)->Var_names,((ParaListNode*)$4)->Arg_List);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}///////
		|VOID IDENTIFIER '(' ParaD ';'
		{
			printerrline(@2.first_line,@2.last_column);
	    		error("%4d(%2d)lack ')' after IDENTIFIER\n",@2.first_line,@2.last_column);
	    		if(MODEL){
				$$=new FunNode($2,NULL,0,((ParaListNode*)$4)->Var_names,((ParaListNode*)$4)->Arg_List);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}	
		}
		|VOID IDENTIFIER ParaD ')' ';'
		{
			printerrline(@2.first_line,@2.last_column);
	    		error("%4d(%2d)lack '(' after IDENTIFIER\n",@2.first_line,@2.last_column);
	    		if(MODEL){
				$$=new FunNode($2,NULL,0,((ParaListNode*)$3)->Var_names,((ParaListNode*)$3)->Arg_List);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}	
		}
		|VOID IDENTIFIER '(' ';'
		{
			printerrline(@2.first_line,@2.last_column);
	    		error("%4d(%2d)lack ')' after IDENTIFIER\n",@2.first_line,@2.last_column);
	    		if(MODEL){
				$$=new FunNode($2,NULL,0,Empty,EmptyL);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}	
		}
		|VOID IDENTIFIER ')' ';'
		{
			printerrline(@2.first_line,@2.last_column);
	    		error("%4d(%2d)lack '(' after IDENTIFIER\n",@2.first_line,@2.last_column);
	    		if(MODEL){
				$$=new FunNode($2,NULL,0,Empty,EmptyL);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}
		}
		|INT IDENTIFIER '(' ')' ';'
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("FuncDef ::= INT IDENTIFIER() \n");
			if(MODEL || !ERROR)
			{
				$$=new FunNode($2,NULL,1,Empty,EmptyL);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|INT IDENTIFIER '(' ParaD ')' ';'
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("FuncDef ::= INT IDENTIFIER(ParaD) \n");
			if(MODEL || !ERROR)
			{
				$$=new FunNode($2,NULL,1,((ParaListNode*)$4)->Var_names,((ParaListNode*)$4)->Arg_List);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}///////
		|INT IDENTIFIER '(' ParaD ';'
		{
			printerrline(@2.first_line,@2.last_column);
	    		error("%4d(%2d)lack ')' after IDENTIFIER\n",@2.first_line,@2.last_column);
	    		if(MODEL){
				$$=new FunNode($2,NULL,1,((ParaListNode*)$4)->Var_names,((ParaListNode*)$4)->Arg_List);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}	
		}
		|INT IDENTIFIER ParaD ')' ';'
		{
			printerrline(@2.first_line,@2.last_column);
	    		error("%4d(%2d)lack '(' after IDENTIFIER\n",@2.first_line,@2.last_column);
	    		if(MODEL){
				$$=new FunNode($2,NULL,1,((ParaListNode*)$3)->Var_names,((ParaListNode*)$3)->Arg_List);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}	
		}
		|INT IDENTIFIER '(' ';'
		{
			printerrline(@2.first_line,@2.last_column);
	    		error("%4d(%2d)lack ')' after IDENTIFIER\n",@2.first_line,@2.last_column);
	    		if(MODEL){
				$$=new FunNode($2,NULL,1,Empty,EmptyL);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}
		}
		|INT IDENTIFIER ')' ';'
		{
			printerrline(@2.first_line,@2.last_column);
	    		error("%4d(%2d)lack '(' after IDENTIFIER\n",@2.first_line,@2.last_column);
	    		if(MODEL){
				$$=new FunNode($2,NULL,1,Empty,EmptyL);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}
		}
		;

Block		:'{' '}'
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Block ::= { }\n");
			if(MODEL || !ERROR)
			{
				$$=new BlockNode(NULL);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
			
		}
		|'{' BlockItem1 '}'
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Block ::= { BlockItem1 }\n");
			if(MODEL || !ERROR)
			{
				$$=new BlockNode((BlockItemListNode*)$2);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		;

BlockItem1	:BlockItem1 BlockItem
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("BlockItem1 ::= BlockItem1 BlockItem\n");
			if(MODEL || !ERROR)
			{
				((BlockItemListNode*)$1)->append((ExpNode*)$2);
				$$=$1;
			}
		}
		|BlockItem
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("BlockItem1 ::= BlockItem\n");
			if(MODEL || !ERROR)
			{
				$$=new BlockItemListNode();
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
				((BlockItemListNode*)$$)->append((ExpNode*)$1);
			}
		}
		;

BlockItem	:Decl
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("BlockItem ::= Decl\n");
			if(MODEL || !ERROR)
			{
				$$=(ExpNode*)$1;
			}
		}
		|Stmt
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("BlockItem ::= stmt\n");
			if(MODEL || !ERROR)
			{
				$$=(ExpNode*)$1;
			}
		}
		;

Stmt		:Lval '=' Exp ';'
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Stmt ::= Lval = Exp;\n");
			if(MODEL || !ERROR)
			{
				$$=new AssignNode((LvalNode*)$1,(ExpNode*)$3);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|Lval '=' Stmt
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Stmt ::= Lval = Stmt\n");
			if(MODEL || !ERROR)
			{
				$$=new AssignNode((LvalNode*)$1,(ExpNode*)$3);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|IDENTIFIER '(' ')' ';'
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Stmt ::= IDENTIFIER();\n");
			if(MODEL || !ERROR)
			{
				$$=new FuncNode($1,Empty,EmptyL);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|IDENTIFIER '(' Var1 ')' ';'
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Stmt ::= IDENTIFIER(Var1);\n");
			if(MODEL || !ERROR)
			{
				$$=new FuncNode($1,((VarListNode*)$3)->Var_names,((VarListNode*)$3)->var_nodes);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|IDENTIFIER '(' ';'
		{
			printerrline(@1.first_line,@1.last_column);
	    		error("%4d(%2d)lack ')' after IDENTIFIER\n",@1.first_line,@1.last_column);
			if(MODEL){
				$$=new FuncNode($1,Empty,EmptyL);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}
		}
		|IDENTIFIER ')' ';'
		{
			printerrline(@1.first_line,@1.last_column);
	    		error("%4d(%2d)lack '(' after IDENTIFIER\n",@1.first_line,@1.last_column);
			if(MODEL){
				$$=new FuncNode($1,Empty,EmptyL);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}
		}
		|Block
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Stmt ::= Block;\n");
			if(MODEL || !ERROR)
			{
				$$=(BlockNode*)$1;
			}
		}
		|IF '(' Cond ')' Stmt %prec LOWER
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Stmt ::= IF(Cond) Stmt\n");
			if(MODEL || !ERROR)
			{
				$$=new IfNode((CondNode*)$3,(ExpNode*)$5,NULL);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|IF '(' Cond Stmt %prec LOWER
		{
			printerrline(@1.first_line,@1.last_column);
	    		error("%4d(%2d)lack ')' \n",@1.first_line,@1.last_column);
			if(MODEL){
				$$=new IfNode((CondNode*)$3,(ExpNode*)$4,NULL);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}
		}
		|IF Cond ')' Stmt %prec LOWER
		{
			printerrline(@2.first_line,@2.last_column);
	    		error("%4d(%2d)lack '(' \n",@2.first_line,@2.last_column);
			if(MODEL){
				$$=new IfNode((CondNode*)$2,(ExpNode*)$4,NULL);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}
		}
		|IF '(' Cond ')' Stmt ELSE Stmt
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Stmt ::= IF(Cond) Stmt Else Stmt\n");
			if(MODEL || !ERROR)
			{
				$$=new IfNode((CondNode*)$3,(ExpNode*)$5,(ExpNode*)$7);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}

		}
		|IF '(' Cond Stmt ELSE Stmt
		{
			printerrline(@1.first_line,@1.last_column);
	    		error("%4d(%2d)lack ')' \n",@1.first_line,@1.last_column);
			if(MODEL){
				$$=new IfNode((CondNode*)$3,(ExpNode*)$4,(ExpNode*)$6);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}
		}
		|IF Cond ')' Stmt ELSE Stmt
		{
			printerrline(@2.first_line,@2.last_column);
	    		error("%4d(%2d)lack '(' \n",@2.first_line,@2.last_column);
			if(MODEL){
				$$=new IfNode((CondNode*)$2,(ExpNode*)$4,(ExpNode*)$6);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}
		}
		|WHILE '(' Cond ')' Stmt
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Stmt ::= WHILE(Cond) Stmt \n");
			if(MODEL || !ERROR)
			{
				$$=new WhileNode((CondNode*)$3,(ExpNode*)$5);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|WHILE '(' Cond Stmt
		{
			printerrline(@1.first_line,@1.last_column);
	    		error("%4d(%2d)lack '(' \n",@1.first_line,@1.last_column);
			if(MODEL){
				$$=new WhileNode((CondNode*)$3,(ExpNode*)$4);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}   
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}
		}
		|WHILE Cond ')' Stmt
		{		
			printerrline(@2.first_line,@2.last_column);
	    		error("%4d(%2d)lack '(' \n",@2.first_line,@2.last_column);
			if(MODEL){
				$$=new WhileNode((CondNode*)$2,(ExpNode*)$4);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}
		}
		|FOR '(' Stmt Cond ';' Stmt ')' Stmt
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Stmt ::= FOR(Exp;Cond;Exp) Stmt ;\n");
			if(MODEL || !ERROR)
			{
				$$= new ForNode((ExpNode*)$3,(CondNode*)$4,(ExpNode*)$6,(ExpNode*)$8);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|FOR '(' Stmt Cond ';' Stmt Stmt
		{
			printerrline(@1.first_line,@1.last_column);
	    		error("%4d(%2d)lack '(' \n",@1.first_line,@1.last_column);
			if(MODEL){
				$$= new ForNode((ExpNode*)$3,(CondNode*)$4,(ExpNode*)$6,(ExpNode*)$7);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}
		}
		|FOR Stmt Cond ';' Stmt ')' Stmt
		{
			printerrline(@5.first_line,@5.last_column);
	    		error("%4d(%2d)lack '(' \n",@5.first_line,@5.last_column);
			if(MODEL){
				$$= new ForNode((ExpNode*)$2,(CondNode*)$3,(ExpNode*)$5,(ExpNode*)$7);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}
		}
		|BREAK ';'
		{	
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Stmt ::= BREAK ;\n");
			if(MODEL || !ERROR)
			{
				//$$= new AssignNode(NULL,NULL);
				$$= new ControlNode($1);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|CONTINUE ';'
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Stmt ::= CONTINIE ;\n");
			if(MODEL || !ERROR)
			{
				$$= new ControlNode($1);
			
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|RETURN Exp ';'
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Stmt ::= RETURN Exp ;\n");
			if(MODEL || !ERROR)
			{
				$$= new ReturnNode($1,(ExpNode*)$2);
				
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|';'
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Stmt ::= ;\n");
			if(MODEL || !ERROR)
			{
				$$= new AssignNode(NULL,NULL);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		;

Lval		:IDENTIFIER
		{	
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("LVal ::= IDENTIFFIER\n");
			if(MODEL || !ERROR)
			{
				$$=new LvalNode($1,NULL);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|IDENTIFIER '[' Exp ']'
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("LVal ::= IDENTIFIER[Exp]\n");
			if(MODEL || !ERROR)
			{
				$$=new LvalNode($1,(ExpNode*)$3);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}	

		|IDENTIFIER '[' Exp error
		{
			printerrline(@1.first_line,@1.last_column);
	    		error("%4d(%2d)lack ']' \n",@1.first_line,@1.last_column);
	    		if(MODEL){
				$$=new LvalNode($1,(ExpNode*)$3);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}
		} 
		|IDENTIFIER error Exp ']' 
		{
			printerrline(@3.first_line,@3.last_column);
	    		error("%4d(%2d)lack '[' \n",@3.first_line,@3.last_column);
	    		if(MODEL){
				$$=new LvalNode($1,(ExpNode*)$3);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}
		} 

		;

Cond		:Exp RelOp Exp
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Cond ::= Exp RelOP Exp\n");
			if(MODEL || !ERROR)
			{
				$$=new CondNode($2,(ExpNode*)$1,(ExpNode*)$3);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|'(' Cond ')'
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug ("Cond ::= ( Cond )\n");
			if(MODEL || !ERROR)
			{
				$$=$2;
			}	
		}
		|NOT Cond
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Cond ::= NOT Cond\n");
			if(MODEL || !ERROR)
			{
				$$=new LogicNode($1,(CondNode*)$2,NULL);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|Cond OR Cond
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Cond ::= Cond OR Cond\n");
			if(MODEL || !ERROR)
			{
				$$=new LogicNode($2,(CondNode*)$1,(CondNode*)$3);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|Cond AND Cond
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Cond ::= Cond AND Cond\n");
			if(MODEL || !ERROR)
			{
				$$=new LogicNode($2,(CondNode*)$1,(CondNode*)$3);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		;

RelOp   : EQ  { 
		lineno("%d to %d ",@$.first_line,@$.last_line);
	   	debug("RelOP ::= EQ\n");
	        if(MODEL || !ERROR)
	    	    $$ = $1;			
	      }
	| UE  {
		lineno("%d to %d ",@$.first_line,@$.last_line);
	    	debug("RelOP ::= UE\n");
	        if(MODEL || !ERROR)
	    	    $$ = $1;
	      }
	| LT  {
		lineno("%d to %d ",@$.first_line,@$.last_line);
	    	debug("RelOP ::= LT\n");
	    	if(MODEL || !ERROR)
	    	    $$ = $1;
	      }
	| GT  { 
		lineno("%d to %d ",@$.first_line,@$.last_line);
	    	debug("RelOP ::= GT\n");
	        if(MODEL || !ERROR)
	    	    $$ = $1;
	      } 
	| LE  {
		lineno("%d to %d ",@$.first_line,@$.last_line);
	    	debug("RelOP ::= LE\n");
		if(MODEL || !ERROR)
	    	    $$ = $1;
	      }
	| GE  {
		lineno("%d to %d ",@$.first_line,@$.last_line);
	    	debug("RelOP ::= GE\n");
	   	if(MODEL || !ERROR)
	    	    $$ = $1;
	      }
	;

Exp		:Lval
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Exp ::= Lval\n");
			if(MODEL || !ERROR)
			{
				$$=$1;
			}
		}
		|NUMBER
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug ("Exp ::= NUMBER\n");

			if(MODEL || !ERROR)
			{
				$$=new NumNode($1);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|Exp '+' Exp
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug ("Exp ::= Exp + Exp\n");
			if(MODEL || !ERROR)
			{
				$$=new BinaryExpNode('+',(ExpNode*)$1,(ExpNode*)$3);	
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|Exp '-' Exp
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug ("Exp ::= Exp - Exp\n");
			if(MODEL || !ERROR)
			{
				$$=new BinaryExpNode('-',(ExpNode*)$1,(ExpNode*)$3);	
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|Exp '*' Exp
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug ("Exp ::= Exp * Exp\n");
			if(MODEL || !ERROR)
			{
				$$=new BinaryExpNode('*',(ExpNode*)$1,(ExpNode*)$3);	
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|Exp '/' Exp
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug ("Exp ::= Exp / Exp\n");
			if(MODEL || !ERROR)
			{
				$$=new BinaryExpNode('/',(ExpNode*)$1,(ExpNode*)$3);	
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|Exp '%' Exp
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug ("Exp ::= Exp %% Exp\n");
			if(MODEL || !ERROR)
			{
				$$=new BinaryExpNode('%',(ExpNode*)$1,(ExpNode*)$3);	
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|'+' Exp %prec '-'
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Exp ::= '+' Exp\n");
			if(MODEL || !ERROR)
			{
				$$=new UnaryExpNode('+',(ExpNode*)$2);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|'-' Exp %prec '-'
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug("Exp ::= '-' Exp\n");
			if(MODEL || !ERROR)
			{
				$$=new UnaryExpNode('-',(ExpNode*)$2);
				$$->setLoc((Loc*)&(@$));
				BUFF.push_back($$);
			}
		}
		|'(' Exp ')'
		{
			lineno("%d to %d ",@$.first_line,@$.last_line);
			debug ("Exp ::= ( Exp )\n");
			if(MODEL || !ERROR)
			{
				$$=$2;
			}
		}
		|'(' Exp error
		{
			printerrline(@1.first_line,@0.last_column);
	    		error("%4d(%2d)lacks ')'\n",@1.first_line,@0.last_column);
	    			//$$ = new BraExpNode(NULL,(ExpNode*)$2,1);
	    			//$$->setLoc((Loc*)&(@$));
	    			//BUFF.push_back($$);
	    		if(MODEL){
				$$=$2;
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}
		}
		|Exp error ')'
		{
			printerrline(@1.first_line,@1.last_column);
	    		error("%4d(%2d)lacks '('\n",@1.first_line,@1.last_column);
	    		//$$ = new BraExpNode(NULL,(ExpNode*)$1,1);
	    		//$$->setLoc((Loc*)&(@$));
	    		//BUFF.push_back($$);
	    		if(MODEL){
				$$=$1;
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}
		}
		|Exp error Exp 
		{	
			printerrline(@1.first_line,@1.last_column);
	    		error("%4d(%2d)lack OP between two Exp\n",@1.first_line,@1.last_column);
	    			//$$ = new BinaryExpNode ('+', (ExpNode*)$1, (ExpNode*)$2);
	    			//$$->setLoc((Loc*)&(@$));
	    			//BUFF.push_back($$);
	    		if(MODEL){
				$$ = new BinaryExpNode ('+', (ExpNode*)$1, (ExpNode*)$3);
	    			$$->setLoc((Loc*)&(@$));
	    			BUFF.push_back($$);
			}
	    		else if(ERROR==0){
				ERROR=1;
				Delete();
	    		}
		}
		;



%%
void yyerror(const char *msg){
	error ("%s", msg);
}

void printerrline(int line,int column){

	FILE *fp;
	fp=fopen(filename,"r");
	int i;
	char c;
	printf("\n");
	int lino=1;
	printf("\033[34m%s:%d:%d:\n",filename+2,line,column);

	while(lino!=line){
		if(c=='\n')
			lino++;
		fscanf(fp,"%c",&c);
	}
/*	for(i=0;i<line-1;i++)
		fscanf(fp,"%*[^\n]%*c");    
*/
//	fscanf(fp,"%c",&c);    //another way
	while(c!='\n'){
		if(c=='	')
			printf(" ");
		else
			printf("%c",c);
		fscanf(fp,"%c",&c);
	}
	printf("\n");
	for(i=0;i<column;i++)
		printf(" ");
    	printf("\033[32m^\n");

}

void Delete(){
	for(std::vector<Node*>::iterator iter=BUFF.begin();iter!=BUFF.end();iter++)
		delete *iter;
	
/*	vector<Node*>::iterator iter=BUFF.begin();
	if(iter==(iter+3)) printf("yes\n");
	delete *iter;  printf("%lu\n",BUFF.size());
	delete *(iter+1);
*/	
		BUFF.clear();
		root=NULL;
}

/////////////////
	
