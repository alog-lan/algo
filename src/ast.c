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

Expr *ast_new_float(double val) {
    Expr *e = (Expr *)malloc(sizeof(Expr));
    e->type = EXPR_FLOAT;
    e->as.float_val = val;
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

Expr *ast_new_unary(TokenType op, Expr *operand) {
    Expr *e = (Expr *)malloc(sizeof(Expr));
    e->type = EXPR_UNARY;
    e->as.unary.op = op;
    e->as.unary.operand = operand;
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

Expr *ast_new_array_literal(Expr **elements, int count) {
    Expr *e = (Expr *)malloc(sizeof(Expr));
    e->type = EXPR_ARRAY_LITERAL;
    e->as.array.elements = elements;
    e->as.array.count = count;
    return e;
}

Expr *ast_new_index(Expr *target, Expr *index) {
    Expr *e = (Expr *)malloc(sizeof(Expr));
    e->type = EXPR_INDEX;
    e->as.index.target = target;
    e->as.index.index = index;
    return e;
}

Expr *ast_new_dot(Expr *target, const char *field) {
    Expr *e = (Expr *)malloc(sizeof(Expr));
    e->type = EXPR_DOT;
    e->as.dot.target = target;
    e->as.dot.field = strdup_safe(field);
    return e;
}

Expr *ast_new_kirit(void) {
    Expr *e = (Expr *)malloc(sizeof(Expr));
    e->type = EXPR_KIRIT;
    return e;
}

Expr *ast_new_uzunlik(Expr *expr) {
    Expr *e = (Expr *)malloc(sizeof(Expr));
    e->type = EXPR_UZUNLIK;
    e->as.uzunlik_expr = expr;
    return e;
}

Expr *ast_new_yangi(const char *struct_name) {
    Expr *e = (Expr *)malloc(sizeof(Expr));
    e->type = EXPR_YANGI;
    e->as.yangi_struct_name = strdup_safe(struct_name);
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

Stmt *ast_new_index_assign(Expr *target, Expr *index, Expr *value) {
    Stmt *s = (Stmt *)malloc(sizeof(Stmt));
    s->type = STMT_INDEX_ASSIGN;
    s->as.index_assign.target = target;
    s->as.index_assign.index = index;
    s->as.index_assign.value = value;
    return s;
}

Stmt *ast_new_dot_assign(Expr *target, const char *field, Expr *value) {
    Stmt *s = (Stmt *)malloc(sizeof(Stmt));
    s->type = STMT_DOT_ASSIGN;
    s->as.dot_assign.target = target;
    s->as.dot_assign.field = strdup_safe(field);
    s->as.dot_assign.value = value;
    return s;
}

Stmt *ast_new_chiqar(Expr **args, int arg_count) {
    Stmt *s = (Stmt *)malloc(sizeof(Stmt));
    s->type = STMT_CHIQAR;
    s->as.chiqar.args = args;
    s->as.chiqar.arg_count = arg_count;
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

Stmt *ast_new_for(Stmt *init, Expr *cond, Stmt *step, Stmt **body, int body_count) {
    Stmt *s = (Stmt *)malloc(sizeof(Stmt));
    s->type = STMT_FOR;
    s->as.for_stmt.init = init;
    s->as.for_stmt.condition = cond;
    s->as.for_stmt.step = step;
    s->as.for_stmt.body = body;
    s->as.for_stmt.body_count = body_count;
    return s;
}

Stmt *ast_new_break(void) {
    Stmt *s = (Stmt *)malloc(sizeof(Stmt));
    s->type = STMT_BREAK;
    return s;
}

Stmt *ast_new_continue(void) {
    Stmt *s = (Stmt *)malloc(sizeof(Stmt));
    s->type = STMT_CONTINUE;
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

StructDef *ast_new_struct(const char *name, char **fields, int field_count) {
    StructDef *s = (StructDef *)malloc(sizeof(StructDef));
    s->name = strdup_safe(name);
    s->fields = fields;
    s->field_count = field_count;
    return s;
}

Program *ast_new_program(void) {
    Program *p = (Program *)malloc(sizeof(Program));
    p->functions = NULL;
    p->function_count = 0;
    p->structs = NULL;
    p->struct_count = 0;
    return p;
}

void ast_program_add_function(Program *prog, Function *func) {
    prog->function_count++;
    prog->functions = (Function **)realloc(prog->functions, sizeof(Function *) * prog->function_count);
    prog->functions[prog->function_count - 1] = func;
}

void ast_program_add_struct(Program *prog, StructDef *strct) {
    prog->struct_count++;
    prog->structs = (StructDef **)realloc(prog->structs, sizeof(StructDef *) * prog->struct_count);
    prog->structs[prog->struct_count - 1] = strct;
}

StructDef *ast_program_find_struct(Program *prog, const char *name) {
    if (!prog || !name) return NULL;
    for (int i = 0; i < prog->struct_count; i++) {
        if (strcmp(prog->structs[i]->name, name) == 0) {
            return prog->structs[i];
        }
    }
    return NULL;
}
