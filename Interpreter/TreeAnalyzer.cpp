#include "TreeAnalyzer.h"
#include "Table.h"
#include "VarCont.h"
#include "Error.h"
#include "SyntaxTree.h"
#include "FuncCont.h"
#include "Interpreter.h"

#include <string>
#include <vector>
#include <algorithm>

using TreeAnalysis::TreeAnalyzer;

typedef ParseTree::ISyntaxTreeNode stn;

bool TreeAnalyzer::PrepareFuncFrames()
{
  unsigned sz=f_cont->Size();
  f_cont->FuncPtrReset();
  FuncCont::FuncType* tmp_f;
 
  for (unsigned i=0;i<sz;++i)
    {
      vc_stack.Reset();
      vc_stack.PushVC();

      tmp_f=f_cont->GetCurrFunc();
      intr.SetFunc(tmp_f->GetAddr());

      AddFuncParameters(tmp_f);

      NodeHandler(tmp_f->GetBody());
      vc_stack.PopVC();
      
      intr.AddCmd(CmdTable::END);
      
      intr.SetFuncVarCount(vc_stack.GetMaxStackSize());
      
      if (!vc_stack.IsResultExist())
	err.NoRetValue(tmp_f->GetFuncName(),tmp_f->GetLine());

      f_cont->NextFunc();
    }

  delete f_cont;
  f_cont=nullptr;
  return !err.Count();
}

void TreeAnalyzer::AddFuncParameters(FuncCont::FuncType* fn)
{
  using FuncCont::ParType;
  using VarCont::VarType;

  unsigned argc=fn->GetArgc();
  for (unsigned i=0;i<argc;++i)
    {
      VarCont::apair res;

      switch (fn->GetParType(i))
	{
	case ParType::VAL:
	  {
	    res=vc_stack.AddVar(fn->GetParName(i),VarType::DOUBLE,1);
	    break;
	  }
	case ParType::REF:
	  {
	    res=vc_stack.AddVar(fn->GetParName(i),VarType::REF,1);
	    break;
	  }
	case ParType::ARR:
	  {
	    res=vc_stack.AddVar(fn->GetParName(i),
				VarType::ARR_REF,
				fn->GetParSize(i));
	    break;
	  }
	}
    }

  for (unsigned i=argc;i>0;--i)
    {
      switch (fn->GetParType(i-1))
	{
	case ParType::VAL:
	  {
	    intr.AddCmd(CmdTable::INIT_VAL);
	    break;
	  }
	case ParType::REF:
	  {
	    intr.AddCmd(CmdTable::INIT_REF);
	    break;
	  }
	case ParType::ARR:
	  {
	    intr.AddCmd(CmdTable::INIT_ARR_REF);
	    intr.AddNumeric(fn->GetParSize(i-1));
	    intr.AddService(intr.GetNumericPos());
	    break;
	  }
	}

      intr.AddService(i);
    }
}

TreeAnalyzer::iv TreeAnalyzer::NodeHandler(stn* curr)
{
  if (!curr) return iv(IntVar::NOTHING);
  switch (curr->meaning)
    {
    case SemTable::ST_SEQ:
      StatementSeq(curr);
      return iv(IntVar::NOTHING);

    case SemTable::FUNC:
      return FuncCall(curr);

    case SemTable::CLEAR:
      ClearHandler(curr);
      return iv(IntVar::NOTHING);

    case SemTable::DO:
      DoWhileHandler(curr);
      return iv(IntVar::NOTHING);

    case SemTable::WHILE:
      WhileHandler(curr);
      return iv(IntVar::NOTHING);
      
    case SemTable::ARR_DECL:
      ArrayDeclaration(curr);
      return iv(IntVar::NOTHING);

    case SemTable::ASSIGN:
      return Assign(curr);

    case SemTable::IF:
    case SemTable::TERN:
      return Tern(curr);

    case SemTable::OR:
    case SemTable::AND:
      return Logical(curr);
      
    case SemTable::CMP:
      return Cmp(curr);
      
    case SemTable::PLUS:    
    case SemTable::MUL:
      return MathBasic(curr);
      
    case SemTable::UN_PLUS:
    case SemTable::UN_MINUS:
    case SemTable::NOT:
      return UnarySimple(curr);
      
    case SemTable::INC:
    case SemTable::DEC:
      return IncDec(curr);

    case SemTable::ARR_ACCESS:
      return ArrayAccess(curr);

    case SemTable::NAME:
      return Name(curr);
    case SemTable::NUM:
      return Num(curr);

    case SemTable::INC_P:
    case SemTable::DEC_P:
      return PostIncDec(curr);

    default:
      err.Error("Is it possible?"); exit(1);
    }
}

