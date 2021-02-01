#include "compiling/compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scanning/scanner.h"
#include "code/opcodes.h"
#include "code/value.h"
#include "code/object.h"
#include "bool.h"

#include "debugging/switches.h"
#ifdef DEBUG_SHOW_COMPILED_CODE
#include "debugging/disassembly.h"
#endif

#define MAX_BYTE_COUNT (UINT8_MAX + 1)

typedef struct {
    Token previous;
    Token current;
    bool had_error;
    bool panic_mode;
} Parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool can_assign);

typedef struct {
    ParseFn prefix_fn;
    ParseFn infix_fn;
    Precedence precedence;
} ParseRule;

typedef struct {
    Token name;
    int depth;
} Local;

typedef struct {
    Local locals[MAX_BYTE_COUNT];
    int locals_count;
    int cur_scope_depth; // the current scope depth
} Compiler;

Parser parser;
Chunk* _compiling_chunk;
static inline Chunk* current_chunk() { return _compiling_chunk; }
Compiler* compiler = NULL;

static void init_compiler(Compiler* _compiler) {
    compiler = _compiler;
    compiler->locals_count = 0;
    compiler->cur_scope_depth = 0;
}

/* Error handling */
#if 1
static void error_token(Token* token, const char* msg) {
    if (parser.panic_mode) return;
    parser.panic_mode = true;
    fprintf(stderr, "[line %d] Syntax error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", msg);
    parser.had_error = true;
}
#endif

/* Token navigation helpers */
#if 1
static void advance() {
    parser.previous = parser.current;
    // jump through all consective error tokens at once in one advance call
    for (;;) {
        parser.current = scan_token();
        if (parser.current.type == TOKEN_ERROR) {
            // report error
            error_token(&parser.current, parser.current.start);
        } else {
            break;
        }
    }
}

static inline bool check_token(TokenType type) { return parser.current.type == type; }

static bool match(TokenType type)
{
    if (check_token(type)) {
        advance();
        return true;
    }
    return false;
}

static void consume(TokenType type, const char* msg)
{
    if (parser.current.type == type) {
        advance();
        return;
    }

    error_token(&parser.current, msg);
}
#endif

/* Code generation helpers */
#if 1
static inline int store_constant(Value val) {
    int constant_loc = chunk_addconst(current_chunk(), val);
    if (constant_loc > UINT8_MAX) {
        error_token(&parser.previous, "Too many constants in one chunk");
    }
    return constant_loc;
}

static inline void emit_byte(byte_t byte) {
    chunk_write(current_chunk(), byte);
}
static inline void emit_bytes(byte_t byte1, byte_t byte2) {
    emit_byte(byte1);
    emit_byte(byte2);
}

static inline void emit_const(Value val) {
    byte_t constant_loc = (byte_t)store_constant(val);
    emit_bytes(OP_LOADCONST, constant_loc);
}


/* =========== JUMPS =========== */
// return the index of the immediate next byte after the jump
static int emit_jump(int jmp_opcode) {
    emit_byte(jmp_opcode);
    emit_bytes(0xff, 0xff);
    return current_chunk()->count - 2;
}

static void patch_jump(int jmp_opcode_offset) {
    Chunk* cnk = current_chunk();

    // calculate the offset
    // at this time count is the index of the taget (future) instruction
    unsigned int offset = cnk->count - jmp_opcode_offset - 2;
    
    if (offset > UINT16_MAX) 
    {
        error_token(&parser.previous, "Too long jump.");
        return;
    }

    cnk->code[jmp_opcode_offset] = (offset >> 8) & 0xff;
    cnk->code[jmp_opcode_offset + 1] = offset & 0xff;
}

static void emit_loop(int start_offset) {
    unsigned int jmp_offset = current_chunk()->count - start_offset + 3;

    if (jmp_offset > UINT16_MAX) {
        error_token(&parser.previous, "Loop body too large.");
    }

    emit_byte(OP_LOOP);
    emit_byte((jmp_offset >> 8) & 0xff);
    emit_byte(jmp_offset & 0xff);
}
#endif

/* Pratt's Parser */
static void parse_precedence(Precedence min_prec);
static ParseRule* get_rule(TokenType token_type);


/* Actual compilation logic */
#if 1 /* ==========EXPRESSIONS============= */
static inline void cmpl_expression() {
    parse_precedence(PREC_ASSIGNMENT);
}

