#include "Interpreter.h"
#include "Table.h"
#include "Error.h"
#include <cstdio>
#include <cassert>

using Interpreting::Interpreter;
using Interpreting::CallStackType;
using Interpreting::TmpStackType;
using Interpreting::FuncFrame;

typedef CmdTable::Cmd cmd;
typedef Interpreting::TmpStackType::TmpStackFrame tmp_frame;

CallStackType::~CallStackType()
{
  csn* tmp=top;
  while (tmp)
    {
      tmp=top->prev;
      delete top;
      top=tmp;
    }
}

TmpStackType::~TmpStackType()
{
  while (curr_node->next)
    curr_node=curr_node->next;
  while (curr_node->prev)
    {
      curr_node=curr_node->prev;
      delete curr_node->next;
    }
  delete curr_node;
}

Interpreter::Interpreter(FuncCont::FuncContainer& fn_cont,
			 IntrError::ErrHandler& r_err,long unsigned sz):
  funcs(fn_cont.Size()),call_stack(sz),tmp_stack(),err(r_err),top(nullptr),
  curr_func(0),curr_pos(0),is_normal(true),result(0.0)
{
  fn_cont.FuncPtrReset();
  funcs.resize(fn_cont.Size());
  for (unsigned i=0;i<funcs.size();++i)
    {
      FuncCont::FuncType* f=fn_cont.GetCurrFunc();
      unsigned curr=f->GetAddr();
      funcs[curr].Init(f->GetSize());
      fn_cont.NextFunc();
    }
}

bool Interpreter::Calculate()
{
  PushFunc(0);

  curr_pos=0;
  
  while (!call_stack.IsEmpty() && is_normal)
    {
      top=&call_stack.Top();

      cmd curr=top->GetCommand(curr_pos);
      while (curr!=CmdTable::END && is_normal)
	{
	  Handle(curr);
	  curr=top->GetCommand(curr_pos);
	}

      tmp_stack.PushRVal(top->var[0].val);
      curr_pos=top->ret_addr;
      call_stack.Pop();
    }
  
  if (is_normal)
    result=tmp_stack.GetVal().val;
  return is_normal;
}

void Interpreter::Handle(cmd command)
{
  switch (command)
    {
    case CmdTable::POP:
      {
	tmp_stack.Pop();
	++curr_pos;
	return;
      }

    case CmdTable::GOTO:
      {
	GotoSet();
	break;
      }

    case CmdTable::IFTRUE_GOTO:
      {
	if (tmp_stack.GetVal().val)
	  GotoSet();
	else 
	  {
	    GotoSkip();
	    ++curr_pos;
	  }
	break;
      }

    case CmdTable::IFFALSE_GOTO:
      {
	if (tmp_stack.GetVal().val)
	  {
	    GotoSkip();
	    ++curr_pos;
	  }
	else
	  GotoSet();
	break;
      }

    case CmdTable::OR_GOTO:
      {
	if (tmp_stack.Top().val)
	  GotoSet();
	else 
	  {
	    GotoSkip();
	    ++curr_pos;
	  }
	break;
      }

    case CmdTable::AND_GOTO:
      {
	if (tmp_stack.Top().val)
	  {
	    GotoSkip();
	    ++curr_pos;
	  }
	else
	  GotoSet();
	break;
      }

    case CmdTable::PUSH_CONST:
      {
	unsigned i=top->GetService();
	is_normal=tmp_stack.PushRVal(top->numeric[i]);
	++curr_pos;
	break;
      }
    case CmdTable::PUSH_VAR:
      {
	double* tmp=reinterpret_cast<double*>(top->var + top->GetService());
	is_normal=tmp_stack.PushLVal(tmp);
	++curr_pos;
	break;
      }
    case CmdTable::PUSH_REF:
      {
	double* tmp=top->var[top->GetService()].ref;
	is_normal=tmp_stack.PushLVal(tmp);
	++curr_pos;
	break;
      }

    case CmdTable::INIT_VAL:
      {
	double rhs=ToRVal(tmp_stack.GetVal());
	top->var[top->GetService()].val=rhs;
	++curr_pos;
	return;
      }
    case CmdTable::INIT_REF:
      {
	double* rhs=tmp_stack.GetVal().addr;
	top->var[top->GetService()].ref=rhs;
	++curr_pos;
	return;
      }
    case CmdTable::INIT_ARR_REF:
      {
	double* rhs=tmp_stack.GetVal().addr;
	double sz=top->numeric[top->GetService()];
	if (sz && sz!=*rhs)
	  {
	    is_normal=false;
	    err.Error("Array length checking failed");
	    return;
	  }
	unsigned what=top->GetService();
	top->var[what].ref=rhs;
	++curr_pos;
	return;
      }

    case CmdTable::CALL:
      {
	++curr_pos;
	PushFunc(top->GetService());
	if (!is_normal)
	  {
	    err.Error("Recursion depth exceeded");
	    return;
	  }

	curr_pos=0;
	top=&call_stack.Top();
	return;
      }

    default:
      {
	HandleBasicOp(command);
	if (!is_normal)
	  return;
	++curr_pos;
      }
      break;
    }
  if (!is_normal)
    err.Error("Tmp_var stack overflow");
}

