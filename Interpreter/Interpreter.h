#ifndef __CALC_INTERPRETER_H__
#define __CALC_INTERPRETER_H__

#include <vector>
#include "FuncCont.h"
#include "Table.h"
#include "Error.h"
#include <cassert>

namespace Interpreting
{

class FuncFrame
{
  friend class Interpreter;
  
  typedef CmdTable::Cmd cmd;
  typedef union {
    double val;
    double* ref;
  } local_var;
  
  unsigned ret_addr;

  local_var* var;

  const cmd* body;
  const unsigned* service;
  const double* numeric;

  unsigned curr_service;

public:
  FuncFrame(unsigned addr,unsigned loc_sz,const cmd* bd,
	    const unsigned* srv,const double* num):
    ret_addr(addr),var(new local_var[loc_sz]),
    body(bd),service(srv),numeric(num),curr_service(0){}

  ~FuncFrame(){delete[] var;}

  cmd GetCommand(unsigned i) const {return body[i];}

  unsigned GetService() {return service[curr_service++];}
  double GetConst(unsigned i) const {return numeric[i];}

  void SetServicePos() {curr_service=service[curr_service];}

  unsigned GetRetAddr() const {return ret_addr;}

  FuncFrame(const FuncFrame&) = delete;
  FuncFrame& operator=(const FuncFrame&) = delete;
};

class CallStackType
{
  typedef CmdTable::Cmd cmd;
  
  struct CallStackNode
  {
    CallStackNode* prev;
    FuncFrame fn;

    CallStackNode(CallStackNode* pr,unsigned addr,
		  unsigned loc_sz,const cmd* bd,
		  const unsigned* srv,const double* num):
      prev(pr),fn(addr,loc_sz,bd,srv,num){}
  };

  typedef CallStackNode csn;

  const long unsigned max_size;

  csn* top;
  long unsigned size;
public:
  CallStackType(long unsigned sz):max_size(sz),top(nullptr),size(0){}
  ~CallStackType();

  bool Push(unsigned addr,unsigned loc_sz,const cmd* body,
	    const unsigned* service,const double* numeric)
  {
    if (size==max_size)
      return false;

    top=new csn(top,addr,loc_sz,body,service,numeric);
    ++size;
    return true;
  }

  void Pop()
  {
    assert(top);
    csn* tmp=top;
    top=top->prev;
    delete tmp;
    --size;
  }

  FuncFrame& Top(){assert(top);return top->fn;}
  bool IsEmpty() const {return !top;}

  CallStackType(const CallStackType&) = delete;
  CallStackType& operator=(const CallStackType&) = delete;
};

class TmpStackType
{
public:
  enum FrameType {RVAL,LVAL};
  struct TmpStackFrame
  {
    FrameType type;
    union
    {    
      double val;
      double* addr;
    };
  };
  
private:
  struct TmpStackNode
  {
    TmpStackNode* prev;
    TmpStackNode* next;
    TmpStackFrame* rep;
    
    TmpStackNode():
      prev(nullptr),next(nullptr),rep(new TmpStackFrame[node_size]){}
    
    TmpStackNode(TmpStackNode* pr):
      prev(pr),next(nullptr),rep(new TmpStackFrame[node_size]){}

    TmpStackFrame& operator[](unsigned i){return rep[i];}

    ~TmpStackNode(){delete[] rep;}
  };

  static const unsigned node_size=100;

  TmpStackNode* curr_node;
  unsigned top;

public:
  TmpStackType():
    curr_node(new TmpStackNode),top(0){}

  // false if full
  bool PushLVal(double* addr)
  {
    if (top==node_size)
      NextNode();
    (*curr_node)[top].type=LVAL;
    (*curr_node)[top++].addr=addr;
    return true;
  }
  bool PushRVal(double val)
  {
    if (top==node_size)
      NextNode();
    (*curr_node)[top].type=RVAL;
    (*curr_node)[top++].val=val;
    return true;
  }

  TmpStackFrame& Top()
  {
    if (top==0)
      PrevNode();
    return (*curr_node)[top-1];
  }

  TmpStackFrame GetVal()
  {
    if (top==0)
	PrevNode();
    return (*curr_node)[--top];
  }

  void Pop()
  {
    if (top==0)
      PrevNode();
    --top;
  }

  ~TmpStackType();

  TmpStackType(const TmpStackType&) = delete;
  TmpStackType& operator=(const TmpStackType&) = delete;
private:
  void GetMoreSpace()
  {
    curr_node->next=new TmpStackNode(curr_node);
  }

  void NextNode()
  {
    if (curr_node->next==nullptr)
      GetMoreSpace();
    curr_node=curr_node->next;
    top=0;
  }

  void PrevNode()
  {
    assert(curr_node->prev);
    curr_node=curr_node->prev;
    top=node_size;
  }
};

class Interpreter
{
  typedef CmdTable::Cmd cmd;

  typedef struct FuncFrameTempl
  {
    unsigned var_count;
    std::vector<cmd> body;
    std::vector<unsigned> service;
    std::vector<double> numeric;
    
    FuncFrameTempl():var_count(0){}
    
    void Init(unsigned bsize)
    {
      body.resize(bsize);
    }
    ~FuncFrameTempl(){}    
  } ff_templ;

  std::vector<ff_templ> funcs;
  CallStackType call_stack;
  TmpStackType tmp_stack;
  IntrError::ErrHandler& err;

  FuncFrame* top;

  unsigned curr_func;
  unsigned curr_pos;
  bool is_normal;
  double result;
public:
  Interpreter(FuncCont::FuncContainer& fn_cont,
	      IntrError::ErrHandler& r_err,long unsigned);
  
  void AddCmd(cmd c){funcs[curr_func].body[curr_pos++]=c;}
  void AddService(unsigned srv){funcs[curr_func].service.push_back(srv);}
  void AddNumeric(double num){funcs[curr_func].numeric.push_back(num);}
  void SetService(unsigned i,unsigned srv)
  {
    assert(i<funcs[curr_func].service.size());
    funcs[curr_func].service[i]=srv;
  }

  void SetFunc(unsigned i){curr_func=i;curr_pos=0;}
  void SetFuncVarCount(unsigned cnt){funcs[curr_func].var_count=cnt;}

  unsigned GetCmdPos() const {return curr_pos;}
  unsigned GetServicePos() const {return funcs[curr_func].service.size();}
  unsigned GetNumericPos() const {return funcs[curr_func].numeric.size()-1;}

  bool Calculate();

  double GetResult() const {return result;}
private:

  void Handle(cmd);
  void HandleBasicOp(cmd);

  double ToRVal(TmpStackType::TmpStackFrame tmp)
  {
    if (tmp.type==TmpStackType::RVAL)
      return tmp.val;
    return *tmp.addr;
  }

  void PushFunc(unsigned addr)
  {
    ff_templ& fn=funcs[addr];
    is_normal=call_stack.Push(curr_pos,
			      fn.var_count,
			      fn.body.data(),
			      fn.service.data(),
			      fn.numeric.data());
  }

  void GotoSet()
  {   
    curr_pos=top->GetService();
    top->SetServicePos();
  }
  void GotoSkip()
  {
    top->GetService();
    top->GetService();
  }
};

} // namespace Interpreting

#endif