static void cmpl_number(bool can_assign) {
    double val = strtod(parser.previous.start, NULL);
    emit_const(MK_VAL_NUM(val));
}
static void cmpl_unary(bool can_assign) {
    TokenType operator_type = parser.previous.type;

    parse_precedence(PREC_UNARY);

    switch (operator_type) {
        case TOKEN_MINUS:   emit_byte(OP_NEGATE);       break;
        case TOKEN_BANG:    emit_byte(OP_LOGIC_NOT);    break;
        default: break; // Unreachable  
    }
}
static void cmpl_binary(bool can_assign) {
    TokenType infix_oper_type = parser.previous.type;

    ParseRule* rule = get_rule(infix_oper_type);
    parse_precedence((Precedence)(rule->precedence + 1));

    switch (infix_oper_type) {
        case TOKEN_PLUS:    emit_byte(OP_ADD);      break;
        case TOKEN_MINUS:   emit_byte(OP_SUBTRACT); break;
        case TOKEN_STAR:    emit_byte(OP_MULTIPLY); break;
        case TOKEN_SLASH:   emit_byte(OP_DIVIDE);   break;
        
        case TOKEN_EQUAL_EQUAL: emit_byte(OP_LOGIC_EQUAL);      break;
        case TOKEN_GREATER:     emit_byte(OP_LOGIC_GREATER);    break;
        case TOKEN_LESS:        emit_byte(OP_LOGIC_LESS);       break;

        case TOKEN_BANG_EQUAL:      emit_bytes(OP_LOGIC_EQUAL, OP_LOGIC_NOT);   break;
        case TOKEN_GREATER_EQUAL:   emit_bytes(OP_LOGIC_LESS, OP_LOGIC_NOT);    break;
        case TOKEN_LESS_EQUAL:      emit_bytes(OP_LOGIC_GREATER, OP_LOGIC_NOT); break;
        default: break; // Unreachable
    }
}
static void cmpl_grouping(bool can_assign) {
    cmpl_expression();
    consume(TOKEN_RIGHT_PAREN, "Expected closing ')'");
}
static void cmpl_literal(bool can_assign) {
    switch(parser.previous.type) {
        case TOKEN_TRUE:    emit_byte(OP_TRUE);     break;
        case TOKEN_FALSE:   emit_byte(OP_FALSE);    break;
        case TOKEN_NIL:     emit_byte(OP_NIL);      break;
        default: break; // Unreachable
    }
}

static void cmpl_string(bool can_assign) {
    emit_const(MK_VAL_OBJ(copy_string( // skip the start and end quotes
        parser.previous.start + 1, 
        parser.previous.length - 2
    )));
}

static void cmpl_lgc_and(bool can_assign) {
    int jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);
    parse_precedence(PREC_AND);
    patch_jump(jump);
}

static void cmpl_lgc_or(bool can_assign) {
    int jump = emit_jump(OP_JUMP_IF_TRUE);
    emit_byte(OP_POP);
    parse_precedence(PREC_OR);
    patch_jump(jump);
}

// ========= VARIABLES AND IDENTIFIERS===============

static inline int identifier_constant(Token* name) {
    return store_constant(MK_VAL_OBJ(copy_string(name->start, name->length)));
}

// reads the next variable identifier, stores it in constants table, and returns it location 
static inline byte_t parse_variable(const char* msg) {
    consume(TOKEN_IDENTIFIER, msg);
    if (compiler->cur_scope_depth > 0) return 0;
    return (byte_t) identifier_constant(&parser.previous);
}

static inline void consume_semicolon() { consume(TOKEN_SEMICOLON, "Expected ';' after statment."); }

static inline bool identifiers_equal(Token* a, Token* b) {
    if (a->length != b->length) 
        return false;
    return memcmp(a->start, b->start, a->length) == 0;
}


static int resolve_local(Token* name) {
    for (int i = compiler->locals_count - 1; i >= 0; i--) {
        Local* local = compiler->locals + i;
        if (identifiers_equal(name, &local->name)) {

            if (local->depth == -1) {
                return -2;
            }

            return i;
        }
    }

    return -1;
}

static void cmpl_variable(bool can_assign) {
    // byte_t varloc = identifier_constant(&parser.previous); 
    byte_t get_op, set_op;
    // error_token(name, "Cannot access variable in its own initializer.");
    Token* name = &parser.previous;
    int varloc = resolve_local(name);
    // defined but not yet initialized
    if (varloc == -2) {
        error_token(name, "Cannot access variable in its own initializer.");
        return;
    }
    // global variable
    else if (varloc == -1) {
        get_op = OP_GET_GLOBAL;
        set_op = OP_SET_GLOBAL;
        varloc = (int)identifier_constant(name);
    }
    // local variable
    else {
        get_op = OP_GET_LOCAL;
        set_op = OP_SET_LOCAL;
    }


    if (can_assign && match(TOKEN_EQUAL)) {
        cmpl_expression();
        emit_bytes(set_op, (byte_t)varloc);
    }
    else {
        emit_bytes(get_op, (byte_t)varloc);
    }
}


#endif