void TreeAnalyzer::StatementSeq(stn* curr)
{
  vc_stack.PushVC();
  
  for (unsigned i=0;i<curr->Size();++i)
    {
      if (stn* tmp=curr->GetOp(i))
	NodeHandler(tmp);
    }
  
  vc_stack.PopVC();
}

TreeAnalyzer::iv TreeAnalyzer::FuncCall(stn* curr)
{
  FuncCont::FuncType* func_tmp=nullptr;
  if (curr->GetOp()->meaning!=SemTable::NAME)
    {
      err.LineError("Expression cannot be used as function",curr->Line());
      return iv(IntVar::NOTHING);
    }
  // Try to find function
  FuncCont::FuncType prot(*curr->GetOp()->GetName(),0);
  for (unsigned i=0;i<curr->Size();++i)
    {
      iv arg=NodeHandler(curr->GetArg(i));
      IsVarFailed(arg,curr->GetArg(i)->Line());
      if (arg.type==IntVar::BOOL)
	err.DoubleWarn(curr->Line());
      DoubleConv(arg,curr->Line());

      switch (arg.type)
	{
	case IntVar::VAR:
	case IntVar::REF:
	  prot.AddPar("",FuncCont::REF);
	  break;
	case IntVar::ARR:
	  prot.AddPar("",FuncCont::ARR,arg.size);
	  break;
	case IntVar::NOTHING:
	  return arg;
	default:
	  prot.AddPar("",FuncCont::VAL);
	  break;
	}
    }
  func_tmp=f_cont->Find(&prot);

  if (!func_tmp)
    {
      err.Undefined(*curr->GetOp()->GetName(),
	  curr->Size(),curr->Line());
      return iv(IntVar::NOTHING);
    }
  intr.AddCmd(CmdTable::CALL);
  intr.AddService(func_tmp->GetAddr());

  // Type check
  bool is_success=true;
  for (unsigned i=0;i<curr->Size();++i)
    {    
      switch (func_tmp->GetParType(i))
	{
	case FuncCont::REF:
	  {
	    if (prot.GetParType(i)!=FuncCont::REF)
	      {
		err.LineError
		  ("Expected lvalue as function argument",curr->Line());
		is_success=false;
	      }
	    break;
	  }
	case FuncCont::ARR:
	  {
	    if (func_tmp->GetParSize(i) && prot.GetParSize(i) &&
		func_tmp->GetParSize(i)!=prot.GetParSize(i))
	      {
		err.LineError("Array size checking failed",curr->Line());
		is_success=false;
	      }
	    break;
	  }
	default:
	  break;
	}
    }
  if (is_success)
    return iv(IntVar::TMP);
  return iv(IntVar::NOTHING);
}

void TreeAnalyzer::ClearHandler(stn* curr)
{
  stn* tmp=curr->GetOp(1);
  iv res=NodeHandler(tmp);
  intr.AddCmd(CmdTable::POP);
  switch (tmp->meaning)
    {
    case SemTable::ASSIGN:
    case SemTable::INC:
    case SemTable::DEC:
    case SemTable::TERN:
      break;
    case SemTable::NAME:
      if (IsVarFailed(res,tmp->Line()))
	break;
    default:
      err.Warn("Unused result",tmp->Line());
    }
}

void TreeAnalyzer::WhileHandler(stn* curr)
{
  vc_stack.PushVC();

  unsigned cond_begin=intr.GetCmdPos();
  unsigned serv_begin=intr.GetServicePos();

  stn* cnode=curr->GetOp(1);
  iv cond_var=NodeHandler(cnode); 
  if (!IsVarFailed(cond_var,curr->Line()) &&
      cond_var.type!=IntVar::BOOL)
    err.BoolWarn(cnode->Line());

  intr.AddCmd(CmdTable::IFFALSE_GOTO);
  unsigned serv_loop=intr.GetServicePos();
  intr.AddService(0);
  intr.AddService(0);
  
  NodeHandler(curr->GetOp(2));

  intr.AddCmd(CmdTable::GOTO);
  intr.AddService(cond_begin);
  intr.AddService(serv_begin);

  intr.SetService(serv_loop++,intr.GetCmdPos());
  intr.SetService(serv_loop,intr.GetServicePos());

  vc_stack.PopVC();
}

