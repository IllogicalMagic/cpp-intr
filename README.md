###Interpreter (written in C++)

Supports variables, arrays, expressions, blocks with scopes, conditions, loops and functions (with references and values as parameters). All variables are double.

####Format of input file:

    functions (optional)
    statements

####Language description:

*Note: every instruction ends with ';'*

**Commentaries are not supported. '//' is just notes for reader.**

**1) Variables**

    a=2;       // variable 'a' is defined with value 2
    result=a;  // 'result' value is 2

*Note: you can define variable everywhere you want, even in the middle of expression (like {result=a=1+(b=3);}). But I do not recommend define it in ternary operator and logical operations (they are lazy). For example, you write something like {1<2 ? a=2 : (b=3);}. If 'a' and 'b' is not defined before, it will be undefined behaviour ^_^, because there only one variable have value, but you can use both of them after.*  
*Note: every function has 'result' variable as returned value.*

**2) Arrays**

    array a[5];            // definition of 'a' size of 5, values uninitialized
    array b[6]=[1,2,4,7];  // definition of 'b' size of 6, first 4 values initialized
    array c[]=[1,2,3];     // size of 'c' is deduced from initializer-list and it is 3.
    a[3]=b[1];             // fourth element of 'a' has value 2

There is size checking for arrays:

    a[7]=5;  // interpreting error - 'a' does not have eighth element

*Note: indexes are starting from zero*

**3) Expressions**

Similar to C. Now supported:  
- unary prefix:
  - +,-
  - ! (not)
  - ++ (increment), -- (decrement)
- unary postfix:
  - ++,--
  - () - function call
  - [] - array access
- binary:
  - =
  - +,-,\*,/
  - \&\& (and), || (or)
  - <,>,<=,>=,==,!=
- ternary:
  - ? :

Some examples:

    a=6+2*2;
    b=9/--a;
    c= (a++ < 13 && a > 0) ? 3 : 6; // ternary
    result=foo(c);  // using of function 'foo' with 'c' as argument

*Note: assign, increment, decrement and array access always return lvalue. Ternary operator can return anything. For example:*

    a= (1<2) ? 2 : 3;  // ternary return rvalue
    b=7;
    c= ((a<b) ? a : b)++;  // now it returns lvalue, so it can be incremented
    array d[4]=[1,2,3,4];
    array e[5]=[5,4,3,2,1];
    result=(c>=a) ? d : e)[3];  // and now it returns array

**4) Blocks**

Block begins with '{' and ends with '}'. Every block can contain local variables, that cannot be used outside. Example:

    a=2;
    {
      b=a;  // new variable 'b' with value of 'a' defined in extern block.
    }
    a=b;  // error: 'b' is not defined.

**5) Conditions**

Simple 'if' and 'if-else' constructions.

    if (a<b)
      {
        ++a;
        --b;
      }
    else --b;

**6) Loops**

'while' and 'do ... while' constructions.

    i=0;
    while (i<4)
      ++i;
    do
      --i;
    while (i>0);

*Note: you can write nothing in condition and loop body. For example, {if (a<b);}.*

**7) Functions**

Functions can be defined in the beginning of input file. Function definition begins with the word 'function'. Then there comes function name and list of parameters. Body of function is simple block.

    function foo(a){...}  // 'foo' takes one parameter by value
    result=foo(2);        // using of 'foo' with argument 2

Function parameters can be values, references and arrays.

    function foo(a,b,c[]){...}  // 'foo' takes two values and one array of any size
    function bar(a&,b[5]){...}  // 'bar' takes one reference and one array of size 5

Array in function argument is reference. So when you call function, it can change array.

There can be functions with the same names, but with different arguments. References and values is not considered different.

    function foo(a,b){...}
    function foo(a[],b){...}   // it is two different functions
    array a[3];
    result=foo(2,3)+foo(a,4);  // first 'foo' is 'foo(a,b)' and second is 'foo(a[],b)'

*Note: statements is not in global scope. It is just another function, but without name, arguments and not in block. It is like 'main'.*

You can also write recursive functions. Default maximum recursion depth is 10000. It can be changed by using '-s <n>' in command line as Interpreter parameter.

####Some other notes:

1. There only one type of variables - double. But logical expressions using bool. Compare operators can make bool values. You will get warning when there will be implicit conversion.
1. Now Interpreter use 'expression.txt' as input file.
1. Some examples can be found is 'Expr' directory. You can make tests by running IntrTests.
2. 'make all' makes debug version. If you want release version use 'make release'.
