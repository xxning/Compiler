#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <map>
#include <string>
#include <vector>
#include <sstream>
#include "node.hh"
#include "dumpdot.hh"

//===----------------------------------------------------------------------===//
// Utilities in Dump DOT
//===----------------------------------------------------------------------===//

// There are two ways to create a dot node:
// 1. newNode(num, str1, str2, ...)
//    num corresponds to the number of strings
//    Each string will appear in the generated image as a port
//    All strings are char*
// 2. newNode(vec)
//    All elements of the vector are std::string
// newNode returns an integer, which is the number of the corresponding
// node in DOT file.
int DumpDOT::newNode(int num, ...) {
    va_list list;
    va_start(list, num);
    fprintf(fp, "    %d [label = \"", count);
    bool first = true;
    for (int i=0; i<num; i++) {
        char* st = va_arg(list, char*);
        if (!first)
            fprintf(fp, "|");
        first = false;
        if (st[0]=='<')
            fprintf(fp, "<%d> \\<", i);
        else
            fprintf(fp, "<%d> %s", i, st);
    }
    va_end(list);
    fprintf(fp, "\"];\n");
    return count++;
}

int DumpDOT::newNode(std::vector<std::string> list) {
    fprintf(fp, "    %d [label = \"", count);
    bool first = true;
    for (int i=0; i<(int)list.size(); i++) {
        std::string st = list[i];
        if (!first)
            fprintf(fp, "|");
        first = false;
        fprintf(fp, "<%d> %s", i, st.c_str());
    }
    fprintf(fp, "\"];\n");
    return count++;
}

void DumpDOT::drawLine(int nSrc, int pSrc, int nDst) {
    fprintf(fp, "    %d", nSrc);
    if (pSrc>=0)
        fprintf(fp, ":%d", pSrc);
    fprintf(fp, " -> %d;\n", nDst);
}
//===----------------------------------------------------------------------===//
// Dump AST to DOT
//===----------------------------------------------------------------------===//

// The following functions convert AST to DOT using DumpDOT.
// Each dumpdot returns an integer, which is corresponding number in DOT file.
// 53+29*71 will become:
// digraph {
// node [shape = record];
//     0 [label = "<0> |<1> +|<2> "];
//     1 [label = "<0> 53"];
//     2 [label = "<0> |<1> *|<2> "];
//     3 [label = "<0> 29"];
//     4 [label = "<0> 71"];
//     0:0 -> 1;
//     0:2 -> 2;
//     2:0 -> 3;
//     2:2 -> 4;
// }
/////////////////////////////////////////////////////////
int CompUnitNode::dumpdot(DumpDOT *dumper) {

    int nThis = dumper->newNode(1, "CompUnit");
    for (Node* n : nodes) {
        int nNode = n->dumpdot(dumper);
        dumper->drawLine(nThis, 0, nNode);
    }
    return nThis;
}

int ConstDeclNode::dumpdot(DumpDOT *dumper) {

    int nThis = dumper->newNode(2, "const int" , " ");
	for(ExpNode *n : ConstDef_list->ConstDef_nodes) {
		int nConstDefNode=n->dumpdot(dumper);
		dumper->drawLine(nThis,1,nConstDefNode);
	}
    return nThis;
}

int ConstDefListNode::dumpdot(DumpDOT *dumper) {

	return 0;
}

int ConstDefS::dumpdot(DumpDOT *dumper) {

    int nThis = dumper->newNode(3, Name->c_str(), "=", " ");
    int nExp = Exp->dumpdot(dumper);
    dumper->drawLine(nThis, 2, nExp);
    return nThis;
}

int NumNode::dumpdot(DumpDOT *dumper) {

    std::ostringstream str;
    str << Val;
    int nThis = dumper->newNode(1,str.str().c_str());
    return nThis;
}

int VarDeclNode::dumpdot(DumpDOT *dumper) {

    int nThis = dumper->newNode(2, "int", " ");
    for(ExpNode *n : var_list->var_nodes) {
        int nExpNode=n->dumpdot(dumper);
        dumper->drawLine(nThis,1,nExpNode);
    }
    return nThis;
}

int ConstDefM::dumpdot(DumpDOT *dumper) {

    int nThis = dumper->newNode(6,Name->c_str(),"["," ","]=\\{"," ","\\}");
    if(Exp!=NULL){
        int nExp=Exp->dumpdot(dumper);
        dumper->drawLine(nThis,2,nExp);
    }
    for(ExpNode *n : Exp_list->Exp_nodes){
        int nExpNode=n->dumpdot(dumper);
        dumper->drawLine(nThis,4,nExpNode);
    }
    return nThis;
}

