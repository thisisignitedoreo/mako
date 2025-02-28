
typedef struct {
    String filename, content;
    size_t cursor, l, c;
} Lexer;

Lexer lexer_new(String filename, String content) {
    return (Lexer) {
        .filename = filename,
        .content = content,
    };
}

typedef enum {
    TOKEN_EOF = 0,

    // Literal tokens
    TOKEN__LT_START,
    
    TOKEN_OCURLY,
    TOKEN_CCURLY,
    TOKEN_BANG,

    TOKEN__LT_END,

    // Word tokens
    TOKEN__WT_START,

    TOKEN_DEBUG,
    
    TOKEN_MACRO,
    TOKEN_CMD,
    TOKEN_RUN,

    TOKEN_IF,
    TOKEN_ELSE,

    TOKEN_DUP,
    TOKEN_SWAP,
    TOKEN_OVER,
    TOKEN_ROT,

    TOKEN_FILEEXISTS,
    TOKEN_DIREXISTS,
    TOKEN_MKDIR,
    TOKEN_CD,
    TOKEN_GETCWD,
    
    TOKEN_LOG,
    TOKEN_ERROR,
    TOKEN_PRINT,

    TOKEN__WT_END,

    // Generic tokens
    TOKEN__GT_START,

    TOKEN_STRING,
    TOKEN_WORD,
    TOKEN_INTEGER,
    
    TOKEN__GT_END,

    COUNT_TOKENS,
} MakoTokenType; // FUCKING WIN API!

#define token_is_lt(token) (token).type > TOKEN__LT_START && (token).type < TOKEN__LT_END
#define token_is_wt(token) (token).type > TOKEN__WT_START && (token).type < TOKEN__WT_END
#define token_is_gt(token) (token).type > TOKEN__GT_START && (token).type < TOKEN__GT_END

typedef struct {
    String filename;
    size_t l, c;
} Location;

#define LOC_FMT SV_FMT":%zu:%zu"
#define LocFmt(loc) SvFmt((loc).filename), (loc).l+1, (loc).c+1

void lexer_error(Location loc, char* fmt, ...) {
    fprintf(stderr, LOC_FMT": ERROR: ", LocFmt(loc));
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(1);
}

typedef struct {
    MakoTokenType type;
    String content;
    int value;
    Location loc;
    size_t corresponding;
} Token;

bool lexer_done(Lexer* lexer) {
    return lexer->cursor >= lexer->content.size;
}

char lexer_char(Lexer* lexer) {
    if (lexer_done(lexer)) return 0;
    return sv_index(lexer->content, lexer->cursor);
}

char lexer_chop_char(Lexer* lexer) {
    if (lexer_done(lexer)) return 0;
    char charachter = sv_index(lexer->content, lexer->cursor);
    if (charachter == '\n') { lexer->l++; lexer->c = 0; }
    else lexer->c++;
    lexer->cursor++;
    return charachter;
}

void lexer_strip_whitespace(Lexer* lexer) {
    while (!lexer_done(lexer) && isspace(lexer_char(lexer))) lexer_chop_char(lexer);
}

Location lexer_loc(Lexer* lexer) {
    return (Location) { lexer->filename, lexer->l, lexer->c };
}

bool lexer_startswith(Lexer* lexer, String str) {
    return sv_compare_at(lexer->content, str, lexer->cursor);
}

void lexer_drop_line(Lexer* lexer) {
    while (!lexer_done(lexer) && lexer_char(lexer) != '\n') lexer_chop_char(lexer);
    if (!lexer_done(lexer)) lexer_chop_char(lexer);
}

#define literal_token(token, token_type) \
    else if (lexer_startswith(lexer, (token))) { \
        Location loc = lexer_loc(lexer); \
        String content = lexer->content; \
        content.bytes += lexer->cursor; \
        content.size = (token).size; \
        for (size_t i = 0; i < (token).size; i++) lexer_chop_char(lexer); \
        return (Token) { .type = (token_type), .content = content, .loc = loc }; \
    }

