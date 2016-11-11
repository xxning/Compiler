#ifndef __NODE_H__
#define __NODE_H__

#include "llvm/Analysis/Passes.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/PassManager.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/Scalar.h"
#include <stdio.h>
#include <string>
#include <vector>
#include "dumpdot.hh"
using namespace llvm;

typedef struct {
	int first_line;
	int first_column;
	int last_line;
	int last_column;
} Loc;

//definition of nodes
class Node {
public:
	Loc* loc;
	Node() {loc = new Loc;}
	virtual ~Node(){delete loc;}
	void setLoc(Loc* loc);
	virtual int dumpdot(DumpDOT *dumper)=0;
	virtual Value *Codegen();
};

class ExpNode:public Node {
public:
	virtual int dumpdot(DumpDOT *dumper) = 0;
	virtual Value *Codegen();
};

class NumNode:public ExpNode {
public:
	int Val;
	NumNode(int val):Val(val) {}
	int dumpdot(DumpDOT *dumper);
	Value *Codegen();
};

class VarNode:public ExpNode {
public:
	std::string *Name;
	ExpNode *Exp;
	VarNode(std::string *name,ExpNode *exp)
		:Name(name),Exp(exp){}
	int dumpdot(DumpDOT *dumper);
	Value *Codegen();
	
};

class BinaryExpNode:public ExpNode {
public:
	char op;
	ExpNode *lhs,*rhs;
	BinaryExpNode(char op,ExpNode *lhs,ExpNode *rhs)
		:op(op),lhs(lhs),rhs(rhs) {}
	int dumpdot(DumpDOT *dumper);
	Value *Codegen();
};

class UnaryExpNode:public ExpNode{
public:
	char op;
	ExpNode *operand;
	UnaryExpNode(char op,ExpNode *operand)
		:op(op),operand(operand) {}
	int dumpdot(DumpDOT *dumper);
	Value *Codegen();	
};

class CondNode:public Node{
public:
	std::string *RelOp;
	ExpNode *lhs,*rhs;
	CondNode(std::string *relop,ExpNode *lhs,ExpNode *rhs)
		:RelOp(relop),lhs(lhs),rhs(rhs) {}
	int dumpdot(DumpDOT *dumper);
	Value *Codegen();
};

class LogicNode:public Node{
public:
	std::string *LogOp;
	CondNode *lhs,*rhs;
	LogicNode(std::string *logop,CondNode *lhs,CondNode *rhs)
		:LogOp(logop),lhs(lhs),rhs(rhs) {}
	int dumpdot(DumpDOT *dumper);
	Value *Codegen();		
};	

class IfNode:public ExpNode {
public:
	CondNode *cond;
	ExpNode *stmt1,*stmt2;
	IfNode(CondNode *cond,ExpNode *stmt1,ExpNode *stmt2)
		:cond(cond),stmt1(stmt1),stmt2(stmt2) {}
	int dumpdot(DumpDOT *dumper);
	Value *Codegen();	
};

class WhileNode:public ExpNode {
public:
	CondNode *cond;
	ExpNode *stmt;
	WhileNode(CondNode *cond,ExpNode *stmt)
		:cond(cond),stmt(stmt) {}
	int dumpdot(DumpDOT *dumper);
	Value *Codegen();	
};

class ForNode:public ExpNode {
public:
	ExpNode *Exp1,*Exp2;
	CondNode *Cond;
	ExpNode *Stmt;
	ForNode(ExpNode *exp1,CondNode *cond,ExpNode *exp2,ExpNode *stmt)
		:Exp1(exp1),Cond(cond),Exp2(exp2),Stmt(stmt) {}
	int dumpdot(DumpDOT *dumper);
	Value *Codegen();
};

class FuncNode:public ExpNode {
public:
	std::string *Name;
	std::vector<std::string*> Args;
	std::vector<ExpNode*> Args_list;
	FuncNode(std::string *name,std::vector<std::string*> args,std::vector<ExpNode*> args_list)
		:Name(name),Args(args),Args_list(args_list) {}
	int dumpdot(DumpDOT *dumper);
	Value *Codegen();
};

class BlockItemListNode:public Node {
public:
	std::vector<ExpNode*> BlockItem_list;
	int dumpdot(DumpDOT *dumper);
	void append(ExpNode* Exp){
		BlockItem_list.push_back(Exp);
	}
	Value *Codegen();	
};

class BlockNode:public ExpNode {
public:
	BlockItemListNode *Block_list;
	BlockNode(BlockItemListNode *block_list)
		:Block_list(block_list) {}
	int dumpdot(DumpDOT *dumper);
	Value *Codegen();
};

