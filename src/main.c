
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

#include "stringview.h"
#include "arena.h"
#include "fileio.h"
#include "shell.h"

Arena arena = {0};

#define DEFAULT_BUILD_FILE "build.mako"

void error(char* fmt, ...) {
    fprintf(stderr, "ERROR: ");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(1);
}

String shift_args(int* argc, char*** argv) {
    if (*argc == 0) return sv_from_bytes(NULL, 0);
    *argc -= 1;
    char* arg = **argv;
    *argv += 1;
    return sv(arg);
}

typedef enum {
    DEFAULT = 0,
    TOKENIZE,
    PARSE,

    PROGRAM_MODES
} ProgramMode;

typedef struct {
    ProgramMode mode;
} Flags;

void print_help(String program) {
    printf("USAGE: "SV_FMT" [filename] [--tokenize] [--parse] [--help]\n", SvFmt(program));
    printf("  filename: defaults to `"DEFAULT_BUILD_FILE"`\n");
    printf("  --tokenize: don't interpret file; just tokenize it instead\n");
    printf("  --parse: don't interpret file; just parse it instead\n");
    printf("  --help: print this and exit\n");
}

String parse_args(int argc, char** argv, Flags* flags) {
    String program = shift_args(&argc, &argv);
    
    String filename = {0};
    bool fn_selected = false;
    *flags = (Flags) {0};
    while (argc > 0) {
        String arg = shift_args(&argc, &argv);
        if      (sv_compare(arg, sv("--tokenize"))) flags->mode = TOKENIZE;
        else if (sv_compare(arg, sv("--parse"))) flags->mode = PARSE;
        else if (sv_compare(arg, sv("--help"))) { print_help(program); exit(0); }
        else {
            if (fn_selected) error("multiple files at once is not supported yet");
            fn_selected = true;
            filename = arg;
        }
    }
    
    if (!fn_selected) filename = sv(DEFAULT_BUILD_FILE);
    else filename = shift_args(&argc, &argv);
    
    if (!file_exists(filename)) error("no file named `"SV_FMT"`", SvFmt(filename));
    return filename;
}

#include "lexer.c"
#include "parser.c"
#include "interpreter.c"

int main(int argc, char** argv) {
    Flags flags = {0};
    String filename = parse_args(argc, argv, &flags);
    String content = file_read(filename, &arena);

    Lexer lexer = lexer_new(filename, content);
    TokenArray* tokens = lexer_tokenize(&lexer);
    lexer_crossreference(tokens);

    if (flags.mode == TOKENIZE) {
        array_foreach(tokens, i) {
            Token token = TokenArray_get(tokens, i);
            printf(LOC_FMT": %zu->%zu; `"SV_FMT"`/%d (%d)\n", LocFmt(token.loc), i, token.corresponding, SvFmt(token.content), token.value, token.type);
        }
        return 0;
    }

    Bytecode* bytecode = parse_bytecode(tokens);
    
    if (flags.mode == PARSE) {
        array_foreach(bytecode, i) {
            Operation op = Bytecode_get(bytecode, i);
            printf(LOC_FMT": %zu: `"SV_FMT"`/%zu (%d)\n", LocFmt(op.loc), i, SvFmt(op.operand), op.location, op.type);
        }
        return 0;
    }

    interpret_bytecode(bytecode);

    arena_free(&arena);
    
    return 0;
}
