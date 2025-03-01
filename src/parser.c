
typedef enum {
    OP_NOP = 0,
    OP_PUSH_STRING,
    OP_DEBUG,
    OP_CMD,
    OP_RUN,
    OP_JUMP,
    OP_JUMPZ,
    OP_JUMPNZ,
    OP_DUP,
    OP_SWAP,
    OP_OVER,
    OP_ROT,
    OP_NOT,
    OP_FILEEXISTS,
    OP_DIREXISTS,
    OP_MKDIR,
    OP_CD,
    OP_GETCWD,
    OP_LOG,
    OP_ERROR,
    OP_PRINT,
    COUNT_OPS
} OpType;

typedef struct {
    OpType type;
    String operand;
    size_t location;
    Location loc;
} Operation;

array_define(Bytecode, Operation)
array_implement(Bytecode, Operation)

typedef struct {
    String name;
    size_t start, end;
} Macro;

array_define(MacroArray, Macro)
array_implement(MacroArray, Macro)

void parse_bytecode_indexed(TokenArray* tokens, size_t start, size_t end, Bytecode* bc, MacroArray* ma, size_t depth) {
    if (depth >= 100) error("too much macro expansion depth");
    for (size_t i = start; i < end; i++) {
        Token token = TokenArray_get(tokens, i);
        if (token.type == TOKEN_MACRO) {
            i++; Token name = TokenArray_get(tokens, i);
            if (name.type != TOKEN_WORD) {
                lexer_error(name.loc, "expected a name, got `"SV_FMT"`", SvFmt(name.content));
            }
            String macro_name = name.content;
            i++; Token ocurly = TokenArray_get(tokens, i);
            if (ocurly.type != TOKEN_OCURLY) {
                lexer_error(ocurly.loc, "expected a `{`, got `"SV_FMT"`", SvFmt(ocurly.content));
            }
            bool macro_found = false;
            array_foreach(ma, m) {
                Macro macro = MacroArray_get(ma, m);
                if (sv_compare(macro.name, macro_name)) {
                    macro_found = true;
                    break;
                }
            }
            if (macro_found) lexer_error(name.loc, "macro redefinition");
            Macro macro = { macro_name, i + 1, ocurly.corresponding };
            MacroArray_push(ma, macro);
            i = ocurly.corresponding;
        } else if (token.type == TOKEN_STRING) {
            Operation op = { .type = OP_PUSH_STRING, .operand = token.content, .loc = token.loc };
            Bytecode_push(bc, op);
        } else if (token.type == TOKEN_DEBUG) {
            Operation op = { .type = OP_DEBUG, .loc = token.loc };
            Bytecode_push(bc, op);
        } else if (token.type == TOKEN_IF) {
            i++; Token ocurly = TokenArray_get(tokens, i);
            if (ocurly.type != TOKEN_OCURLY) {
                lexer_error(ocurly.loc, "expected a `{`, got `"SV_FMT"`", SvFmt(ocurly.content));
            }
            
            size_t jz_index = bc->size;
            Bytecode_push(bc, (Operation) { .type = OP_JUMPZ, .location = 0, .loc = token.loc });

            parse_bytecode_indexed(tokens, i + 1, ocurly.corresponding, bc, ma, depth + 1);

            Operation jz = Bytecode_get(bc, jz_index);
            jz.location = bc->size;
            Bytecode_set(bc, jz_index, jz);
            i = ocurly.corresponding;

            if (i >= end) break;
            i++; Token else_token = TokenArray_get(tokens, i);
            if (else_token.type != TOKEN_ELSE) { i--; continue; }
            
            i++; Token else_ocurly = TokenArray_get(tokens, i);
            if (else_ocurly.type != TOKEN_OCURLY) {
                lexer_error(ocurly.loc, "expected a `{`, got `"SV_FMT"`", SvFmt(ocurly.content));
            }

            size_t j_index = bc->size;
            Bytecode_push(bc, (Operation) { .type = OP_JUMP, .location = 0, .loc = else_token.loc });

            parse_bytecode_indexed(tokens, i + 1, else_ocurly.corresponding, bc, ma, depth + 1);

            Operation j = Bytecode_get(bc, j_index);
            j.location = bc->size;
            Bytecode_set(bc, j_index, j);
            i = else_ocurly.corresponding;
            
        } else if (token.type == TOKEN_WORD) {
            // Assume macro expansion
            bool macro_found = false;
            Macro macro;
            array_foreach(ma, m) {
                macro = MacroArray_get(ma, m);
                if (sv_compare(macro.name, token.content)) {
                    macro_found = true;
                    break;
                }
            }
            if (!macro_found) lexer_error(token.loc, "no such macro as `"SV_FMT"`", SvFmt(token.content));
            parse_bytecode_indexed(tokens, macro.start, macro.end, bc, ma, depth+1);
        } else if (token.type == TOKEN_BANG) Bytecode_push(bc, (Operation) { .type = OP_NOT, .loc = token.loc });
        else if (token_is_wt(token)) {
            // Intrinsic call
            Operation op = { .loc = token.loc };
            if (token.type == TOKEN_RUN) op.type = OP_RUN;
            else if (token.type == TOKEN_CMD) op.type = OP_CMD;
            else if (token.type == TOKEN_DUP) op.type = OP_DUP;
            else if (token.type == TOKEN_SWAP) op.type = OP_SWAP;
            else if (token.type == TOKEN_OVER) op.type = OP_OVER;
            else if (token.type == TOKEN_ROT) op.type = OP_ROT;
            else if (token.type == TOKEN_FILEEXISTS) op.type = OP_FILEEXISTS;
            else if (token.type == TOKEN_DIREXISTS) op.type = OP_DIREXISTS;
            else if (token.type == TOKEN_MKDIR) op.type = OP_MKDIR;
            else if (token.type == TOKEN_CD) op.type = OP_CD;
            else if (token.type == TOKEN_GETCWD) op.type = OP_GETCWD;
            else if (token.type == TOKEN_LOG) op.type = OP_LOG;
            else if (token.type == TOKEN_ERROR) op.type = OP_ERROR;
            else if (token.type == TOKEN_PRINT) op.type = OP_PRINT;
            else lexer_error(token.loc, "intrinsic `"SV_FMT"` is not mapped to an operation", SvFmt(token.content));
            Bytecode_push(bc, op);
        } else {
            lexer_error(token.loc, "unexpected `"SV_FMT"`", SvFmt(token.content));
        }
    }
}

Bytecode* parse_bytecode(TokenArray* tokens) {
    Bytecode* bc = Bytecode_new(&arena);
    MacroArray* ma = MacroArray_new(&arena);
    parse_bytecode_indexed(tokens, 0, tokens->size, bc, ma, 0);
    return bc;
}
