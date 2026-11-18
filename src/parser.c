#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void advance(Parser *parser) {
    parser->previous = parser->current;
    parser->current = lexer_next_token(&parser->lexer);
}

static int check(Parser *parser, TokenType type) {
    return parser->current.type == type;
}

static int match(Parser *parser, TokenType type) {
    if (check(parser, type)) {
        advance(parser);
        return 1;
    }
    return 0;
}

static void error_at_current(Parser *parser, const char *msg) {
    fprintf(stderr, "Xatolik [qator %d, ustun %d]: %s, kutildi: '%s'\n",
            parser->current.line, parser->current.col, msg,
            token_type_to_string(parser->current.type));
    parser->had_error = 1;
}

static void consume(Parser *parser, TokenType type, const char *msg) {
    if (check(parser, type)) {
        advance(parser);
        return;
    }
    error_at_current(parser, msg);
}

void parser_init(Parser *parser, const char *source) {
    lexer_init(&parser->lexer, source);
    parser->had_error = 0;
    advance(parser); // birinchi tokenni o'qiymiz
}

// Rekursiv tahlil prototiplari
static Expr *parse_expr(Parser *parser);
static Stmt *parse_stmt(Parser *parser);

static Expr *parse_primary(Parser *parser) {
    if (match(parser, TOKEN_INT)) {
        return ast_new_int(parser->previous.int_val);
    }
    if (match(parser, TOKEN_STRING)) {
        return ast_new_string(parser->previous.text);
    }
    if (match(parser, TOKEN_IDENT)) {
        char *name = strdup(parser->previous.text);
        // Agar keyin '(' kelsa, bu funksiya chaqiruvi
        if (match(parser, TOKEN_LPAREN)) {
            Expr **args = NULL;
            int arg_count = 0;
            if (!check(parser, TOKEN_RPAREN)) {
                do {
                    arg_count++;
                    args = (Expr **)realloc(args, sizeof(Expr *) * arg_count);
                    args[arg_count - 1] = parse_expr(parser);
                } while (match(parser, TOKEN_COMMA));
            }
            consume(parser, TOKEN_RPAREN, "Funksiya chaqiruvida ')' qavs yopilmadi");
            Expr *call = ast_new_call(name, args, arg_count);
            free(name);
            return call;
        }
        Expr *var = ast_new_var(name);
        free(name);
        return var;
    }
    if (match(parser, TOKEN_LPAREN)) {
        Expr *expr = parse_expr(parser);
        consume(parser, TOKEN_RPAREN, "Ifodada ')' qavs yopilmadi");
        return expr;
    }

    error_at_current(parser, "Noto'g'ri ifoda");
    advance(parser);
    return ast_new_int(0);
}

static Expr *parse_unary(Parser *parser) {
    if (match(parser, TOKEN_MINUS)) {
        Expr *right = parse_unary(parser);
        return ast_new_binary(TOKEN_MINUS, ast_new_int(0), right);
    }
    return parse_primary(parser);
}

static Expr *parse_factor(Parser *parser) {
    Expr *expr = parse_unary(parser);
    while (check(parser, TOKEN_STAR) || check(parser, TOKEN_SLASH) || check(parser, TOKEN_PERCENT)) {
        advance(parser);
        TokenType op = parser->previous.type;
        Expr *right = parse_unary(parser);
        expr = ast_new_binary(op, expr, right);
    }
    return expr;
}

static Expr *parse_term(Parser *parser) {
    Expr *expr = parse_factor(parser);
    while (check(parser, TOKEN_PLUS) || check(parser, TOKEN_MINUS)) {
        advance(parser);
        TokenType op = parser->previous.type;
        Expr *right = parse_factor(parser);
        expr = ast_new_binary(op, expr, right);
    }
    return expr;
}

static Expr *parse_comparison(Parser *parser) {
    Expr *expr = parse_term(parser);
    while (check(parser, TOKEN_EQ) || check(parser, TOKEN_NEQ) ||
           check(parser, TOKEN_LT) || check(parser, TOKEN_LTE) ||
           check(parser, TOKEN_GT) || check(parser, TOKEN_GTE)) {
        advance(parser);
        TokenType op = parser->previous.type;
        Expr *right = parse_term(parser);
        expr = ast_new_binary(op, expr, right);
    }
    return expr;
}

static Expr *parse_expr(Parser *parser) {
    return parse_comparison(parser);
}

