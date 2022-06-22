#ifndef LIMBO_TYPE_H
#define LIMBO_TYPE_H

#include "lexer.h"

typedef enum TypeKind TypeKind;
typedef struct Member Member;
typedef struct Type Type;

enum TypeKind {
    TNone,
    TAdt,
    TAdtPick,
    TArray,
    TBig,
    TByte,
    TChan,
    TReal,
    TFn,
    TInt,
    TList,
    TModule,
    TRef,
    TString,
    TTuple,
    TException,
    TFix,
    TPoly,
    TAInit,
    TAlt,
    TAny,
    TArrow,
    TCase,
    TCaseL,
    TCaseC,
    TDot,
    TError,
    TGoto,
    TId,
    TIFace,
    TExcept,
    TInst,
};

struct Member {
    Type *type;
    Token *token;
    Token *name;
    uptr index;
    uptr align;
    uptr offset;
};

struct Type {
    TypeKind kind;
    uptr size, align;

    Token *name;

    // Attributes
    bool is_ptr;
    bool can_ref;
    bool can_con;
    bool big;
    bool visible;

    // Composite types
    uptr n_members;
    Member *members;

    // Functions
    Type *return_type;
    uptr n_params;
    Type *params;
};

extern Type *type_none;
extern Type *type_big;
extern Type *type_byte;
extern Type *type_int;
extern Type *type_real;
extern Type *type_string;

#endif //LIMBO_TYPE_H
