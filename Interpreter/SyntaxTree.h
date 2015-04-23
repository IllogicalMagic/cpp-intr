#ifndef __PARSE_TREE_H__
#define __PARSE_TREE_H__

#include "Table.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cassert>

namespace ParseTree
{

struct ISyntaxTreeNode
{   
  const SemTable::Token meaning;

  ISyntaxTreeNode(SemTable::Token mn):
    meaning(mn){}

  virtual ~ISyntaxTreeNode(){}

  virtual void SetOp(ISyntaxTreeNode*,unsigned i=0) = 0;
  virtual ISyntaxTreeNode* GetOp(unsigned i=0) const = 0;

  // For binary operator -,+,*,/ and cmp
  virtual void SetType(SemTable::Token) = 0;
  virtual SemTable::Token GetType(unsigned) const = 0;

  virtual const std::string* GetName() const = 0;
  virtual double GetVal() const = 0;

  virtual unsigned Size() const = 0;

  virtual unsigned Line() const = 0;

  virtual void SetArg(ISyntaxTreeNode*) = 0;
  virtual ISyntaxTreeNode* GetArg(unsigned) const = 0;


  ISyntaxTreeNode(const ISyntaxTreeNode&) = delete;
  ISyntaxTreeNode& operator=(const ISyntaxTreeNode&) = delete;
};

typedef ISyntaxTreeNode stn;

// Statement sequence
struct SyntaxTreeNodeSeq: public ISyntaxTreeNode
{
  std::vector<ISyntaxTreeNode*> stm;

  SyntaxTreeNodeSeq():
    ISyntaxTreeNode(SemTable::ST_SEQ){}

  void SetOp(stn* op,unsigned i=0){stm.push_back(op);}

  stn* GetOp(unsigned i) const {return stm[i];}

  unsigned Size() const {return stm.size();}

  virtual ~SyntaxTreeNodeSeq()
  {
    for_each(stm.begin(),stm.end(),[](stn* a){delete a;});
  }

  void SetType(SemTable::Token){}
  SemTable::Token GetType(unsigned) const {return SemTable::ST_SEQ;}
  const std::string* GetName() const {return nullptr;}
  double GetVal() const {return 0.0;} 
  unsigned Line() const {return 0;}

  void SetArg(stn*){}
  stn* GetArg(unsigned) const {return nullptr;}
};

// With line
struct ISyntaxTreeNodeWLine: public ISyntaxTreeNode
{
  const unsigned line;

  ISyntaxTreeNodeWLine(SemTable::Token mn,unsigned ln):
    ISyntaxTreeNode(mn),line(ln){}

  unsigned Line() const {return line;}
  virtual ~ISyntaxTreeNodeWLine(){}
  
  virtual void SetOp(stn* op,unsigned i=0){}
  virtual stn* GetOp(unsigned i) const {return nullptr;}
  virtual unsigned Size() const {return 0;}
  virtual void SetType(SemTable::Token){}
  virtual SemTable::Token GetType(unsigned) const {return SemTable::ST_SEQ;}
  const std::string* GetName() const {return nullptr;}
  virtual double GetVal() const {return 0.0;}

  virtual void SetArg(stn*){}
  virtual stn* GetArg(unsigned) const {return nullptr;}
};

struct SyntaxTreeNodeLoop: public ISyntaxTreeNodeWLine
{
  stn* cond1;
  stn* body;

  SyntaxTreeNodeLoop(SemTable::Token mn,unsigned ln):
    ISyntaxTreeNodeWLine(mn,ln),cond1(nullptr),body(nullptr){}

  void SetOp(stn* op,unsigned i=0)
  {
    assert(i==1 || i==2);
    if (i==1) cond1=op;
    if (i==2) body=op;
  }
  stn* GetOp(unsigned i=0) const
  {
    assert(i==1 || i==2);
    if (i==1) return cond1;
    if (i==2) return body;
    return nullptr;
  }

  virtual ~SyntaxTreeNodeLoop()
  {
    delete cond1;
    delete body;
  }
};

struct SyntaxTreeNodeUnary: public ISyntaxTreeNodeWLine
{
  ISyntaxTreeNode* op1;

  SyntaxTreeNodeUnary(SemTable::Token mn,unsigned ln):
    ISyntaxTreeNodeWLine(mn,ln),op1(nullptr){}
  
  void SetOp(stn* op,unsigned i=0){op1=op;}
  stn* GetOp(unsigned i=0) const {return op1;}

  virtual ~SyntaxTreeNodeUnary(){delete op1;}
};

// Assignment
struct SyntaxTreeNodeBinSimple: public SyntaxTreeNodeUnary
{
  ISyntaxTreeNode* op2;

  SyntaxTreeNodeBinSimple(SemTable::Token mn,unsigned ln):
    SyntaxTreeNodeUnary(mn,ln),op2(nullptr){}

  void SetOp(stn* op,unsigned i)
  {
    if (i==1) op1=op;
    if (i==2) op2=op;
  }
  stn* GetOp(unsigned i) const
  {
    switch (i)
      {
      case 1: return op1;
      case 2: return op2;
      default: return nullptr;
      }
  }

  virtual ~SyntaxTreeNodeBinSimple(){delete op2;}
};

struct SyntaxTreeNodeTern: public SyntaxTreeNodeBinSimple
{
  ISyntaxTreeNode* op3;