static Stmt **parse_block(Parser *parser, int *count_out) {
    consume(parser, TOKEN_LBRACE, "Blok boshlanishida '{' kutilmoqda");
    Stmt **stmts = NULL;
    int count = 0;

    while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
        Stmt *s = parse_stmt(parser);
        if (s != NULL) {
            count++;
            stmts = (Stmt **)realloc(stmts, sizeof(Stmt *) * count);
            stmts[count - 1] = s;
        }
    }
    consume(parser, TOKEN_RBRACE, "Blok yakunida '}' kutilmoqda");
    *count_out = count;
    return stmts;
}

static Stmt *parse_stmt(Parser *parser) {
    // Qaytarish (return)
    if (match(parser, TOKEN_QAYTAR)) {
        Expr *val = NULL;
        if (!check(parser, TOKEN_SEMICOLON) && !check(parser, TOKEN_RBRACE)) {
            val = parse_expr(parser);
        } else {
            val = ast_new_int(0);
        }
        match(parser, TOKEN_SEMICOLON); // ixtiyoriy ';'
        return ast_new_return(val);
    }

    // Chiqarish (print)
    if (match(parser, TOKEN_CHIQAR)) {
        consume(parser, TOKEN_LPAREN, "'chiqar' dan keyin '(' ochilishi kerak");
        Expr *val = parse_expr(parser);
        consume(parser, TOKEN_RPAREN, "'chiqar' da ')' yopilishi kerak");
        match(parser, TOKEN_SEMICOLON);
        return ast_new_chiqar(val);
    }

    // Shart (agar ... aks ...)
    if (match(parser, TOKEN_AGAR)) {
        Expr *cond = parse_expr(parser);
        int then_count = 0;
        Stmt **then_branch = parse_block(parser, &then_count);

        Stmt **else_branch = NULL;
        int else_count = 0;
        if (match(parser, TOKEN_AKS)) {
            if (check(parser, TOKEN_AGAR)) {
                // aks agar (else if)
                else_count = 1;
                else_branch = (Stmt **)malloc(sizeof(Stmt *));
                else_branch[0] = parse_stmt(parser);
            } else {
                else_branch = parse_block(parser, &else_count);
            }
        }
        return ast_new_if(cond, then_branch, then_count, else_branch, else_count);
    }

    // Takrorlash (toki ...)
    if (match(parser, TOKEN_TOKI)) {
        Expr *cond = parse_expr(parser);
        int body_count = 0;
        Stmt **body = parse_block(parser, &body_count);
        return ast_new_while(cond, body, body_count);
    }

    // O'zgaruvchiga qiymat berish yoki funksiya chaqiruvi / ifoda
    if (check(parser, TOKEN_IDENT)) {
        // Lookahead: agar keyingi belgi '=' bo'lsa
        Lexer lookahead = parser->lexer;
        Token next = lexer_next_token(&lookahead);
        if (next.type == TOKEN_ASSIGN) {
            advance(parser); // IDENT
            char *var_name = strdup(parser->previous.text);
            advance(parser); // '='
            Expr *val = parse_expr(parser);
            match(parser, TOKEN_SEMICOLON);
            Stmt *s = ast_new_var_assign(var_name, val);
            free(var_name);
            return s;
        }
    }

    // Oddiy ifoda amali (masalan funksiya chaqiruvi)
    Expr *e = parse_expr(parser);
    match(parser, TOKEN_SEMICOLON);
    return ast_new_expr_stmt(e);
}

static Function *parse_function(Parser *parser) {
    consume(parser, TOKEN_AMAL, "Funksiya 'amal' kalit so'zi bilan boshlanishi kerak");
    consume(parser, TOKEN_IDENT, "Funksiya nomi kutilmoqda");
    char *func_name = strdup(parser->previous.text);

    consume(parser, TOKEN_LPAREN, "Funksiya nomidan keyin '(' kutilmoqda");
    char **params = NULL;
    int param_count = 0;
    if (!check(parser, TOKEN_RPAREN)) {
        do {
            consume(parser, TOKEN_IDENT, "Parametr nomi kutilmoqda");
            param_count++;
            params = (char **)realloc(params, sizeof(char *) * param_count);
            params[param_count - 1] = strdup(parser->previous.text);
        } while (match(parser, TOKEN_COMMA));
    }
    consume(parser, TOKEN_RPAREN, "Parametrlardan keyin ')' kutilmoqda");

    int body_count = 0;
    Stmt **body = parse_block(parser, &body_count);

    Function *func = ast_new_function(func_name, params, param_count, body, body_count);
    free(func_name);
    return func;
}

Program *parser_parse(Parser *parser) {
    Program *prog = ast_new_program();
    while (!check(parser, TOKEN_EOF)) {
        Function *f = parse_function(parser);
        if (f) {
            ast_program_add_function(prog, f);
        }
    }
    return prog;
}
