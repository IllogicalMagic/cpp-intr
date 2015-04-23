#ifndef __PARSING_DRIVER__H__
#define __PARSING_DRIVER__H__

#include "parser.tab.hpp"
#include "lexer.flex.h"
#include <cstdio>
#include "../SyntaxTree.h"
#include "../Table.h"
#include "../FuncCont.h"
#include "../Error.h"

namespace yy
{

class ParsingDriver
{
  typedef ParseTree::ISyntaxTreeNode stn;
  friend class yy::ParserXX;

  friend ParserXX::token_type yylex
  (ParserXX::semantic_type*,ParserXX::location_type*,ParsingDriver*);

  yyscan_t lexer;

  stn* mn;

  FILE* curr_file;
  unsigned func_size;
  FuncCont::FuncContainer* fc;
  FuncCont::FuncType* fn;

  IntrError::ErrHandler& err;
public:
  ParsingDriver(FILE*,IntrError::ErrHandler&);

  bool Parse()
  {
    yy::ParserXX parser(this);
    bool res=parser.parse();
    return !(res || err.Count());
  }

  FuncCont::FuncContainer* GetFC()
  {
    FuncCont::FuncContainer* tmp=fc;
    fc=nullptr;
    return tmp;
  }

  void Reset(FILE* in)
  {
    yyrestart(in,lexer);
    delete fc;
    fc=new FuncCont::FuncContainer;
  }

  ~ParsingDriver();
private:
  void MakeFunc(stn*);
  stn* Merge(stn* l,stn* r);
  stn* MakeBin(SemTable::Token,stn* lhs,stn* rhs,unsigned);
  stn* MakePref(SemTable::Token,stn*,unsigned);
  stn* MakePrim(stn* prim,stn* post);
  stn* MakePost(SemTable::Token,stn*,unsigned);

  yy::ParserXX::token_type yylex
  (yy::ParserXX::semantic_type*,yy::ParserXX::location_type*);

  ParsingDriver(const ParsingDriver&) = delete;
  ParsingDriver& operator=(const ParsingDriver&) = delete;
};

} // namespace Parsing

#endif