#if 1 /* ==========STATEMENTS============= */
// forward declare because they refer each other
static void cmpl_statement();
static void cmpl_declaration();

static inline void begin_scope() { compiler->cur_scope_depth++; }
static void end_scope() { 
    int scope_local_count = 0;
    while (compiler->locals_count > 0 && compiler->locals[compiler->locals_count - 1].depth == compiler->cur_scope_depth) {
        scope_local_count++;
        compiler->locals_count--;
    }
    compiler->cur_scope_depth--;

    switch(scope_local_count) {
        case 0: break;  // do nothing
        case 1: emit_byte(OP_POP); break;
        default: emit_bytes(OP_POPN, (byte_t)scope_local_count); break;
    }
}

static inline void mark_initialized() {
    compiler->locals[compiler->locals_count - 1].depth = compiler->cur_scope_depth;
}

// stores the local name in locals array
static void declare_local(Token name) {

    if (compiler->cur_scope_depth == 0)
        return;

    if (compiler->locals_count > MAX_BYTE_COUNT) {
        error_token(&name, "Too many locals in a scope.");
        return;
    }

    for (int i = compiler->locals_count - 1; i >= 0; i--) {
        Local* local = compiler->locals + i;
        if (compiler->cur_scope_depth != local->depth) {
            break;
        }
        if (identifiers_equal(&name, &local->name)) {
            error_token(&name, "Variable with the same name already exists in the given scope.");
            return;
        }
    }

    Local* local = &compiler->locals[compiler->locals_count++];
    local->name = name;
    local->depth = -1; // keep it uninitialized
}

static void cmpl_var_decl() {

    // if local on 'var' keyword:
    // get the name
    // add the name to the locals array
    // compile the expression

    byte_t varloc = parse_variable("Expected variable name after 'var' keyword.");

    declare_local(parser.previous);

    if (match(TOKEN_EQUAL))
        cmpl_expression();
    else
        emit_byte(OP_NIL);

    consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration.");

    if (compiler->cur_scope_depth > 0) {
        mark_initialized();
        return;
    }

    emit_bytes(OP_DEFINE_GLOBAL, varloc);
}

static inline void cmpl_print_stmt()
{
    cmpl_expression();
    consume_semicolon();
    emit_byte(OP_PRINT);
}

static void cmpl_while_stmt() {
    consume(TOKEN_LEFT_PAREN, "Expected '(' after while statement.");
    int loop_start = current_chunk()->count;
    cmpl_expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after while statement's condition.");

    int exit_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);

    cmpl_statement();

    emit_loop(loop_start);
    patch_jump(exit_jump);
    emit_byte(OP_POP);
}

static void cmpl_if_stmt() {
    consume(TOKEN_LEFT_PAREN, "Expected '(' after if statement.");
    cmpl_expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after if statement's condition.");

    int then_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);
    cmpl_statement();

    int else_jump = emit_jump(OP_JUMP); 
    patch_jump(then_jump);

    emit_byte(OP_POP);

    if (match(TOKEN_ELSE))
        cmpl_statement();

    patch_jump(else_jump);
}

