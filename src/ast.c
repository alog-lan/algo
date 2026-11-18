#include "ast.h"
#include <stdlib.h>
#include <string.h>

static char *strdup_safe(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *res = (char *)malloc(len + 1);
    strcpy(res, s);
    return res;
}

Expr *ast_new_int(int64_t val) {
    Expr *e = (Expr *)malloc(sizeof(Expr));
    e->type = EXPR_INT;
    e->as.int_val = val;
    return e;
}

Expr *ast_new_string(const char *str) {
    Expr *e = (Expr *)malloc(sizeof(Expr));
    e->type = EXPR_STRING;
    e->as.str_val = strdup_safe(str);
    return e;
}

Expr *ast_new_var(const char *name) {
    Expr *e = (Expr *)malloc(sizeof(Expr));
    e->type = EXPR_VAR;
    e->as.var_name = strdup_safe(name);
    return e;
}

Expr *ast_new_binary(TokenType op, Expr *left, Expr *right) {
    Expr *e = (Expr *)malloc(sizeof(Expr));
    e->type = EXPR_BINARY;
    e->as.binary.op = op;
    e->as.binary.left = left;
    e->as.binary.right = right;
    return e;
}

Expr *ast_new_call(const char *name, Expr **args, int arg_count) {
    Expr *e = (Expr *)malloc(sizeof(Expr));
    e->type = EXPR_CALL;
    e->as.call.name = strdup_safe(name);
    e->as.call.args = args;
    e->as.call.arg_count = arg_count;
    return e;
}

Stmt *ast_new_return(Expr *expr) {
    Stmt *s = (Stmt *)malloc(sizeof(Stmt));
    s->type = STMT_RETURN;
    s->as.return_expr = expr;
    return s;
}

Stmt *ast_new_var_assign(const char *name, Expr *value) {
    Stmt *s = (Stmt *)malloc(sizeof(Stmt));
    s->type = STMT_VAR_ASSIGN;
    s->as.var_assign.var_name = strdup_safe(name);
    s->as.var_assign.value = value;
    return s;
}

Stmt *ast_new_chiqar(Expr *expr) {
    Stmt *s = (Stmt *)malloc(sizeof(Stmt));
    s->type = STMT_CHIQAR;
    s->as.chiqar_expr = expr;
    return s;
}

Stmt *ast_new_if(Expr *cond, Stmt **then_branch, int then_count, Stmt **else_branch, int else_count) {
    Stmt *s = (Stmt *)malloc(sizeof(Stmt));
    s->type = STMT_IF;
    s->as.if_stmt.condition = cond;
    s->as.if_stmt.then_branch = then_branch;
    s->as.if_stmt.then_count = then_count;
    s->as.if_stmt.else_branch = else_branch;
    s->as.if_stmt.else_count = else_count;
    return s;
}

Stmt *ast_new_while(Expr *cond, Stmt **body, int body_count) {
    Stmt *s = (Stmt *)malloc(sizeof(Stmt));
    s->type = STMT_WHILE;
    s->as.while_stmt.condition = cond;
    s->as.while_stmt.body = body;
    s->as.while_stmt.body_count = body_count;
    return s;
}

Stmt *ast_new_expr_stmt(Expr *expr) {
    Stmt *s = (Stmt *)malloc(sizeof(Stmt));
    s->type = STMT_EXPR;
    s->as.expr_stmt = expr;
    return s;
}

Function *ast_new_function(const char *name, char **params, int param_count, Stmt **body, int body_count) {
    Function *f = (Function *)malloc(sizeof(Function));
    f->name = strdup_safe(name);
    f->params = params;
    f->param_count = param_count;
    f->body = body;
    f->body_count = body_count;
    return f;
}

Program *ast_new_program(void) {
    Program *p = (Program *)malloc(sizeof(Program));
    p->functions = NULL;
    p->function_count = 0;
    return p;
}

void ast_program_add_function(Program *prog, Function *func) {
    prog->function_count++;
    prog->functions = (Function **)realloc(prog->functions, sizeof(Function *) * prog->function_count);
    prog->functions[prog->function_count - 1] = func;
}