  SyntaxTreeNodeTern(SemTable::Token mn,unsigned ln):
    SyntaxTreeNodeBinSimple(mn,ln),op3(nullptr){}

  void SetOp(stn* op,unsigned i)
  {
    if (i==1) op1=op;
    if (i==2) op2=op;
    if (i==3) op3=op;
  }
  stn* GetOp(unsigned i) const
  {
    switch(i)
      {
      case 1: return op1;
      case 2: return op2;
      case 3: return op3;
      default: return nullptr;
      }
  }

  virtual ~SyntaxTreeNodeTern(){delete op3;}
};

struct SyntaxTreeNodeBinBool: public ISyntaxTreeNodeWLine
{
  std::vector<ISyntaxTreeNode*> op;

  SyntaxTreeNodeBinBool(SemTable::Token mn,unsigned ln):
    ISyntaxTreeNodeWLine(mn,ln){}

  void SetOp(stn* op1,unsigned i=0)
  {
    op.push_back(op1);
  }
  stn* GetOp(unsigned i) const {return op[i];}
  unsigned Size() const {return op.size();}

  virtual ~SyntaxTreeNodeBinBool()
  {
    std::for_each(op.begin(),op.end(),[](stn* a){delete a;});
  }
};

struct SyntaxTreeNodeBinCmp: public SyntaxTreeNodeBinBool
{ 
  std::vector<SemTable::Token> type;

  SyntaxTreeNodeBinCmp(SemTable::Token mn,unsigned ln):
    SyntaxTreeNodeBinBool(mn,ln){}
  void SetType(SemTable::Token tp){type.push_back(tp);}
  SemTable::Token GetType(unsigned i) const {return type[i];}
};

struct SyntaxTreeLeafVar: public ISyntaxTreeNodeWLine
{
  const std::string name;

  SyntaxTreeLeafVar(unsigned ln,const std::string& nm):
    ISyntaxTreeNodeWLine(SemTable::NAME,ln),
    name(nm){}

  const std::string* GetName() const {return &name;}

  virtual ~SyntaxTreeLeafVar(){}
};

struct SyntaxTreeLeafNum: public ISyntaxTreeNodeWLine
{
  const double value;

  SyntaxTreeLeafNum(unsigned ln,double val):
    ISyntaxTreeNodeWLine(SemTable::NUM,ln),value(val){}

  double GetVal() const {return value;}
};

struct SyntaxTreeNodeArgs: public ISyntaxTreeNodeWLine
{
  stn* name;
  std::vector<stn*> args;
  
  SyntaxTreeNodeArgs(unsigned ln):
    ISyntaxTreeNodeWLine(SemTable::FUNC,ln),name(nullptr),args(){}

  unsigned Size() const {return args.size();}
  void SetArg(stn* arg){args.push_back(arg);}
  void SetOp(stn* nm,unsigned i=0){name=nm;}
  stn* GetOp(unsigned i=0) const {return name;}
  stn* GetArg(unsigned i) const {assert(i<args.size());return args[i];}

  ~SyntaxTreeNodeArgs()
  {
    delete name;
    std::for_each(args.begin(),args.end(),[](stn* a){delete a;});
  }
};

struct SyntaxTreeNodeClear: public ISyntaxTreeNode
{
  stn* body;

  SyntaxTreeNodeClear():
    ISyntaxTreeNode(SemTable::CLEAR),body(nullptr){}

  void SetOp(stn* op,unsigned i=0){body=op;}
  stn* GetOp(unsigned i=0) const {return body;}

  ~SyntaxTreeNodeClear(){delete body;}

  unsigned Line() const {return 0;} 
  unsigned Size() const {return 0;}
  void SetType(SemTable::Token){}
  SemTable::Token GetType(unsigned) const {return SemTable::ST_SEQ;}
  const std::string* GetName() const {return nullptr;}
  double GetVal() const {return 0.0;}
  void SetArg(stn*){}
  stn* GetArg(unsigned) const {return nullptr;}

};

struct SyntaxTreeNodeArray: public ISyntaxTreeNodeWLine
{
  std::string name;
  stn* size;
  stn* init;
  SyntaxTreeNodeArray(const std::string& nm,unsigned ln):
    ISyntaxTreeNodeWLine(SemTable::ARR_DECL,ln),
    name(nm),size(nullptr),init(nullptr){}
  ~SyntaxTreeNodeArray()
  {
    delete size;
    delete init;
  }

  void SetOp(stn* op,unsigned i)
  {
    assert(i==1 || i==2);
    if (i==1)
      size=op;
    else init=op;
  }
  stn* GetOp(unsigned i) const
  {
    assert(i==1 || i==2);
    if (i==1)
      return size;
    return init;
  }

  const std::string* GetName() const {return &name;}
};

struct SyntaxTreeNodeAccess: public SyntaxTreeNodeUnary
{
  stn* arg;

  SyntaxTreeNodeAccess(unsigned ln):
    SyntaxTreeNodeUnary(SemTable::ARR_ACCESS,ln),arg(nullptr){}
  ~SyntaxTreeNodeAccess(){delete arg;}

  void SetArg(stn* op){arg=op;}
  stn* GetArg(unsigned) const {return arg;}
};

} // namespace ParseTree

#endif
