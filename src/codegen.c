#include "codegen.h"
#include <stdlib.h>
#include <string.h>

typedef enum {
    TYPE_INT,
    TYPE_STRING,
    TYPE_ARRAY,
    TYPE_STRUCT
} VarType;

typedef struct {
    char *name;
    int offset;
    VarType type;
    char *struct_name;
} LocalVar;

typedef struct {
    LocalVar vars[256];
    int count;
    int stack_size;
    int loop_start;
    int loop_step;
    int loop_end;
} Scope;

typedef struct {
    int id;
    char *str;
    int len;
} StringLiteral;

static StringLiteral strings[512];
static int string_count = 0;
static int label_counter = 0;
static Program *current_prog = NULL;

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

static LocalVar *scope_find_var(Scope *scope, const char *name) {
    for (int i = 0; i < scope->count; i++) {
        if (strcmp(scope->vars[i].name, name) == 0) {
            return &scope->vars[i];
        }
    }
    return NULL;
}

static LocalVar *scope_add_var(Scope *scope, const char *name, VarType type) {
    LocalVar *existing = scope_find_var(scope, name);
    if (existing) {
        existing->type = type;
        return existing;
    }

    scope->stack_size += 8;
    LocalVar *v = &scope->vars[scope->count++];
    v->name = strdup(name);
    v->offset = scope->stack_size;
    v->type = type;
    v->struct_name = NULL;
    return v;
}

typedef struct {
    char *name;
    VarType return_type;
} FunctionMeta;

static FunctionMeta func_metas[256];
static int func_meta_count = 0;

static void register_function_meta(const char *name, VarType ret_type) {
    for (int i = 0; i < func_meta_count; i++) {
        if (strcmp(func_metas[i].name, name) == 0) {
            func_metas[i].return_type = ret_type;
            return;
        }
    }
    func_metas[func_meta_count].name = strdup(name);
    func_metas[func_meta_count].return_type = ret_type;
    func_meta_count++;
}

static VarType find_function_return_type(const char *name) {
    for (int i = 0; i < func_meta_count; i++) {
        if (strcmp(func_metas[i].name, name) == 0) {
            return func_metas[i].return_type;
        }
    }
    return TYPE_INT;
}

static int find_struct_field_offset(const char *struct_name, const char *field) {
    if (!current_prog || !field) return 0;
    if (struct_name) {
        StructDef *s = ast_program_find_struct(current_prog, struct_name);
        if (s) {
            for (int i = 0; i < s->field_count; i++) {
                if (strcmp(s->fields[i], field) == 0) {
                    return i * 8;
                }
            }
        }
    }
    for (int i = 0; i < current_prog->struct_count; i++) {
        StructDef *cur = current_prog->structs[i];
        for (int f = 0; f < cur->field_count; f++) {
            if (strcmp(cur->fields[f], field) == 0) {
                return f * 8;
            }
        }
    }
    return 0;
}

