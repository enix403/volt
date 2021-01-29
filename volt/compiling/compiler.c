#include "compiling/compiler.h"

#include <stdio.h>
#include <stdlib.h>

#include "scanning/scanner.h"
#include "code/opcodes.h"
#include "code/value.h"
#include "code/object.h"
#include "bool.h"

#include "debugging/switches.h"
#ifdef DEBUG_SHOW_COMPILED_CODE
#include "debugging/disassembly.h"
#endif

typedef struct {
    Token previous;
    Token current;
    bool had_error;
    bool panic_mode;
} Parser;

Parser parser;
Chunk* _compiling_chunk;
static inline Chunk* current_chunk() { return _compiling_chunk; }

/* Error handling */
static void error_token(Token* token, const char* msg) {
    if (parser.panic_mode) return;
    parser.panic_mode = true;
    fprintf(stderr, "[line %d] Compilation error ", token->line);

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

/* Token navigation helpers */
static void _advance() {
    parser.previous = parser.current;
    // jump through all consective error tokens at once in one _advance call
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

static void _consume(TokenType type, const char* msg) {
    if (parser.current.type == type) {
        _advance();
        return;
    }

    error_token(&parser.current, msg);
}

/* Code generation */
static inline void emit_byte(byte_t byte) {
    chunk_write(current_chunk(), byte);
}
static inline void emit_bytes(byte_t byte1, byte_t byte2) {
    emit_byte(byte1);
    emit_byte(byte2);
}

static void emit_const(Value val) {
    int constant_loc = chunk_addconst(current_chunk(), val);
    if (constant_loc >= UINT8_MAX) {
        error_token(&parser.previous, "Too many constants in one chunk");
    }
    emit_bytes(OP_LOADCONST, (byte_t)constant_loc);
}

/* Pratt's Parser */
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


static void parse_precedence(Precedence min_prec);
static ParseRule* get_rule(TokenType token_type);


/* Actual compilation logic */
static inline void cmpl_expression(bool can_assign) {
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
    cmpl_expression(can_assign);
    _consume(TOKEN_RIGHT_PAREN, "Expected closing ')'");
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
    [TOKEN_IDENTIFIER]      = {NULL,            NULL,           PREC_NONE},
    [TOKEN_STRING]          = {cmpl_string,     NULL,           PREC_NONE},
    [TOKEN_NUMBER]          = {cmpl_number,     NULL,           PREC_NONE},
    [TOKEN_AND]             = {NULL,            NULL,           PREC_NONE},
    [TOKEN_CLASS]           = {NULL,            NULL,           PREC_NONE},
    [TOKEN_ELSE]            = {NULL,            NULL,           PREC_NONE},
    [TOKEN_FALSE]           = {cmpl_literal,    NULL,           PREC_NONE},
    [TOKEN_FOR]             = {NULL,            NULL,           PREC_NONE},
    [TOKEN_FUN]             = {NULL,            NULL,           PREC_NONE},
    [TOKEN_IF]              = {NULL,            NULL,           PREC_NONE},
    [TOKEN_NIL]             = {cmpl_literal,    NULL,           PREC_NONE},
    [TOKEN_OR]              = {NULL,            NULL,           PREC_NONE},
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


static ParseRule* get_rule(TokenType token_type) {
    return &rules[token_type];
}

static void parse_precedence(Precedence min_prec) {
    ParseFn prefix_fn = get_rule(parser.current.type)->prefix_fn;
    if (prefix_fn == NULL) {
        error_token(&parser.previous, "Expected expression.");
        return;
    }

    _advance();
    prefix_fn(false);

    for (;;) {
        ParseRule* current_rule = get_rule(parser.current.type);
        if (current_rule->precedence < min_prec) {
            break;
        }
        _advance();
        current_rule->infix_fn(false);
    }
}

bool compile(const char* source, Chunk* result_cnk) {
    scanner_init(source);
    _compiling_chunk = result_cnk;
    
    parser.had_error = false;
    parser.panic_mode = false;

    _advance();
    cmpl_expression(false);
    emit_byte(OP_RETURN);

    _compiling_chunk = NULL;
#ifdef DEBUG_SHOW_COMPILED_CODE
    if (!parser.had_error) {
        disassemble_chunk(result_cnk, "COMPILED CODE");
    }
#endif

    return !parser.had_error;
}