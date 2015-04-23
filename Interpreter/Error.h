#ifndef __CALC_ERROR_H__
#define __CALC_ERROR_H__

#include <string>

namespace IntrError
{

class ErrHandler
{
  typedef std::string str;
  unsigned err_count;
public:
 ErrHandler():err_count(0){}
  ~ErrHandler(){}

  void Warn(const str&,unsigned);
  
  void BoolWarn(unsigned);
  void DoubleWarn(unsigned);

  void LineError(const str&,unsigned);
  void VarError(const str&,const str&,unsigned);
  void Error(const str&);
  void SymbolError(const str&,char,unsigned);

  void ODR_Error(const str&,unsigned ln1,unsigned ln2,unsigned argc);

  void NoRetValue(const str&,unsigned);

  void Undefined(const str&,unsigned argc,unsigned ln);
  
  unsigned Count() const {return err_count;}

  void Reset(){err_count=0;}
};

} // namespace CalcError

#endif
