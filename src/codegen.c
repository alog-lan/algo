#include "codegen.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *name;
    int offset;
} LocalVar;

typedef struct {
    LocalVar vars[256];
    int count;
    int stack_size;
} Scope;

typedef struct {
    int id;
    char *str;
    int len;
} StringLiteral;

static StringLiteral strings[512];
static int string_count = 0;
static int label_counter = 0;

static int add_string_literal(const char *str) {
    for (int i = 0; i < string_count; i++) {
        if (strcmp(strings[i].str, str) == 0) {
            return strings[i].id;
        }
    }
    int id = string_count++;
    strings[id].id = id;
    strings[id].str = strdup(str);
    strings[id].len = (int)strlen(str);
    return id;
}

static int scope_find_var(Scope *scope, const char *name) {
    for (int i = 0; i < scope->count; i++) {
        if (strcmp(scope->vars[i].name, name) == 0) {
            return scope->vars[i].offset;
        }
    }
    return -1;
}

static int scope_add_var(Scope *scope, const char *name) {
    int existing = scope_find_var(scope, name);
    if (existing != -1) return existing;

    scope->stack_size += 8;
    scope->vars[scope->count].name = strdup(name);
    scope->vars[scope->count].offset = scope->stack_size;
    scope->count++;
    return scope->stack_size;
}

// Prototip
static void gen_expr(Expr *expr, Scope *scope, FILE *out);
static void gen_stmt(Stmt *stmt, Scope *scope, FILE *out);

static void gen_expr(Expr *expr, Scope *scope, FILE *out) {
    if (!expr) return;

    switch (expr->type) {
        case EXPR_INT:
            fprintf(out, "    mov rax, %ld\n", (long)expr->as.int_val);
            break;

        case EXPR_STRING: {
            int str_id = add_string_literal(expr->as.str_val);
            fprintf(out, "    lea rax, [rip + .LC%d]\n", str_id);
            break;
        }

        case EXPR_VAR: {
            int offset = scope_find_var(scope, expr->as.var_name);
            if (offset == -1) {
                // Aniqlanmagan o'zgaruvchi -> 0
                fprintf(out, "    mov rax, 0\n");
            } else {
                fprintf(out, "    mov rax, [rbp - %d]\n", offset);
            }
            break;
        }

        case EXPR_BINARY: {
            gen_expr(expr->as.binary.left, scope, out);
            fprintf(out, "    push rax\n");
            gen_expr(expr->as.binary.right, scope, out);
            fprintf(out, "    mov rbx, rax\n");
            fprintf(out, "    pop rax\n");

            switch (expr->as.binary.op) {
                case TOKEN_PLUS:
                    fprintf(out, "    add rax, rbx\n");
                    break;
                case TOKEN_MINUS:
                    fprintf(out, "    sub rax, rbx\n");
                    break;
                case TOKEN_STAR:
                    fprintf(out, "    imul rax, rbx\n");
                    break;
                case TOKEN_SLASH:
                    fprintf(out, "    cqo\n");
                    fprintf(out, "    idiv rbx\n");
                    break;
                case TOKEN_PERCENT:
                    fprintf(out, "    cqo\n");
                    fprintf(out, "    idiv rbx\n");
                    fprintf(out, "    mov rax, rdx\n");
                    break;
                case TOKEN_EQ:
                    fprintf(out, "    cmp rax, rbx\n");
                    fprintf(out, "    sete al\n");
                    fprintf(out, "    movzx rax, al\n");
                    break;
                case TOKEN_NEQ:
                    fprintf(out, "    cmp rax, rbx\n");
                    fprintf(out, "    setne al\n");
                    fprintf(out, "    movzx rax, al\n");
                    break;
                case TOKEN_LT:
                    fprintf(out, "    cmp rax, rbx\n");
                    fprintf(out, "    setl al\n");
                    fprintf(out, "    movzx rax, al\n");
                    break;
                case TOKEN_LTE:
                    fprintf(out, "    cmp rax, rbx\n");
                    fprintf(out, "    setle al\n");
                    fprintf(out, "    movzx rax, al\n");
                    break;
                case TOKEN_GT:
                    fprintf(out, "    cmp rax, rbx\n");
                    fprintf(out, "    setg al\n");
                    fprintf(out, "    movzx rax, al\n");
                    break;
                case TOKEN_GTE:
                    fprintf(out, "    cmp rax, rbx\n");
                    fprintf(out, "    setge al\n");
                    fprintf(out, "    movzx rax, al\n");
                    break;
                default:
                    break;
            }
            break;
        }

        case EXPR_CALL: {
            const char *arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
            for (int i = 0; i < expr->as.call.arg_count && i < 6; i++) {
                gen_expr(expr->as.call.args[i], scope, out);
                fprintf(out, "    push rax\n");
            }
            // Argumentlarni registrlarga yuklaymiz (teskari tartibda pop qilib)
            for (int i = (expr->as.call.arg_count > 6 ? 5 : expr->as.call.arg_count - 1); i >= 0; i--) {
                fprintf(out, "    pop %s\n", arg_regs[i]);
            }
            fprintf(out, "    call algo_%s\n", expr->as.call.name);
            break;
        }
    }
}

