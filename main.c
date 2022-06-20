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

    printf("%s: %s\n", kind, buffer);
}

int main() {
    char *program =
            "implement Command;"
            "include \"sys.m\";"
            "include \"draw.m\";"
            "sys:    Sys;"
            "Command: module {"
            "    init: fn (ctxt: ref Draw->Context, argv: list of string);"
            "};"
            "# The canonical \"Hello world\" program, enhanced"
            "init(ctxt: ref Draw->Context, argv: list of string)"
            "{"
            "    sys = load Sys Sys->PATH;"
            "    sys->print(\"hello world!\\n\");"
            "    for (; argv!=nil; argv = tl argv)"
            "        sys->print(\"%s \", hd argv);"
            "    sys->print(\"\\n\");"
            "}";
    SourceFile file = {
        .name = "test.m",
        .contents = program,
        .file_number = 1
    };
    Token *head = lex(&file);

    while (head != NULL) {
        print_token(head);
        head = head->next;
    }

    return EXIT_SUCCESS;
}
