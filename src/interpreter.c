
typedef enum {
    STACK_ITEM_STRING,
    STACK_ITEM_BOOL,
    STACK_ITEM_INT,
    STACK_ITEM_CMD_MARKER,
    
    COUNT_STACK_ITEMS
} StackItemType;

typedef struct {
    StackItemType type;
    String string;
    int number;
    Location loc;
} StackItem;

array_define(Stack, StackItem)
array_implement(Stack, StackItem)

void interpret_bytecode(Bytecode* bc) {
    Stack* stack = Stack_new(&arena);
    
    array_foreach(bc, pc) {
        Operation op = Bytecode_get(bc, pc);
        if (op.type == OP_PUSH_STRING) Stack_push(stack, (StackItem) { .type = STACK_ITEM_STRING, .string = op.operand, .loc = op.loc });
        else if (op.type == OP_PUSH_INT) Stack_push(stack, (StackItem) { .type = STACK_ITEM_INT, .number = op.value, .loc = op.loc });
        else if (op.type == OP_PUSH_BOOL) Stack_push(stack, (StackItem) { .type = STACK_ITEM_BOOL, .number = op.value, .loc = op.loc });
        else if (op.type == OP_DEBUG) {
            fprintf(stderr, "DEBUG CRASH\nINITIATED AT "LOC_FMT"\nStack state: %zu items\n", LocFmt(op.loc), stack->size);
            array_foreach(stack, i) {
                StackItem si = Stack_get(stack, i);
                if (si.type == STACK_ITEM_STRING) fprintf(stderr, " `"SV_FMT"` ", SvFmt(si.string));
                if (si.type == STACK_ITEM_INT) fprintf(stderr, " %d ", si.number);
                if (si.type == STACK_ITEM_BOOL) fprintf(stderr, " %s ", si.number ? "true" : "false");
                if (si.type == STACK_ITEM_CMD_MARKER) fprintf(stderr, " CMD MARKER ");
                fprintf(stderr, "\n");
            }
            fprintf(stderr, "STACK ^ TOP\n");
            exit(2);
        } else if (op.type == OP_CMD) Stack_push(stack, (StackItem) { .type = STACK_ITEM_CMD_MARKER, .loc = op.loc });
        else if (op.type == OP_RUN) {
            if (stack->size == 0) lexer_error(op.loc, "the stack is empty");
            int cmd_location = 0;
            for (int i = (int) stack->size-1; i > -1; i--) {
                StackItem si = Stack_get(stack, i);
                if (si.type == STACK_ITEM_CMD_MARKER) { cmd_location = i + 1; break; }
            }
            StringArray* arguments = StringArray_new(&arena);
            for (size_t i = cmd_location + 1; i < stack->size; i++) {
                StackItem si = Stack_get(stack, i);
                if (si.type != STACK_ITEM_STRING) lexer_error(si.loc, "use of a non-string as an argument");
                StringArray_push(arguments, si.string);
            }
            StackItem si = Stack_get(stack, cmd_location);
            if (si.type != STACK_ITEM_STRING) lexer_error(si.loc, "use of a non-string as an program name");
            String program = si.string;
            stack->size = cmd_location;
            StringBuilder* cmd = shell_render_command(&arena, program, arguments);
            printf("CMD: "SV_FMT"\n", SvFmt(sv_from_sb(cmd)));
            exitcode_t exitcode = shell_run_program(&arena, program, arguments);
            if (exitcode != 0) error("command exited with non-zero exitcode");
        } else if (op.type == OP_JUMP) pc = op.location-1;
        else if (op.type == OP_JUMPZ) {
            if (stack->size == 0) lexer_error(op.loc, "expected a boolean on the stack, got nothing");
            StackItem si = Stack_get(stack, stack->size-1);
            Stack_pop(stack);
            if (si.type != STACK_ITEM_BOOL) lexer_error(si.loc, "expected a boolean on the stack, got a non-boolean");
            if (si.number == 0) pc = op.location-1;
        } else if (op.type == OP_JUMPNZ) {
            if (stack->size == 0) lexer_error(op.loc, "expected a boolean on the stack, got nothing");
            StackItem si = Stack_get(stack, stack->size-1);
            Stack_pop(stack);
            if (si.type != STACK_ITEM_BOOL) lexer_error(si.loc, "expected a boolean on the stack");
            if (si.number != 0) pc = op.location-1;
        } else if (op.type == OP_DUP) {
            if (stack->size == 0) lexer_error(op.loc, "expected stack to not be empty");
            StackItem si = Stack_get(stack, stack->size-1);
            Stack_push(stack, si);
        } else if (op.type == OP_DROP) {
            if (stack->size == 0) lexer_error(op.loc, "expected stack to not be empty");
            Stack_pop(stack);
        } else if (op.type == OP_SWAP) {
            if (stack->size <= 1) lexer_error(op.loc, "expected stack to have at least 2 items");
            StackItem a = Stack_get(stack, stack->size-1);
            StackItem b = Stack_get(stack, stack->size-2);
            Stack_pop(stack);
            Stack_pop(stack);
            Stack_push(stack, a);
            Stack_push(stack, b);
        } else if (op.type == OP_OVER) {
            if (stack->size <= 1) lexer_error(op.loc, "expected stack to have at least 2 items");
            StackItem si = Stack_get(stack, stack->size-2);
            Stack_push(stack, si);
        } else if (op.type == OP_ROT) {
            if (stack->size <= 2) lexer_error(op.loc, "expected stack to have at least 3 items");
            StackItem a = Stack_get(stack, stack->size-1);
            StackItem b = Stack_get(stack, stack->size-2);
            StackItem c = Stack_get(stack, stack->size-3);
            stack->size -= 3;
            Stack_push(stack, c);
            Stack_push(stack, a);
            Stack_push(stack, b);
        } else if (op.type == OP_NOT) {
            if (stack->size == 0) lexer_error(op.loc, "expected stack to not be empty");
            StackItem si = Stack_get(stack, stack->size-1);
            if (si.type != STACK_ITEM_BOOL) lexer_error(op.loc, "expected a boolean on the stack");
            Stack_pop(stack);
            Stack_push(stack, (StackItem) { .type = STACK_ITEM_BOOL, .number = !si.number, .loc = op.loc });
        } else if (op.type == OP_GTEQ) {
            if (stack->size <= 1) lexer_error(op.loc, "expected stack to have at least 2 items");
            StackItem b = Stack_get(stack, stack->size-1);
            StackItem a = Stack_get(stack, stack->size-2);
            if (a.type != STACK_ITEM_INT || b.type != STACK_ITEM_INT) lexer_error(op.loc, "expected both values to be integers");
            Stack_pop(stack);
            Stack_pop(stack);
            Stack_push(stack, (StackItem) { .type = STACK_ITEM_BOOL, .number = a.number >= b.number, .loc = op.loc });
        } else if (op.type == OP_LTEQ) {
            if (stack->size <= 1) lexer_error(op.loc, "expected stack to have at least 2 items");
            StackItem b = Stack_get(stack, stack->size-1);
            StackItem a = Stack_get(stack, stack->size-2);
            if (a.type != STACK_ITEM_INT || b.type != STACK_ITEM_INT) lexer_error(op.loc, "expected both values to be integers");
            Stack_pop(stack);
            Stack_pop(stack);
            Stack_push(stack, (StackItem) { .type = STACK_ITEM_BOOL, .number = a.number <= b.number, .loc = op.loc });
        } else if (op.type == OP_GT) {
            if (stack->size <= 1) lexer_error(op.loc, "expected stack to have at least 2 items");
            StackItem b = Stack_get(stack, stack->size-1);
            StackItem a = Stack_get(stack, stack->size-2);
            if (a.type != STACK_ITEM_INT || b.type != STACK_ITEM_INT) lexer_error(op.loc, "expected both values to be integers");
            Stack_pop(stack);
            Stack_pop(stack);
            Stack_push(stack, (StackItem) { .type = STACK_ITEM_BOOL, .number = a.number > b.number, .loc = op.loc });
        } else if (op.type == OP_LT) {
            if (stack->size <= 1) lexer_error(op.loc, "expected stack to have at least 2 items");
            StackItem b = Stack_get(stack, stack->size-1);
            StackItem a = Stack_get(stack, stack->size-2);
            if (a.type != STACK_ITEM_INT || b.type != STACK_ITEM_INT) lexer_error(op.loc, "expected both values to be integers");
            Stack_pop(stack);
            Stack_pop(stack);
            Stack_push(stack, (StackItem) { .type = STACK_ITEM_BOOL, .number = a.number < b.number, .loc = op.loc });
        } else if (op.type == OP_EQ) {
            if (stack->size <= 1) lexer_error(op.loc, "expected stack to have at least 2 items");
            StackItem b = Stack_get(stack, stack->size-1);
            StackItem a = Stack_get(stack, stack->size-2);
            if (a.type != STACK_ITEM_INT || b.type != STACK_ITEM_INT) lexer_error(op.loc, "expected both values to be integers");
            Stack_pop(stack);
            Stack_pop(stack);
            Stack_push(stack, (StackItem) { .type = STACK_ITEM_BOOL, .number = a.number == b.number, .loc = op.loc });
        } else if (op.type == OP_ADD) {
            if (stack->size <= 1) lexer_error(op.loc, "expected stack to have at least 2 items");
            StackItem b = Stack_get(stack, stack->size-1);
            StackItem a = Stack_get(stack, stack->size-2);
            if (a.type != STACK_ITEM_INT || b.type != STACK_ITEM_INT) lexer_error(op.loc, "expected both values to be integers");
            Stack_pop(stack);
            Stack_pop(stack);
            Stack_push(stack, (StackItem) { .type = STACK_ITEM_INT, .number = a.number + b.number, .loc = op.loc });
        } else if (op.type == OP_SUB) {
            if (stack->size <= 1) lexer_error(op.loc, "expected stack to have at least 2 items");
            StackItem b = Stack_get(stack, stack->size-1);
            StackItem a = Stack_get(stack, stack->size-2);
            if (a.type != STACK_ITEM_INT || b.type != STACK_ITEM_INT) lexer_error(op.loc, "expected both values to be integers");
            Stack_pop(stack);
            Stack_pop(stack);
            Stack_push(stack, (StackItem) { .type = STACK_ITEM_INT, .number = a.number - b.number, .loc = op.loc });
        } else if (op.type == OP_MUL) {
            if (stack->size <= 1) lexer_error(op.loc, "expected stack to have at least 2 items");
            StackItem b = Stack_get(stack, stack->size-1);
            StackItem a = Stack_get(stack, stack->size-2);
            if (a.type != STACK_ITEM_INT || b.type != STACK_ITEM_INT) lexer_error(op.loc, "expected both values to be integers");
            Stack_pop(stack);
            Stack_pop(stack);
            Stack_push(stack, (StackItem) { .type = STACK_ITEM_INT, .number = a.number * b.number, .loc = op.loc });
        } else if (op.type == OP_DIV) {
            if (stack->size <= 1) lexer_error(op.loc, "expected stack to have at least 2 items");
            StackItem b = Stack_get(stack, stack->size-1);
            StackItem a = Stack_get(stack, stack->size-2);
            if (a.type != STACK_ITEM_INT || b.type != STACK_ITEM_INT) lexer_error(op.loc, "expected both values to be integers");
            Stack_pop(stack);
            Stack_pop(stack);
            Stack_push(stack, (StackItem) { .type = STACK_ITEM_INT, .number = a.number / b.number, .loc = op.loc });
        } else if (op.type == OP_FILEEXISTS) {
            if (stack->size == 0) lexer_error(op.loc, "expected stack to not be empty");
            StackItem si = Stack_get(stack, stack->size-1);
            if (si.type != STACK_ITEM_STRING) lexer_error(op.loc, "expected a string on the stack");
            bool exists = file_exists(si.string);
            Stack_pop(stack);
            printf("FILEIO: file `"SV_FMT"` %s\n", SvFmt(si.string), exists ? "exists" : "doesn't exist");
            Stack_push(stack, (StackItem) { .type = STACK_ITEM_BOOL, .number = exists, .loc = op.loc });
        } else if (op.type == OP_DIREXISTS) {
            if (stack->size == 0) lexer_error(op.loc, "expected stack to not be empty");
            StackItem si = Stack_get(stack, stack->size-1);
            if (si.type != STACK_ITEM_STRING) lexer_error(op.loc, "expected a string on the stack");
            bool exists = dir_exists(si.string);
            Stack_pop(stack);
            printf("FILEIO: directory `"SV_FMT"` %s\n", SvFmt(si.string), exists ? "exists" : "doesn't exist");
            Stack_push(stack, (StackItem) { .type = STACK_ITEM_BOOL, .number = exists, .loc = op.loc });
        } else if (op.type == OP_MKDIR) {
            if (stack->size == 0) lexer_error(op.loc, "expected stack to not be empty");
            StackItem si = Stack_get(stack, stack->size-1);
            if (si.type != STACK_ITEM_STRING) lexer_error(op.loc, "expected a string on the stack");
            dir_make_directory(si.string);
            Stack_pop(stack);
            printf("FILEIO: created directory `"SV_FMT"`\n", SvFmt(si.string));
        } else if (op.type == OP_CD) {
            if (stack->size == 0) lexer_error(op.loc, "expected stack to not be empty");
            StackItem si = Stack_get(stack, stack->size-1);
            if (si.type != STACK_ITEM_STRING) lexer_error(op.loc, "expected a string on the stack");
            dir_change_cwd(si.string);
            Stack_pop(stack);
            printf("FILEIO: changed cwd to `"SV_FMT"`\n", SvFmt(si.string));
        } else if (op.type == OP_GETCWD) {
            String cwd = dir_get_cwd(&arena);
            Stack_pop(stack);
            printf("FILEIO: cwd = `"SV_FMT"`\n", SvFmt(cwd));
            Stack_push(stack, (StackItem) { .type = STACK_ITEM_STRING, .string = cwd, .loc = op.loc });
        } else if (op.type == OP_LISTDIR) {
            if (stack->size == 0) lexer_error(op.loc, "expected stack to not be empty");
            StackItem si = Stack_get(stack, stack->size-1);
            if (si.type != STACK_ITEM_STRING) lexer_error(op.loc, "expected a string on the stack");
            Stack_pop(stack);
            StringArray* content = dir_list(si.string, &arena);
            printf("FILEIO: listed `"SV_FMT"`\n", SvFmt(si.string));
            array_foreach(content, i) {
                String dir = StringArray_get(content, i);
                Stack_push(stack, (StackItem) { .type = STACK_ITEM_STRING, .string = dir, .loc = op.loc });
            }
            Stack_push(stack, (StackItem) { .type = STACK_ITEM_INT, .number = content->size });
        } else if (op.type == OP_FNMATCH) {
            if (stack->size == 0) lexer_error(op.loc, "expected stack to not be empty");
            StackItem si = Stack_get(stack, stack->size-1);
            if (si.type != STACK_ITEM_STRING) lexer_error(op.loc, "expected a string on the stack");
            Stack_pop(stack);
            StringArray* content = dir_fnmatch(si.string, &arena);
            printf("FILEIO: fnmatched `"SV_FMT"`\n", SvFmt(si.string));
            array_foreach(content, i) {
                String dir = StringArray_get(content, i);
                Stack_push(stack, (StackItem) { .type = STACK_ITEM_STRING, .string = dir, .loc = op.loc });
            }
            Stack_push(stack, (StackItem) { .type = STACK_ITEM_INT, .number = content->size });
        } else if (op.type == OP_LOG) {
            if (stack->size == 0) lexer_error(op.loc, "expected stack to not be empty");
            StackItem si = Stack_get(stack, stack->size-1);
            if (si.type != STACK_ITEM_STRING) lexer_error(op.loc, "expected a string on the stack");
            Stack_pop(stack);
            printf("INFO: "SV_FMT"\n", SvFmt(si.string));
        } else if (op.type == OP_ERROR) {
            if (stack->size == 0) lexer_error(op.loc, "expected stack to not be empty");
            StackItem si = Stack_get(stack, stack->size-1);
            if (si.type != STACK_ITEM_STRING) lexer_error(op.loc, "expected a string on the stack");
            Stack_pop(stack);
            error(SV_FMT, SvFmt(si.string));
        } else if (op.type == OP_PRINT) {
            if (stack->size == 0) lexer_error(op.loc, "expected stack to not be empty");
            StackItem si = Stack_get(stack, stack->size-1);
            if (si.type != STACK_ITEM_STRING) lexer_error(op.loc, "expected a string on the stack");
            Stack_pop(stack);
            printf(SV_FMT, SvFmt(si.string));
        } else lexer_error(op.loc, "intrinsic %d is not implemented", op.type);
    }
}
