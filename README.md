STAR language is a small scripting language that can be used for performing basic arithmetical operations. It supports only two data types: integers and strings (called text). Integers can only take positive values and would take the value zero when forced to take a negative value. Assignments in STAR can have arithmetic expressions with two, operands only. Complicated arithmetic expressions are not allowed.
The lexical details of the language have already been described in the former project. This document will focus on general syntax and semantics.
Variables and Declarations:
Variables should be declared before they are used. They can be declared with the simple keywords int or text. Declaration does not have to be at the beginning of the file. It has to precede every usage of the variable though. 
int myInt.
text myString.
In this case they would be initialized as zero and empty string respectively. It is also possible to declare multiple variables on one line:
int a,b,c,d.
It is also possible to combine the declaration with initialization.
int count is 5.
text errMsg is “An error has just occurred:”.
Or multiple declarations with initialization:
int a is 1, b is 2.    /*is possible*/
Initialization is done with constant values only. It cannot be done with expressions or variables.
Each variable should have a separate name that does not clash with another variable or keyword. Variables hold their value throughout the program unless they are overridden by an assignment statement. All the variables have a global scope (no local variables) but they cannot be used before they are declared.
Maximum size for a string variable is 256 characters. Any assignment that exceeds this size would cause the result to be truncated back to 256.
Because maximum integer size is 8 digits, maximum value that an integer can take is 99999999. If at any time this value is exceeded the interpreter should issue a suitable error message. If an integer is forced to have a negative value, it will take the value zero instead (without any error message).

Assignments:
Assignments are performed via the “is” keyword.
myString is “Hello world”.   /*This is an assignment*/
It is also possible to write an arithmetic expression on the right hand side of the assignment. Arithmetic expressions are composed of two values and an operator.
myInt is myInt +1.  /* Can be used in order to increment myInt */
Negative values are not allowed. And if you intend to force it through an operation:
myInt is 1 – 3 .  /* This will assign value 0 to myInt*/

Comments: Anything between /* and */ is a comment. Comments can span more than one line.

Lines of Code
STAR supports lines of code that are terminated with a period ‘.’. The lines of text are not important.
text myString  /* This line can be divided into two*/
is “Hello”.    /* like this. And comments may be within*/

/* As you might have noticed
   this is a comment. */

Input / Output
read, write and newLine commands are used for I/O.
All I/O takes place on a command line console. For input the general format is:
read “prompt:”, varName.  /* Prompt is printed and the cursor waits for an input next to it*/
If the variable is an integer and an appropriate numeric value is not specified, the interpreter should issue a warning message, assign 0 to the value and continue running. Prompt text can be a variable name of text type. Prompt text is also optional and can be omitted.
read varName.  /* is enough*/
“write” keyword can print values on the screen. It can take as many attributes as you wish and would print everything on its right , separated with commas, until it reaches the end of line symbol.
write “The values are”, a ,“  “,  b, “  “,  c.   /*Blanks are inserted between values*/
The write statement of STAR can print constants and variables only (being both text or int). It does not accept arithmetic expressions.
A new line character is printed with the single plain command newLine.
newLine.

Loops
There is just a basic for loop in STAR. This is accomplished through the loop statement.
loop 5 times write “*”.     /*Prints 5 asterix characters next to each other*/
Loops can also support code blocks specified within curly braces:
int i.
i is 1.
loop 10 times
{ write i.  newLine.
   i is i+1.
}   /* Will print numbers from 1 to 10 */

Loops can also be nested.

int i.
i is 1.
loop 10 times
{  loop i times write “*”.
   newLine.
   i is i+1.
}   /* Will print numbers from 1 to 10 */


Operations:
Text values (both variables and constants) can also be added and subtracted. +  operator between two text values would concatenate these values. 
text Str1 is “ like home.” , Str2.
Str2 is “There is no place” + Str1.
write Str1.  /* Would print “There is no place like home”.*/

Subtraction should remove the first occurrence of the second string from the first string.
text s is “Ohh ice ice baby!”.
text s2.
s2 is s- “ice”.
write s2.  /* Prints “Ohh  ice baby!”*/
s2 is s2- “ ice”.
write s2.  /* Prints “Ohh  baby!”*/
