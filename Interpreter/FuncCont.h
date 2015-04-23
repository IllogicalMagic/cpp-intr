#ifndef __FUNCTION_CONTAINER_H__
#define __FUNCTION_CONTAINER_H__

#include <string>
#include <vector>
#include <algorithm>
#include "SyntaxTree.h"
#include <cassert>
#include <set>

namespace FuncCont
{

enum ParType {VAL,REF,ARR};

class Parameter
{
  friend class FuncType;

  std::string name;
  ParType type;
  unsigned size;

public:
  Parameter(const std::string& nm,ParType t,unsigned sz):
    name(nm),type(t),size(sz){}

  bool operator<(const Parameter& rhs) const
  {
    if ((type==VAL || type==REF) &&
    	(rhs.type==VAL || rhs.type==REF))
      return false;
    return type<rhs.type;
  }

  Parameter(){}
};

class FuncType
{
  friend class FuncContainer;

  typedef std::string str;
  typedef ParseTree::ISyntaxTreeNode stn;

  str name;
  std::vector<Parameter> par;
  stn* body;
  unsigned def_line;

  unsigned addr;
  unsigned size;
public:

  FuncType(const str& nm,unsigned ln):
    name(nm),par(),body(nullptr),
    def_line(ln),addr(0),size(0){}

  ~FuncType(){delete body;}

  FuncType(const FuncType&) = delete;
  FuncType& operator=(const FuncType&) = delete;

  bool operator<(const FuncType&) const;

  void AddPar(const str& pname,ParType t,unsigned sz=0)
  {
    par.push_back(Parameter(pname,t,sz));
  }

  void SetBody(stn* bd){body=bd;}
  void SetSize(unsigned sz){size=sz;}

  stn* GetBody() const {return body;}
  const str& GetFuncName() const {return name;}
  const str& GetParName(unsigned i) const 
  {
    assert(i<par.size());
    return par[i].name;
  }
  ParType GetParType(unsigned i) const
  {
    assert(i<par.size());
    return par[i].type;
  }
  unsigned GetParSize(unsigned i) const
  {
    assert(i<par.size());
    return par[i].size;
  }

  unsigned GetArgc() const {return par.size();}

  unsigned GetLine() const {return def_line;}
  unsigned GetAddr() const {return addr;}
  unsigned GetSize() const {return size;}
};

struct fptr_cmp
{

bool operator()(const FuncType* lhs,const FuncType* rhs) const
{
  return *lhs<*rhs;
}

};

class FuncContainer
{
  typedef FuncType ft;
  typedef std::set<FuncType*>::iterator si;

  std::set<FuncType*,fptr_cmp> rep;
  si curr_fn;
  unsigned curr_addr;
public:
  FuncContainer():
    rep(),curr_fn(rep.begin()),curr_addr(1){}

  FuncContainer(const FuncContainer&) = delete;
  FuncContainer& operator=(const FuncContainer&) = delete;

  unsigned Size() const {return rep.size();}
  
  void FuncPtrReset() {curr_fn=rep.begin();}
  void NextFunc() {++curr_fn;}

  ft* GetCurrFunc() const {return *curr_fn;}
  
  ft* Find(ft*) const;
  void Insert(ft*);
  void InsertMain(ft::stn*,unsigned);
  ~FuncContainer()
  {
    std::for_each(rep.begin(),rep.end(),[](ft* a){delete a;});
  }
};

} // namespace FuncCont

#endif
