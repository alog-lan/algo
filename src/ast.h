#ifndef AST_H
#define AST_H

#include <stdint.h>
#include "token.h"

typedef enum {
    EXPR_INT,
    EXPR_STRING,
    EXPR_VAR,
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_CALL,
    EXPR_ARRAY_LITERAL,
    EXPR_INDEX,
    EXPR_DOT,
    EXPR_KIRIT,
    EXPR_UZUNLIK,
    EXPR_YANGI
} ExprType;

typedef struct Expr Expr;

typedef struct {
    TokenType op;
    Expr *left;
    Expr *right;
} BinaryExpr;

typedef struct {
    TokenType op;
    Expr *operand;
} UnaryExpr;

typedef struct {
    char *name;
    Expr **args;
    int arg_count;
} CallExpr;

typedef struct {
    Expr **elements;
    int count;
} ArrayLiteralExpr;

typedef struct {
    Expr *target;
    Expr *index;
} IndexExpr;

typedef struct {
    Expr *target;
    char *field;
} DotExpr;

struct Expr {
    ExprType type;
    union {
        int64_t int_val;
        char *str_val;
        char *var_name;
        BinaryExpr binary;
        UnaryExpr unary;
        CallExpr call;
        ArrayLiteralExpr array;
        IndexExpr index;
        DotExpr dot;
        Expr *uzunlik_expr;
        char *yangi_struct_name;
    } as;
};

typedef enum {
    STMT_RETURN,
    STMT_VAR_ASSIGN,
    STMT_INDEX_ASSIGN,
    STMT_DOT_ASSIGN,
    STMT_CHIQAR,
    STMT_IF,
    STMT_WHILE,
    STMT_FOR,
    STMT_BREAK,
    STMT_CONTINUE,
    STMT_EXPR
} StmtType;

typedef struct Stmt Stmt;

typedef struct {
    char *var_name;
    Expr *value;
} VarAssignStmt;

typedef struct {
    Expr *target;
    Expr *index;
    Expr *value;
} IndexAssignStmt;

typedef struct {
    Expr *target;
    char *field;
    Expr *value;
} DotAssignStmt;

typedef struct {
    Expr **args;
    int arg_count;
} ChiqarStmt;

typedef struct {
    Expr *condition;
    Stmt **then_branch;
    int then_count;
    Stmt **else_branch;
    int else_count;
} IfStmt;

typedef struct {
    Expr *condition;
    Stmt **body;
    int body_count;
} WhileStmt;

typedef struct {
    Stmt *init;
    Expr *condition;
    Stmt *step;
    Stmt **body;
    int body_count;
} ForStmt;

struct Stmt {
    StmtType type;
    union {
        Expr *return_expr;
        VarAssignStmt var_assign;
        IndexAssignStmt index_assign;
        DotAssignStmt dot_assign;
        ChiqarStmt chiqar;
        IfStmt if_stmt;
        WhileStmt while_stmt;
        ForStmt for_stmt;
        Expr *expr_stmt;
    } as;
};

typedef struct {
    char *name;
    char **params;
    int param_count;
    Stmt **body;
    int body_count;
} Function;

typedef struct {
    char *name;
    char **fields;
    int field_count;
} StructDef;

typedef struct {
    Function **functions;
    int function_count;
    StructDef **structs;
    int struct_count;
} Program;

// Yordamchi konstruktorlar
Expr *ast_new_int(int64_t val);
Expr *ast_new_string(const char *str);
Expr *ast_new_var(const char *name);
Expr *ast_new_binary(TokenType op, Expr *left, Expr *right);
Expr *ast_new_unary(TokenType op, Expr *operand);
Expr *ast_new_call(const char *name, Expr **args, int arg_count);
Expr *ast_new_array_literal(Expr **elements, int count);
Expr *ast_new_index(Expr *target, Expr *index);
Expr *ast_new_dot(Expr *target, const char *field);
Expr *ast_new_kirit(void);
Expr *ast_new_uzunlik(Expr *expr);
Expr *ast_new_yangi(const char *struct_name);

Stmt *ast_new_return(Expr *expr);
Stmt *ast_new_var_assign(const char *name, Expr *value);
Stmt *ast_new_index_assign(Expr *target, Expr *index, Expr *value);
Stmt *ast_new_dot_assign(Expr *target, const char *field, Expr *value);
Stmt *ast_new_chiqar(Expr **args, int arg_count);
Stmt *ast_new_if(Expr *cond, Stmt **then_branch, int then_count, Stmt **else_branch, int else_count);
Stmt *ast_new_while(Expr *cond, Stmt **body, int body_count);
Stmt *ast_new_for(Stmt *init, Expr *cond, Stmt *step, Stmt **body, int body_count);
Stmt *ast_new_break(void);
Stmt *ast_new_continue(void);
Stmt *ast_new_expr_stmt(Expr *expr);

Function *ast_new_function(const char *name, char **params, int param_count, Stmt **body, int body_count);
StructDef *ast_new_struct(const char *name, char **fields, int field_count);
Program *ast_new_program(void);
void ast_program_add_function(Program *prog, Function *func);
void ast_program_add_struct(Program *prog, StructDef *strct);
StructDef *ast_program_find_struct(Program *prog, const char *name);

#endif // AST_H
