#include "FuncCont.h"
#include <string>
#include <vector>
#include "SyntaxTree.h"
#include <set>
#include <algorithm>

using FuncCont::FuncType;
using FuncCont::FuncContainer;
using FuncCont::Parameter;

bool FuncType::operator<(const FuncType& rhs) const
{
  if (name==rhs.name)
    {
      if (par.size()==rhs.par.size())
	{
	  return std::lexicographical_compare
	    (par.begin(),par.end(),rhs.par.begin(),rhs.par.end());
	}
      return par.size()<rhs.par.size();
    }
  return name<rhs.name;
}

FuncType* FuncContainer::Find(FuncType* func) const
{
  auto res=rep.find(func);
  if (res==rep.end()) return nullptr;
  return *res;
}

void FuncContainer::Insert(FuncType* func)
{
  assert(func);
  func->addr=curr_addr;
  ++curr_addr;
  rep.insert(func);
}

void FuncContainer::InsertMain(ft::stn* body,unsigned sz)
{
  ft* tmp=new ft(std::string(),1);
  tmp->SetBody(body);
  tmp->size=sz;
  rep.insert(tmp);
}
