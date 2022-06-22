#ifndef LIMBO_PARSER_H
#define LIMBO_PARSER_H

#include "lexer.h"

typedef enum NodeKind {
    NODE_NOP,     // no operation

    // Binary

    NODE_ADD,     // addition
    NODE_SUB,     // subtraction
    NODE_MUL,     // multiplication
    NODE_DIV,     // division
    NODE_MOD,     // modulo
    NODE_BIT_AND, // bitwise and
    NODE_BIT_OR,  // bitwise or
    NODE_BIT_XOR, // bitwise xor
    NODE_EQ,      // equality
    NODE_LT,      // less than
    NODE_GT,      // greater than
    NODE_LTE,     // less than or equal to
    NODE_GTE,     // greater than or equal to
    NODE_NEQ,     // not equal to
    NODE_SHL,     // shift left
    NODE_SHR,     // shift right
    NODE_AND,     // logical and
    NODE_OR,      // logical or
    NODE_CHAN_TX, // sending and receiving
    NODE_CONS,    // list cons

    // Assignment-related

    NODE_ASSIGN,         // assignment
    NODE_ASSIGN_ADD,     // addition assignment
    NODE_ASSIGN_SUB,     // subtraction assignment
    NODE_ASSIGN_MUL,     // multiplication assignment
    NODE_ASSIGN_DIV,     // division assignment
    NODE_ASSIGN_MOD,     // modulo assignment
    NODE_ASSIGN_BIT_AND, // bitwise and assignment
    NODE_ASSIGN_BIT_OR,  // bitwise or assignment
    NODE_ASSIGN_BIT_XOR, // bitwise xor assignment
    NODE_ASSIGN_SHL,     // shift left assignment
    NODE_ASSIGN_SHR,     // shift right assignment
    NODE_DECL,           // variable declaration
    NODE_DECL_EXP,       // declaration expression
    NODE_EXP,            // exponentiation

    // Unary

    NODE_NEG,     // negation
    NODE_BIT_NOT, // bitwise not
    NODE_INC,     // increment
    NODE_DEC,     // decrement
    NODE_NOT,     // logical not

    // Keywords

    NODE_ADT,      // abstract data type
    NODE_ALT,      // control transfer
    NODE_BREAK,    // break
    NODE_CASE,     // case statement
    NODE_CAST,     // cast to a different type
    NODE_CHAN,     // channel
    NODE_CON,      // constant
    NODE_CONTINUE, // continue
    NODE_CYCLIC,   // self-referential type
    NODE_DO,       // do-while loop
    NODE_ELSE,     // else clause
    NODE_EXIT,     // exit statement
    NODE_FN,       // function
    NODE_FOR,      // for loop
    NODE_HD,       // head of a list
    NODE_IF,       // if statement
    NODE_IMPL,     // implementation of a module
    NODE_IMPORT,   // import a module
    NODE_INCL,     // include a module
    NODE_LEN,      // length of some type
    NODE_LIST,     // list of some type
    NODE_MODULE,   // module definition
    NODE_NIL,      // nil constant
    NODE_OF,       // generic type
    NODE_PICK,     // pick adt aka discriminated union
    NODE_REF,      // references
    NODE_RETURN,   // return from a function
    NODE_SELF,     // adt self reference
    NODE_SPAWN,    // spawn a new thread (inferno calls these processes)
    NODE_TAGOF,    // tag of a pick adt
    NODE_TL,       // tail of a list
    NODE_TO,       // range keyword
    NODE_TYPE,     // reserved keyword
    NODE_WHILE,    // while or do-while loop

    // Literals

    NODE_STRING,   // string literal
    NODE_INTEGRAL, // integral literal
    NODE_REAL,     // real literal

    // Misc

    NODE_BLOCK,         // block of statements { ... }
    NODE_FUNCTION_CALL, // function call
    NODE_FUNCTION,      // function definition

} NodeKind;

typedef struct Node Node;
struct Node {
    NodeKind kind;
    Node *next;
    Type *type;
    Token *token;

    Node *left;
    Node *right;

    // For control flow, i.e.
    // NODE_IF, NODE_WHILE, NODE_DO, NODE_FOR, NODE_CASE
    Node *cond;
    Node *then;
    Node *else_;
    Node *init;
    Node *inc;

    // Block or statement
    Node *body;


};

typedef struct ParserContext {
    // locals
    // globals
    // current scope
    // switch scope
} ParserContext;

#endif //LIMBO_PARSER_H
