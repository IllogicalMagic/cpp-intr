#ifndef __CALC_VAR_CONTAINER_H__
#define __CALC_VAR_CONTAINER_H__

#include <string>
#include <map>

namespace VarCont
{

enum VarType {DOUBLE,REF,ARRAY,ARR_REF};

class Var
{
  VarType type;
  unsigned stack_pos;
  unsigned size;
public:
  Var(VarType t,unsigned pos,unsigned sz):
    type(t),stack_pos(pos),size(sz){};

  VarType GetType() const {return type;}
  unsigned GetStackPos() const {return stack_pos;}
  unsigned GetSize() const {return size;}
  ~Var(){}
};


typedef std::pair<bool,const Var*> apair;

class VarContainer
{
  unsigned curr_stack_pos;
  std::map<std::string,Var> rep;
public:
  VarContainer(unsigned pos):
    curr_stack_pos(pos){}
  ~VarContainer(){}
  apair Insert(const std::string&,VarType,unsigned sz);
  const Var* Find(const std::string&) const;
  unsigned GetStackPos() const {return curr_stack_pos;}
};

class VarContStack
{
  struct VNode
  {
    VNode* prev;
    VarContainer vc;
    VNode(VNode* pr,unsigned pos):
      prev(pr),vc(pos){}
  };

  VNode* top;
  Var* result;

  unsigned max_stack_size;
public:

  VarContStack():
    top(nullptr),result(nullptr),
    max_stack_size(1){}

  VarContStack(const VarContStack&) = delete;
  VarContStack& operator=(const VarContStack&) = delete;
  ~VarContStack();
  
  void PushVC();  
  void PopVC();
  apair AddVar(const std::string& name,VarType,unsigned size);
  apair GetVar(const std::string&) const;

  unsigned GetMaxStackSize() const {return max_stack_size;}
  bool IsResultExist() const {return result;}

  void Reset();
private:
  const Var* Find(const std::string&) const;
};

} // namespace VarCont

#endif
