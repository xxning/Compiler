#include "node.hh"
#include <map>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
using namespace llvm;
using namespace std;

Module *TheModule;
IRBuilder<> Builder(getGlobalContext());

bool Global=true;//used to judge the scope
int Control=0;//0:have no break or continue; 1:otherwise

Value *ErrorV(const char *Str){
	fprintf(stderr,"Error: %s\n",Str);
	return 0;
}
std::map<std::string, GlobalVariable*> GlobalValues;
std::map<std::string, GlobalVariable*> GlobalArray;
std::map<std::string, GlobalVariable*> GlobalConstValues;
std::map<std::string, AllocaInst *> LocalValues;
std::map<std::string, AllocaInst *> LocalArray;
std::map<std::string, AllocaInst *> LocalConstValues;
std::map<std::string, AllocaInst *> ScopeValues;
std::map<std::string, AllocaInst *> ScopeConstValues;

std::map<std::string,Value*> NamedValues;
std::vector<BasicBlock*> ContinueBr;
std::vector<BasicBlock*> BreakBr;

Value *BinaryExpNode::Codegen(){
	Value *L = lhs->Codegen();
	Value *R = rhs->Codegen();
	if (L == 0 || R == 0) return 0;

	switch (op) {
		case '+': return Builder.CreateAdd(L, R, "addtmp");
		case '-': return Builder.CreateSub(L, R, "subtmp");
		case '*': return Builder.CreateMul(L, R, "multmp");
		case '/': return Builder.CreateSDiv(L, R, "divtmp");
		case '%': return Builder.CreateSRem(L, R, "remtmp");
		default: return ErrorV("invalid binary operator");
	}
	return ErrorV("invalid binary operator");
}

Value *UnaryExpNode::Codegen(){
	Value *val = operand->Codegen();
	switch(op){
		case '+': return val;
		case '-': return Builder.CreateNeg(val, "negtmp");
		default: return ErrorV("invalid unary operator");
	}
}

Value *IfNode::Codegen(){
	Value *CondV = cond->Codegen();
	if (CondV == 0)
		return 0;

	// Convert condition to a bool by comparing equal to 0.0.
	CondV = Builder.CreateICmpNE(
			CondV, ConstantInt::get(getGlobalContext(), APInt(1,0)), "ifcond");

	Function *TheFunction = Builder.GetInsertBlock()->getParent();

	// Create blocks for the then and else cases.  Insert the 'then' block at the
	// end of the function.
	if(stmt2) {	
		BasicBlock *ThenBB =
			BasicBlock::Create(getGlobalContext(), "then", TheFunction);
		BasicBlock *ElseBB = BasicBlock::Create(getGlobalContext(), "else");
		BasicBlock *MergeBB = BasicBlock::Create(getGlobalContext(), "ifcont");

		Builder.CreateCondBr(CondV, ThenBB, ElseBB);

		// Emit then value.
		Builder.SetInsertPoint(ThenBB);

		Value *ThenV = stmt1->Codegen();
		if (ThenV == 0)
			return 0;

		Builder.CreateBr(MergeBB);
		// Codegen of 'Then' can change the current block, update ThenBB for the PHI.
		ThenBB = Builder.GetInsertBlock();

		// Emit else block.
		TheFunction->getBasicBlockList().push_back(ElseBB);
		Builder.SetInsertPoint(ElseBB);

		Value *ElseV = stmt2->Codegen();
		if (ElseV == 0)
			return 0;

		Builder.CreateBr(MergeBB);
		// Codegen of 'Else' can change the current block, update ElseBB for the PHI.
		ElseBB = Builder.GetInsertBlock();

		// Emit merge block.
		TheFunction->getBasicBlockList().push_back(MergeBB);
		Builder.SetInsertPoint(MergeBB);
		PHINode *PN =
			Builder.CreatePHI(Type::getInt32Ty(getGlobalContext()), 2, "iftmp");

		PN->addIncoming(ThenV, ThenBB);
		PN->addIncoming(ElseV, ElseBB);
		return PN;
	}
	else{
		BasicBlock *ThenBB =
			BasicBlock::Create(getGlobalContext(), "then", TheFunction);
		BasicBlock *EndBB = BasicBlock::Create(getGlobalContext(), "ifend", TheFunction);
		Builder.CreateCondBr(CondV, ThenBB, EndBB);
		Builder.SetInsertPoint(ThenBB);
		Value *ThenV = stmt1->Codegen();
		if (ThenV == 0)
			return 0;
		if(!Control)
			Builder.CreateBr(EndBB);/////////
		Builder.SetInsertPoint(EndBB);
		Control=0;
		return Constant::getNullValue(Type::getInt32Ty(getGlobalContext()));
	}
}