int ArrayNode::dumpdot(DumpDOT *dumper) {

    int nThis;
    if(Exp_list==NULL) {//id[Exp]
    
        nThis = dumper->newNode(4,Name->c_str(),"["," ","]");
        int nExp=Exp->dumpdot(dumper);
        dumper->drawLine(nThis,2,nExp);
    }
    else {//id[Exp]={Exp1}
    
        nThis = dumper->newNode(6,Name->c_str(),"["," ","]=\\{"," ","\\}");
        int nExp=Exp->dumpdot(dumper);
        dumper->drawLine(nThis,2,nExp);
        for(ExpNode *n : Exp_list->Exp_nodes)
        {
            int nExpNode=n->dumpdot(dumper);
            dumper->drawLine(nThis,4,nExpNode);
        }
    }
	
    return nThis;
}

int VarNode::dumpdot(DumpDOT *dumper) {

    int nThis;
    if(Exp==NULL)  
        nThis = dumper->newNode(1,Name->c_str());  
    else {
        nThis = dumper->newNode(3,Name->c_str(),"="," ");
        int nExp = Exp->dumpdot(dumper);
        dumper->drawLine(nThis,2,nExp);
    }
    return nThis;
}

int ParaListNode::dumpdot(DumpDOT *dumper) {

	return 0;
}

int FunNode::dumpdot(DumpDOT *dumper) {

    int nThis;
    if(block==NULL){
		if(Flag==0)
        	nThis = dumper->newNode(3,"void",Name->c_str(),"();");
		else
			nThis = dumper->newNode(3,"int",Name->c_str(),"();");
	}
    else {
		if(Flag==0)
        	nThis = dumper->newNode(4,"void",Name->c_str(),"()"," ");
		else
			nThis = dumper->newNode(4,"int",Name->c_str(),"()"," ");
        int nBlock = block->dumpdot(dumper);
        dumper->drawLine(nThis,3,nBlock);
    }
	for(ExpNode *n : Args_List) {
        int nExpNode=n->dumpdot(dumper);
        dumper->drawLine(nThis,2,nExpNode);
    }
    return nThis;
}

int BlockNode::dumpdot(DumpDOT *dumper) {

	int nThis;
	if(Block_list==NULL)
		nThis=dumper->newNode(1,"\\{ \\}");
	else {
    		nThis = dumper->newNode(3,"\\{"," ","\\}");
    		for (ExpNode *n : Block_list->BlockItem_list)
    		{
    	    	int nBlockItemNode=n->dumpdot(dumper);
    	    	dumper->drawLine(nThis,1,nBlockItemNode);
    		}
	}

    return nThis;
}

int BlockItemListNode::dumpdot(DumpDOT *dumper) {

	return 0;
}

int AssignNode::dumpdot(DumpDOT *dumper) {

    int nThis;
    if(lval==NULL && Exp==NULL) {//empty stmt: ";"
    
        nThis=dumper->newNode(1,";");
    }
    else {//lval=exp;
    
        nThis=dumper->newNode(4," ","="," ",";");
        int nLval=lval->dumpdot(dumper);
        dumper->drawLine(nThis,0,nLval);
        int nExp=Exp->dumpdot(dumper);
        dumper->drawLine(nThis,2,nExp);
    }
    return nThis;
}

int ControlNode::dumpdot(DumpDOT *dumper) {
	
	int nThis=dumper->newNode(2,(*Name).c_str(),";");
	return nThis;
}

int ReturnNode::dumpdot(DumpDOT *dumper) {
	
	int nThis=dumper->newNode(2,(*Name).c_str(),"Exp");
	int nExp=Exp->dumpdot(dumper);
	dumper->drawLine(nThis,1,nExp);
	return nThis;
}

int FuncNode::dumpdot(DumpDOT *dumper) {

    int nThis=dumper->newNode(2,Name->c_str(),"();");
	for(ExpNode *n : Args_list) {
        int nExpNode=n->dumpdot(dumper);
        dumper->drawLine(nThis,1,nExpNode);
    }
    return nThis;
}