class FunNode:public Node {
public:
	std::string *Name;
	BlockNode *block;
	std::vector<std::string*> Args;
	std::vector<ExpNode*> Args_List;
	int Flag;
	FunNode(std::string *name,BlockNode *block,int flag,std::vector<std::string*> args,std::vector<ExpNode*> args_List)
		:Name(name),block(block),Flag(flag),Args(args),Args_List(args_List) {}
	
	int dumpdot(DumpDOT *dumper);
	Function *Codegen();	
};

class ConstDefListNode:public Node {
public:
	std::vector<ExpNode*> ConstDef_nodes;
	ConstDefListNode(){}
	int dumpdot(DumpDOT *dumper);
	void append(ExpNode* Exp){
		ConstDef_nodes.push_back(Exp);
	}
	Value *Codegen();	
};

class ConstDeclNode:public ExpNode {
public:
	ConstDefListNode* ConstDef_list;
	ConstDeclNode(ConstDefListNode* constdef_list)
		:ConstDef_list(constdef_list){}
	int dumpdot(DumpDOT *dumper);
	Value *Codegen();
};

class ConstNode:public ExpNode {
public:
	ConstDefListNode* Const_list;
	ConstNode(ConstDefListNode* const_list)
		:Const_list(const_list){}
	int dumpdot(DumpDOT *dumper);
	Value *Codegen();
};

class ConstDefS:public ExpNode{
public:
	std::string *Name;
	ExpNode *Exp;
	ConstDefS(std::string *name,ExpNode *exp)
		:Name(name),Exp(exp){}
	int dumpdot(DumpDOT *dumper);
	Value *Codegen();
};

class CompUnitNode:public Node {
public:
	std::vector<Node*> nodes;
	CompUnitNode() {}
	void append(Node* n){nodes.push_back(n);}
	int dumpdot(DumpDOT *dumper);
	Value *Codegen();
};

class ParaListNode:public Node {
public:
	std::vector<std::string*> Var_names;
	std::vector<ExpNode*> Arg_List;
	int dumpdot(DumpDOT *dumper);
	void append(std::string* v) {Var_names.push_back(v);}
	void appendn(ExpNode *n) {Arg_List.push_back(n);}
	Value *Codegen();
};

class VarListNode:public Node {
public:
	std::vector<ExpNode*> var_nodes;
	std::vector<std::string*> Var_names;
	int dumpdot(DumpDOT *dumper);
	void append(ExpNode *n) {var_nodes.push_back(n);}
	void appendv(std::string *v) {Var_names.push_back(v);}
	Value *Codegen();
};

class VarDeclNode:public ExpNode {
public:
	VarListNode *var_list;
	VarDeclNode(VarListNode *var_list):var_list(var_list){}
	int dumpdot(DumpDOT *dumper);
	Value *Codegen();	
};

class ExpListNode:public Node {
public:
	std::vector<ExpNode*> Exp_nodes;
	int dumpdot(DumpDOT *dumper);
	void append(ExpNode* Exp){Exp_nodes.push_back(Exp);}
	Value *Codegen();
};

//ConstDefM and ArrayNode have the same struct
class ConstDefM:public ExpNode{
public:
	std::string *Name;
	ExpNode *Exp;
	ExpListNode *Exp_list;
	ConstDefM(std::string *name,ExpNode *exp,ExpListNode *exp_list)
		:Name(name),Exp(exp),Exp_list(exp_list){}
	int dumpdot(DumpDOT *dumper);
	Value *Codegen();	
};

class ArrayNode:public ExpNode {
public:
	std::string *Name;
	ExpNode *Exp;
	ExpListNode *Exp_list;
	ArrayNode(std::string *name,ExpNode *exp,ExpListNode *exp_list)
		:Name(name),Exp(exp),Exp_list(exp_list){}
	int dumpdot(DumpDOT *dumper);
	Value *Codegen();	
};

class LvalNode:public ExpNode {
public:
	
	std::string *Name;
	ExpNode *Exp;
	LvalNode(std::string *name,ExpNode *exp)
		:Name(name),Exp(exp) {}
	int dumpdot(DumpDOT *dumper);
	Value *Codegen();
	std::string getName(){return *Name;}
	bool isArray(){return Exp?true:false;}	
};

class AssignNode:public ExpNode {
public:
	LvalNode *lval;
	ExpNode *Exp;
	int dumpdot(DumpDOT *dumper);
	AssignNode(LvalNode *lval,ExpNode *exp):lval(lval),Exp(exp) {}
	Value *Codegen();
};

class ControlNode:public ExpNode {
public:
	std::string *Name;
	ControlNode(std::string *name)
		:Name(name) {}
	int dumpdot(DumpDOT *dumper);
	Value *Codegen();
};

class ReturnNode:public ExpNode {
public:
	std::string *Name;
	ExpNode *Exp;
	ReturnNode(std::string *name,ExpNode *exp)
		:Name(name),Exp(exp) {}
	int dumpdot(DumpDOT *dumper);
	Value *Codegen();
};
#endif