void TreeAnalyzer::DoWhileHandler(stn* curr)
{
  vc_stack.PushVC();

  unsigned loop_begin=intr.GetCmdPos();
  unsigned serv_begin=intr.GetServicePos();

  NodeHandler(curr->GetOp(2));	// Body
    
  stn* cnode=curr->GetOp(1);
  iv cond_var=NodeHandler(cnode); // Condition

  if (!IsVarFailed(cond_var,curr->Line()) && 
      cond_var.type!=IntVar::BOOL)
    err.BoolWarn(cnode->Line());

  intr.AddCmd(CmdTable::IFTRUE_GOTO);
  intr.AddService(loop_begin);
  intr.AddService(serv_begin);

  vc_stack.PopVC();
}

void TreeAnalyzer::ArrayDeclaration(stn* curr)
{
  unsigned arr_size;
  if (curr->GetOp(1))
    arr_size=curr->GetOp(1)->GetVal();
  else arr_size=curr->GetOp(2)->Size();
  auto ins=vc_stack.AddVar(*curr->GetName(),VarCont::ARRAY,arr_size);
  if (!ins.first)
    {
      if (ins.second->GetType()==VarCont::ARRAY)
	{
	  err.VarError("Redefinition of array",*curr->GetName(),curr->Line());
	  return;
	}
      err.VarError("Redefinition of",*curr->GetName(),curr->Line());
      return;
    }
  intr.AddCmd(CmdTable::PUSH_CONST);
  intr.AddNumeric(arr_size);
  intr.AddService(intr.GetNumericPos());
  intr.AddCmd(CmdTable::INIT_VAL);
  intr.AddService(ins.second->GetStackPos());

  ArrayInitialization(curr->GetOp(2),ins.second->GetStackPos()+1);
}

void TreeAnalyzer::ArrayInitialization(stn* curr,unsigned addr)
{
  if (!curr)
    return;

  for (unsigned i=0;i<curr->Size();++i)
    {
      iv arg=NodeHandler(curr->GetArg(i));
      IsVarFailed(arg,curr->Line());
      IsVal(arg,curr->Line());
      
      intr.AddCmd(CmdTable::INIT_VAL);
      intr.AddService(addr+i);
    }
}

TreeAnalyzer::iv TreeAnalyzer::Assign(stn* curr)
{
  iv lhs=NodeHandler(curr->GetOp(1));
  if (lhs.type==IntVar::BOOL || lhs.type==IntVar::TMP)
    {
      err.LineError("Lvalue required as left operand of assignment",
		    curr->Line());
      return iv(IntVar::NOTHING);
    }
  if (lhs.type==IntVar::NOTHING)
    return iv(IntVar::NOTHING);

  if (!IsVal(lhs,curr->Line()))
      return iv(IntVar::NOTHING);

  if (lhs.name)
    {
      auto res=vc_stack.AddVar(lhs.name->name,VarCont::DOUBLE,1);
      if (res.first) 
	{
	  intr.AddCmd(CmdTable::PUSH_VAR);
	  intr.AddService(res.second->GetStackPos());
	}
    }

  iv rhs=NodeHandler(curr->GetOp(2));
  if (IsVarFailed(rhs,curr->Line()))
    return iv(IntVar::NOTHING);

  DoubleConv(rhs,curr->Line());
  
  intr.AddCmd(CmdTable::ASSIGN);
  return lhs;
}

