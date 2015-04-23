#ifndef __TREE_ANALYZER_H__
#define __TREE_ANALYZER_H__

#include "Table.h"
#include "SyntaxTree.h"
#include "VarCont.h"
#include "Error.h"
#include "FuncCont.h"
#include "Interpreter.h"
#include <string>

namespace TreeAnalysis
{

class TreeAnalyzer
{
  // Internal representation of function result
  struct IntVar
  {
    enum IntVarType {TMP,VAR,REF,ARR,BOOL,NOTHING};
    struct Buf
    {
      std::string name;
      unsigned users;
      Buf(const std::string nm):
	name(nm),users(1){}
    };
    IntVarType type;
    Buf* name;
    double value;
    unsigned size;
    
    IntVar(IntVarType tp):
      type(tp),name(nullptr),value(1.0),size(1){}

    IntVar(const IntVar& rhs):
      type(rhs.type),name(rhs.name),
      value(rhs.value),size(rhs.size)
    {
      if (name)
	++name->users;;
    }
    
    ~IntVar()
    {
      if (name && --name->users==0)
	delete name;
    }
    IntVar& operator=(const IntVar&) = delete;
  }; // struct IntVar
  
  typedef ParseTree::ISyntaxTreeNode stn;
  typedef IntVar iv;
  
  FuncCont::FuncContainer* f_cont;
  VarCont::VarContStack vc_stack;
  IntrError::ErrHandler& err;

  Interpreting::Interpreter& intr;
public:
  TreeAnalyzer(FuncCont::FuncContainer* f,
	       IntrError::ErrHandler& r_err,
	       Interpreting::Interpreter& r_intr):
    f_cont(f),vc_stack(),err(r_err),intr(r_intr){}
  ~TreeAnalyzer()
  {
    delete f_cont;
  }

  bool PrepareFuncFrames();	// True is success

private:
  void AddFuncParameters(FuncCont::FuncType*);

  iv NodeHandler(stn*);

  void StatementSeq(stn*);

  iv FuncCall(stn*);

  void ClearHandler(stn*);

  void WhileHandler(stn*);
  void DoWhileHandler(stn*);

  void ArrayDeclaration(stn*);
  void ArrayInitialization(stn*,unsigned);

  iv Assign(stn*);
  iv Tern(stn*);
  iv Logical(stn*);
  iv Cmp(stn*);			
  iv MathBasic(stn*);		// +,-,*,/		
  iv UnarySimple(stn*);		// -,+,!
  iv IncDec(stn*);
  iv ArrayAccess(stn*);
  iv Name(stn*);
  iv Num(stn*);
  iv PostIncDec(stn*);

  bool CheckVar(const iv&,unsigned);
  bool IsVarFailed(const iv&,unsigned);
  bool IsVal(const iv&,unsigned);

  void BoolConv(const iv&,unsigned);
  void DoubleConv(const iv&,unsigned);

}; // class TreeAnalyzer

} // namespace TreeAnalysis

#endif
