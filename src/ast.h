#ifndef AST_H
#define AST_H

#include <stdint.h>
#include "token.h"

typedef enum {
    EXPR_INT,
    EXPR_STRING,
    EXPR_VAR,
    EXPR_BINARY,
    EXPR_CALL
} ExprType;

typedef struct Expr Expr;

typedef struct {
    TokenType op;
    Expr *left;
    Expr *right;
} BinaryExpr;

typedef struct {
    char *name;
    Expr **args;
    int arg_count;
} CallExpr;

struct Expr {
    ExprType type;
    union {
        int64_t int_val;
        char *str_val;
        char *var_name;
        BinaryExpr binary;
        CallExpr call;
    } as;
};

typedef enum {
    STMT_RETURN,
    STMT_VAR_ASSIGN,
    STMT_CHIQAR,
    STMT_IF,
    STMT_WHILE,
    STMT_EXPR
} StmtType;

typedef struct Stmt Stmt;

typedef struct {
    char *var_name;
    Expr *value;
} VarAssignStmt;

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

struct Stmt {
    StmtType type;
    union {
        Expr *return_expr;
        VarAssignStmt var_assign;
        Expr *chiqar_expr;
        IfStmt if_stmt;
        WhileStmt while_stmt;
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
    Function **functions;
    int function_count;
} Program;

// Yordamchi konstruktorlar
Expr *ast_new_int(int64_t val);
Expr *ast_new_string(const char *str);
Expr *ast_new_var(const char *name);
Expr *ast_new_binary(TokenType op, Expr *left, Expr *right);
Expr *ast_new_call(const char *name, Expr **args, int arg_count);

Stmt *ast_new_return(Expr *expr);
Stmt *ast_new_var_assign(const char *name, Expr *value);
Stmt *ast_new_chiqar(Expr *expr);
Stmt *ast_new_if(Expr *cond, Stmt **then_branch, int then_count, Stmt **else_branch, int else_count);
Stmt *ast_new_while(Expr *cond, Stmt **body, int body_count);
Stmt *ast_new_expr_stmt(Expr *expr);

Function *ast_new_function(const char *name, char **params, int param_count, Stmt **body, int body_count);
Program *ast_new_program(void);
void ast_program_add_function(Program *prog, Function *func);

#endif // AST_H
