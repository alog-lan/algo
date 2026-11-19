#include "lexer.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

void lexer_init(Lexer *lexer, const char *source) {
    lexer->source = source;
    lexer->cursor = 0;
    lexer->line = 1;
    lexer->col = 1;
}

static char peek(Lexer *lexer) {
    return lexer->source[lexer->cursor];
}

static char peek_next(Lexer *lexer) {
    if (lexer->source[lexer->cursor] == '\0') return '\0';
    return lexer->source[lexer->cursor + 1];
}

static char advance(Lexer *lexer) {
    char c = lexer->source[lexer->cursor++];
    if (c == '\n') {
        lexer->line++;
        lexer->col = 1;
    } else {
        lexer->col++;
    }
    return c;
}

static void skip_whitespace_and_comments(Lexer *lexer) {
    while (1) {
        char c = peek(lexer);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance(lexer);
        } else if (c == '/' && peek_next(lexer) == '/') {
            while (peek(lexer) != '\n' && peek(lexer) != '\0') {
                advance(lexer);
            }
        } else if (c == '#') {
            while (peek(lexer) != '\n' && peek(lexer) != '\0') {
                advance(lexer);
            }
        } else {
            break;
        }
    }
}

static int is_ident_start(char c) {
    return isalpha((unsigned char)c) || c == '_';
}

static int is_ident_part(char c) {
    return isalnum((unsigned char)c) || c == '_' || c == '\'';
}

static TokenType check_keyword(const char *text, int len) {
    if (len == 4 && strncmp(text, "amal", 4) == 0) return TOKEN_AMAL;
    if (len == 6 && strncmp(text, "qaytar", 6) == 0) return TOKEN_QAYTAR;
    if (len == 4 && strncmp(text, "agar", 4) == 0) return TOKEN_AGAR;
    if (len == 3 && strncmp(text, "aks", 3) == 0) return TOKEN_AKS;
    if (len == 4 && strncmp(text, "toki", 4) == 0) return TOKEN_TOKI;
    if (len == 5 && strncmp(text, "uchun", 5) == 0) return TOKEN_UCHUN;
    if (len == 6 && strncmp(text, "chiqar", 6) == 0) return TOKEN_CHIQAR;
    if (len == 4 && strncmp(text, "chop", 4) == 0) return TOKEN_CHIQAR;
    if (len == 5 && strncmp(text, "kirit", 5) == 0) return TOKEN_KIRIT;
    if ((len == 7 && strncmp(text, "to'xtat", 7) == 0) || (len == 6 && strncmp(text, "toxtat", 6) == 0)) return TOKEN_TOXTAT;
    if (len == 5 && strncmp(text, "davom", 5) == 0) return TOKEN_DAVOM;
    if (len == 7 && strncmp(text, "tuzilma", 7) == 0) return TOKEN_TUZILMA;
    if (len == 5 && strncmp(text, "yangi", 5) == 0) return TOKEN_YANGI;
    if (len == 7 && strncmp(text, "uzunlik", 7) == 0) return TOKEN_UZUNLIK;
    if (len == 2 && strncmp(text, "va", 2) == 0) return TOKEN_VA;
    if (len == 4 && strncmp(text, "yoki", 4) == 0) return TOKEN_YOKI;
    if (len == 5 && strncmp(text, "inkor", 5) == 0) return TOKEN_INKOR;
    if (len == 5 && strncmp(text, "yukla", 5) == 0) return TOKEN_YUKLA;
    return TOKEN_IDENT;
}