static void gen_stmt(Stmt *stmt, Scope *scope, FILE *out) {
    if (!stmt) return;

    switch (stmt->type) {
        case STMT_RETURN:
            gen_expr(stmt->as.return_expr, scope, out);
            fprintf(out, "    mov rsp, rbp\n");
            fprintf(out, "    pop rbp\n");
            fprintf(out, "    ret\n");
            break;

        case STMT_VAR_ASSIGN: {
            int offset = scope_add_var(scope, stmt->as.var_assign.var_name);
            gen_expr(stmt->as.var_assign.value, scope, out);
            fprintf(out, "    mov [rbp - %d], rax\n", offset);
            break;
        }

        case STMT_CHIQAR: {
            Expr *e = stmt->as.chiqar_expr;
            if (e && e->type == EXPR_STRING) {
                int id = add_string_literal(e->as.str_val);
                fprintf(out, "    mov rax, 1\n");
                fprintf(out, "    mov rdi, 1\n");
                fprintf(out, "    lea rsi, [rip + .LC%d]\n", id);
                fprintf(out, "    mov rdx, %d\n", strings[id].len);
                fprintf(out, "    syscall\n");
                // Yangi qator
                fprintf(out, "    mov rax, 1\n");
                fprintf(out, "    mov rdi, 1\n");
                fprintf(out, "    lea rsi, [rip + algo_newline]\n");
                fprintf(out, "    mov rdx, 1\n");
                fprintf(out, "    syscall\n");
            } else {
                gen_expr(e, scope, out);
                fprintf(out, "    call algo_chiqar_son\n");
            }
            break;
        }

        case STMT_IF: {
            int id = label_counter++;
            gen_expr(stmt->as.if_stmt.condition, scope, out);
            fprintf(out, "    cmp rax, 0\n");
            if (stmt->as.if_stmt.else_count > 0) {
                fprintf(out, "    je .L_else_%d\n", id);
            } else {
                fprintf(out, "    je .L_end_%d\n", id);
            }

            for (int i = 0; i < stmt->as.if_stmt.then_count; i++) {
                gen_stmt(stmt->as.if_stmt.then_branch[i], scope, out);
            }

            if (stmt->as.if_stmt.else_count > 0) {
                fprintf(out, "    jmp .L_end_%d\n", id);
                fprintf(out, ".L_else_%d:\n", id);
                for (int i = 0; i < stmt->as.if_stmt.else_count; i++) {
                    gen_stmt(stmt->as.if_stmt.else_branch[i], scope, out);
                }
            }
            fprintf(out, ".L_end_%d:\n", id);
            break;
        }

        case STMT_WHILE: {
            int id = label_counter++;
            fprintf(out, ".L_loop_start_%d:\n", id);
            gen_expr(stmt->as.while_stmt.condition, scope, out);
            fprintf(out, "    cmp rax, 0\n");
            fprintf(out, "    je .L_loop_end_%d\n", id);

            for (int i = 0; i < stmt->as.while_stmt.body_count; i++) {
                gen_stmt(stmt->as.while_stmt.body[i], scope, out);
            }

            fprintf(out, "    jmp .L_loop_start_%d\n", id);
            fprintf(out, ".L_loop_end_%d:\n", id);
            break;
        }

        case STMT_EXPR:
            gen_expr(stmt->as.expr_stmt, scope, out);
            break;
    }
}