Value *CondNode::Codegen() {

	Value *L = lhs->Codegen();
	Value *R = rhs->Codegen();
	if (L == 0 || R == 0) return 0;

	if(*RelOp=="<")
		return Builder.CreateICmpSLT(L, R, "lt_tmp");
	if(*RelOp=="<=")
		return Builder.CreateICmpSLE(L, R, "le_tmp");
	if(*RelOp=="==")
		return Builder.CreateICmpEQ(L, R, "eq_tmp");
	if(*RelOp==">")
		return Builder.CreateICmpSGT(L, R, "gt_tmp");
	if(*RelOp==">=")
		return Builder.CreateICmpSGE(L, R, "ge_tmp");
	if(*RelOp=="!=")
		return Builder.CreateICmpNE(L, R, "ne_tmp");

	return ErrorV("invalid conditon operator");
}

Value *LogicNode::Codegen() {

	Value *L = lhs->Codegen();
	Value *R;
	if(rhs){
		R = rhs->Codegen();
		if (L == 0 || R == 0) return 0;
	}
	//if (L == 0 || R == 0) return 0;
	if(L==0) return 0;	
	
	Value *Cond1,*Cond2,*Cond3,*Cond4,*Cond5;
	Value *Cond;
	if(strcmp((*LogOp).c_str(),"||")==0){ //logic or
		//L or R = (L+R)+L*R
		//	 = 1+(1+L)*(1+R)
		Cond1 = Builder.CreateICmpNE(
			L, ConstantInt::get(getGlobalContext(), APInt(1,0)), "Lhs");
		Cond2 = Builder.CreateICmpNE(
			R, ConstantInt::get(getGlobalContext(), APInt(1,0)), "Rhs");  		
		Cond3 = Builder.CreateAdd(Cond1, Cond2, "Testor1");
		Cond4 = Builder.CreateMul(Cond1, Cond2, "Testor2");
		Cond5 = Builder.CreateAdd(Cond3, Cond4, "Testor3");
		Cond = Builder.CreateICmpEQ(
			Cond5, ConstantInt::get(getGlobalContext(), APInt(1,1)), "testor4");
		return Cond;
			
	}else if(strcmp((*LogOp).c_str(),"&&")==0){//logic and
		// L and R = L*R
		Cond1 = Builder.CreateICmpNE(
			L, ConstantInt::get(getGlobalContext(), APInt(1,0)), "Lhs");
		Cond2 = Builder.CreateICmpNE(
			R, ConstantInt::get(getGlobalContext(), APInt(1,0)), "Rhs");  		
		Cond3 = Builder.CreateMul(Cond1, Cond2, "Testand1");
		Cond = Builder.CreateICmpEQ(
			Cond3, ConstantInt::get(getGlobalContext(), APInt(1,1)), "Testand2");
			return Cond;
			 
	}else {//logic not  
		//not L = L+1
		Cond1 = Builder.CreateICmpNE(
			L, ConstantInt::get(getGlobalContext(), APInt(1,0)), "Lhs");
		Cond2 = Builder.CreateAdd(Cond1, ConstantInt::get(getGlobalContext(), APInt(1,1)), "Testnot1");
		Cond = Builder.CreateICmpEQ(
			Cond2, ConstantInt::get(getGlobalContext(), APInt(1,1)), "Testnot2");
		return Cond;
        }

	return Constant::getNullValue(Type::getInt32Ty(getGlobalContext()));
}

Value *ControlNode::Codegen() {

	BasicBlock *Block;
	Control=1;
	switch(strcmp((*Name).c_str(),"continue")){
		case 0: {
			std::vector<BasicBlock*>::iterator iter=ContinueBr.end();	
			Block=ContinueBr[ContinueBr.size()-1];
			Builder.CreateBr(Block);
			delete *iter;
			break;
			}
		default: {
			std::vector<BasicBlock*>::iterator iter=BreakBr.end();	
			
			Block=BreakBr[BreakBr.size()-1];
			Builder.CreateBr(Block);
			delete *iter;
			break;
			}
		//default: return ErrorV("unknown keyWord.\n");	
	}
/*
	Function *TheFunction = Builder.GetInsertBlock()->getParent();
	BasicBlock *AfterBB =
		BasicBlock::Create(getGlobalContext(), "breakpoint", TheFunction);
*/	
	
	return Constant::getNullValue(Type::getInt32Ty(getGlobalContext()));	
	
}

