#include "Error.h"
#include <iostream>
#include <string>

using IntrError::ErrHandler;

void ErrHandler::LineError(const str& msg,unsigned line)
{
  ++err_count;
  std::cerr << "Line " << line << ": " << msg << std::endl;
}

void ErrHandler::VarError(const str& msg,const str& vr, unsigned line)
{
  ++err_count;
  std::cerr << "Line " << line << ": " << msg << " '" << vr 
       << '\'' << std::endl;
}

void ErrHandler::Error(const str& msg)
{
  ++err_count;
  std::cerr << msg << std::endl;
}

void ErrHandler::SymbolError(const str& msg,char c,unsigned line)
{
  ++err_count;
  std::cerr << "Line " << line << ": " << msg << " '" << c
	    << '\'' << std::endl;
}

void ErrHandler::Warn(const str& msg,unsigned line)
{
  std::cerr << "Warning: " <<msg << " on line " << line << std::endl;
}

void ErrHandler::BoolWarn(unsigned line)
{  
  std::cerr << "Warning: conversion double->bool on line "
	    << line << std::endl;
}

void ErrHandler::DoubleWarn(unsigned line)
{
  std::cerr << "Warning: conversion bool->double on line "
	    << line << std::endl;
}

void ErrHandler::ODR_Error(const str& func,unsigned ln1,
		     unsigned ln2,unsigned args)
{
  std::cerr << "Function '" << func << "' with";
  if (args==0)
    std::cerr << "out arguments ";
  else
    {
      std::cerr << ' ' << args << " argument";
    if (args>1)
      std::cerr << 's';
    }
  std::cerr << " defined on line " << ln1 
	    << " was previously defined on line "
	    << ln2 << std::endl;
  ++err_count;
}

void ErrHandler::NoRetValue(const str& fn,unsigned ln)
{
  if (fn.size()==0)
    {
      std::cerr << "Program has no returning value" << std::endl;
      ++err_count;
      return;
    }
  std::cerr << "Function '" << fn << "' defined on line "
	    << ln << " has no returning value" << std::endl;
  ++err_count;
}

void ErrHandler::Undefined
(const str& fn,unsigned argc,unsigned ln)
{
  std::cerr << "Using of undefined function '" << fn << "' with";
  if (argc==0)
    std::cerr << "out arguments";
  else
    {
      std::cerr << ' ' << argc << " argument";
      if (argc>1)
	std::cerr << 's';
    }
  std::cerr << " on line " << ln << std::endl;
  ++err_count;
}
