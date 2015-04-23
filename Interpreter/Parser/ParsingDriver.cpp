#include "ParsingDriver.h"
#include <string>
#include <cstdio>
#include <iostream>
#include "lexer.flex.h"
#include "parser.tab.hpp"
#include "../SyntaxTree.h"
#include "../Table.h"
#include "../FuncCont.h"
#include "../Error.h"
#include <cassert>

using yy::ParsingDriver;
typedef ParseTree::ISyntaxTreeNode stn;

yy::ParserXX::token_type ParsingDriver::yylex
(yy::ParserXX::semantic_type* lval,yy::ParserXX::location_type* lloc)
{
  yy::ParserXX::token_type tmp=
    static_cast<yy::ParserXX::token_type>(::yylex(lexer));

  if (tmp==yy::ParserXX::token_type::ERR)
    {
      do
	{
	  err.SymbolError
	    ("Unknown symbol",yyget_text(lexer)[0],yyget_lineno(lexer));
	  tmp=static_cast<yy::ParserXX::token_type>(::yylex(lexer));
	}
      while (tmp==yy::ParserXX::token_type::ERR);
    }

  // printf("%s\n",yyget_text(lexer));

  *lloc=yyget_lineno(lexer);
  switch (tmp)
    {
    case yy::ParserXX::token_type::ID:
      {
	*lval=new ParseTree::SyntaxTreeLeafVar
	  (*lloc,std::string(yyget_text(lexer),yyget_leng(lexer)));
	break;
      }
    case yy::ParserXX::token_type::NUM:
      {
	std::string num(yyget_text(lexer),yyget_leng(lexer));
	double d=1.0;
	try
	  {
	    d=std::stod(num);
	  }
	catch (std::out_of_range)
	  {
	    err.LineError("Too big number",*lloc);
	  }
	*lval=new ParseTree::SyntaxTreeLeafNum(*lloc,d);
	break;
      }
    default:
      break;
    }
  return tmp;
}

void ParsingDriver::MakeFunc(stn* body)
{
  if (!body)
    {
      err.LineError("Function declarations are not allowed",fn->GetLine());
      delete fn;
      return;
    }
  FuncCont::FuncType* tmp=fc->Find(fn);
  fn->SetBody(body);
  if (tmp)
    {
      err.ODR_Error(fn->GetFuncName(),fn->GetLine(),
		    tmp->GetLine(),fn->GetArgc());
      delete fn;
      return;
    }
  fn->SetSize(func_size+1);
  fc->Insert(fn);
}

// For statements
stn* ParsingDriver::Merge(stn* lhs,stn* rhs)
{
  if (lhs)
    {
      lhs->SetOp(rhs);
      return lhs;
    }
  stn* res=new ParseTree::SyntaxTreeNodeSeq();
  res->SetOp(rhs);
  return res;
}

// Merge for binary operators
stn* ParsingDriver::MakeBin(SemTable::Token t,stn* lhs,stn* rhs,unsigned ln)
{
  if (!lhs)
    return rhs;
  stn* res=nullptr;
  switch (t)
    {
    case SemTable::OR:
    case SemTable::AND:
      {
	if (lhs->meaning==t)
	  res=lhs;
	else
	  {
	    res=new ParseTree::SyntaxTreeNodeBinBool(t,ln);
	    res->SetOp(lhs);
	  }
	res->SetOp(rhs);
	func_size+=2;
	return res;
      }
    default:
      {
	SemTable::Token type;
	switch (t)
	  {
	  case SemTable::MUL:
	  case SemTable::DIV:
	    type=SemTable::MUL;
	    break;
	  case SemTable::PLUS:
	  case SemTable::MINUS:
	    type=SemTable::PLUS;
	    break;
	  default:
	    type=SemTable::CMP;
	    break;
	  }
	if (lhs->meaning==type)
	  res=lhs;
	else
	  {
	    res=new ParseTree::SyntaxTreeNodeBinCmp(type,ln);
	    res->SetOp(lhs);
	  }
	assert(res);
	res->SetOp(rhs);
	res->SetType(t);
	++func_size;
	break;
      }
    }
  return res;
}

stn* ParsingDriver::MakePref(SemTable::Token t,stn* op,unsigned ln)
{
  stn* res=new ParseTree::SyntaxTreeNodeUnary(t,ln);
  ++func_size;
  res->SetOp(op);
  return res;
}

stn* ParsingDriver::MakePrim(stn* prim,stn* post)
{
  if (post)
    {
      stn* tmp=post;
      while (tmp->GetOp())
	tmp=tmp->GetOp();
      tmp->SetOp(prim);
      return post;
    }
  return prim; 
}

stn* ParsingDriver::MakePost(SemTable::Token t,stn* op,unsigned ln)
{
  stn* res=new ParseTree::SyntaxTreeNodeUnary(t,ln);
  ++func_size;
  if (op)
    {
      op->SetOp(res);
      return op;
    }
  return res; 
}

ParsingDriver::ParsingDriver(FILE* in,IntrError::ErrHandler& err_r):
  mn(nullptr),curr_file(in),func_size(0),
  fc(new FuncCont::FuncContainer),fn(nullptr),err(err_r)
{
  yylex_init(&lexer);
  yyset_in(in,lexer);
}

ParsingDriver::~ParsingDriver()
{
  yylex_destroy(lexer);
  delete fc;
}