Value *ReturnNode::Codegen() {

	Value *RetVal=Exp->Codegen();
	Builder.CreateRet(RetVal);
	return Constant::getNullValue(Type::getInt32Ty(getGlobalContext()));	
}

static AllocaInst *CreateEntryBlockAlloca(Function *TheFunction,
                                          const std::string &VarName) {
  IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                   TheFunction->getEntryBlock().begin());
  return TmpB.CreateAlloca(Type::getInt32Ty(getGlobalContext()), 0,
                           VarName.c_str());
}

Value *ForNode::Codegen() {

	Function *TheFunction = Builder.GetInsertBlock()->getParent();
	
	std::string *VarName = ((AssignNode*)Exp1)->lval->Name;
  	// Create an alloca for the variable in the entry block.
  	AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, *VarName);

  	// Emit the start code first, without 'variable' in scope.
  	Value *StartVal = Exp1->Codegen();
  	if (StartVal == 0)
    		return 0;

  	// Store the value into the alloca.
  	Builder.CreateStore(StartVal, Alloca);

  	// Make the new basic block for the loop header, inserting after current
  	// block.
  	BasicBlock *LoopBB =
      		BasicBlock::Create(getGlobalContext(), "loop", TheFunction);

  	// Insert an explicit fall through from the current block to the LoopBB.
  	Builder.CreateBr(LoopBB);

  	// Start insertion in LoopBB.
  	Builder.SetInsertPoint(LoopBB);

  	// Within the loop, the variable is defined equal to the PHI node.  If it
  	// shadows an existing variable, we have to restore it, so save it now.
	AllocaInst *OldVal;
	if(!LocalValues[*VarName]){
		return ErrorV("use of undeclar variab\n");
	} else {
		OldVal = LocalValues[*VarName];
  		LocalValues[*VarName] = Alloca;
	}
  	//AllocaInst *OldVal = NamedValues[*VarName];
  	//NamedValues[*VarName] = Alloca;

  	// Emit the body of the loop.  This, like any other expr, can change the
  	// current BB.  Note that we ignore the value computed by the body, but don't
  	// allow an error.
  	if (Stmt->Codegen() == 0)
    		return 0;
	if(Exp2)
		Exp2->Codegen();
	else 
		return ErrorV("lacks step expression\n");
  	// Emit the step value.
  	//Value *StepVal;
/*
  	if (Exp2) {
    		StepVal = Exp2->Codegen();
    		if (StepVal == 0)
      			return 0;
  	} else {
  	// If not specified, use 1.0.
	//const_int32 = ConstantInt::get(getGlobalContext(), APInt(32,0));
    	StepVal = ConstantInt::get(getGlobalContext(), APInt(32,1));//APInt(32,1)
  	}
*/
  	// Compute the end condition.
  	Value *EndCond = Cond->Codegen();
  	if (EndCond == 0)
    		return EndCond;

  	// Reload, increment, and restore the alloca.  This handles the case where
  	// the body of the loop mutates the variable.
  	//Value *CurVar = Builder.CreateLoad(Alloca, (*VarName).c_str());
  	//Value *NextVar = Builder.CreateAdd(CurVar, StepVal, "nextvar");
  	//Builder.CreateStore(NextVar, Alloca);

  	// Convert condition to a bool by comparing equal to 0.0.
  	EndCond = Builder.CreateICmpNE(
      		EndCond, ConstantInt::get(getGlobalContext(), APInt(1,0)), "loopcond");

  	// Create the "after loop" block and insert it.
  	BasicBlock *AfterBB =
      		BasicBlock::Create(getGlobalContext(), "afterloop", TheFunction);

  	// Insert the conditional branch into the end of LoopEndBB.
  	Builder.CreateCondBr(EndCond, LoopBB, AfterBB);

  	// Any new code will be inserted in AfterBB.
  	Builder.SetInsertPoint(AfterBB);

  	// Restore the unshadowed variable.
	
  	if (OldVal)
    		LocalValues[*VarName] = OldVal;
  	else
    		LocalValues.erase(*VarName);

  	// for expr always returns 0.0.
  	return Constant::getNullValue(Type::getDoubleTy(getGlobalContext()));
}

