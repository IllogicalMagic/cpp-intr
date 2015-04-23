#ifndef __CALC_TABLE_H__
#define __CALC_TABLE_H__

namespace SemTable
{

  // Tree token table
  enum Token {ST_SEQ,
	      CLEAR,
	      WHILE,DO,
	      IF,
	      FUNC,
	      ARR_DECL,
	      ASSIGN,
	      NUM,NAME,
	      INC_P,DEC_P,INC,DEC,
	      CMP,LESS,LESS_EQ,GR,GR_EQ,EQUAL,NOT_EQUAL,
	      OR,AND,
	      UN_PLUS,UN_MINUS,NOT,
	      ARR_ACCESS,
	      TERN,
	      PLUS,MINUS,
	      MUL,DIV};
}

namespace CmdTable
{

  // Interpreter commands
  enum Cmd {END,
	    POP,
	    GOTO,
	    IFTRUE_GOTO,IFFALSE_GOTO,
	    OR_GOTO,AND_GOTO,
	    PUSH_CONST,PUSH_VAR,PUSH_REF,
	    INIT_VAL,INIT_REF,INIT_ARR_REF,
	    CALL,

	    ASSIGN,
	    OR,AND,
	    LESS,LESS_EQ,GR,GR_EQ,EQUAL,NOT_EQUAL,
	    INC,DEC,INC_P,DEC_P,
	    NOT,U_PLUS,U_MINUS,
	    PLUS,MINUS,MUL,DIV,
	    ARR_ACCESS};
	    
}

#endif
