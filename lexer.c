#include <stdlib.h>
#include "lexer.h"

static bool Token_new(Token *self, TokenKind kind, char *start, char *end) {
    self->kind = kind;
    self->next = NULL;
    self->location = start;
    self->length = end - start;
    self->string_value = NULL;
    self->at_beginning_of_line = self->follows_space = false;
    return true;
}