int IfNode::dumpdot(DumpDOT *dumper) {

    int nThis;
    if(stmt2==NULL) {//if without else
    
        nThis=dumper->newNode(3,"if","cond","stmt");
        int nCond=cond->dumpdot(dumper);
        dumper->drawLine(nThis,1,nCond);
        int nStmt1=stmt1->dumpdot(dumper);
        dumper->drawLine(nThis,2,nStmt1);
    }
    else {//if with else
    
        nThis=dumper->newNode(5,"if","cond","stmt","else","stmt");
        int nCond=cond->dumpdot(dumper);
        dumper->drawLine(nThis,1,nCond);
        int nStmt1=stmt1->dumpdot(dumper);
        dumper->drawLine(nThis,2,nStmt1);
        int nStmt2=stmt2->dumpdot(dumper);
        dumper->drawLine(nThis,4,nStmt2);
    }
    return nThis;
}

int VarListNode::dumpdot(DumpDOT *dumper) {

	return 0;
}

int WhileNode::dumpdot(DumpDOT *dumper) {

    int nThis=dumper->newNode(3,"while","cond","stmt");
    int nCond=cond->dumpdot(dumper);
    dumper->drawLine(nThis,1,nCond);
    int nStmt=stmt->dumpdot(dumper);
    dumper->drawLine(nThis,2,nStmt);
    return nThis;
}

int ForNode::dumpdot(DumpDOT *dumper) {

	int nThis=dumper->newNode(5,"for","start","cond","stmt","step");
	int nExp1=Exp1->dumpdot(dumper);
	dumper->drawLine(nThis,1,nExp1);
	int nCond=Cond->dumpdot(dumper);
	dumper->drawLine(nThis,2,nCond);
	int nStmt=Stmt->dumpdot(dumper);
	dumper->drawLine(nThis,3,nStmt);
	int nExp2=Exp2->dumpdot(dumper);
	dumper->drawLine(nThis,4,nExp2);
	return nThis;
}

int CondNode::dumpdot(DumpDOT *dumper) {

    int nThis=dumper->newNode(3," ",("\\"+*RelOp).c_str()," ");
    int nExpLHS=lhs->dumpdot(dumper);
    dumper->drawLine(nThis,0,nExpLHS);
    int nExpRHS=rhs->dumpdot(dumper);
    dumper->drawLine(nThis,2,nExpRHS);
    return nThis;
}

int LogicNode::dumpdot(DumpDOT *dumper) {
	
	if(rhs){
		int nThis;
		if(strcmp((*LogOp).c_str(),"||")==0)
			nThis=dumper->newNode(3," ","\\|\\|"," ");
		else
			nThis=dumper->newNode(3," ",(*LogOp).c_str()," ");
		int nLHS=lhs->dumpdot(dumper);
		dumper->drawLine(nThis,0,nLHS);
		int nRHS=rhs->dumpdot(dumper);
		dumper->drawLine(nThis,2,nRHS);
		return nThis;
	}
	else {
		int nThis=dumper->newNode(2,("\\"+*LogOp).c_str()," ");
		int nLHS=lhs->dumpdot(dumper);
		dumper->drawLine(nThis,1,nLHS);
		return nThis;
	}
}

int LvalNode::dumpdot(DumpDOT *dumper) {

    int nThis;
    if(Exp==NULL) //id
    {
        nThis=dumper->newNode(1,Name->c_str());
    }
    else {//id[Exp]
    
        nThis=dumper->newNode(4,Name->c_str(),"["," ","]");
        int nExp=Exp->dumpdot(dumper);
        dumper->drawLine(nThis,2,nExp);
    }
    return nThis;
}

int BinaryExpNode::dumpdot(DumpDOT *dumper) {

    char st[2] = " ";
    st[0] = op;
    int nThis = dumper->newNode(3, " ", st, " ");
    int nlhs = lhs->dumpdot(dumper);
    int nrhs = rhs->dumpdot(dumper);
    dumper->drawLine(nThis, 0, nlhs);
    dumper->drawLine(nThis, 2, nrhs);
    return nThis;
}

int UnaryExpNode::dumpdot(DumpDOT *dumper) {

    char st[2] = " ";
    st[0] = op;
    int nThis = dumper->newNode(2, st, " ");
    int nOperand = operand->dumpdot(dumper);
    dumper->drawLine(nThis, 1, nOperand);
    return nThis;
}

int ExpListNode::dumpdot(DumpDOT *dumper) {

	return 0;
}