Value *WhileNode::Codegen() {

	Function *TheFunction = Builder.GetInsertBlock()->getParent();
	BasicBlock *CondBB =
		BasicBlock::Create(getGlobalContext(), "condition", TheFunction);
	BasicBlock *LoopBB =
		BasicBlock::Create(getGlobalContext(), "loop", TheFunction);
	BasicBlock *AfterBB =
		BasicBlock::Create(getGlobalContext(), "afterloop", TheFunction);
	ContinueBr.push_back(CondBB);
	BreakBr.push_back(AfterBB);
	Builder.CreateBr(CondBB);
	Builder.SetInsertPoint(CondBB);
	Value *CondVal = cond->Codegen();
	if (CondVal == 0)
		return 0;
	Builder.CreateCondBr(CondVal,LoopBB,AfterBB);

	Builder.SetInsertPoint(LoopBB);

	if (stmt->Codegen() == 0)
		return 0;

	Builder.CreateBr(CondBB);
	Builder.SetInsertPoint(AfterBB);
	
	ContinueBr.clear();
	BreakBr.clear();

	return Constant::getNullValue(Type::getInt32Ty(getGlobalContext()));
}

Value *CompUnitNode::Codegen() {
	for(size_t i=0;i<nodes.size();++i) {
		if(!nodes[i]->Codegen()) {
			return ErrorV("Error in CompUnitNode::Codegen()");
		}
	}
	return Constant::getNullValue(Type::getInt32Ty(getGlobalContext()));
}

Value *ParaListNode::Codegen() {

	return Constant::getNullValue(Type::getInt32Ty(getGlobalContext()));
}

Function *FunNode::Codegen() {
	
	NamedValues.clear();
	std::vector<Type*> Ints(Args.size(),
                             Type::getInt32Ty(getGlobalContext()));
	FunctionType *FT = FunctionType::get(Type::getInt32Ty(getGlobalContext()),Ints,true);//false
	Function *F = Function::Create(FT, Function::ExternalLinkage, *Name, TheModule);

	// If F conflicted, there was already something named 'Name'.  If it has a
  	// body, don't allow redefinition or reextern.
	if (F->getName() != *Name) {
		// Delete the one we just made and get the existing one.
		F->eraseFromParent();
		F = TheModule->getFunction(*Name);

		// If F already has a body, reject this.
		if (!F->empty()) {
			ErrorV("redefinition of function");
			return 0;
		}
	
	// If F took a different number of args, reject.
    		if (F->arg_size() != Args.size()) {
      			ErrorV("redefinition of function with different # args");
      			return 0;
    		}
  	}
	// Set names for all arguments.
  	unsigned Idx = 0;
  	for (Function::arg_iterator AI = F->arg_begin(); Idx != Args.size();
       		++AI, ++Idx) {
    		AI->setName(*(Args[Idx]));
    
   	 // Add arguments to variable symbol table.
    		NamedValues[*(Args[Idx])]= AI;
  	}	
	
	if(block==NULL)	
		return F;	
	
	Function *TheFunction=F;
	BasicBlock *BB = BasicBlock::Create(getGlobalContext(), "entry", TheFunction);
	Builder.SetInsertPoint(BB);
	if (Value *RetVal=block->Codegen()) {
		// Finish off the function.
		Builder.CreateRet(RetVal); //可以不要
		//Builder.CreateRetVoid();
		// Validate the generated code, checking for consistency.
		verifyFunction(*TheFunction);
		return TheFunction;
	}
	// Error reading body, remove function.
	TheFunction->eraseFromParent();
	return 0;
}

Value *FuncNode::Codegen() {
	// Look up the name in the global module table.
	Function *CalleeF = TheModule->getFunction(*Name);
	if (CalleeF == 0)
		return ErrorV("Unknown function referenced");
	//return Builder.CreateCall(CalleeF, None, "calltmp");

	// If argument mismatch error.
  	if (CalleeF->arg_size() != Args.size())
    		return ErrorV("Incorrect # arguments passed");

  	std::vector<Value*> ArgsV;
  	for (unsigned i = 0, e = Args_list.size(); i != e; ++i) {
    		ArgsV.push_back(Args_list[i]->Codegen());
    		if (ArgsV.back() == 0) return 0;
  	}
  
  	return Builder.CreateCall(CalleeF, ArgsV, "calltmp");

	//return Builder.CreateCall(CalleeF);
}

