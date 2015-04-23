#include <string>
#include "TreeAnalyzer.h"
#include "Error.h"
#include "FuncCont.h"
#include "Interpreter.h"
#include <cstdio>
#include <cstdlib>
#include "Parser/ParsingDriver.h"

const long unsigned stack_size=10000;

const unsigned t_sz=19;
const unsigned err_sz=3;
const double t_res[t_sz] = {0.0,15.0,-27.0,62.0,-16.0,
			    1.0,55.0,3.0,45.0,1.125,
			    45.0,11111111.0,1.0,342.0,
			    1.0,144.0,1.0,45.6,7.0};

struct Fail
{
  unsigned fails;
  Fail():fails(0){}
  void operator()(unsigned i)
  {
    printf("Test %u failed\n",i);
    ++fails;
  }
};

static void success(unsigned);
static void check(double,unsigned,Fail&);
static void file_check(FILE*,const char*);

int main()
{
  IntrError::ErrHandler err;
  Fail fail;
  printf("Testing...\n");
  std::string s="Exprs/expr";
  unsigned expr_sz=s.size();
  yy::ParsingDriver pd(nullptr,err);
  for (unsigned i=0;i<t_sz;++i)
    {
      bool is_failed=false;
      s+=std::to_string(i);
      FILE* in=fopen(s.c_str(),"r");
      file_check(in,s.c_str());
      pd.Reset(in);

      if (pd.Parse())
	{
	  FuncCont::FuncContainer* fc=pd.GetFC();
	  Interpreting::Interpreter intr(*fc,err,stack_size);
	  TreeAnalysis::TreeAnalyzer ta(fc,err,intr);
	  if (ta.PrepareFuncFrames())
	    {
	      try
		{
		  if (intr.Calculate())
		    check(intr.GetResult(),i,fail);
		}
	      catch (std::bad_alloc)
		{
		  is_failed=true;
		}
	    }
	  else is_failed=true;
	}
      else is_failed=true;

      if (is_failed)
	  fail(i);
      err.Reset();
      s.erase(expr_sz);
      fclose(in);
    }

  if (!fail.fails)
    printf("Everything seems to be in order...\n");
  printf("\nStarting second part with error handling\n");

  printf("Expressions in this part are wrong\n\n");
  for (unsigned i=t_sz;i<t_sz+err_sz;++i)
    {
      s+=std::to_string(i);
      FILE* in=fopen(s.c_str(),"r");
      file_check(in,s.c_str());
      pd.Reset(in);

      if (pd.Parse())
	{
	  FuncCont::FuncContainer* fc=pd.GetFC();
	  Interpreting::Interpreter intr(*fc,err,stack_size);
	  TreeAnalysis::TreeAnalyzer ta(fc,err,intr);
	  if (ta.PrepareFuncFrames())
	    {
	      try
		{
		  if (intr.Calculate())
		    fail(i);
		  else success(i);
		}
	      catch (std::bad_alloc)
		{
		  fail(i);
		}
	    }
	  else success(i);
	}
      else success(i);

      printf("\n");
      err.Reset();
      s.erase(expr_sz);
      fclose(in);
    }
  printf("Testing over\nTotal fails: %u\n",fail.fails);
}

static void success(unsigned i)
{
  printf("Test %u passed\n",i);
}

static void check(double res,unsigned t_num,Fail& fail)
{
  if (res==t_res[t_num]) success(t_num);
  else fail(t_num);
}

static void file_check(FILE* in,const char* f_name)
{
  if (!in)
    {
      printf("Can't open '%s'\n",f_name);
      exit(1);
    }
}