Token lexer_next_token(Lexer* lexer) {
    lexer_strip_whitespace(lexer);
    if (lexer_done(lexer)) return (Token) { .type = TOKEN_EOF, .loc = lexer_loc(lexer) };

    while (lexer_startswith(lexer, sv("#"))) {
        lexer_drop_line(lexer);
        lexer_strip_whitespace(lexer);
        if (lexer_done(lexer)) return (Token) { .type = TOKEN_EOF, .loc = lexer_loc(lexer) };
    }

    if (false) {} // else if chain start
    literal_token(sv("{"), TOKEN_OCURLY)
    literal_token(sv("}"), TOKEN_CCURLY)
    literal_token(sv("!"), TOKEN_BANG)
    else if (isalpha(lexer_char(lexer))) {
        Location loc = lexer_loc(lexer);
        String string = sv_from_bytes(lexer->content.bytes + lexer->cursor, 0);
        while (!lexer_done(lexer) && isalnum(lexer_char(lexer))) { string.size++; lexer_chop_char(lexer); }
        MakoTokenType token_type = TOKEN_WORD;
        if (sv_compare(string, sv("debug"))) token_type = TOKEN_DEBUG;
        if (sv_compare(string, sv("macro"))) token_type = TOKEN_MACRO;
        if (sv_compare(string, sv("cmd"))) token_type = TOKEN_CMD;
        if (sv_compare(string, sv("run"))) token_type = TOKEN_RUN;
        if (sv_compare(string, sv("if"))) token_type = TOKEN_IF;
        if (sv_compare(string, sv("else"))) token_type = TOKEN_ELSE;
        if (sv_compare(string, sv("dup"))) token_type = TOKEN_DUP;
        if (sv_compare(string, sv("swap"))) token_type = TOKEN_SWAP;
        if (sv_compare(string, sv("over"))) token_type = TOKEN_OVER;
        if (sv_compare(string, sv("rot"))) token_type = TOKEN_ROT;
        if (sv_compare(string, sv("fileexists"))) token_type = TOKEN_FILEEXISTS;
        if (sv_compare(string, sv("direxists"))) token_type = TOKEN_DIREXISTS;
        if (sv_compare(string, sv("mkdir"))) token_type = TOKEN_MKDIR;
        if (sv_compare(string, sv("log"))) token_type = TOKEN_LOG;
        if (sv_compare(string, sv("error"))) token_type = TOKEN_ERROR;
        if (sv_compare(string, sv("print"))) token_type = TOKEN_PRINT;
        return (Token) { .type = token_type, .content = string, .loc = loc };
    } else if (isdigit(lexer_char(lexer)) || lexer_char(lexer) == '-') {
        Location loc = lexer_loc(lexer);
        String string = sv_from_bytes(lexer->content.bytes + lexer->cursor, 0);
        while (!lexer_done(lexer) && (isdigit(lexer_char(lexer)) || lexer_char(lexer) == '-')) { string.size++; lexer_chop_char(lexer); }
        return (Token) { .type = TOKEN_INTEGER, .content = string, .value = sv_to_int(string), .loc = loc };
    } else if (lexer_char(lexer) == '"' || lexer_char(lexer) == '\'') {
        char quote = lexer_char(lexer);
        StringBuilder* str = StringBuilder_new(&arena);
        Location loc = lexer_loc(lexer);
        lexer_chop_char(lexer);
        while (lexer_char(lexer) != quote) {
            if (lexer_done(lexer)) lexer_error(loc, "unclosed string literal");
            if (lexer_char(lexer) == '\\') {
                lexer_chop_char(lexer);
                if (lexer_done(lexer)) lexer_error(loc, "expected escape sequence, got EOF");
                char seq = lexer_char(lexer);
                if (seq == 'r') StringBuilder_push(str, '\r');
                else if (seq == 'n') StringBuilder_push(str, '\n');
                else if (seq == 't') StringBuilder_push(str, '\t');
                else if (seq == '\"') StringBuilder_push(str, '\"');
                else if (seq == '\'') StringBuilder_push(str, '\'');
                else lexer_error(lexer_loc(lexer), "undefined escape sequence");
                lexer_chop_char(lexer);
            } else if (lexer_char(lexer) == '\n') {
                lexer_error(lexer_loc(lexer), "unclosed string literal");
            } else {
                StringBuilder_push(str, lexer_char(lexer));
                lexer_chop_char(lexer);
            }
        }
        lexer_chop_char(lexer);
        return (Token) { .type = TOKEN_STRING, .content = sv_from_sb(str), .loc = loc };
    } else {
        lexer_error(lexer_loc(lexer), "unexpected charachter: `%c`", lexer_char(lexer));
    }

    return (Token) {0};
}

array_define(TokenArray, Token)
array_implement(TokenArray, Token)

TokenArray* lexer_tokenize(Lexer* lexer) {
    TokenArray* ta = TokenArray_new(&arena);
    Token token = lexer_next_token(lexer);
    while (token.type) {
        TokenArray_push(ta, token);
        token = lexer_next_token(lexer);
    }
    return ta;
}

void lexer_crossreference(TokenArray* tokens) {
    U32Array* stack = U32Array_new(&arena);
    array_foreach(tokens, i) {
        Token token = TokenArray_get(tokens, i);
        if (token.type == TOKEN_OCURLY) U32Array_push(stack, i);
        if (token.type == TOKEN_CCURLY) {
            if (stack->size == 0) lexer_error(token.loc, "stray `}`");
            size_t index = U32Array_get(stack, stack->size-1); U32Array_pop(stack);
            token.corresponding = index;
            TokenArray_set(tokens, i, token);
            Token open = TokenArray_get(tokens, index);
            open.corresponding = i;
            TokenArray_set(tokens, index, open);
        }
    }
    if (stack->size != 0) lexer_error(TokenArray_get(tokens, U32Array_get(stack, 0)).loc, "stray {");
}