static VarType infer_expr_type(Expr *expr, Scope *scope) {
    if (!expr) return TYPE_INT;
    switch (expr->type) {
        case EXPR_INT: return TYPE_INT;
        case EXPR_STRING: return TYPE_STRING;
        case EXPR_KIRIT: return TYPE_STRING;
        case EXPR_ARRAY_LITERAL: return TYPE_ARRAY;
        case EXPR_YANGI: return TYPE_STRUCT;
        case EXPR_UZUNLIK: return TYPE_INT;
        case EXPR_CALL:
            return find_function_return_type(expr->as.call.name);
        case EXPR_VAR: {
            LocalVar *v = scope_find_var(scope, expr->as.var_name);
            return v ? v->type : TYPE_INT;
        }
        case EXPR_BINARY: {
            if (expr->as.binary.op == TOKEN_PLUS) {
                VarType lt = infer_expr_type(expr->as.binary.left, scope);
                VarType rt = infer_expr_type(expr->as.binary.right, scope);
                if (lt == TYPE_STRING || rt == TYPE_STRING) return TYPE_STRING;
            }
            return TYPE_INT;
        }
        default: return TYPE_INT;
    }
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
            LocalVar *v = scope_find_var(scope, expr->as.var_name);
            if (!v) {
                fprintf(out, "    mov rax, 0\n");
            } else {
                fprintf(out, "    mov rax, [rbp - %d]\n", v->offset);
            }
            break;
        }

        case EXPR_UNARY: {
            gen_expr(expr->as.unary.operand, scope, out);
            if (expr->as.unary.op == TOKEN_MINUS) {
                fprintf(out, "    neg rax\n");
            } else if (expr->as.unary.op == TOKEN_INKOR) {
                fprintf(out, "    cmp rax, 0\n");
                fprintf(out, "    sete al\n");
                fprintf(out, "    movzx rax, al\n");
            }
            break;
        }

        case EXPR_BINARY: {
            VarType lt = infer_expr_type(expr->as.binary.left, scope);
            VarType rt = infer_expr_type(expr->as.binary.right, scope);

            // Matnlarni birlashtirish (string concatenation: str + str)
            if (expr->as.binary.op == TOKEN_PLUS && (lt == TYPE_STRING || rt == TYPE_STRING)) {
                gen_expr(expr->as.binary.left, scope, out);
                fprintf(out, "    push rax\n");
                gen_expr(expr->as.binary.right, scope, out);
                fprintf(out, "    mov rsi, rax\n");
                fprintf(out, "    pop rdi\n");
                fprintf(out, "    call algo_matn_birlashtir\n");
                break;
            }

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
                case TOKEN_VA:
                    fprintf(out, "    test rax, rax\n");
                    fprintf(out, "    setnz al\n");
                    fprintf(out, "    test rbx, rbx\n");
                    fprintf(out, "    setnz bl\n");
                    fprintf(out, "    and al, bl\n");
                    fprintf(out, "    movzx rax, al\n");
                    break;
                case TOKEN_YOKI:
                    fprintf(out, "    or rax, rbx\n");
                    fprintf(out, "    setnz al\n");
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
            for (int i = (expr->as.call.arg_count > 6 ? 5 : expr->as.call.arg_count - 1); i >= 0; i--) {
                fprintf(out, "    pop %s\n", arg_regs[i]);
            }
            fprintf(out, "    call algo_%s\n", expr->as.call.name);
            break;
        }

        case EXPR_ARRAY_LITERAL: {
            int count = expr->as.array.count;
            // Xotira ajratamiz: 16 bayt header (uzunlik, sig'im) + count * 8 bayt elementlar
            fprintf(out, "    mov rax, %d\n", 16 + count * 8);
            fprintf(out, "    call algo_ajrat\n");
            fprintf(out, "    push rax\n"); // massiv ko'rsatkichi

            // Header yozish
            fprintf(out, "    mov qword ptr [rax], %d\n", count);     // uzunlik
            fprintf(out, "    mov qword ptr [rax + 8], %d\n", count); // sig'im

            // Elementlarni yozish
            for (int i = 0; i < count; i++) {
                gen_expr(expr->as.array.elements[i], scope, out);
                fprintf(out, "    mov rbx, [rsp]\n"); // massiv ko'rsatkichini olamiz
                fprintf(out, "    mov [rbx + %d], rax\n", 16 + i * 8);
            }
            fprintf(out, "    pop rax\n");
            break;
        }

        case EXPR_INDEX: {
            gen_expr(expr->as.index.target, scope, out);
            fprintf(out, "    push rax\n");
            gen_expr(expr->as.index.index, scope, out);
            fprintf(out, "    pop rbx\n"); // rbx = massiv ko'rsatkichi, rax = indeks
            fprintf(out, "    mov rax, [rbx + 16 + rax * 8]\n");
            break;
        }

        case EXPR_DOT: {
            gen_expr(expr->as.dot.target, scope, out);
            int offset = find_struct_field_offset(NULL, expr->as.dot.field);
            fprintf(out, "    mov rax, [rax + %d]\n", offset);
            break;
        }

        case EXPR_KIRIT: {
            fprintf(out, "    call algo_kirit\n");
            break;
        }

        case EXPR_UZUNLIK: {
            gen_expr(expr->as.uzunlik_expr, scope, out);
            // Massiv yoki satrning birinchi 8 bayti - bu uzunlik
            fprintf(out, "    mov rax, [rax]\n");
            break;
        }

        case EXPR_YANGI: {
            StructDef *s = current_prog ? ast_program_find_struct(current_prog, expr->as.yangi_struct_name) : NULL;
            int fields = s ? s->field_count : 8;
            fprintf(out, "    mov rax, %d\n", fields * 8);
            fprintf(out, "    call algo_ajrat\n");
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

        case STMT_BREAK:
            if (scope->loop_end != -1) {
                fprintf(out, "    jmp .L_loop_end_%d\n", scope->loop_end);
            }
            break;

        case STMT_CONTINUE:
            if (scope->loop_step != -1) {
                fprintf(out, "    jmp .L_loop_step_%d\n", scope->loop_step);
            } else if (scope->loop_start != -1) {
                fprintf(out, "    jmp .L_loop_start_%d\n", scope->loop_start);
            }
            break;

        case STMT_VAR_ASSIGN: {
            VarType type = infer_expr_type(stmt->as.var_assign.value, scope);
            LocalVar *v = scope_add_var(scope, stmt->as.var_assign.var_name, type);
            gen_expr(stmt->as.var_assign.value, scope, out);
            fprintf(out, "    mov [rbp - %d], rax\n", v->offset);
            break;
        }

        case STMT_INDEX_ASSIGN: {
            gen_expr(stmt->as.index_assign.target, scope, out);
            fprintf(out, "    push rax\n");
            gen_expr(stmt->as.index_assign.index, scope, out);
            fprintf(out, "    push rax\n");
            gen_expr(stmt->as.index_assign.value, scope, out);
            fprintf(out, "    pop rcx\n"); // indeks
            fprintf(out, "    pop rbx\n"); // massiv ko'rsatkichi
            fprintf(out, "    mov [rbx + 16 + rcx * 8], rax\n");
            break;
        }

        case STMT_DOT_ASSIGN: {
            gen_expr(stmt->as.dot_assign.target, scope, out);
            fprintf(out, "    push rax\n");
            gen_expr(stmt->as.dot_assign.value, scope, out);
            fprintf(out, "    pop rbx\n"); // obyekt
            int offset = find_struct_field_offset(NULL, stmt->as.dot_assign.field);
            fprintf(out, "    mov [rbx + %d], rax\n", offset);
            break;
        }

        case STMT_CHIQAR: {
            for (int i = 0; i < stmt->as.chiqar.arg_count; i++) {
                Expr *arg = stmt->as.chiqar.args[i];
                VarType type = infer_expr_type(arg, scope);
                gen_expr(arg, scope, out);
                if (type == TYPE_STRING) {
                    fprintf(out, "    call algo_chiqar_satr\n");
                } else if (type == TYPE_ARRAY) {
                    fprintf(out, "    call algo_chiqar_massiv\n");
                } else {
                    fprintf(out, "    call algo_chiqar_son_oddiy\n");
                }
                // Argumentlar orasiga bo'sh joy
                if (i < stmt->as.chiqar.arg_count - 1) {
                    fprintf(out, "    lea rsi, [rip + algo_space]\n");
                    fprintf(out, "    mov rdx, 1\n");
                    fprintf(out, "    mov rax, 1\n");
                    fprintf(out, "    mov rdi, 1\n");
                    fprintf(out, "    syscall\n");
                }
            }
            // Satr oxirida yangi qator (\n)
            fprintf(out, "    lea rsi, [rip + algo_newline]\n");
            fprintf(out, "    mov rdx, 1\n");
            fprintf(out, "    mov rax, 1\n");
            fprintf(out, "    mov rdi, 1\n");
            fprintf(out, "    syscall\n");
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
            int old_start = scope->loop_start;
            int old_end = scope->loop_end;
            int old_step = scope->loop_step;

            scope->loop_start = id;
            scope->loop_end = id;
            scope->loop_step = id;

            fprintf(out, ".L_loop_start_%d:\n", id);
            gen_expr(stmt->as.while_stmt.condition, scope, out);
            fprintf(out, "    cmp rax, 0\n");
            fprintf(out, "    je .L_loop_end_%d\n", id);

            for (int i = 0; i < stmt->as.while_stmt.body_count; i++) {
                gen_stmt(stmt->as.while_stmt.body[i], scope, out);
            }

            fprintf(out, "    jmp .L_loop_start_%d\n", id);
            fprintf(out, ".L_loop_end_%d:\n", id);

            scope->loop_start = old_start;
            scope->loop_end = old_end;
            scope->loop_step = old_step;
            break;
        }

        case STMT_FOR: {
            int id = label_counter++;
            int old_start = scope->loop_start;
            int old_end = scope->loop_end;
            int old_step = scope->loop_step;

            scope->loop_start = id;
            scope->loop_end = id;
            scope->loop_step = id;

            if (stmt->as.for_stmt.init) {
                gen_stmt(stmt->as.for_stmt.init, scope, out);
            }

            fprintf(out, ".L_loop_start_%d:\n", id);
            if (stmt->as.for_stmt.condition) {
                gen_expr(stmt->as.for_stmt.condition, scope, out);
                fprintf(out, "    cmp rax, 0\n");
                fprintf(out, "    je .L_loop_end_%d\n", id);
            }

            for (int i = 0; i < stmt->as.for_stmt.body_count; i++) {
                gen_stmt(stmt->as.for_stmt.body[i], scope, out);
            }

            fprintf(out, ".L_loop_step_%d:\n", id);
            if (stmt->as.for_stmt.step) {
                gen_stmt(stmt->as.for_stmt.step, scope, out);
            }

            fprintf(out, "    jmp .L_loop_start_%d\n", id);
            fprintf(out, ".L_loop_end_%d:\n", id);

            scope->loop_start = old_start;
            scope->loop_end = old_end;
            scope->loop_step = old_step;
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
    scope.loop_start = -1;
    scope.loop_step = -1;
    scope.loop_end = -1;

    const char *arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

    fprintf(out, "\n.global algo_%s\n", func->name);
    fprintf(out, "algo_%s:\n", func->name);
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    sub rsp, 256\n");

    for (int i = 0; i < func->param_count && i < 6; i++) {
        LocalVar *v = scope_add_var(&scope, func->params[i], TYPE_INT);
        fprintf(out, "    mov [rbp - %d], %s\n", v->offset, arg_regs[i]);
    }

    for (int i = 0; i < func->body_count; i++) {
        gen_stmt(func->body[i], &scope, out);
    }

    fprintf(out, "    mov rax, 0\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n");
}

void codegen_generate(Program *prog, FILE *out) {
    current_prog = prog;

    // Funksiyalarning qaytarish turlarini tahlil qilish
    for (int f = 0; f < prog->function_count; f++) {
        Function *func = prog->functions[f];
        VarType ret_type = TYPE_INT;
        for (int s = 0; s < func->body_count; s++) {
            if (func->body[s]->type == STMT_RETURN) {
                Expr *ret_expr = func->body[s]->as.return_expr;
                if (ret_expr) {
                    if (ret_expr->type == EXPR_ARRAY_LITERAL) ret_type = TYPE_ARRAY;
                    else if (ret_expr->type == EXPR_STRING || ret_expr->type == EXPR_KIRIT) ret_type = TYPE_STRING;
                    else if (ret_expr->type == EXPR_VAR) {
                        // Agar parametr bo'lsa yoki o'zgaruvchi
                        // Masalan "qaytar massiv"
                        if (strstr(ret_expr->as.var_name, "massiv") || strstr(ret_expr->as.var_name, "royxat")) {
                            ret_type = TYPE_ARRAY;
                        } else if (strstr(ret_expr->as.var_name, "matn") || strstr(ret_expr->as.var_name, "satr")) {
                            ret_type = TYPE_STRING;
                        }
                    }
                }
            }
        }
        register_function_meta(func->name, ret_type);
    }

    fprintf(out, ".intel_syntax noprefix\n");
    fprintf(out, ".text\n");

    // Dastur kirish nuqtasi (_start)
    fprintf(out, ".global _start\n");
    fprintf(out, "_start:\n");
    fprintf(out, "    call algo_asosiy\n");
    fprintf(out, "    mov rdi, rax\n");
    fprintf(out, "    mov rax, 60\n"); // sys_exit
    fprintf(out, "    syscall\n\n");

    // 1. Dinamik Xotira Taqsimlagich (Heap Allocator via mmap)
    fprintf(out, "algo_ajrat:\n");
    fprintf(out, "    # rax = so'ralgan baytlar soni\n");
    fprintf(out, "    add rax, 15\n");
    fprintf(out, "    and rax, -16\n");
    fprintf(out, "    mov r8, rax\n"); // r8 = tekislangan hajm\n");
    fprintf(out, "    mov rbx, [rip + algo_heap_ptr]\n");
    fprintf(out, "    test rbx, rbx\n");
    fprintf(out, "    jnz .L_alloc_have_heap\n");
    fprintf(out, "    # 1MB mmap bilan xotira havzasini ochamiz\n");
    fprintf(out, "    push r8\n");
    fprintf(out, "    mov rax, 9\n");
    fprintf(out, "    xor rdi, rdi\n");
    fprintf(out, "    mov rsi, 1048576\n");
    fprintf(out, "    mov rdx, 3\n");
    fprintf(out, "    mov r10, 34\n");
    fprintf(out, "    mov r8, -1\n");
    fprintf(out, "    xor r9, r9\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    mov [rip + algo_heap_start], rax\n");
    fprintf(out, "    mov [rip + algo_heap_ptr], rax\n");
    fprintf(out, "    add rax, 1048576\n");
    fprintf(out, "    mov [rip + algo_heap_end], rax\n");
    fprintf(out, "    pop r8\n");
    fprintf(out, "    mov rbx, [rip + algo_heap_ptr]\n");
    fprintf(out, ".L_alloc_have_heap:\n");
    fprintf(out, "    mov rcx, rbx\n");
    fprintf(out, "    add rcx, r8\n");
    fprintf(out, "    cmp rcx, [rip + algo_heap_end]\n");
    fprintf(out, "    jle .L_alloc_ok\n");
    fprintf(out, "    push r8\n");
    fprintf(out, "    mov rax, 9\n");
    fprintf(out, "    xor rdi, rdi\n");
    fprintf(out, "    mov rsi, 1048576\n");
    fprintf(out, "    mov rdx, 3\n");
    fprintf(out, "    mov r10, 34\n");
    fprintf(out, "    mov r8, -1\n");
    fprintf(out, "    xor r9, r9\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    mov [rip + algo_heap_ptr], rax\n");
    fprintf(out, "    add rax, 1048576\n");
    fprintf(out, "    mov [rip + algo_heap_end], rax\n");
    fprintf(out, "    pop r8\n");
    fprintf(out, "    mov rbx, [rip + algo_heap_ptr]\n");
    fprintf(out, ".L_alloc_ok:\n");
    fprintf(out, "    mov rax, rbx\n");
    fprintf(out, "    add rbx, r8\n");
    fprintf(out, "    mov [rip + algo_heap_ptr], rbx\n");
    fprintf(out, "    ret\n\n");

    // 2. Matnlarni birlashtirish (String Concatenation)
    fprintf(out, "algo_matn_birlashtir:\n");
    fprintf(out, "    # rdi = str1, rsi = str2\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    push r13\n");
    fprintf(out, "    push r14\n");
    fprintf(out, "    mov r12, rdi\n");
    fprintf(out, "    mov r13, rsi\n");
    fprintf(out, "    mov r14, [r12]\n"); // len1
    fprintf(out, "    add r14, [r13]\n"); // len1 + len2
    fprintf(out, "    mov rax, r14\n");
    fprintf(out, "    add rax, 16\n");    // 8 bayt len + matn + null
    fprintf(out, "    call algo_ajrat\n");
    fprintf(out, "    mov [rax], r14\n"); // yangi uzunlik
    fprintf(out, "    # str1 baytlarini ko'chirish\n");
    fprintf(out, "    mov rcx, [r12]\n");
    fprintf(out, "    lea rsi, [r12 + 8]\n");
    fprintf(out, "    lea rdi, [rax + 8]\n");
    fprintf(out, "    push rax\n");
    fprintf(out, "    rep movsb\n");
    fprintf(out, "    # str2 baytlarini ko'chirish\n");
    fprintf(out, "    mov rcx, [r13]\n");
    fprintf(out, "    lea rsi, [r13 + 8]\n");
    fprintf(out, "    rep movsb\n");
    fprintf(out, "    mov byte ptr [rdi], 0\n"); // null terminator
    fprintf(out, "    pop rax\n");
    fprintf(out, "    pop r14\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    ret\n\n");

    // 3. Foydalanuvchidan kiritish (kirit via sys_read)
    fprintf(out, "algo_kirit:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    mov rax, 272\n"); // 16 bayt header + 256 bayt matn
    fprintf(out, "    call algo_ajrat\n");
    fprintf(out, "    push rax\n");
    fprintf(out, "    lea rsi, [rax + 8]\n");
    fprintf(out, "    mov rdx, 255\n");
    fprintf(out, "    mov rdi, 0\n");   // stdin
    fprintf(out, "    mov rax, 0\n");   // sys_read
    fprintf(out, "    syscall\n");
    fprintf(out, "    # rax = o'qilgan baytlar soni\n");
    fprintf(out, "    pop rbx\n");
    fprintf(out, "    test rax, rax\n");
    fprintf(out, "    jle .L_kirit_empty\n");
    fprintf(out, "    # trailing newline tozalash\n");
    fprintf(out, "    lea rcx, [rbx + 8 + rax - 1]\n");
    fprintf(out, "    cmp byte ptr [rcx], 10\n");
    fprintf(out, "    jne .L_kirit_done\n");
    fprintf(out, "    dec rax\n");
    fprintf(out, "    mov byte ptr [rcx], 0\n");
    fprintf(out, "    jmp .L_kirit_done\n");
    fprintf(out, ".L_kirit_empty:\n");
    fprintf(out, "    xor rax, rax\n");
    fprintf(out, ".L_kirit_done:\n");
    fprintf(out, "    mov [rbx], rax\n"); // uzunlik
    fprintf(out, "    mov rax, rbx\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 4. Matn chiqarish
    fprintf(out, "algo_chiqar_satr:\n");
    fprintf(out, "    mov rdx, [rax]\n");     // uzunlik
    fprintf(out, "    lea rsi, [rax + 8]\n"); // matn manzili
    fprintf(out, "    mov rdi, 1\n");         // stdout
    fprintf(out, "    mov rax, 1\n");         // sys_write
    fprintf(out, "    syscall\n");
    fprintf(out, "    ret\n\n");

    // 5. Son chiqarish (yangi qatorsiz)
    fprintf(out, "algo_chiqar_son_oddiy:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    sub rsp, 64\n");
    fprintf(out, "    lea rcx, [rbp - 1]\n");
    fprintf(out, "    mov r8, 0\n");
    fprintf(out, "    test rax, rax\n");
    fprintf(out, "    jnz .L_num_nonzero\n");
    fprintf(out, "    dec rcx\n");
    fprintf(out, "    mov byte ptr [rcx], 48\n"); // '0'
    fprintf(out, "    inc r8\n");
    fprintf(out, "    jmp .L_num_print\n");
    fprintf(out, ".L_num_nonzero:\n");
    fprintf(out, "    mov r9, 0\n");
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
    fprintf(out, "    mov rax, 1\n");
    fprintf(out, "    mov rdi, 1\n");
    fprintf(out, "    mov rsi, rcx\n");
    fprintf(out, "    mov rdx, r8\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 6. Massivni chiqarish ([elem1, elem2, ...])
    fprintf(out, "algo_chiqar_massiv:\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    push r13\n");
    fprintf(out, "    mov r12, rax\n");   // massiv ko'rsatkichi
    fprintf(out, "    mov r13, [r12]\n"); // uzunlik
    fprintf(out, "    lea rsi, [rip + algo_lbracket]\n");
    fprintf(out, "    mov rdx, 1\n");
    fprintf(out, "    mov rax, 1\n");
    fprintf(out, "    mov rdi, 1\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    xor rbx, rbx\n");   // index = 0
    fprintf(out, ".L_arr_loop:\n");
    fprintf(out, "    cmp rbx, r13\n");
    fprintf(out, "    jge .L_arr_done\n");
    fprintf(out, "    mov rax, [r12 + 16 + rbx * 8]\n");
    fprintf(out, "    push rbx\n");
    fprintf(out, "    call algo_chiqar_son_oddiy\n");
    fprintf(out, "    pop rbx\n");
    fprintf(out, "    inc rbx\n");
    fprintf(out, "    cmp rbx, r13\n");
    fprintf(out, "    jge .L_arr_done\n");
    fprintf(out, "    lea rsi, [rip + algo_comma_space]\n");
    fprintf(out, "    mov rdx, 2\n");
    fprintf(out, "    mov rax, 1\n");
    fprintf(out, "    mov rdi, 1\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    jmp .L_arr_loop\n");
    fprintf(out, ".L_arr_done:\n");
    fprintf(out, "    lea rsi, [rip + algo_rbracket]\n");
    fprintf(out, "    mov rdx, 1\n");
    fprintf(out, "    mov rax, 1\n");
    fprintf(out, "    mov rdi, 1\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    ret\n\n");

    // 7. Fayl tizimi amallari (Built-in File I/O)
    fprintf(out, "algo_fayl_och:\n");
    fprintf(out, "    # rdi = fayl_nomi (str), rsi = flags (0=read, 577=write/create/trunc)\n");
    fprintf(out, "    lea rdi, [rdi + 8]\n"); // matn boshiga
    fprintf(out, "    mov rdx, 420\n");       // 0644 ruxsati
    fprintf(out, "    mov rax, 2\n");         // sys_open
    fprintf(out, "    syscall\n");
    fprintf(out, "    ret\n\n");

    fprintf(out, "algo_fayl_yop:\n");
    fprintf(out, "    # rdi = fd\n");
    fprintf(out, "    mov rax, 3\n");         // sys_close
    fprintf(out, "    syscall\n");
    fprintf(out, "    ret\n\n");

    fprintf(out, "algo_fayl_yoz:\n");
    fprintf(out, "    # rdi = fd, rsi = matn (str)\n");
    fprintf(out, "    mov rdx, [rsi]\n");     // matn uzunligi
    fprintf(out, "    lea rsi, [rsi + 8]\n");
    fprintf(out, "    mov rax, 1\n");         // sys_write
    fprintf(out, "    syscall\n");
    fprintf(out, "    ret\n\n");

    // Barcha foydalanuvchi funksiyalarini generatsiya qilish
    for (int i = 0; i < prog->function_count; i++) {
        gen_function(prog->functions[i], out);
    }

    // .data va .rodata bo'limi
    fprintf(out, "\n.section .data\n");
    fprintf(out, "algo_heap_start: .quad 0\n");
    fprintf(out, "algo_heap_ptr:   .quad 0\n");
    fprintf(out, "algo_heap_end:   .quad 0\n");

    fprintf(out, "\n.section .rodata\n");
    fprintf(out, "algo_newline:\n    .byte 10\n");
    fprintf(out, "algo_space:\n    .byte 32\n");
    fprintf(out, "algo_lbracket:\n    .byte 91\n");
    fprintf(out, "algo_rbracket:\n    .byte 93\n");
    fprintf(out, "algo_comma_space:\n    .ascii \", \"\n");

    for (int i = 0; i < string_count; i++) {
        fprintf(out, ".LC%d:\n", strings[i].id);
        fprintf(out, "    .quad %d\n", strings[i].len);
        fprintf(out, "    .ascii \"");
        for (int c = 0; c < strings[i].len; c++) {
            unsigned char ch = (unsigned char)strings[i].str[c];
            if (ch == '\n') fprintf(out, "\\n");
            else if (ch == '\r') fprintf(out, "\\r");
            else if (ch == '\t') fprintf(out, "\\t");
            else if (ch == '\"') fprintf(out, "\\\"");
            else if (ch == '\\') fprintf(out, "\\\\");
            else if (ch >= 32 && ch <= 126) fputc(ch, out);
            else fprintf(out, "\\%03o", ch);
        }
        fprintf(out, "\\0\"\n");
    }
}
