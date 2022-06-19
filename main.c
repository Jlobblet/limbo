#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"

int main() {
    char *num = "621";
    SourceFile file = {
        .name = "test",
        .contents = num,
        .file_number = 1
    };
    LexerContext context = {
        .source_file = &file,
        .at_beginning_of_line = true,
        .follows_space = false,
    };
    char *position;
    Token *token = read_number_literal(&context, num, &position);
    printf("%ld\n", token->int_value);

    num = "420.69";
    token = read_number_literal(&context, num, &position);
    printf("%f\n", token->real_value);

    return EXIT_SUCCESS;
}