TreeAnalyzer::iv TreeAnalyzer::Tern(stn* curr)
{
  if (curr->meaning==SemTable::IF)
    vc_stack.PushVC();

  iv cond_var=NodeHandler(curr->GetOp(1));
  if (IsVarFailed(cond_var,curr->Line()) || 
      !IsVal(cond_var,curr->Line()))
    return iv(IntVar::NOTHING);
  BoolConv(cond_var,curr->Line());

  intr.AddCmd(CmdTable::IFFALSE_GOTO);
  unsigned first_begin=intr.GetServicePos();
  intr.AddService(0);
  intr.AddService(0);
  
  iv br_true=NodeHandler(curr->GetOp(2));
  if (curr->meaning==SemTable::IF && !curr->GetOp(3))
    { 
      intr.SetService(first_begin++,intr.GetCmdPos());
      intr.SetService(first_begin,intr.GetServicePos());
      vc_stack.PopVC();
      return iv(IntVar::NOTHING);
    }

  intr.AddCmd(CmdTable::GOTO);
  unsigned first_end=intr.GetServicePos();
  intr.AddService(0);
  intr.AddService(0);
  
  intr.SetService(first_begin++,intr.GetCmdPos());
  intr.SetService(first_begin,intr.GetServicePos());

  iv br_false=NodeHandler(curr->GetOp(3));
  intr.SetService(first_end++,intr.GetCmdPos());
  intr.SetService(first_end,intr.GetServicePos());

  if (curr->meaning==SemTable::IF)
    {
      vc_stack.PopVC();
      return iv(IntVar::NOTHING);
    }
  
  bool is_corr=!IsVarFailed(br_true,curr->Line());
  if (is_corr)
    DoubleConv(br_true,curr->Line());

  is_corr=!IsVarFailed(br_false,curr->Line());
  if (is_corr)
    DoubleConv(br_false,curr->Line());

  if (is_corr)
    {
      if ((br_true.type==IntVar::VAR || br_true.type==IntVar::REF) &&
	  (br_false.type==IntVar::VAR || br_false.type==IntVar::REF))
	return iv(IntVar::VAR);

      if (br_true.type==IntVar::ARR && br_false.type==IntVar::ARR)
	return iv(IntVar::ARR);

      if ((br_true.type==IntVar::ARR && br_false.type!=IntVar::ARR) ||
	  (br_false.type==IntVar::ARR && br_true.type!=IntVar::ARR))
	{
	  err.LineError("Array name cannot be used as value",curr->Line());
	  return iv(IntVar::NOTHING);
	}      

      return iv(IntVar::TMP);
    }
  return iv(IntVar::NOTHING);
}

TreeAnalyzer::iv TreeAnalyzer::Logical(stn* curr)
{
  iv tmp=NodeHandler(curr->GetOp(0));
  if (IsVarFailed(tmp,curr->Line()) || 
      !IsVal(tmp,curr->Line()))
    return iv(IntVar::NOTHING);
  BoolConv(tmp,curr->Line());

  std::vector<unsigned> lazy_goto;
  lazy_goto.resize(curr->Size()-1);

  for (unsigned i=1; i<curr->Size(); ++i)
    {
      if (curr->meaning==SemTable::OR)
	intr.AddCmd(CmdTable::OR_GOTO);
      else intr.AddCmd(CmdTable::AND_GOTO);
      lazy_goto[i-1]=intr.GetServicePos();
      intr.AddService(0);
      intr.AddService(0);

      iv rhs=NodeHandler(curr->GetOp(i));
      if (!IsVarFailed(rhs,curr->Line()) ||
	  !IsVal(rhs,curr->Line()))
	BoolConv(rhs,curr->Line());

      if (curr->meaning==SemTable::OR)
	intr.AddCmd(CmdTable::OR);
      else intr.AddCmd(CmdTable::AND);
    }

  for (unsigned i=0;i<lazy_goto.size();++i)
    {
      intr.SetService(lazy_goto[i]++,intr.GetCmdPos());
      intr.SetService(lazy_goto[i],intr.GetServicePos());
    }

  return iv(IntVar::BOOL);
}

