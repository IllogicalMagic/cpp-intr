#include "Error.h"
#include "Table.h"
#include "SyntaxTree.h"
#include "TreeAnalyzer.h"
#include "FuncCont.h"
#include "Interpreter.h"

#include <cstdio>
#include <string>

#include "Parser/ParsingDriver.h"

const long unsigned default_stack_size=10000;

void wrong_param();
long unsigned get_stack_size(int argc,const char** argv);

int main(int argc,const char** argv)
{
  long unsigned stack_size=get_stack_size(argc,argv);

  IntrError::ErrHandler err;
  FILE* in=fopen("expression.txt","r");
  if (!in)
    {
      fprintf(stderr,"'expression.txt' is not found\n");
      exit(1);
    }
  yy::ParsingDriver pd(in,err);
  pd.Reset(in);
  if (pd.Parse())
    {
      FuncCont::FuncContainer* fc=pd.GetFC();
      Interpreting::Interpreter intr(*fc,err,stack_size);
      TreeAnalysis::TreeAnalyzer ta(fc,err,intr);
      if (ta.PrepareFuncFrames())
      	try
      	  {
      	    if (intr.Calculate())
      	      printf("Result is %lg\n",intr.GetResult());
      	  }
      	catch (std::bad_alloc)
      	  {
      	    err.Error("Out of memory");
      	  }
    }
  fclose(in);
  return err.Count();
}

long unsigned get_stack_size(int argc,const char** argv)
{
  if (argc==1)
    return default_stack_size;
  if (argc!=3)
    wrong_param();
  std::string par(argv[1]);
  if (par!="-s")
    wrong_param();
  par=argv[2];
  try
    {
      return std::stoul(par);
    }
  catch (std::out_of_range)
    {
      fprintf(stderr,"Number is too big\n");
      exit(1);
    }
  catch (std::invalid_argument)
    {
      wrong_param();
    }
  return default_stack_size;
}

void wrong_param()
{
  fprintf(stderr,"Using: 'Calc -s [num]' or 'Calc'\n");
  exit(1);
}
