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
    advance(parser);
}

static Expr *parse_expr(Parser *parser);
static Stmt *parse_stmt(Parser *parser);

static Expr *parse_primary(Parser *parser) {
    if (match(parser, TOKEN_INT)) {
        return ast_new_int(parser->previous.int_val);
    }
    if (match(parser, TOKEN_STRING)) {
        return ast_new_string(parser->previous.text);
    }
    if (match(parser, TOKEN_KIRIT)) {
        consume(parser, TOKEN_LPAREN, "'kirit' dan keyin '(' bo'lishi kerak");
        consume(parser, TOKEN_RPAREN, "'kirit()' da ')' yopilishi kerak");
        return ast_new_kirit();
    }
    if (match(parser, TOKEN_UZUNLIK)) {
        consume(parser, TOKEN_LPAREN, "'uzunlik' dan keyin '(' bo'lishi kerak");
        Expr *e = parse_expr(parser);
        consume(parser, TOKEN_RPAREN, "'uzunlik' da ')' yopilishi kerak");
        return ast_new_uzunlik(e);
    }
    if (match(parser, TOKEN_YANGI)) {
        consume(parser, TOKEN_IDENT, "'yangi' dan keyin tuzilma nomi kutilmoqda");
        return ast_new_yangi(parser->previous.text);
    }
    // Massiv literali: [elem1, elem2, ...]
    if (match(parser, TOKEN_LBRACKET)) {
        Expr **elems = NULL;
        int count = 0;
        if (!check(parser, TOKEN_RBRACKET)) {
            do {
                count++;
                elems = (Expr **)realloc(elems, sizeof(Expr *) * count);
                elems[count - 1] = parse_expr(parser);
            } while (match(parser, TOKEN_COMMA));
        }
        consume(parser, TOKEN_RBRACKET, "Massiv oxirida ']' kutilmoqda");
        return ast_new_array_literal(elems, count);
    }
    if (match(parser, TOKEN_IDENT)) {
        char *name = strdup(parser->previous.text);
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

static Expr *parse_postfix(Parser *parser) {
    Expr *expr = parse_primary(parser);
    while (1) {
        if (match(parser, TOKEN_LBRACKET)) {
            Expr *index = parse_expr(parser);
            consume(parser, TOKEN_RBRACKET, "Indeksdan keyin ']' kutilmoqda");
            expr = ast_new_index(expr, index);
        } else if (match(parser, TOKEN_DOT)) {
            consume(parser, TOKEN_IDENT, "Nuqtadan keyin maydon nomi kutilmoqda");
            expr = ast_new_dot(expr, parser->previous.text);
        } else {
            break;
        }
    }
    return expr;
}

static Expr *parse_unary(Parser *parser) {
    if (match(parser, TOKEN_MINUS) || match(parser, TOKEN_INKOR)) {
        TokenType op = parser->previous.type;
        Expr *right = parse_unary(parser);
        return ast_new_unary(op, right);
    }
    return parse_postfix(parser);
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

static Expr *parse_logical_and(Parser *parser) {
    Expr *expr = parse_comparison(parser);
    while (match(parser, TOKEN_VA)) {
        Expr *right = parse_comparison(parser);
        expr = ast_new_binary(TOKEN_VA, expr, right);
    }
    return expr;
}

static Expr *parse_logical_or(Parser *parser) {
    Expr *expr = parse_logical_and(parser);
    while (match(parser, TOKEN_YOKI)) {
        Expr *right = parse_logical_and(parser);
        expr = ast_new_binary(TOKEN_YOKI, expr, right);
    }
    return expr;
}

static Expr *parse_expr(Parser *parser) {
    return parse_logical_or(parser);
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
        match(parser, TOKEN_SEMICOLON);
        return ast_new_return(val);
    }

    // toxtat / davom
    if (match(parser, TOKEN_TOXTAT)) {
        match(parser, TOKEN_SEMICOLON);
        return ast_new_break();
    }
    if (match(parser, TOKEN_DAVOM)) {
        match(parser, TOKEN_SEMICOLON);
        return ast_new_continue();
    }

    // Chiqarish (print) - barcha argumentlarni qo'llab-quvvatlaydi
    if (match(parser, TOKEN_CHIQAR)) {
        consume(parser, TOKEN_LPAREN, "'chiqar' dan keyin '(' kutilmoqda");
        Expr **args = NULL;
        int arg_count = 0;
        if (!check(parser, TOKEN_RPAREN)) {
            do {
                arg_count++;
                args = (Expr **)realloc(args, sizeof(Expr *) * arg_count);
                args[arg_count - 1] = parse_expr(parser);
            } while (match(parser, TOKEN_COMMA));
        }
        consume(parser, TOKEN_RPAREN, "'chiqar' da ')' yopilishi kerak");
        match(parser, TOKEN_SEMICOLON);
        return ast_new_chiqar(args, arg_count);
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

    // Takrorlash (uchun i = 0; i < 10; i = i + 1 { ... })
    if (match(parser, TOKEN_UCHUN)) {
        Stmt *init = parse_stmt(parser);
        Expr *cond = parse_expr(parser);
        consume(parser, TOKEN_SEMICOLON, "'uchun' shartidan keyin ';' kutilmoqda");
        Stmt *step = parse_stmt(parser);
        int body_count = 0;
        Stmt **body = parse_block(parser, &body_count);
        return ast_new_for(init, cond, step, body, body_count);
    }

    // Ifoda yoki O'zlashtirish (x = val, arr[i] = val, obj.field = val)
    Expr *expr = parse_expr(parser);
    if (match(parser, TOKEN_ASSIGN)) {
        Expr *val = parse_expr(parser);
        match(parser, TOKEN_SEMICOLON);
        if (expr->type == EXPR_VAR) {
            Stmt *s = ast_new_var_assign(expr->as.var_name, val);
            return s;
        } else if (expr->type == EXPR_INDEX) {
            Stmt *s = ast_new_index_assign(expr->as.index.target, expr->as.index.index, val);
            return s;
        } else if (expr->type == EXPR_DOT) {
            Stmt *s = ast_new_dot_assign(expr->as.dot.target, expr->as.dot.field, val);
            return s;
        } else {
            error_at_current(parser, "Qiymat berish uchun noto'g'ri nishon");
        }
    }

    match(parser, TOKEN_SEMICOLON);
    return ast_new_expr_stmt(expr);
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

static StructDef *parse_struct(Parser *parser) {
    consume(parser, TOKEN_TUZILMA, "Tuzilma 'tuzilma' kalit so'zi bilan boshlanishi kerak");
    consume(parser, TOKEN_IDENT, "Tuzilma nomi kutilmoqda");
    char *name = strdup(parser->previous.text);

    consume(parser, TOKEN_LBRACE, "Tuzilma tanasida '{' kutilmoqda");
    char **fields = NULL;
    int field_count = 0;
    if (!check(parser, TOKEN_RBRACE)) {
        do {
            consume(parser, TOKEN_IDENT, "Maydon nomi kutilmoqda");
            field_count++;
            fields = (char **)realloc(fields, sizeof(char *) * field_count);
            fields[field_count - 1] = strdup(parser->previous.text);
        } while (match(parser, TOKEN_COMMA));
    }
    consume(parser, TOKEN_RBRACE, "Tuzilma oxirida '}' kutilmoqda");

    StructDef *s = ast_new_struct(name, fields, field_count);
    free(name);
    return s;
}

static char *visited_modules[256];
static int visited_module_count = 0;

static int is_module_visited(const char *path) {
    for (int i = 0; i < visited_module_count; i++) {
        if (strcmp(visited_modules[i], path) == 0) return 1;
    }
    return 0;
}

static void mark_module_visited(const char *path) {
    if (visited_module_count < 256) {
        visited_modules[visited_module_count++] = strdup(path);
    }
}

static char *read_module_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) return NULL;
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char *)malloc(length + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    size_t read_bytes = fread(buffer, 1, length, file);
    buffer[read_bytes] = '\0';
    fclose(file);
    return buffer;
}

Program *parser_parse(Parser *parser) {
    Program *prog = ast_new_program();
    while (!check(parser, TOKEN_EOF)) {
        if (match(parser, TOKEN_YUKLA)) {
            consume(parser, TOKEN_STRING, "'yukla' dan keyin fayl yo'li (\"...\") kutilmoqda");
            char *module_path = strdup(parser->previous.text);
            match(parser, TOKEN_SEMICOLON);

            if (!is_module_visited(module_path)) {
                mark_module_visited(module_path);
                char *mod_source = read_module_file(module_path);
                if (!mod_source) {
                    fprintf(stderr, "Xatolik: '%s' moduli topilmadi!\n", module_path);
                    parser->had_error = 1;
                } else {
                    Parser mod_parser;
                    parser_init(&mod_parser, mod_source);
                    Program *mod_prog = parser_parse(&mod_parser);
                    if (mod_parser.had_error) {
                        parser->had_error = 1;
                    } else {
                        for (int i = 0; i < mod_prog->struct_count; i++) {
                            ast_program_add_struct(prog, mod_prog->structs[i]);
                        }
                        for (int i = 0; i < mod_prog->function_count; i++) {
                            ast_program_add_function(prog, mod_prog->functions[i]);
                        }
                    }
                    free(mod_source);
                }
            }
            free(module_path);
        } else if (check(parser, TOKEN_TUZILMA)) {
            StructDef *s = parse_struct(parser);
            if (s) ast_program_add_struct(prog, s);
        } else if (check(parser, TOKEN_AMAL)) {
            Function *f = parse_function(parser);
            if (f) ast_program_add_function(prog, f);
        } else {
            error_at_current(parser, "Faqat 'amal', 'tuzilma' yoki 'yukla' e'lon qilinishi mumkin");
            advance(parser);
        }
    }
    return prog;
}