TreeAnalyzer::iv TreeAnalyzer::Cmp(stn* curr)
{
  iv tmp=NodeHandler(curr->GetOp(0));
  if (IsVarFailed(tmp,curr->Line()) || 
      !IsVal(tmp,curr->Line()))
    return iv(IntVar::NOTHING);
  DoubleConv(tmp,curr->Line());

  for (unsigned i=1;i<curr->Size();++i)
    {
      iv rhs=NodeHandler(curr->GetOp(i));
      if (IsVarFailed(rhs,curr->Line()) ||
	  !IsVal(rhs,curr->Line()))
	return iv(IntVar::NOTHING);
      else 
	{
	  DoubleConv(rhs,curr->Line());
	  
	  CmdTable::Cmd c;
	  switch (curr->GetType(i-1))
	    {
	    case SemTable::LESS:
	      c=CmdTable::LESS;
	      break;
	    case SemTable::LESS_EQ:
	      c=CmdTable::LESS_EQ;
	      break;
	    case SemTable::GR:
	      c=CmdTable::GR;
	      break;
	    case SemTable::GR_EQ:
	      c=CmdTable::GR_EQ;
	      break;
	    case SemTable::EQUAL:
	      c=CmdTable::EQUAL;
	      break;
	    case SemTable::NOT_EQUAL:
	      c=CmdTable::NOT_EQUAL;
	      break;
	    default:
	      err.Error("Unexpected type in Cmp()");
	      exit(1);
	    }
	  intr.AddCmd(c);	   
	}      
    }
  
  return iv(IntVar::BOOL);
}

TreeAnalyzer::iv TreeAnalyzer::MathBasic(stn* curr)
{
  iv tmp=NodeHandler(curr->GetOp(0));
  if (IsVarFailed(tmp,curr->Line()) ||
      !IsVal(tmp,curr->Line()))
    return iv(IntVar::NOTHING);
  DoubleConv(tmp,curr->Line());

  for (unsigned i=1;i<curr->Size();++i)
    {
      iv rhs=NodeHandler(curr->GetOp(i));
      if (IsVarFailed(rhs,curr->Line()) ||
	  !IsVal(rhs,curr->Line()))
	return iv(IntVar::NOTHING);
      else 
	{
	  DoubleConv(rhs,curr->Line());

	  CmdTable::Cmd c;
	  switch (curr->GetType(i-1))
	    {
	    case SemTable::PLUS:
	      c=CmdTable::PLUS;
	      break;
	    case SemTable::MINUS:
	      c=CmdTable::MINUS;
	      break;
	    case SemTable::MUL:
	      c=CmdTable::MUL;
	      break;
	    case SemTable::DIV:
	      c=CmdTable::DIV;
	      intr.AddService(curr->Line());
	      break;
	    default:
	      err.Error("Unexpected type in MathBasic()");
	      exit(1);
	    }
	  intr.AddCmd(c);
	  
	  if (curr->GetType(i-1)==SemTable::DIV && rhs.value==0)
	    err.LineError("Division by zero",curr->Line());		
	}      
    }
  return iv(IntVar::TMP);
}

TreeAnalyzer::iv TreeAnalyzer::UnarySimple(stn* curr)
{
  iv op=NodeHandler(curr->GetOp());
  if (IsVarFailed(op,curr->Line()) ||
      !IsVal(op,curr->Line()))
    return iv(IntVar::NOTHING);
  
  switch (curr->meaning)
    {
    case SemTable::NOT:
      {
	intr.AddCmd(CmdTable::NOT);
	BoolConv(op,curr->Line());
	return iv(IntVar::BOOL);
      }
    case SemTable::UN_MINUS:
      {
	intr.AddCmd(CmdTable::U_MINUS);
	DoubleConv(op,curr->Line());
	return iv(IntVar::TMP);
      }
    case SemTable::UN_PLUS:
      {
	intr.AddCmd(CmdTable::U_PLUS);
	DoubleConv(op,curr->Line());
	return iv(IntVar::TMP);
      }
    default:
      err.Error("Unexpected type in UnarySimple()");
      exit(1);
    }
}

TreeAnalyzer::iv TreeAnalyzer::IncDec(stn* curr)
{
  iv op=NodeHandler(curr->GetOp());

  if (IsVarFailed(op,curr->Line()) ||
      !IsVal(op,curr->Line()))  
    return iv(IntVar::NOTHING);

  if (op.type==IntVar::BOOL || op.type==IntVar::TMP)
    {
      if (curr->meaning==SemTable::INC)
	err.LineError("Increment requires lvalue as argument",curr->Line());
      else 
	err.LineError("Decrement requires lvalue as argument",curr->Line());
      return iv(IntVar::NOTHING);
    }
  
  if (curr->meaning==SemTable::INC)
    intr.AddCmd(CmdTable::INC);
  else
    intr.AddCmd(CmdTable::DEC);

  return op;
}