static void gen_function(Function *func, FILE *out) {
    Scope scope;
    scope.count = 0;
    scope.stack_size = 0;

    // Parametrlarni ro'yxatga olamiz va registrlardan o'qiymiz
    const char *arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

    fprintf(out, "\n.global algo_%s\n", func->name);
    fprintf(out, "algo_%s:\n", func->name);
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    sub rsp, 256\n"); // Xavfsiz stek maydoni

    for (int i = 0; i < func->param_count && i < 6; i++) {
        int offset = scope_add_var(&scope, func->params[i]);
        fprintf(out, "    mov [rbp - %d], %s\n", offset, arg_regs[i]);
    }

    for (int i = 0; i < func->body_count; i++) {
        gen_stmt(func->body[i], &scope, out);
    }

    // Default qaytish (agar qaytar yozilmagan bo'lsa)
    fprintf(out, "    mov rax, 0\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n");
}

void codegen_generate(Program *prog, FILE *out) {
    fprintf(out, ".intel_syntax noprefix\n");
    fprintf(out, ".text\n");

    // Dastur boshlanish nuqtasi (_start)
    fprintf(out, ".global _start\n");
    fprintf(out, "_start:\n");
    fprintf(out, "    call algo_asosiy\n");
    fprintf(out, "    mov rdi, rax\n");      // asosiy() qaytargan qiymat
    fprintf(out, "    mov rax, 60\n");       // sys_exit
    fprintf(out, "    syscall\n\n");

    // Sonlarni ekranga chiqaruvchi o'rnatilgan pure assembler funksiya (zero-libc)
    fprintf(out, "algo_chiqar_son:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    sub rsp, 64\n");
    fprintf(out, "    lea rcx, [rbp - 1]\n");
    fprintf(out, "    mov byte ptr [rcx], 10\n"); // yangi qator '\n'
    fprintf(out, "    mov r8, 1\n");              // uzunlik hisoblagich
    fprintf(out, "    test rax, rax\n");
    fprintf(out, "    jnz .L_num_nonzero\n");
    fprintf(out, "    dec rcx\n");
    fprintf(out, "    mov byte ptr [rcx], 48\n"); // '0'
    fprintf(out, "    inc r8\n");
    fprintf(out, "    jmp .L_num_print\n");
    fprintf(out, ".L_num_nonzero:\n");
    fprintf(out, "    mov r9, 0\n");              // manfiy belgisi
    fprintf(out, "    cmp rax, 0\n");
    fprintf(out, "    jge .L_num_loop\n");
    fprintf(out, "    mov r9, 1\n");
    fprintf(out, "    neg rax\n");
    fprintf(out, ".L_num_loop:\n");
    fprintf(out, "    test rax, rax\n");
    fprintf(out, "    jz .L_num_done\n");
    fprintf(out, "    mov rdx, 0\n");
    fprintf(out, "    mov rbx, 10\n");
    fprintf(out, "    div rbx\n");
    fprintf(out, "    add dl, 48\n");
    fprintf(out, "    dec rcx\n");
    fprintf(out, "    mov byte ptr [rcx], dl\n");
    fprintf(out, "    inc r8\n");
    fprintf(out, "    jmp .L_num_loop\n");
    fprintf(out, ".L_num_done:\n");
    fprintf(out, "    test r9, r9\n");
    fprintf(out, "    jz .L_num_print\n");
    fprintf(out, "    dec rcx\n");
    fprintf(out, "    mov byte ptr [rcx], 45\n"); // '-'
    fprintf(out, "    inc r8\n");
    fprintf(out, ".L_num_print:\n");
    fprintf(out, "    mov rax, 1\n");             // sys_write
    fprintf(out, "    mov rdi, 1\n");             // stdout
    fprintf(out, "    mov rsi, rcx\n");
    fprintf(out, "    mov rdx, r8\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // Dasturdagi barcha funksiyalarni yozish
    for (int i = 0; i < prog->function_count; i++) {
        gen_function(prog->functions[i], out);
    }

    // Ma'lumotlar bo'limi (.rodata)
    fprintf(out, "\n.section .rodata\n");
    fprintf(out, "algo_newline:\n    .byte 10\n");
    for (int i = 0; i < string_count; i++) {
        fprintf(out, ".LC%d:\n", strings[i].id);
        fprintf(out, "    .string \"%s\"\n", strings[i].str);
        fprintf(out, ".LC%d_len = . - .LC%d - 1\n", strings[i].id, strings[i].id);
    }
}