static void cmpl_block() {
    while (!check_token(TOKEN_RIGHT_BRACE) && !check_token(TOKEN_EOF)) {
        cmpl_declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expected '}' after block.");
}

static void cmpl_statement()
{
    // print statement
    if (match(TOKEN_PRINT)) {
        cmpl_print_stmt();
    }
    // if statement
    else if (match(TOKEN_IF)) {
        cmpl_if_stmt();
    }
    // while statement
    else if (match(TOKEN_WHILE)) {
        cmpl_while_stmt();
    }
    // block statement
    else if (match(TOKEN_LEFT_BRACE)) {
        begin_scope();
        cmpl_block();
        end_scope();
    }
    else {
        // expression statement
        cmpl_expression();
        consume_semicolon();
        emit_byte(OP_POP);
    }
}

static void cmpl_declaration()
{
    if (match(TOKEN_VAR)) {
        cmpl_var_decl();
    }
    else {
        cmpl_statement();
    }
}
#endif
/* Precedence and rule table */
// clang-format off
ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]      = {cmpl_grouping,   NULL,           PREC_NONE},
    [TOKEN_RIGHT_PAREN]     = {NULL,            NULL,           PREC_NONE},
    [TOKEN_LEFT_BRACE]      = {NULL,            NULL,           PREC_NONE},
    [TOKEN_RIGHT_BRACE]     = {NULL,            NULL,           PREC_NONE},
    [TOKEN_COMMA]           = {NULL,            NULL,           PREC_NONE},
    [TOKEN_DOT]             = {NULL,            NULL,           PREC_NONE},
    [TOKEN_MINUS]           = {cmpl_unary,      cmpl_binary,    PREC_TERM},
    [TOKEN_PLUS]            = {NULL,            cmpl_binary,    PREC_TERM},
    [TOKEN_SEMICOLON]       = {NULL,            NULL,           PREC_NONE},
    [TOKEN_SLASH]           = {NULL,            cmpl_binary,    PREC_FACTOR},
    [TOKEN_STAR]            = {NULL,            cmpl_binary,    PREC_FACTOR},
    [TOKEN_BANG]            = {cmpl_unary,      NULL,           PREC_NONE},
    [TOKEN_BANG_EQUAL]      = {NULL,            cmpl_binary,    PREC_EQUALITY},
    [TOKEN_EQUAL]           = {NULL,            NULL,           PREC_NONE},
    [TOKEN_EQUAL_EQUAL]     = {NULL,            cmpl_binary,    PREC_EQUALITY},
    [TOKEN_GREATER]         = {NULL,            cmpl_binary,    PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL]   = {NULL,            cmpl_binary,    PREC_COMPARISON},
    [TOKEN_LESS]            = {NULL,            cmpl_binary,    PREC_COMPARISON},
    [TOKEN_LESS_EQUAL]      = {NULL,            cmpl_binary,    PREC_COMPARISON},
    [TOKEN_IDENTIFIER]      = {cmpl_variable,   NULL,           PREC_NONE},
    [TOKEN_STRING]          = {cmpl_string,     NULL,           PREC_NONE},
    [TOKEN_NUMBER]          = {cmpl_number,     NULL,           PREC_NONE},
    [TOKEN_AND]             = {NULL,            cmpl_lgc_and,   PREC_AND},
    [TOKEN_CLASS]           = {NULL,            NULL,           PREC_NONE},
    [TOKEN_ELSE]            = {NULL,            NULL,           PREC_NONE},
    [TOKEN_FALSE]           = {cmpl_literal,    NULL,           PREC_NONE},
    [TOKEN_FOR]             = {NULL,            NULL,           PREC_NONE},
    [TOKEN_FUN]             = {NULL,            NULL,           PREC_NONE},
    [TOKEN_IF]              = {NULL,            NULL,           PREC_NONE},
    [TOKEN_NIL]             = {cmpl_literal,    NULL,           PREC_NONE},
    [TOKEN_OR]              = {NULL,            cmpl_lgc_or,    PREC_OR},
    [TOKEN_PRINT]           = {NULL,            NULL,           PREC_NONE},
    [TOKEN_RETURN]          = {NULL,            NULL,           PREC_NONE},
    [TOKEN_SUPER]           = {NULL,            NULL,           PREC_NONE},
    [TOKEN_THIS]            = {NULL,            NULL,           PREC_NONE},
    [TOKEN_TRUE]            = {cmpl_literal,    NULL,           PREC_NONE},
    [TOKEN_VAR]             = {NULL,            NULL,           PREC_NONE},
    [TOKEN_WHILE]           = {NULL,            NULL,           PREC_NONE},
    [TOKEN_ERROR]           = {NULL,            NULL,           PREC_NONE},
    [TOKEN_EOF]             = {NULL,            NULL,           PREC_NONE},
};
// clang-format on

/* Misc helpers */
#if 1

static ParseRule* get_rule(TokenType token_type) {
    return &rules[token_type];
}

static void parse_precedence(Precedence min_prec) {
    ParseFn prefix_fn = get_rule(parser.current.type)->prefix_fn;
    if (prefix_fn == NULL) {
        error_token(&parser.previous, "Expected expression.");
        return;
    }

    advance();
    bool can_assign = min_prec <= PREC_ASSIGNMENT;
    prefix_fn(can_assign);

    for (;;) {
        ParseRule* current_rule = get_rule(parser.current.type);
        if (current_rule->precedence < min_prec) {
            break;
        }
        advance();
        current_rule->infix_fn(can_assign);
    }

    if (can_assign && match(TOKEN_EQUAL))
        error_token(&parser.previous, "Invalid assignment target.");
}

static void syncronize()
{
    parser.panic_mode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON)
            return;

        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;

            default: break; // Do nothing.
        }

        advance();
    }
}
#endif

/* Code drivers */
#if 1

bool compile(const char* source, Chunk* result_cnk) {
    scanner_init(source);

    Compiler compiler;
    init_compiler(&compiler);

    _compiling_chunk = result_cnk;
    
    parser.had_error = false;
    parser.panic_mode = false;

    advance();

    while (!match(TOKEN_EOF)) {
        cmpl_declaration();
    }

    emit_byte(OP_RETURN);

    _compiling_chunk = NULL;
#ifdef DEBUG_SHOW_COMPILED_CODE
    if (!parser.had_error) {
        disassemble_chunk(result_cnk, "COMPILED CODE");
    }
#endif

    return !parser.had_error;
}

#endif