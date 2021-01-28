#include "scanning/scanner.h"
#include <string.h>
#include "bool.h"

typedef struct {
    int line;
    // Note: pointers to chars instead of offsets
    const char* start;
    const char* current;
} Scanner;

Scanner scanner;

void scanner_init(const char* source) {
    scanner.line = 1;
    scanner.start = source;
    scanner.current = source;
}

static inline bool is_at_end() {
    return *scanner.current == '\0';
}

static Token make_token(TokenType type) {
    Token token;
    token.line = scanner.line;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.type = type;

    return token;
}

static Token error_token(const char* msg) {
    Token token;
    token.line = scanner.line;
    token.start = msg;
    token.length = (int)strlen(msg);
    token.type = TOKEN_ERROR;

    return token;
}

static inline char advance() {
    scanner.current++;
    return *(scanner.current - 1);
}

static inline char peek() {
    return *scanner.current;
}

static inline char peek_next() {
    if (is_at_end()) return '\0';
    return *(scanner.current + 1);
}

static bool match_next(char expected) {
    if (is_at_end()) return false;
    if (peek() != expected) return false;
    scanner.current++;
    return true;
}

/* NUMBERS AND STRINGS */
static Token scan_string() {
    while (peek() != '"' && !is_at_end()) {
        if (peek() == '\n') scanner.line++;
        advance();
    }

    if (is_at_end()) return error_token("Unterminated string.");

    // The closing quote.
    advance();
    return make_token(TOKEN_STRING);
}

static inline bool is_digit(char c) { return c >= '0' && c <= '9'; }
static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';
}

static Token scan_number() {
    while (is_digit(peek())) advance();

    // Look for a fractional part.
    if (peek() == '.' && is_digit(peek_next())) {
        // Consume the ".".
        advance();

        while (is_digit(peek())) advance();
    }

    return make_token(TOKEN_NUMBER);
}

/* IDENTIFIER TOKEN SCANNING */
static TokenType check_keyword(int start, int length, const char* rest, TokenType type) {
    if (scanner.current - scanner.start == start + length &&
        memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

static TokenType identifier_type() {
    switch (scanner.start[0]) {
        case 'a': return check_keyword(1, 2, "nd",      TOKEN_AND);
        case 'c': return check_keyword(1, 4, "lass",    TOKEN_CLASS);
        case 'e': return check_keyword(1, 3, "lse",     TOKEN_ELSE);
        case 'i': return check_keyword(1, 1, "f",       TOKEN_IF);
        case 'n': return check_keyword(1, 2, "il",      TOKEN_NIL);
        case 'o': return check_keyword(1, 1, "r",       TOKEN_OR);
        case 'p': return check_keyword(1, 4, "rint",    TOKEN_PRINT);
        case 'r': return check_keyword(1, 5, "eturn",   TOKEN_RETURN);
        case 's': return check_keyword(1, 4, "uper",    TOKEN_SUPER);
        case 'v': return check_keyword(1, 2, "ar",      TOKEN_VAR);
        case 'w': return check_keyword(1, 4, "hile",    TOKEN_WHILE);
        case 'f': {
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a': return check_keyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o': return check_keyword(2, 1, "r",   TOKEN_FOR);
                    case 'u': return check_keyword(2, 1, "n",   TOKEN_FUN);
                }
            }
            break;
        }

        case 't': {
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'h': return check_keyword(2, 2, "is", TOKEN_THIS);
                    case 'r': return check_keyword(2, 2, "ue", TOKEN_TRUE);
                }
            }
            break;
        }
    }
    return TOKEN_IDENTIFIER;
}

static Token scan_identifier() {
    while (is_alpha(peek()) || is_digit(peek())) advance();
    return make_token(identifier_type());
}


static void skip_whitespaces() {
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;

            case '\n': {
                scanner.line++;
                advance();
                break;
            }

            case '/': {
                if (peek_next() == '/') {
                    // A comment goes until the end of the line.
                    while (peek() != '\n' && !is_at_end()) advance();
                } else {
                    return;
                }
                break;
            }

            default:
                return;
        }
    }
}

// Scan the next token
Token scan_token() {
    skip_whitespaces();
    scanner.start = scanner.current;
    if (is_at_end()) return make_token(TOKEN_EOF);

    char c = advance();
    switch (c) {
        // single character tokens
        case '(': return make_token(TOKEN_LEFT_PAREN);
        case ')': return make_token(TOKEN_RIGHT_PAREN);
        case '{': return make_token(TOKEN_LEFT_BRACE);
        case '}': return make_token(TOKEN_RIGHT_BRACE);
        case ';': return make_token(TOKEN_SEMICOLON);
        case ',': return make_token(TOKEN_COMMA);
        case '.': return make_token(TOKEN_DOT);
        case '-': return make_token(TOKEN_MINUS);
        case '+': return make_token(TOKEN_PLUS);
        case '/': return make_token(TOKEN_SLASH);
        case '*': return make_token(TOKEN_STAR);

        // single and double character tokens
        case '!': return make_token( match_next('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG );
        case '=': return make_token( match_next('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL );
        case '<': return make_token( match_next('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS );
        case '>': return make_token( match_next('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER );

        case '"': return scan_string();

        default: {
            if (is_digit(c)) return scan_number();
            if (is_alpha(c)) return scan_identifier();
            break;
        }
    }
    return error_token("Unexpected character.");
}