Token lexer_next_token(Lexer *lexer) {
    skip_whitespace_and_comments(lexer);

    int start_col = lexer->col;
    int start_line = lexer->line;
    char c = peek(lexer);

    if (c == '\0') {
        Token tok = {TOKEN_EOF, NULL, 0, 0, 0.0, start_line, start_col};
        return tok;
    }

    // Identifikator yoki Kalit so'z
    if (is_ident_start(c)) {
        int start_pos = lexer->cursor;
        while (is_ident_part(peek(lexer))) {
            advance(lexer);
        }
        int len = lexer->cursor - start_pos;
        const char *text = &lexer->source[start_pos];
        TokenType type = check_keyword(text, len);

        char *token_str = (char *)malloc(len + 1);
        strncpy(token_str, text, len);
        token_str[len] = '\0';

        Token tok = {type, token_str, len, 0, 0.0, start_line, start_col};
        return tok;
    }

    // Butun yoki Haqiqiy son (Integer yoki Float)
    if (isdigit((unsigned char)c)) {
        int start_pos = lexer->cursor;
        int is_float = 0;
        int64_t val = 0;
        while (isdigit((unsigned char)peek(lexer))) {
            char digit = advance(lexer);
            val = val * 10 + (digit - '0');
        }
        if (peek(lexer) == '.' && isdigit((unsigned char)peek_next(lexer))) {
            is_float = 1;
            advance(lexer); // skip '.'
            while (isdigit((unsigned char)peek(lexer))) {
                advance(lexer);
            }
        }
        int len = lexer->cursor - start_pos;
        char *token_str = (char *)malloc(len + 1);
        strncpy(token_str, &lexer->source[start_pos], len);
        token_str[len] = '\0';

        if (is_float) {
            double f_val = strtod(token_str, NULL);
            Token tok = {TOKEN_FLOAT, token_str, len, 0, f_val, start_line, start_col};
            return tok;
        } else {
            Token tok = {TOKEN_INT, token_str, len, val, 0.0, start_line, start_col};
            return tok;
        }
    }

    // Matn (String literal)
    if (c == '"') {
        advance(lexer); // skip '"'
        int cap = 64;
        int len = 0;
        char *buf = (char *)malloc(cap);

        while (peek(lexer) != '"' && peek(lexer) != '\0') {
            char ch = advance(lexer);
            if (ch == '\\') {
                char esc = advance(lexer);
                if (esc == 'n') ch = '\n';
                else if (esc == 't') ch = '\t';
                else if (esc == 'r') ch = '\r';
                else if (esc == '0') ch = '\0';
                else if (esc == '"') ch = '"';
                else if (esc == '\\') ch = '\\';
                else ch = esc;
            }
            if (len + 2 >= cap) {
                cap *= 2;
                buf = (char *)realloc(buf, cap);
            }
            buf[len++] = ch;
        }

        if (peek(lexer) == '"') {
            advance(lexer);
        }
        buf[len] = '\0';

        Token tok = {TOKEN_STRING, buf, len, 0, 0.0, start_line, start_col};
        return tok;
    }

    // Operatorlar va belgilar
    advance(lexer);
    TokenType type = TOKEN_ERROR;

    switch (c) {
        case '{': type = TOKEN_LBRACE; break;
        case '}': type = TOKEN_RBRACE; break;
        case '(': type = TOKEN_LPAREN; break;
        case ')': type = TOKEN_RPAREN; break;
        case '[': type = TOKEN_LBRACKET; break;
        case ']': type = TOKEN_RBRACKET; break;
        case ',': type = TOKEN_COMMA; break;
        case ';': type = TOKEN_SEMICOLON; break;
        case '.': type = TOKEN_DOT; break;
        case ':': type = TOKEN_COLON; break;
        case '+': type = TOKEN_PLUS; break;
        case '-': type = TOKEN_MINUS; break;
        case '*': type = TOKEN_STAR; break;
        case '/': type = TOKEN_SLASH; break;
        case '%': type = TOKEN_PERCENT; break;
        case '^': type = TOKEN_CARET; break;
        case '~': type = TOKEN_TILDE; break;
        case '&':
            if (peek(lexer) == '&') {
                advance(lexer);
                type = TOKEN_VA;
            } else {
                type = TOKEN_AMP;
            }
            break;
        case '|':
            if (peek(lexer) == '|') {
                advance(lexer);
                type = TOKEN_YOKI;
            } else {
                type = TOKEN_PIPE;
            }
            break;
        case '=':
            if (peek(lexer) == '=') {
                advance(lexer);
                type = TOKEN_EQ;
            } else {
                type = TOKEN_ASSIGN;
            }
            break;
        case '!':
            if (peek(lexer) == '=') {
                advance(lexer);
                type = TOKEN_NEQ;
            } else {
                type = TOKEN_INKOR;
            }
            break;
        case '<':
            if (peek(lexer) == '=') {
                advance(lexer);
                type = TOKEN_LTE;
            } else if (peek(lexer) == '<') {
                advance(lexer);
                type = TOKEN_SHL;
            } else {
                type = TOKEN_LT;
            }
            break;
        case '>':
            if (peek(lexer) == '=') {
                advance(lexer);
                type = TOKEN_GTE;
            } else if (peek(lexer) == '>') {
                advance(lexer);
                type = TOKEN_SHR;
            } else {
                type = TOKEN_GT;
            }
            break;
        default:
            type = TOKEN_ERROR;
            break;
    }

    Token tok = {type, NULL, 0, 0, 0.0, start_line, start_col};
    return tok;
}