Value *BlockNode::Codegen() {
	if(Block_list) {
		Global=false;
		std::map<std::string, AllocaInst *> OldNamedValues=LocalValues;
		ScopeValues.clear();
		Value *RetVal=Block_list->Codegen();
		LocalValues=OldNamedValues;
		Global=true;
		return RetVal;
	}
	return Constant::getNullValue(Type::getInt32Ty(getGlobalContext()));
}


Value *ConstDefListNode::Codegen() {
	for(size_t i=0;i<ConstDef_nodes.size();++i) {
		if(!ConstDef_nodes[i]->Codegen()) {
			return ErrorV("Error in ConstDefListNode::Codegen()");
		}
	}
	return Constant::getNullValue(Type::getInt32Ty(getGlobalContext()));
}


Value *ConstDefS::Codegen() {

	if(Global) {
		if(GlobalConstValues[*Name]) {
			return ErrorV("redefinition of global constant.");
		}
		else {
			GlobalVariable* gvar_int32 = new GlobalVariable(/*Module=*/*TheModule, 
					/*Type=*/IntegerType::get(TheModule->getContext(), 32),
					/*isConstant=*/false,
					/*Linkage=*/GlobalValue::ExternalLinkage,
					/*Initializer=*/0, // has initializer, specified below
					/*Name=*/Name->c_str());
			gvar_int32->setAlignment(4);

			// Constant Definitions
			ConstantInt* const_int32;
			if(Exp) {
				if((const_int32 = dynamic_cast<ConstantInt*>(Exp->Codegen()))==0)
				{
					return ErrorV("initializer element is not a compile-time constant.");
				}
			}
			else{
				//const_int32 = ConstantInt::get(getGlobalContext(), APInt(32,0));
				return ErrorV("Const definition must have init value.");
			}

			// Global Variable Definitions
			gvar_int32->setInitializer(const_int32);
			GlobalConstValues[*Name]=gvar_int32;
			return gvar_int32;
		}
	}
	else {
		if(ScopeConstValues[*Name]) {
			return ErrorV("redefinition of local constant.");
		}
		else {
			Function *TheFunction = Builder.GetInsertBlock()->getParent();
			ConstantInt* const_int32;
			if(Exp)
			{
				if((const_int32 = dynamic_cast<ConstantInt*>(Exp->Codegen()))==0)
				{
					return ErrorV("initializer element is not a compile-time constant.");
				}
			}
			else{
				return ErrorV("Const definition must have init value.");
			}
			AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction,*Name);
			Builder.CreateStore(const_int32, Alloca);
			LocalConstValues[*Name] = Alloca;
			ScopeConstValues[*Name] = Alloca;
			return const_int32;
		}
	}
}

Value *Node::Codegen() {
	return Constant::getNullValue(Type::getInt32Ty(getGlobalContext()));
}

