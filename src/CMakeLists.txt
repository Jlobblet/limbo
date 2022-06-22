add_executable(limbo main.c lexer.c lexer.h unicode.c unicode.h num.c num.h error.c error.h parser.c parser.h type.c type.h)
target_link_libraries(limbo m)
