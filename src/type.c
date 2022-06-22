#include <stdalign.h>
#include "type.h"

Type *type_none = &(Type) {
    .kind = TNone,
};

Type *type_big = &(Type) {
    .kind = TBig,
    .size = sizeof(i64),
    .align = alignof(i64),
    .is_ptr = false,
    .can_ref = false,
    .can_con = true,
    .big = true,
    .visible = true,
};

Type *type_byte = &(Type) {
    .kind = TByte,
    .size = sizeof(u8),
    .align = alignof(u8),
    .is_ptr = false,
    .can_ref = false,
    .can_con = true,
    .big = false,
    .visible = true,
};

Type *type_int = &(Type) {
    .kind = TInt,
    .size = sizeof(i32),
    .align = alignof(i32),
    .is_ptr = false,
    .can_ref = false,
    .can_con = true,
    .big = false,
    .visible = true,
};

Type *type_real = &(Type) {
    .kind = TReal,
    .size = sizeof(f64),
    .align = alignof(f64),
    .is_ptr = false,
    .can_ref = false,
    .can_con = true,
    .big = true,
    .visible = true,
};

Type *type_string = &(Type) {

};