Value *ConstDefM::Codegen() {
	ConstantInt *Len;
	if(Exp) {
		Len=dynamic_cast<ConstantInt *>(Exp->Codegen());
		if(Len==0)
		{
			return ErrorV("array size must be a constant");
		}
	}
	else if(Exp_list) {
		Len = ConstantInt::get(TheModule->getContext(), APInt(32, Exp_list->Exp_nodes.size(), 10));
	}
	else {
		return ErrorV("missing array size or initializer");
	}

	if(Global){
		// Type Definitions
		ArrayType* ArrayTy = ArrayType::get(IntegerType::get(TheModule->getContext(), 32), Len->getSExtValue());

		// Global Variable Declarations
		GlobalVariable* gvar_array = new GlobalVariable(/*Module=*/*TheModule, 
				/*Type=*/ArrayTy,
				/*isConstant=*/false,
				/*Linkage=*/GlobalValue::ExternalLinkage,
				/*Initializer=*/0, // has initializer, specified below
				/*Name=*/Name->c_str());
		gvar_array->setAlignment(4);

		// Constant Definitions
		//ConstantAggregateZero* const_array_2 = ConstantAggregateZero::get(ArrayTy);
		Constant* const_array;
		if(Exp_list){
			std::vector<Constant*> const_array_elems;
			for(size_t i=0;i<Exp_list->Exp_nodes.size();++i){
				const_array_elems.push_back((ConstantInt*)(Exp_list->Exp_nodes[i]->Codegen()));
			}
			const_array = ConstantArray::get(ArrayTy, const_array_elems);
		}
		else{
			const_array = ConstantAggregateZero::get(ArrayTy);
		}

		// Global Variable Definitions
		gvar_array->setInitializer(const_array);
		GlobalArray[*Name]=gvar_array;
		return gvar_array;
	}
	else{
		ArrayType* ArrayTy = ArrayType::get(IntegerType::get(TheModule->getContext(), 32), Len->getSExtValue());
		Function *TheFunction = Builder.GetInsertBlock()->getParent();
		AllocaInst* array = new AllocaInst(ArrayTy, Name->c_str(),&TheFunction->getEntryBlock());

		Constant* const_array;
		if(Exp_list) {
			size_t s=Exp_list->Exp_nodes.size();
			size_t l=Len->getSExtValue();
			std::vector<Value*> ptr_indices;
			ConstantInt* const_0 = ConstantInt::get(TheModule->getContext(), APInt(32, StringRef("0"), 10));
			if(s>l)	{	
				return ErrorV("excess elements in array initializer");
			}
			for(size_t i=0;i<s;++i) {
				Value *Val = Exp_list->Exp_nodes[i]->Codegen();
				ConstantInt* const_int64_index = ConstantInt::get(TheModule->getContext(), APInt(64, i, 10));
				ptr_indices.clear();
				ptr_indices.push_back(const_0);
				ptr_indices.push_back(const_int64_index);
				Instruction *ptr=GetElementPtrInst::CreateInBounds(array,ptr_indices,"",Builder.GetInsertBlock());
				StoreInst* si = new StoreInst(Val, ptr, false, Builder.GetInsertBlock());
				si->setAlignment(4);
			}
			for(size_t i=s;i<l;++i) {
				ConstantInt* const_int64_index = ConstantInt::get(TheModule->getContext(), APInt(64, i, 10));
				ptr_indices.clear();
				ptr_indices.push_back(const_0);
				ptr_indices.push_back(const_int64_index);
				Instruction *ptr=GetElementPtrInst::CreateInBounds(array,ptr_indices,"",Builder.GetInsertBlock());
				StoreInst* si = new StoreInst(const_0, ptr, false, Builder.GetInsertBlock());
				si->setAlignment(4);
			}
		}
		else {	
			const_array = ConstantAggregateZero::get(ArrayTy);
		}
		array->setAlignment(4);
		LocalArray[*Name]=array;
		return array;
	}
}

Value *ExpListNode::Codegen() {
	for(size_t i=0;i<Exp_nodes.size();++i) {
		if(!Exp_nodes[i]->Codegen()) {
			return ErrorV("Error in ExpListNode::Codegen()");
		}
	}
	return Constant::getNullValue(Type::getInt32Ty(getGlobalContext()));
}

Value *ConstDeclNode::Codegen() {
	return ConstDef_list->Codegen();
}

Value *VarListNode::Codegen() {
	for(size_t i=0;i<var_nodes.size();++i) {
		if(!var_nodes[i]->Codegen()) {
			return ErrorV("Error in VarListNode::Codegen()");
		}
	}
	return Constant::getNullValue(Type::getInt32Ty(getGlobalContext()));
}

