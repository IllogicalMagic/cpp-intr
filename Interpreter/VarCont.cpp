#include "VarCont.h"
#include <string>
#include <map>
#include <utility>
#include <cassert>

using namespace VarCont;

apair VarContainer::Insert(const std::string& name,VarType t,unsigned sz)
{
  // We do not need to replace old variable
  auto res=rep.emplace
    (std::make_pair(name,Var(t,curr_stack_pos,sz) ));
  auto res_apair=std::make_pair(res.second, &(res.first->second) );

  if (res.second)
    {
      if (t==ARRAY)
	curr_stack_pos+=(sz+1);
      else ++curr_stack_pos;
    }
  return res_apair;
}

const Var* VarContainer::Find(const std::string& name) const
{
  auto curr=rep.find(name);
  if (curr==rep.end()) return nullptr;
  return &(curr->second);
}

VarContStack::~VarContStack()
{
  VNode* tmp=top;
  while (top!=nullptr)
    {
      top=top->prev;
      delete tmp;
      tmp=top;
    }
  delete result;
}

void VarContStack::PushVC()
{
  // if stack is empty
  unsigned tmp=1;

  if (top)
    tmp=top->vc.GetStackPos();
  top=new VNode(top,tmp);
}

void VarContStack::PopVC()
{
  if (top->vc.GetStackPos() > max_stack_size)
    max_stack_size=top->vc.GetStackPos();

  VNode* tmp=top;
  assert(top);
  top=top->prev;
  delete tmp;
}

apair VarContStack::AddVar(const std::string& v_name,VarType t,unsigned size)
{
  if (v_name=="result")
    {
      if (t==DOUBLE)
	{
	  bool is_added=!result;
	  Var* res=result ? result : (result=new Var(DOUBLE,0,1)) ;
	  return std::make_pair(is_added,res);
	}
      else 
	return std::make_pair(false,nullptr);
    }

  const Var* res=Find(v_name);
  if (res)
    return apair(false,res);
  return top->vc.Insert(v_name,t,size);
}

apair 
VarContStack::GetVar(const std::string& v_name) const
{  
  if (v_name=="result")
    // Without explicit cast it looks strange - apair(result,result)
    return apair(static_cast<bool>(result),result);

  const Var* res=Find(v_name);
  return apair(static_cast<bool>(res),res);
}

void VarContStack::Reset() 
{
  max_stack_size=1;
  delete result;
  result=nullptr;
}

const Var* VarContStack::Find(const std::string& v_name) const
{
  VNode* curr=top;
  const Var* res=nullptr;
  while (curr && !(res=curr->vc.Find(v_name)))
    curr=curr->prev;
  return res;
}