void Interpreter::HandleBasicOp(cmd command)
{

  tmp_frame tmp=tmp_stack.GetVal();
  double rhs=ToRVal(tmp);

  switch (command)
    {
    case CmdTable::ASSIGN:
      {
	*(tmp_stack.Top().addr)=rhs;
	return;
      }

    case CmdTable::INC:
      {
	*tmp.addr+=1.0;
	tmp_stack.PushLVal(tmp.addr);
	return;
      }
    case CmdTable::DEC:
      {
	*tmp.addr-=1.0;
	tmp_stack.PushLVal(tmp.addr);
	return;
      }

    case CmdTable::U_PLUS:
      break;
    case CmdTable::U_MINUS:
      rhs=-rhs;
      break;
    case CmdTable::NOT:
      rhs=!rhs;
      break;

    case CmdTable::PLUS:
      rhs+=ToRVal(tmp_stack.GetVal());
      break;
    case CmdTable::MINUS:
      rhs=ToRVal(tmp_stack.GetVal())-rhs;
      break;

    case CmdTable::MUL:
      rhs=ToRVal(tmp_stack.GetVal())*rhs;
      break;
    case CmdTable::DIV:
      {
	unsigned ln=top->GetService();
	if (rhs==0.0)
	  {
	    err.LineError("Division by zero",ln);
	    is_normal=false;
	    return;
	  }
	rhs=ToRVal(tmp_stack.GetVal())/rhs;
	break;
      }

    case CmdTable::LESS:
      rhs=ToRVal(tmp_stack.GetVal())<rhs;
      break;
    case CmdTable::LESS_EQ:
      rhs=ToRVal(tmp_stack.GetVal())<=rhs;
      break;
    case CmdTable::GR:
      rhs=ToRVal(tmp_stack.GetVal())>rhs;
      break;
    case CmdTable::GR_EQ:
      rhs=ToRVal(tmp_stack.GetVal())>=rhs;
      break;
    case CmdTable::EQUAL:
      rhs=ToRVal(tmp_stack.GetVal())==rhs;
      break;
    case CmdTable::NOT_EQUAL:
      rhs=ToRVal(tmp_stack.GetVal())!=rhs;
      break;

    case CmdTable::OR:
      rhs=ToRVal(tmp_stack.GetVal()) || rhs;
      break;
    case CmdTable::AND:
      rhs=ToRVal(tmp_stack.GetVal()) && rhs;
      break;

    case CmdTable::INC_P:
      *tmp.addr+=1.0;
      break;
    case CmdTable::DEC_P:
      *tmp.addr-=1.0;
      break;
    case CmdTable::ARR_ACCESS:
      {
	double i=ToRVal(tmp_stack.GetVal());
	if (i<0 || i>=rhs)
	  {
	    is_normal=false;
	    err.Error("Out of range");
	    return;
	  }
	tmp_stack.PushLVal(tmp.addr+static_cast<unsigned>(i)+1);
	return;
      }

    default:
      assert(0);
      return;      
    }
  tmp_stack.PushRVal(rhs);
}