Value *ArrayNode::Codegen() {
	ConstantInt *Len;
	if(Exp) {
		Len=dynamic_cast<ConstantInt *>(Exp->Codegen());
		if(Len==0) {
			return ErrorV("array size must be a constant");
		}
	}
	else if(Exp_list) {
		Len = ConstantInt::get(TheModule->getContext(), APInt(32, Exp_list->Exp_nodes.size(), 10));
	}
	else {
		return ErrorV("missing array size or initializer");
	}

	if(Global) {
		// Type Definitions
		ArrayType* ArrayTy = ArrayType::get(IntegerType::get(TheModule->getContext(), 32), Len->getSExtValue());

		// Global Variable Declarations
		GlobalVariable* gvar_array = new GlobalVariable(/*Module=*/*TheModule, 
				/*Type=*/ArrayTy,
				/*isConstant=*/false,
				/*Linkage=*/GlobalValue::ExternalLinkage,
				/*Initializer=*/0, // has initializer, specified below
				/*Name=*/Name->c_str());
		gvar_array->setAlignment(4);

		// Constant Definitions
		//ConstantAggregateZero* const_array_2 = ConstantAggregateZero::get(ArrayTy);
		Constant* const_array;
		if(Exp_list) {
			std::vector<Constant*> const_array_elems;
			for(size_t i=0;i<Exp_list->Exp_nodes.size();++i) {
				const_array_elems.push_back((ConstantInt*)(Exp_list->Exp_nodes[i]->Codegen()));
			}
			const_array = ConstantArray::get(ArrayTy, const_array_elems);
		}
		else {
			const_array = ConstantAggregateZero::get(ArrayTy);
		}
		// Global Variable Definitions
		gvar_array->setInitializer(const_array);
		GlobalArray[*Name]=gvar_array;
		return gvar_array;
	}
	else {
		ArrayType* ArrayTy = ArrayType::get(IntegerType::get(TheModule->getContext(), 32), Len->getSExtValue());
		Function *TheFunction = Builder.GetInsertBlock()->getParent();
		AllocaInst* array = new AllocaInst(ArrayTy, Name->c_str(),&TheFunction->getEntryBlock());

		Constant* const_array;
		if(Exp_list) {	
		
			size_t s=Exp_list->Exp_nodes.size();
			size_t l=Len->getSExtValue();
			std::vector<Value*> ptr_indices;
			ConstantInt* const_0 = ConstantInt::get(TheModule->getContext(), APInt(32, StringRef("0"), 10));
			if(s>l)	{	
				return ErrorV("excess elements in array initializer");
			}
			for(size_t i=0;i<s;++i) {
				Value *Val = Exp_list->Exp_nodes[i]->Codegen();
				ConstantInt* const_int64_index = ConstantInt::get(TheModule->getContext(), APInt(64, i, 10));
				ptr_indices.clear();
				ptr_indices.push_back(const_0);
				ptr_indices.push_back(const_int64_index);
				Instruction *ptr=GetElementPtrInst::CreateInBounds(array,ptr_indices,"",Builder.GetInsertBlock());
				StoreInst* si = new StoreInst(Val, ptr, false, Builder.GetInsertBlock());
				si->setAlignment(4);
			}
			for(size_t i=s;i<l;++i) {
				ConstantInt* const_int64_index = ConstantInt::get(TheModule->getContext(), APInt(64, i, 10));
				ptr_indices.clear();
				ptr_indices.push_back(const_0);
				ptr_indices.push_back(const_int64_index);
				Instruction *ptr=GetElementPtrInst::CreateInBounds(array,ptr_indices,"",Builder.GetInsertBlock());
				StoreInst* si = new StoreInst(const_0, ptr, false, Builder.GetInsertBlock());
				si->setAlignment(4);
			}
		}
		else {	
			const_array = ConstantAggregateZero::get(ArrayTy);
		}
		array->setAlignment(4);
		LocalArray[*Name]=array;
		return array;
	}
}

Value *VarDeclNode::Codegen() {
	return var_list->Codegen();
}

Value *VarNode::Codegen() {
	if(Global){
		if(GlobalValues[*Name]) {
			return ErrorV("redefinition of global variable.");
		}
		else {
			GlobalVariable* gvar_int32 = new GlobalVariable(/*Module=*/*TheModule, 
					/*Type=*/IntegerType::get(TheModule->getContext(), 32),
					/*isConstant=*/false,
					/*Linkage=*/GlobalValue::ExternalLinkage,
					/*Initializer=*/0, // has initializer, specified below
					/*Name=*/Name->c_str());
			gvar_int32->setAlignment(4);

			// Constant Definitions
			ConstantInt* const_int32;
			if(Exp) {
				if((const_int32 = dynamic_cast<ConstantInt*>(Exp->Codegen()))==0)
				{
					return ErrorV("initializer element is not a compile-time constant.");
				}
			}
			else {
				const_int32 = ConstantInt::get(getGlobalContext(), APInt(32,0));
			}

			// Global Variable Definitions
			gvar_int32->setInitializer(const_int32);
			GlobalValues[*Name]=gvar_int32;
			return gvar_int32;
		}
	}
	else {
		if(ScopeValues[*Name]) {
			return ErrorV("redefinition of local variable.");
		}
		else {
			Function *TheFunction = Builder.GetInsertBlock()->getParent();
			Value *InitVal;
			if (Exp) {
				InitVal = Exp->Codegen();
				if (InitVal == 0)
					return 0;
			} else { // If not specified, use 0.0.
				InitVal = ConstantInt::get(getGlobalContext(), APInt(32,0));
			}
			AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction,*Name);
			Builder.CreateStore(InitVal, Alloca);
			LocalValues[*Name] = Alloca;
			ScopeValues[*Name] = Alloca;
			return InitVal;
		}
	}
}

Value *NumNode::Codegen() {
	return ConstantInt::get(getGlobalContext(), APInt(32,Val));
}