TreeAnalyzer::iv TreeAnalyzer::ArrayAccess(stn* curr)
{
  iv expr=NodeHandler(curr->GetArg(0));
  if (!IsVal(expr,curr->Line()))
      return iv(IntVar::NOTHING);

  iv arr=NodeHandler(curr->GetOp());
  if (IsVarFailed(arr,curr->Line()))
      return iv(IntVar::NOTHING);

  if (arr.type!=IntVar::ARR)
    {
      err.LineError("Variable cannot be used as array",curr->Line());
      return iv(IntVar::NOTHING);
    }

  intr.AddCmd(CmdTable::ARR_ACCESS);
  arr.type=IntVar::VAR;
  return arr;
}

TreeAnalyzer::iv TreeAnalyzer::Name(stn* curr)
{
  VarCont::apair p=vc_stack.GetVar(*curr->GetName());
  IntVar::IntVarType t;
  if (!p.first)
    t=IntVar::VAR;
  else
    {
      switch (p.second->GetType())
	{
	case VarCont::DOUBLE:
	  {
	    intr.AddCmd(CmdTable::PUSH_VAR);
	    t=IntVar::VAR;
	    break;
	  }
	case VarCont::REF:
	  {
	    intr.AddCmd(CmdTable::PUSH_REF);
	    t=IntVar::REF;
	    break;
	  }
	case VarCont::ARRAY:
	  {
	    intr.AddCmd(CmdTable::PUSH_VAR);
	    t=IntVar::ARR;
	    break;
	  }
	case VarCont::ARR_REF:
	  {
	    intr.AddCmd(CmdTable::PUSH_REF);
	    t=IntVar::ARR;
	    break;
	  }
	default:
	  err.Error("Unexpected type in Name()");
	  exit(1);
	}
      intr.AddService(p.second->GetStackPos());
    }

  iv res(t);
  res.size=p.first ? p.second->GetSize() : 1;
  res.name=new iv::Buf(*curr->GetName()); 
  return res;
}

TreeAnalyzer::iv TreeAnalyzer::Num(stn* curr)
{
  iv res(IntVar::TMP);
  res.value=curr->GetVal();
  intr.AddCmd(CmdTable::PUSH_CONST);
  intr.AddNumeric(res.value);
  intr.AddService(intr.GetNumericPos());
  return res;
}

TreeAnalyzer::iv TreeAnalyzer::PostIncDec(stn* curr)
{
  iv op=NodeHandler(curr->GetOp());
  if (IsVarFailed(op,curr->Line()) ||
      !IsVal(op,curr->Line()))
    return iv(IntVar::NOTHING);
  if (op.type==IntVar::BOOL || op.type==IntVar::TMP)
    {
      if (curr->meaning==SemTable::INC_P)
	err.LineError("Increment requires lvalue as argument",curr->Line());
      else 
	err.LineError("Decrement requires lvalue as argument",curr->Line());
      return iv(IntVar::NOTHING);
    }

  if (curr->meaning==SemTable::INC_P)
    intr.AddCmd(CmdTable::INC_P);
  else intr.AddCmd(CmdTable::DEC_P);
  return iv(IntVar::TMP);
}

bool TreeAnalyzer::CheckVar(const iv& var,unsigned line)
{
  if (!var.name)
    return false;
  VarCont::apair p=vc_stack.GetVar(var.name->name);
  if (p.first)
    return true;
  err.VarError("Unknown variable",var.name->name,line);
  return false;
}

bool TreeAnalyzer::IsVarFailed(const iv& var,unsigned line)
{
  return var.type==IntVar::NOTHING || 
    (var.name && !CheckVar(var,line));
}

void TreeAnalyzer::BoolConv(const iv& var,unsigned ln)
{
  if (var.type!=IntVar::BOOL)
    err.BoolWarn(ln);
}

void TreeAnalyzer::DoubleConv(const iv& var,unsigned ln)
{
  if (var.type==IntVar::BOOL)
    err.DoubleWarn(ln);
}

bool TreeAnalyzer::IsVal(const iv& var,unsigned ln)
{
  if (var.type==IntVar::ARR)
    {
      err.LineError("Array name cannot be used as value",ln);
      return false;
    }
  return true;
}
