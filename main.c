#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"

void print_token(Token* token) {
    char *kind;
    char buffer[100] = { 0 };
    switch (token->kind) {
        case TOKEN_IDENTIFIER:
            strncpy(buffer, token->location, token->length);
            buffer[token->length] = '\0';
            kind = "IDENTIFIER";
            break;
        case TOKEN_PUNCTUATOR:
            strncpy(buffer, token->location, token->length);
            buffer[token->length] = '\0';
            kind = "PUNCTUATOR";
            break;
        case TOKEN_KEYWORD:
            strncpy(buffer, token->location, token->length);
            buffer[token->length] = '\0';
            kind = "KEYWORD";
            break;
        case TOKEN_STRING:
            strncpy(buffer, token->string_value, token->length);
            buffer[token->length] = '\0';
            kind = "STRING";
            break;
        case TOKEN_INTEGRAL:
            sprintf(buffer, "%ld", token->int_value);
            kind = "INTEGRAL";
            break;
        case TOKEN_REAL:
            sprintf(buffer, "%f", token->real_value);
            kind = "REAL";
            break;
        case TOKEN_EOF:
            kind = "EOF";
            break;
    }

    printf("%s at (%lu, %lu): %s\n", kind,
           token->source_file_line, token->source_file_column, buffer);
}

int main() {
    char *program =
            "implement Command;\n"
            "include \"sys.m\";\n"
            "include \"draw.m\";\n"
            "sys:    Sys;\n"
            "Command: module {\n"
            "    init: fn (ctxt: ref Draw->Context, argv: list of string);\n"
            "};\n"
            "# The canonical \"Hello world\" program, enhanced\n"
            "init(ctxt: ref Draw->Context, argv: list of string)\n"
            "{\n"
            "    sys = load Sys Sys->PATH;\n"
            "    sys->print(\"hello world!\\n\");\n"
            "    for (; argv!=nil; argv = tl argv)\n"
            "        sys->print(\"%s \", hd argv);\n"
            "    sys->print(\"\\n\");\n"
            "}\n";
    SourceFile file = {
        .name = "test.m",
        .contents = program,
        .file_number = 1
    };
    Token *head = lex(&file), *current = head;

    while (current != NULL) {
        print_token(current);
        current = current->next;
    }

    Token_free(head);

    return EXIT_SUCCESS;
}