Value *BlockItemListNode::Codegen() {
	for(size_t i=0;i<BlockItem_list.size();++i) {
		if(!BlockItem_list[i]->Codegen()){
			return ErrorV("Error in BlockItemListNode::Codegen()");
		}
	}
	return Constant::getNullValue(Type::getInt32Ty(getGlobalContext()));
}

Value *AssignNode::Codegen()
{
	if(!lval && !Exp) {
	
		return Constant::getNullValue(Type::getInt32Ty(getGlobalContext()));
	}
	Value *Val = Exp->Codegen();
	if (Val == 0)
		return 0;
	// Look up the type.
	if(lval->isArray()) {
		Value *index = lval->Exp->Codegen();
		AllocaInst *array = LocalArray[lval->getName()];
		GlobalVariable *global_array = GlobalArray[lval->getName()];
		if (array) {
			CastInst* int64_index = new SExtInst(index, IntegerType::get(TheModule->getContext(), 64), "", Builder.GetInsertBlock());
			ConstantInt* const_0 = ConstantInt::get(TheModule->getContext(), APInt(32, StringRef("0"), 10));
			std::vector<Value*> ptr_indices;
			ptr_indices.push_back(const_0);
			ptr_indices.push_back(int64_index);
			Instruction *ptr=GetElementPtrInst::CreateInBounds(array,ptr_indices,"",Builder.GetInsertBlock());
			StoreInst* si = new StoreInst(Val, ptr, false, Builder.GetInsertBlock());
			si->setAlignment(4);
			return si;
		}
		else if (global_array){
			CastInst* int64_index = new SExtInst(index, IntegerType::get(TheModule->getContext(), 64), "", Builder.GetInsertBlock());
			ConstantInt* const_0 = ConstantInt::get(TheModule->getContext(), APInt(32, StringRef("0"), 10));
			std::vector<Value*> ptr_indices;
			ptr_indices.push_back(const_0);
			ptr_indices.push_back(int64_index);
			Instruction *ptr=GetElementPtrInst::CreateInBounds(global_array,ptr_indices,"",Builder.GetInsertBlock());
			StoreInst* si = new StoreInst(Val, ptr, false, Builder.GetInsertBlock());
			si->setAlignment(4);
			return si;
		}
		else {
			return ErrorV("Unknown array name");
		}
	}
	else {
		Value *Variable = LocalValues[lval->getName()];
		if (Variable == 0)
			Variable = GlobalValues[lval->getName()];
		if (Variable == 0)
			return ErrorV("Unknown variable name");
		Builder.CreateStore(Val, Variable);
		return Val;
	}
}

Value *LvalNode::Codegen() {
	if(Exp){
		Value *index=Exp->Codegen();
		AllocaInst *array=LocalArray[*Name];
		if(array){
			CastInst* int64_index = new SExtInst(index, IntegerType::get(TheModule->getContext(), 64), "", Builder.GetInsertBlock());
			ConstantInt* const_0 = ConstantInt::get(TheModule->getContext(), APInt(32, StringRef("0"), 10));
			std::vector<Value*> ptr_indices;
			ptr_indices.push_back(const_0);
			ptr_indices.push_back(int64_index);
			Instruction *ptr=GetElementPtrInst::CreateInBounds(array,ptr_indices,"",Builder.GetInsertBlock());
			LoadInst* li = new LoadInst(ptr, "", false, Builder.GetInsertBlock());
			li->setAlignment(4);
			return li;
		}
		GlobalVariable *global_array=GlobalArray[*Name];
		if(global_array){
			CastInst* int64_index = new SExtInst(index, IntegerType::get(TheModule->getContext(), 64), "", Builder.GetInsertBlock());
			ConstantInt* const_0 = ConstantInt::get(TheModule->getContext(), APInt(32, StringRef("0"), 10));
			std::vector<Value*> ptr_indices;
			ptr_indices.push_back(const_0);
			ptr_indices.push_back(int64_index);
			Instruction *ptr=GetElementPtrInst::CreateInBounds(global_array,ptr_indices,"",Builder.GetInsertBlock());
			LoadInst* li = new LoadInst(ptr, "", false, Builder.GetInsertBlock());
			li->setAlignment(4);
			return li;
		}
		return ErrorV("Unknown array name");
	}
	else{
		// Look this variable up in the function.
		Value *V = LocalValues[*Name];
		if (V == 0)
			V=GlobalValues[*Name];
		if(V==0)
			return ErrorV("Unknown variable name");

		// Load the value.
		return Builder.CreateLoad(V, Name->c_str());
	}
}

Value *ExpNode::Codegen() {
	return 0;
}
