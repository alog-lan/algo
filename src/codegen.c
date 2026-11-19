#include "codegen.h"
#include <stdlib.h>
#include <string.h>

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
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

typedef struct {
    int id;
    double val;
} FloatLiteral;

static StringLiteral strings[512];
static int string_count = 0;
static FloatLiteral floats[256];
static int float_count = 0;
static int label_counter = 0;
static Program *current_prog = NULL;

static int add_float_literal(double val) {
    for (int i = 0; i < float_count; i++) {
        if (floats[i].val == val) return floats[i].id;
    }
    int id = float_count++;
    floats[id].id = id;
    floats[id].val = val;
    return id;
}

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
    if (strcmp(name, "matn_kesish") == 0 ||
        strcmp(name, "matn_katta") == 0 ||
        strcmp(name, "matn_kichik") == 0 ||
        strcmp(name, "matn_qirqish") == 0 ||
        strcmp(name, "matn_almashtir") == 0 ||
        strcmp(name, "matn_birlashtir_massiv") == 0 ||
        strcmp(name, "belgi") == 0 ||
        strcmp(name, "son_matnga") == 0 ||
        strcmp(name, "fayl_oqish") == 0 ||
        strcmp(name, "faylni_oqish") == 0 ||
        strcmp(name, "muhit") == 0 ||
        strcmp(name, "muhit_ol") == 0) {
        return TYPE_STRING;
    }
    if (strcmp(name, "ildiz") == 0 ||
        strcmp(name, "kvadrat_ildiz") == 0 ||
        strcmp(name, "pi") == 0 ||
        strcmp(name, "e") == 0) {
        return TYPE_FLOAT;
    }
    if (strcmp(name, "massiv_kesish") == 0 ||
        strcmp(name, "massiv_nusxa") == 0 ||
        strcmp(name, "massiv_birlashtir") == 0 ||
        strcmp(name, "massiv_sarala") == 0 ||
        strcmp(name, "massiv_teskari") == 0 ||
        strcmp(name, "massiv_qosh") == 0 ||
        strcmp(name, "matn_bolish") == 0 ||
        strcmp(name, "algo_argumentlar") == 0 ||
        strcmp(name, "argumentlar") == 0) {
        return TYPE_ARRAY;
    }
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
        case EXPR_FLOAT: return TYPE_FLOAT;
        case EXPR_STRING: return TYPE_STRING;
        case EXPR_KIRIT: return TYPE_STRING;
        case EXPR_ARRAY_LITERAL: return TYPE_ARRAY;
        case EXPR_YANGI: return TYPE_STRUCT;
        case EXPR_UZUNLIK: return TYPE_INT;
        case EXPR_CALL:
            return find_function_return_type(expr->as.call.name);
        case EXPR_VAR: {
            LocalVar *v = scope_find_var(scope, expr->as.var_name);
            if (v && v->type != TYPE_INT) return v->type;
            const char *vn = expr->as.var_name;
            if (strstr(vn, "matn") || strstr(vn, "kalit") || strstr(vn, "key") || strstr(vn, "str") || strstr(vn, "nom") || strstr(vn, "yol")) return TYPE_STRING;
            if (strstr(vn, "massiv") || strstr(vn, "royxat") || strstr(vn, "arr")) return TYPE_ARRAY;
            return v ? v->type : TYPE_INT;
        }
        case EXPR_BINARY: {
            VarType lt = infer_expr_type(expr->as.binary.left, scope);
            VarType rt = infer_expr_type(expr->as.binary.right, scope);
            if (expr->as.binary.op == TOKEN_PLUS) {
                if (lt == TYPE_STRING || rt == TYPE_STRING) return TYPE_STRING;
            }
            if (lt == TYPE_FLOAT || rt == TYPE_FLOAT) {
                if (expr->as.binary.op == TOKEN_PLUS || expr->as.binary.op == TOKEN_MINUS ||
                    expr->as.binary.op == TOKEN_STAR || expr->as.binary.op == TOKEN_SLASH) {
                    return TYPE_FLOAT;
                }
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

        case EXPR_FLOAT: {
            int fid = add_float_literal(expr->as.float_val);
            fprintf(out, "    mov rax, [rip + .LC_f%d]\n", fid);
            break;
        }

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
            } else if (expr->as.unary.op == TOKEN_TILDE) {
                fprintf(out, "    not rax\n");
            }
            break;
        }

        case EXPR_BINARY: {
            VarType lt = infer_expr_type(expr->as.binary.left, scope);
            VarType rt = infer_expr_type(expr->as.binary.right, scope);

            // Matnlarni taqqoslash (string equality: str == str yoki str != str)
            if ((expr->as.binary.op == TOKEN_EQ || expr->as.binary.op == TOKEN_NEQ) &&
                (lt == TYPE_STRING || rt == TYPE_STRING)) {
                gen_expr(expr->as.binary.left, scope, out);
                fprintf(out, "    push rax\n");
                gen_expr(expr->as.binary.right, scope, out);
                fprintf(out, "    mov rsi, rax\n");
                fprintf(out, "    pop rdi\n");
                fprintf(out, "    call algo_matn_teng\n");
                if (expr->as.binary.op == TOKEN_NEQ) {
                    fprintf(out, "    xor rax, 1\n");
                }
                break;
            }

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

            // Haqiqiy (o'nlik) sonlar arifmetikasi (SSE xmm registers)
            if (lt == TYPE_FLOAT || rt == TYPE_FLOAT) {
                gen_expr(expr->as.binary.left, scope, out);
                fprintf(out, "    push rax\n");
                gen_expr(expr->as.binary.right, scope, out);
                fprintf(out, "    movq xmm1, rax\n");
                fprintf(out, "    pop rax\n");
                fprintf(out, "    movq xmm0, rax\n");
                if (lt == TYPE_INT) fprintf(out, "    cvtsi2sd xmm0, rax\n");
                if (rt == TYPE_INT) {
                    fprintf(out, "    movq rax, xmm1\n");
                    fprintf(out, "    cvtsi2sd xmm1, rax\n");
                }
                switch (expr->as.binary.op) {
                    case TOKEN_PLUS:  fprintf(out, "    addsd xmm0, xmm1\n"); break;
                    case TOKEN_MINUS: fprintf(out, "    subsd xmm0, xmm1\n"); break;
                    case TOKEN_STAR:  fprintf(out, "    mulsd xmm0, xmm1\n"); break;
                    case TOKEN_SLASH: fprintf(out, "    divsd xmm0, xmm1\n"); break;
                    default: break;
                }
                fprintf(out, "    movq rax, xmm0\n");
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
                case TOKEN_AMP:
                    fprintf(out, "    and rax, rbx\n");
                    break;
                case TOKEN_PIPE:
                    fprintf(out, "    or rax, rbx\n");
                    break;
                case TOKEN_CARET:
                    fprintf(out, "    xor rax, rbx\n");
                    break;
                case TOKEN_SHL:
                    fprintf(out, "    mov rcx, rbx\n");
                    fprintf(out, "    shl rax, cl\n");
                    break;
                case TOKEN_SHR:
                    fprintf(out, "    mov rcx, rbx\n");
                    fprintf(out, "    sar rax, cl\n");
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
            VarType idx_type = infer_expr_type(expr->as.index.index, scope);
            int is_dict = (idx_type == TYPE_STRING);
            if (!is_dict) {
                if (expr->as.index.target->type == EXPR_VAR) {
                    const char *tn = expr->as.index.target->as.var_name;
                    if (strstr(tn, "lugat") || strstr(tn, "dict") || strstr(tn, "map") || strstr(tn, "baza")) is_dict = 1;
                } else if (expr->as.index.target->type == EXPR_DOT) {
                    const char *fn = expr->as.index.target->as.dot.field;
                    if (strstr(fn, "lugat") || strstr(fn, "dict") || strstr(fn, "map") || strstr(fn, "malumot")) is_dict = 1;
                }
            }
            if (is_dict) {
                gen_expr(expr->as.index.target, scope, out);
                fprintf(out, "    push rax\n");
                gen_expr(expr->as.index.index, scope, out);
                fprintf(out, "    mov rsi, rax\n");
                fprintf(out, "    pop rdi\n");
                fprintf(out, "    call algo_lugat_ol\n");
            } else {
                gen_expr(expr->as.index.target, scope, out);
                fprintf(out, "    push rax\n");
                gen_expr(expr->as.index.index, scope, out);
                fprintf(out, "    pop rbx\n");
                fprintf(out, "    mov rax, [rbx + 16 + rax * 8]\n");
            }
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
            VarType idx_type = infer_expr_type(stmt->as.index_assign.index, scope);
            int is_dict = (idx_type == TYPE_STRING);
            if (!is_dict) {
                if (stmt->as.index_assign.target->type == EXPR_VAR) {
                    const char *tn = stmt->as.index_assign.target->as.var_name;
                    if (strstr(tn, "lugat") || strstr(tn, "dict") || strstr(tn, "map") || strstr(tn, "baza")) is_dict = 1;
                } else if (stmt->as.index_assign.target->type == EXPR_DOT) {
                    const char *fn = stmt->as.index_assign.target->as.dot.field;
                    if (strstr(fn, "lugat") || strstr(fn, "dict") || strstr(fn, "map") || strstr(fn, "malumot")) is_dict = 1;
                }
            }
            if (is_dict) {
                gen_expr(stmt->as.index_assign.target, scope, out);
                fprintf(out, "    push rax\n");
                gen_expr(stmt->as.index_assign.index, scope, out);
                fprintf(out, "    push rax\n");
                gen_expr(stmt->as.index_assign.value, scope, out);
                fprintf(out, "    mov rdx, rax\n"); // qiymat
                fprintf(out, "    pop rsi\n");     // kalit
                fprintf(out, "    pop rdi\n");     // lug'at
                fprintf(out, "    call algo_lugat_qoy\n");
            } else {
                gen_expr(stmt->as.index_assign.target, scope, out);
                fprintf(out, "    push rax\n");
                gen_expr(stmt->as.index_assign.index, scope, out);
                fprintf(out, "    push rax\n");
                gen_expr(stmt->as.index_assign.value, scope, out);
                fprintf(out, "    pop rcx\n"); // indeks
                fprintf(out, "    pop rbx\n"); // massiv ko'rsatkichi
                fprintf(out, "    mov [rbx + 16 + rcx * 8], rax\n");
            }
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
                } else if (type == TYPE_FLOAT) {
                    fprintf(out, "    call algo_chiqar_haqiqiy\n");
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
        VarType pt = TYPE_INT;
        const char *pname = func->params[i];
        if (strstr(pname, "matn") || strstr(pname, "kalit") || strstr(pname, "key") || strstr(pname, "str") || strstr(pname, "nom") || strstr(pname, "yol") || strstr(pname, "path") || strstr(pname, "sorov") || strstr(pname, "prefiks") || strstr(pname, "suffiks") || strstr(pname, "eski") || strstr(pname, "yangi") || strstr(pname, "ajratuvchi")) {
            pt = TYPE_STRING;
        } else if (strstr(pname, "massiv") || strstr(pname, "royxat") || strstr(pname, "arr")) {
            pt = TYPE_ARRAY;
        }
        LocalVar *v = scope_add_var(&scope, func->params[i], pt);
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

typedef struct {
    char name[64];
    VarType type;
} LocalTypeInfo;

static VarType get_expr_type_with_locals(Expr *e, LocalTypeInfo *locals, int local_count) {
    if (!e) return TYPE_INT;
    if (e->type == EXPR_INT) return TYPE_INT;
    if (e->type == EXPR_FLOAT) return TYPE_FLOAT;
    if (e->type == EXPR_STRING || e->type == EXPR_KIRIT) return TYPE_STRING;
    if (e->type == EXPR_ARRAY_LITERAL) return TYPE_ARRAY;
    if (e->type == EXPR_CALL) return find_function_return_type(e->as.call.name);
    if (e->type == EXPR_VAR) {
        for (int i = 0; i < local_count; i++) {
            if (strcmp(locals[i].name, e->as.var_name) == 0) return locals[i].type;
        }
        return TYPE_INT;
    }
    if (e->type == EXPR_BINARY) {
        VarType lt = get_expr_type_with_locals(e->as.binary.left, locals, local_count);
        VarType rt = get_expr_type_with_locals(e->as.binary.right, locals, local_count);
        if (e->as.binary.op == TOKEN_PLUS && (lt == TYPE_STRING || rt == TYPE_STRING)) return TYPE_STRING;
        if (lt == TYPE_FLOAT || rt == TYPE_FLOAT) return TYPE_FLOAT;
    }
    return TYPE_INT;
}

static VarType scan_stmt_return_type(Stmt *stmt, LocalTypeInfo *locals, int *local_count) {
    if (!stmt) return TYPE_INT;
    if (stmt->type == STMT_VAR_ASSIGN) {
        VarType vt = get_expr_type_with_locals(stmt->as.var_assign.value, locals, *local_count);
        const char *vname = stmt->as.var_assign.var_name;
        int found = 0;
        for (int i = 0; i < *local_count; i++) {
            if (strcmp(locals[i].name, vname) == 0) {
                locals[i].type = vt;
                found = 1;
                break;
            }
        }
        if (!found && *local_count < 64) {
            strncpy(locals[*local_count].name, vname, 63);
            locals[*local_count].name[63] = '\0';
            locals[*local_count].type = vt;
            (*local_count)++;
        }
        return TYPE_INT;
    }
    if (stmt->type == STMT_RETURN) {
        return get_expr_type_with_locals(stmt->as.return_expr, locals, *local_count);
    }
    if (stmt->type == STMT_IF) {
        for (int i = 0; i < stmt->as.if_stmt.then_count; i++) {
            VarType t = scan_stmt_return_type(stmt->as.if_stmt.then_branch[i], locals, local_count);
            if (t != TYPE_INT) return t;
        }
        for (int i = 0; i < stmt->as.if_stmt.else_count; i++) {
            VarType t = scan_stmt_return_type(stmt->as.if_stmt.else_branch[i], locals, local_count);
            if (t != TYPE_INT) return t;
        }
    }
    if (stmt->type == STMT_WHILE) {
        for (int i = 0; i < stmt->as.while_stmt.body_count; i++) {
            VarType t = scan_stmt_return_type(stmt->as.while_stmt.body[i], locals, local_count);
            if (t != TYPE_INT) return t;
        }
    }
    if (stmt->type == STMT_FOR) {
        for (int i = 0; i < stmt->as.for_stmt.body_count; i++) {
            VarType t = scan_stmt_return_type(stmt->as.for_stmt.body[i], locals, local_count);
            if (t != TYPE_INT) return t;
        }
    }
    return TYPE_INT;
}

void codegen_generate(Program *prog, FILE *out) {
    current_prog = prog;
    string_count = 0;
    float_count = 0;
    label_counter = 0;
    func_meta_count = 0;

    // Funksiyalarning qaytarish turlarini tahlil qilish (3 o'tish bilan)
    for (int pass = 0; pass < 3; pass++) {
        for (int f = 0; f < prog->function_count; f++) {
            Function *func = prog->functions[f];
            VarType ret_type = find_function_return_type(func->name);
            LocalTypeInfo locals[64];
            int local_count = 0;
            for (int s = 0; s < func->body_count; s++) {
                VarType t = scan_stmt_return_type(func->body[s], locals, &local_count);
                if (t != TYPE_INT) {
                    ret_type = t;
                    break;
                }
            }
            register_function_meta(func->name, ret_type);
        }
    }

    fprintf(out, ".intel_syntax noprefix\n");
    fprintf(out, ".text\n");

    // Dastur kirish nuqtasi (_start)
    fprintf(out, ".global _start\n");
    fprintf(out, "_start:\n");
    fprintf(out, "    mov rax, [rsp]\n");
    fprintf(out, "    mov [rip + algo_argc], rax\n");
    fprintf(out, "    lea rax, [rsp + 8]\n");
    fprintf(out, "    mov [rip + algo_argv], rax\n");
    fprintf(out, "    mov rcx, [rsp]\n");
    fprintf(out, "    lea rax, [rsp + 16 + rcx * 8]\n");
    fprintf(out, "    mov [rip + algo_envp], rax\n");
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
    fprintf(out, "    mov rdx, [rip + algo_heap_ptr]\n");
    fprintf(out, "    test rdx, rdx\n");
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
    fprintf(out, "    mov rdx, [rip + algo_heap_ptr]\n");
    fprintf(out, ".L_alloc_have_heap:\n");
    fprintf(out, "    mov rcx, rdx\n");
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
    fprintf(out, "    mov rdx, [rip + algo_heap_ptr]\n");
    fprintf(out, ".L_alloc_ok:\n");
    fprintf(out, "    mov rax, rdx\n");
    fprintf(out, "    add rdx, r8\n");
    fprintf(out, "    mov [rip + algo_heap_ptr], rdx\n");
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

    // 5. Son chiqarish (yoki avtomatik matn aniqlash)
    fprintf(out, "algo_chiqar_son_oddiy:\n");
    fprintf(out, "    # Avval qiymat rodata yoki heap manzilida ekanligini tekshiramiz\n");
    fprintf(out, "    lea rbx, [rip + algo_rodata_start]\n");
    fprintf(out, "    lea rcx, [rip + algo_rodata_end]\n");
    fprintf(out, "    cmp rax, rbx\n");
    fprintf(out, "    jb .L_chk_heap\n");
    fprintf(out, "    cmp rax, rcx\n");
    fprintf(out, "    jb .L_chk_str_valid\n");
    fprintf(out, ".L_chk_heap:\n");
    fprintf(out, "    mov rbx, [rip + algo_heap_start]\n");
    fprintf(out, "    test rbx, rbx\n");
    fprintf(out, "    jz .L_not_str_obj\n");
    fprintf(out, "    cmp rax, rbx\n");
    fprintf(out, "    jb .L_not_str_obj\n");
    fprintf(out, "    cmp rax, [rip + algo_heap_ptr]\n");
    fprintf(out, "    jae .L_not_str_obj\n");
    fprintf(out, ".L_chk_str_valid:\n");
    fprintf(out, "    mov rdx, [rax]\n");
    fprintf(out, "    cmp rdx, 0\n");
    fprintf(out, "    jl .L_not_str_obj\n");
    fprintf(out, "    cmp rdx, 1000000\n");
    fprintf(out, "    jg .L_not_str_obj\n");
    fprintf(out, "    cmp byte ptr [rax + 8 + rdx], 0\n");
    fprintf(out, "    jne .L_not_str_obj\n");
    fprintf(out, "    # Matn aniqlandi! Uni chiqaramiz\n");
    fprintf(out, "    lea rsi, [rax + 8]\n");
    fprintf(out, "    mov rdi, 1\n");
    fprintf(out, "    mov rax, 1\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    ret\n");
    fprintf(out, ".L_not_str_obj:\n");
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
    fprintf(out, "    lea rdi, [rdi + 8]\n");
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

    // 8. Vaqt va Kutish (Time & Sleep)
    fprintf(out, "algo_vaqt:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    sub rsp, 16\n");
    fprintf(out, "    mov rax, 228\n"); // sys_clock_gettime
    fprintf(out, "    xor rdi, rdi\n");
    fprintf(out, "    mov rsi, rsp\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    mov rax, [rsp]\n");
    fprintf(out, "    imul rax, 1000\n");
    fprintf(out, "    mov rdx, [rsp + 8]\n");
    fprintf(out, "    push rax\n");
    fprintf(out, "    mov rax, rdx\n");
    fprintf(out, "    xor rdx, rdx\n");
    fprintf(out, "    mov rbx, 1000000\n");
    fprintf(out, "    div rbx\n");
    fprintf(out, "    mov rdx, rax\n");
    fprintf(out, "    pop rax\n");
    fprintf(out, "    add rax, rdx\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    fprintf(out, "algo_kutish:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    sub rsp, 16\n");
    fprintf(out, "    mov rax, rdi\n");
    fprintf(out, "    xor rdx, rdx\n");
    fprintf(out, "    mov rbx, 1000\n");
    fprintf(out, "    div rbx\n");
    fprintf(out, "    mov [rsp], rax\n");
    fprintf(out, "    imul rdx, 1000000\n");
    fprintf(out, "    mov [rsp + 8], rdx\n");
    fprintf(out, "    mov rax, 35\n"); // sys_nanosleep
    fprintf(out, "    mov rdi, rsp\n");
    fprintf(out, "    xor rsi, rsi\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 9. Matn amallari (String functions)
    fprintf(out, "algo_matn_teng:\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    push r13\n");
    fprintf(out, "    cmp rdi, rsi\n");
    fprintf(out, "    je .L_streq_yes\n");
    fprintf(out, "    test rdi, rdi\n");
    fprintf(out, "    jz .L_streq_no\n");
    fprintf(out, "    test rsi, rsi\n");
    fprintf(out, "    jz .L_streq_no\n");
    fprintf(out, "    mov rcx, [rdi]\n");
    fprintf(out, "    cmp rcx, [rsi]\n");
    fprintf(out, "    jne .L_streq_no\n");
    fprintf(out, "    lea r12, [rdi + 8]\n");
    fprintf(out, "    lea r13, [rsi + 8]\n");
    fprintf(out, ".L_streq_loop:\n");
    fprintf(out, "    test rcx, rcx\n");
    fprintf(out, "    jz .L_streq_yes\n");
    fprintf(out, "    mov al, [r12]\n");
    fprintf(out, "    cmp al, [r13]\n");
    fprintf(out, "    jne .L_streq_no\n");
    fprintf(out, "    inc r12\n");
    fprintf(out, "    inc r13\n");
    fprintf(out, "    dec rcx\n");
    fprintf(out, "    jmp .L_streq_loop\n");
    fprintf(out, ".L_streq_yes:\n");
    fprintf(out, "    mov rax, 1\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    ret\n");
    fprintf(out, ".L_streq_no:\n");
    fprintf(out, "    xor rax, rax\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    ret\n\n");

    fprintf(out, "algo_matn_kesish:\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    push r13\n");
    fprintf(out, "    push r14\n");
    fprintf(out, "    push r15\n");
    fprintf(out, "    mov r12, rdi\n"); // str
    fprintf(out, "    mov r13, rsi\n"); // bosh
    fprintf(out, "    mov r14, rdx\n"); // uzunlik
    fprintf(out, "    mov r15, [r12]\n");
    fprintf(out, "    cmp r13, r15\n");
    fprintf(out, "    jge .L_substr_empty\n");
    fprintf(out, "    mov rax, r13\n");
    fprintf(out, "    add rax, r14\n");
    fprintf(out, "    cmp rax, r15\n");
    fprintf(out, "    jle .L_substr_alloc\n");
    fprintf(out, "    mov r14, r15\n");
    fprintf(out, "    sub r14, r13\n");
    fprintf(out, ".L_substr_alloc:\n");
    fprintf(out, "    mov rax, r14\n");
    fprintf(out, "    add rax, 16\n");
    fprintf(out, "    call algo_ajrat\n");
    fprintf(out, "    mov [rax], r14\n");
    fprintf(out, "    mov rcx, r14\n");
    fprintf(out, "    lea rsi, [r12 + 8 + r13]\n");
    fprintf(out, "    lea rdi, [rax + 8]\n");
    fprintf(out, "    push rax\n");
    fprintf(out, "    rep movsb\n");
    fprintf(out, "    mov byte ptr [rdi], 0\n");
    fprintf(out, "    pop rax\n");
    fprintf(out, "    pop r15\n");
    fprintf(out, "    pop r14\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    ret\n");
    fprintf(out, ".L_substr_empty:\n");
    fprintf(out, "    mov rax, 16\n");
    fprintf(out, "    call algo_ajrat\n");
    fprintf(out, "    mov qword ptr [rax], 0\n");
    fprintf(out, "    mov byte ptr [rax + 8], 0\n");
    fprintf(out, "    pop r15\n");
    fprintf(out, "    pop r14\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    ret\n\n");

    fprintf(out, "algo_matn_ichida:\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    push r13\n");
    fprintf(out, "    push r14\n");
    fprintf(out, "    push r15\n");
    fprintf(out, "    mov r12, rdi\n");
    fprintf(out, "    mov r13, rsi\n");
    fprintf(out, "    mov r14, [r12]\n");
    fprintf(out, "    mov r15, [r13]\n");
    fprintf(out, "    cmp r15, r14\n");
    fprintf(out, "    jg .L_str_not_found\n");
    fprintf(out, "    test r15, r15\n");
    fprintf(out, "    jz .L_str_found\n");
    fprintf(out, "    mov r8, r14\n");
    fprintf(out, "    sub r8, r15\n");
    fprintf(out, "    xor r9, r9\n");
    fprintf(out, ".L_needle_outer:\n");
    fprintf(out, "    cmp r9, r8\n");
    fprintf(out, "    jg .L_str_not_found\n");
    fprintf(out, "    xor r10, r10\n");
    fprintf(out, ".L_needle_inner:\n");
    fprintf(out, "    cmp r10, r15\n");
    fprintf(out, "    jge .L_str_found\n");
    fprintf(out, "    mov r11, r9\n");
    fprintf(out, "    add r11, r10\n");
    fprintf(out, "    mov al, [r12 + 8 + r11]\n");
    fprintf(out, "    mov bl, [r13 + 8 + r10]\n");
    fprintf(out, "    cmp al, bl\n");
    fprintf(out, "    jne .L_needle_next\n");
    fprintf(out, "    inc r10\n");
    fprintf(out, "    jmp .L_needle_inner\n");
    fprintf(out, ".L_needle_next:\n");
    fprintf(out, "    inc r9\n");
    fprintf(out, "    jmp .L_needle_outer\n");
    fprintf(out, ".L_str_found:\n");
    fprintf(out, "    mov rax, 1\n");
    fprintf(out, "    pop r15\n");
    fprintf(out, "    pop r14\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    ret\n");
    fprintf(out, ".L_str_not_found:\n");
    fprintf(out, "    xor rax, rax\n");
    fprintf(out, "    pop r15\n");
    fprintf(out, "    pop r14\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    ret\n\n");

    fprintf(out, "algo_matn_katta:\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    push r13\n");
    fprintf(out, "    mov r12, rdi\n");
    fprintf(out, "    mov r13, [r12]\n");
    fprintf(out, "    mov rax, r13\n");
    fprintf(out, "    add rax, 16\n");
    fprintf(out, "    call algo_ajrat\n");
    fprintf(out, "    mov [rax], r13\n");
    fprintf(out, "    xor rcx, rcx\n");
    fprintf(out, ".L_upper_loop:\n");
    fprintf(out, "    cmp rcx, r13\n");
    fprintf(out, "    jge .L_upper_done\n");
    fprintf(out, "    mov dl, [r12 + 8 + rcx]\n");
    fprintf(out, "    cmp dl, 97\n"); // 'a'
    fprintf(out, "    jb .L_upper_store\n");
    fprintf(out, "    cmp dl, 122\n"); // 'z'
    fprintf(out, "    ja .L_upper_store\n");
    fprintf(out, "    sub dl, 32\n");
    fprintf(out, ".L_upper_store:\n");
    fprintf(out, "    mov [rax + 8 + rcx], dl\n");
    fprintf(out, "    inc rcx\n");
    fprintf(out, "    jmp .L_upper_loop\n");
    fprintf(out, ".L_upper_done:\n");
    fprintf(out, "    mov byte ptr [rax + 8 + r13], 0\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    ret\n\n");

    fprintf(out, "algo_matn_kichik:\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    push r13\n");
    fprintf(out, "    mov r12, rdi\n");
    fprintf(out, "    mov r13, [r12]\n");
    fprintf(out, "    mov rax, r13\n");
    fprintf(out, "    add rax, 16\n");
    fprintf(out, "    call algo_ajrat\n");
    fprintf(out, "    mov [rax], r13\n");
    fprintf(out, "    xor rcx, rcx\n");
    fprintf(out, ".L_lower_loop:\n");
    fprintf(out, "    cmp rcx, r13\n");
    fprintf(out, "    jge .L_lower_done\n");
    fprintf(out, "    mov dl, [r12 + 8 + rcx]\n");
    fprintf(out, "    cmp dl, 65\n"); // 'A'
    fprintf(out, "    jb .L_lower_store\n");
    fprintf(out, "    cmp dl, 90\n"); // 'Z'
    fprintf(out, "    ja .L_lower_store\n");
    fprintf(out, "    add dl, 32\n");
    fprintf(out, ".L_lower_store:\n");
    fprintf(out, "    mov [rax + 8 + rcx], dl\n");
    fprintf(out, "    inc rcx\n");
    fprintf(out, "    jmp .L_lower_loop\n");
    fprintf(out, ".L_lower_done:\n");
    fprintf(out, "    mov byte ptr [rax + 8 + r13], 0\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    ret\n\n");

    fprintf(out, "algo_son_matnga:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    sub rsp, 48\n");
    fprintf(out, "    mov rax, rdi\n");
    fprintf(out, "    lea rcx, [rbp - 1]\n");
    fprintf(out, "    mov byte ptr [rcx], 0\n");
    fprintf(out, "    mov r8, 0\n");
    fprintf(out, "    test rax, rax\n");
    fprintf(out, "    jnz .L_s2m_nonzero\n");
    fprintf(out, "    dec rcx\n");
    fprintf(out, "    mov byte ptr [rcx], 48\n");
    fprintf(out, "    mov r8, 1\n");
    fprintf(out, "    jmp .L_s2m_alloc\n");
    fprintf(out, ".L_s2m_nonzero:\n");
    fprintf(out, "    mov r9, 0\n");
    fprintf(out, "    cmp rax, 0\n");
    fprintf(out, "    jge .L_s2m_loop\n");
    fprintf(out, "    mov r9, 1\n");
    fprintf(out, "    neg rax\n");
    fprintf(out, ".L_s2m_loop:\n");
    fprintf(out, "    test rax, rax\n");
    fprintf(out, "    jz .L_s2m_check_neg\n");
    fprintf(out, "    mov rdx, 0\n");
    fprintf(out, "    mov rbx, 10\n");
    fprintf(out, "    div rbx\n");
    fprintf(out, "    add dl, 48\n");
    fprintf(out, "    dec rcx\n");
    fprintf(out, "    mov byte ptr [rcx], dl\n");
    fprintf(out, "    inc r8\n");
    fprintf(out, "    jmp .L_s2m_loop\n");
    fprintf(out, ".L_s2m_check_neg:\n");
    fprintf(out, "    test r9, r9\n");
    fprintf(out, "    jz .L_s2m_alloc\n");
    fprintf(out, "    dec rcx\n");
    fprintf(out, "    mov byte ptr [rcx], 45\n");
    fprintf(out, "    inc r8\n");
    fprintf(out, ".L_s2m_alloc:\n");
    fprintf(out, "    push rcx\n");
    fprintf(out, "    push r8\n");
    fprintf(out, "    mov rax, r8\n");
    fprintf(out, "    add rax, 16\n");
    fprintf(out, "    call algo_ajrat\n");
    fprintf(out, "    pop r8\n");
    fprintf(out, "    pop rcx\n");
    fprintf(out, "    mov [rax], r8\n");
    fprintf(out, "    lea rdi, [rax + 8]\n");
    fprintf(out, "    mov rsi, rcx\n");
    fprintf(out, "    mov rcx, r8\n");
    fprintf(out, "    push rax\n");
    fprintf(out, "    rep movsb\n");
    fprintf(out, "    mov byte ptr [rdi], 0\n");
    fprintf(out, "    pop rax\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    fprintf(out, "algo_matn_songa:\n");
    fprintf(out, "    mov rcx, [rdi]\n");
    fprintf(out, "    test rcx, rcx\n");
    fprintf(out, "    jz .L_m2s_zero\n");
    fprintf(out, "    lea rsi, [rdi + 8]\n");
    fprintf(out, "    xor rax, rax\n");
    fprintf(out, "    mov r8, 0\n");
    fprintf(out, "    mov dl, [rsi]\n");
    fprintf(out, "    cmp dl, 45\n");
    fprintf(out, "    jne .L_m2s_loop\n");
    fprintf(out, "    mov r8, 1\n");
    fprintf(out, "    inc rsi\n");
    fprintf(out, "    dec rcx\n");
    fprintf(out, ".L_m2s_loop:\n");
    fprintf(out, "    test rcx, rcx\n");
    fprintf(out, "    jz .L_m2s_done\n");
    fprintf(out, "    movzx rdx, byte ptr [rsi]\n");
    fprintf(out, "    cmp dl, 48\n");
    fprintf(out, "    jb .L_m2s_done\n");
    fprintf(out, "    cmp dl, 57\n");
    fprintf(out, "    ja .L_m2s_done\n");
    fprintf(out, "    sub dl, 48\n");
    fprintf(out, "    imul rax, 10\n");
    fprintf(out, "    add rax, rdx\n");
    fprintf(out, "    inc rsi\n");
    fprintf(out, "    dec rcx\n");
    fprintf(out, "    jmp .L_m2s_loop\n");
    fprintf(out, ".L_m2s_done:\n");
    fprintf(out, "    test r8, r8\n");
    fprintf(out, "    jz .L_m2s_ret\n");
    fprintf(out, "    neg rax\n");
    fprintf(out, ".L_m2s_ret:\n");
    fprintf(out, "    ret\n");
    fprintf(out, ".L_m2s_zero:\n");
    fprintf(out, "    xor rax, rax\n");
    fprintf(out, "    ret\n\n");

    // 10. Lug'at (Hashmap / Dictionary)
    fprintf(out, "algo_hash_fnv1a:\n");
    fprintf(out, "    mov rcx, [rdi]\n");
    fprintf(out, "    lea rsi, [rdi + 8]\n");
    fprintf(out, "    mov rax, 0xcbf29ce484222325\n");
    fprintf(out, "    mov r8, 0x100000001b3\n");
    fprintf(out, ".L_fnv_loop:\n");
    fprintf(out, "    test rcx, rcx\n");
    fprintf(out, "    jz .L_fnv_done\n");
    fprintf(out, "    movzx rdx, byte ptr [rsi]\n");
    fprintf(out, "    xor rax, rdx\n");
    fprintf(out, "    imul rax, r8\n");
    fprintf(out, "    inc rsi\n");
    fprintf(out, "    dec rcx\n");
    fprintf(out, "    jmp .L_fnv_loop\n");
    fprintf(out, ".L_fnv_done:\n");
    fprintf(out, "    ret\n\n");

    fprintf(out, "algo_lugat_yarat:\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    mov rax, 6168\n");
    fprintf(out, "    call algo_ajrat\n");
    fprintf(out, "    mov qword ptr [rax], 0\n");
    fprintf(out, "    mov qword ptr [rax + 8], 256\n");
    fprintf(out, "    lea rbx, [rax + 24]\n");
    fprintf(out, "    mov [rax + 16], rbx\n");
    fprintf(out, "    mov rcx, 768\n");
    fprintf(out, "    lea rdi, [rax + 24]\n");
    fprintf(out, "    push rax\n");
    fprintf(out, "    xor rax, rax\n");
    fprintf(out, "    rep stosq\n");
    fprintf(out, "    pop rax\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    ret\n\n");

    fprintf(out, "algo_lugat_qoy:\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    push r13\n");
    fprintf(out, "    push r14\n");
    fprintf(out, "    push r15\n");
    fprintf(out, "    push rbx\n");
    fprintf(out, "    mov r12, rdi\n");
    fprintf(out, "    mov r13, rsi\n");
    fprintf(out, "    mov r14, rdx\n");
    fprintf(out, "    mov rdi, r13\n");
    fprintf(out, "    call algo_hash_fnv1a\n");
    fprintf(out, "    mov r15, rax\n");
    fprintf(out, "    and rax, 255\n");
    fprintf(out, "    mov rbx, [r12 + 16]\n");
    fprintf(out, ".L_hset_probe:\n");
    fprintf(out, "    imul r10, rax, 24\n");
    fprintf(out, "    lea rcx, [rbx + r10]\n");
    fprintf(out, "    cmp qword ptr [rcx], 0\n");
    fprintf(out, "    je .L_hset_empty\n");
    fprintf(out, "    cmp [rcx + 8], r15\n");
    fprintf(out, "    jne .L_hset_next\n");
    fprintf(out, "    push rax\n");
    fprintf(out, "    push rcx\n");
    fprintf(out, "    mov rdi, [rcx]\n");
    fprintf(out, "    mov rsi, r13\n");
    fprintf(out, "    call algo_matn_teng\n");
    fprintf(out, "    pop rcx\n");
    fprintf(out, "    pop rax\n");
    fprintf(out, "    test rax, rax\n");
    fprintf(out, "    jnz .L_hset_update\n");
    fprintf(out, ".L_hset_next:\n");
    fprintf(out, "    inc rax\n");
    fprintf(out, "    and rax, 255\n");
    fprintf(out, "    jmp .L_hset_probe\n");
    fprintf(out, ".L_hset_empty:\n");
    fprintf(out, "    inc qword ptr [r12]\n");
    fprintf(out, ".L_hset_update:\n");
    fprintf(out, "    mov [rcx], r13\n");
    fprintf(out, "    mov [rcx + 8], r15\n");
    fprintf(out, "    mov [rcx + 16], r14\n");
    fprintf(out, "    pop rbx\n");
    fprintf(out, "    pop r15\n");
    fprintf(out, "    pop r14\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    ret\n\n");

    fprintf(out, "algo_lugat_ol:\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    push r13\n");
    fprintf(out, "    push r14\n");
    fprintf(out, "    push rbx\n");
    fprintf(out, "    mov r12, rdi\n");
    fprintf(out, "    mov r13, rsi\n");
    fprintf(out, "    mov rdi, r13\n");
    fprintf(out, "    call algo_hash_fnv1a\n");
    fprintf(out, "    mov r14, rax\n");
    fprintf(out, "    and rax, 255\n");
    fprintf(out, "    mov rbx, [r12 + 16]\n");
    fprintf(out, ".L_hget_probe:\n");
    fprintf(out, "    imul r10, rax, 24\n");
    fprintf(out, "    lea rcx, [rbx + r10]\n");
    fprintf(out, "    cmp qword ptr [rcx], 0\n");
    fprintf(out, "    je .L_hget_notfound\n");
    fprintf(out, "    cmp [rcx + 8], r14\n");
    fprintf(out, "    jne .L_hget_next\n");
    fprintf(out, "    push rax\n");
    fprintf(out, "    push rcx\n");
    fprintf(out, "    mov rdi, [rcx]\n");
    fprintf(out, "    mov rsi, r13\n");
    fprintf(out, "    call algo_matn_teng\n");
    fprintf(out, "    pop rcx\n");
    fprintf(out, "    pop rax\n");
    fprintf(out, "    test rax, rax\n");
    fprintf(out, "    jnz .L_hget_found\n");
    fprintf(out, ".L_hget_next:\n");
    fprintf(out, "    inc rax\n");
    fprintf(out, "    and rax, 255\n");
    fprintf(out, "    jmp .L_hget_probe\n");
    fprintf(out, ".L_hget_found:\n");
    fprintf(out, "    mov rax, [rcx + 16]\n");
    fprintf(out, "    pop rbx\n");
    fprintf(out, "    pop r14\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    ret\n");
    fprintf(out, ".L_hget_notfound:\n");
    fprintf(out, "    xor rax, rax\n");
    fprintf(out, "    pop rbx\n");
    fprintf(out, "    pop r14\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    ret\n\n");

    fprintf(out, "algo_lugat_bormi:\n");
    fprintf(out, "    call algo_lugat_ol\n");
    fprintf(out, "    test rax, rax\n");
    fprintf(out, "    setnz al\n");
    fprintf(out, "    movzx rax, al\n");
    fprintf(out, "    ret\n\n");

    // 11. Tarmoq va Soketlar (TCP Networking)
    fprintf(out, "algo_tarmoq_server_och:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    push r13\n");
    fprintf(out, "    mov r12, rdi\n");
    fprintf(out, "    mov rax, 41\n"); // sys_socket (AF_INET=2, SOCK_STREAM=1, 0)
    fprintf(out, "    mov rdi, 2\n");
    fprintf(out, "    mov rsi, 1\n");
    fprintf(out, "    xor rdx, rdx\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    test rax, rax\n");
    fprintf(out, "    js .L_sock_err\n");
    fprintf(out, "    mov r13, rax\n");
    fprintf(out, "    sub rsp, 16\n");
    fprintf(out, "    mov dword ptr [rsp], 1\n");
    fprintf(out, "    mov rax, 54\n"); // setsockopt SO_REUSEADDR
    fprintf(out, "    mov rdi, r13\n");
    fprintf(out, "    mov rsi, 1\n");
    fprintf(out, "    mov rdx, 2\n");
    fprintf(out, "    mov r10, rsp\n");
    fprintf(out, "    mov r8, 4\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    add rsp, 16\n");
    fprintf(out, "    sub rsp, 16\n");
    fprintf(out, "    mov word ptr [rsp], 2\n");
    fprintf(out, "    mov rax, r12\n");
    fprintf(out, "    xchg al, ah\n");
    fprintf(out, "    mov word ptr [rsp + 2], ax\n");
    fprintf(out, "    mov dword ptr [rsp + 4], 0\n");
    fprintf(out, "    mov qword ptr [rsp + 8], 0\n");
    fprintf(out, "    mov rax, 49\n"); // sys_bind
    fprintf(out, "    mov rdi, r13\n");
    fprintf(out, "    mov rsi, rsp\n");
    fprintf(out, "    mov rdx, 16\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    add rsp, 16\n");
    fprintf(out, "    test rax, rax\n");
    fprintf(out, "    js .L_sock_err\n");
    fprintf(out, "    mov rax, 50\n"); // sys_listen
    fprintf(out, "    mov rdi, r13\n");
    fprintf(out, "    mov rsi, 10\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    mov rax, r13\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n");
    fprintf(out, ".L_sock_err:\n");
    fprintf(out, "    mov rax, -1\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    fprintf(out, "algo_tarmoq_mijoz_qabul:\n");
    fprintf(out, "    mov rax, 43\n"); // sys_accept
    fprintf(out, "    xor rsi, rsi\n");
    fprintf(out, "    xor rdx, rdx\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    ret\n\n");

    fprintf(out, "algo_tarmoq_yubor:\n");
    fprintf(out, "    mov rdx, [rsi]\n");
    fprintf(out, "    lea rsi, [rsi + 8]\n");
    fprintf(out, "    mov rax, 1\n"); // sys_write
    fprintf(out, "    syscall\n");
    fprintf(out, "    ret\n\n");

    fprintf(out, "algo_tarmoq_qabul:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    push r13\n");
    fprintf(out, "    mov r12, rdi\n");
    fprintf(out, "    mov r13, rsi\n");
    fprintf(out, "    mov rax, r13\n");
    fprintf(out, "    add rax, 16\n");
    fprintf(out, "    call algo_ajrat\n");
    fprintf(out, "    push rax\n");
    fprintf(out, "    mov rdi, r12\n");
    fprintf(out, "    lea rsi, [rax + 8]\n");
    fprintf(out, "    mov rdx, r13\n");
    fprintf(out, "    mov rax, 0\n"); // sys_read
    fprintf(out, "    syscall\n");
    fprintf(out, "    pop rbx\n");
    fprintf(out, "    test rax, rax\n");
    fprintf(out, "    jle .L_recv_empty\n");
    fprintf(out, "    mov [rbx], rax\n");
    fprintf(out, "    mov byte ptr [rbx + 8 + rax], 0\n");
    fprintf(out, "    mov rax, rbx\n");
    fprintf(out, "    jmp .L_recv_ret\n");
    fprintf(out, ".L_recv_empty:\n");
    fprintf(out, "    mov qword ptr [rbx], 0\n");
    fprintf(out, "    mov byte ptr [rbx + 8], 0\n");
    fprintf(out, "    mov rax, rbx\n");
    fprintf(out, ".L_recv_ret:\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    fprintf(out, "algo_tarmoq_yop:\n");
    fprintf(out, "    mov rax, 3\n"); // sys_close
    fprintf(out, "    syscall\n");
    fprintf(out, "    ret\n\n");

    // 12. Buyruqlar qatori argumentlari (CLI arguments)
    fprintf(out, "algo_argumentlar:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    push r13\n");
    fprintf(out, "    push r14\n");
    fprintf(out, "    push rbx\n");
    fprintf(out, "    mov r12, [rip + algo_argc]\n");
    fprintf(out, "    mov rax, r12\n");
    fprintf(out, "    imul rax, 8\n");
    fprintf(out, "    add rax, 16\n");
    fprintf(out, "    call algo_ajrat\n");
    fprintf(out, "    mov [rax], r12\n");
    fprintf(out, "    mov [rax + 8], r12\n");
    fprintf(out, "    mov r13, rax\n");
    fprintf(out, "    xor r14, r14\n");
    fprintf(out, "    mov rbx, [rip + algo_argv]\n");
    fprintf(out, ".L_args_loop:\n");
    fprintf(out, "    cmp r14, r12\n");
    fprintf(out, "    jge .L_args_done\n");
    fprintf(out, "    mov rdi, [rbx + r14 * 8]\n");
    fprintf(out, "    push rbx\n");
    fprintf(out, "    push rdi\n");
    fprintf(out, "    xor rcx, rcx\n");
    fprintf(out, ".L_arg_slen:\n");
    fprintf(out, "    cmp byte ptr [rdi + rcx], 0\n");
    fprintf(out, "    je .L_arg_slen_done\n");
    fprintf(out, "    inc rcx\n");
    fprintf(out, "    jmp .L_arg_slen\n");
    fprintf(out, ".L_arg_slen_done:\n");
    fprintf(out, "    pop rsi\n");
    fprintf(out, "    push rcx\n");
    fprintf(out, "    push rsi\n");
    fprintf(out, "    mov rax, rcx\n");
    fprintf(out, "    add rax, 16\n");
    fprintf(out, "    call algo_ajrat\n");
    fprintf(out, "    pop rsi\n");
    fprintf(out, "    pop rcx\n");
    fprintf(out, "    pop rbx\n");
    fprintf(out, "    mov [rax], rcx\n");
    fprintf(out, "    push rax\n");
    fprintf(out, "    lea rdi, [rax + 8]\n");
    fprintf(out, "    rep movsb\n");
    fprintf(out, "    mov byte ptr [rdi], 0\n");
    fprintf(out, "    pop rax\n");
    fprintf(out, "    mov [r13 + 16 + r14 * 8], rax\n");
    fprintf(out, "    inc r14\n");
    fprintf(out, "    jmp .L_args_loop\n");
    fprintf(out, ".L_args_done:\n");
    fprintf(out, "    mov rax, r13\n");
    fprintf(out, "    pop rbx\n");
    fprintf(out, "    pop r14\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 13. Tasdiqlash / Xatolik (Assertions / Panic)
    fprintf(out, "algo_tasdiqla:\n");
    fprintf(out, "    test rdi, rdi\n");
    fprintf(out, "    jnz .L_assert_ok\n");
    fprintf(out, "    push rsi\n");
    fprintf(out, "    lea rsi, [rip + algo_assert_msg]\n");
    fprintf(out, "    mov rdx, 17\n");
    fprintf(out, "    mov rax, 1\n");
    fprintf(out, "    mov rdi, 1\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    pop rsi\n");
    fprintf(out, "    test rsi, rsi\n");
    fprintf(out, "    jz .L_assert_exit\n");
    fprintf(out, "    mov rdx, [rsi]\n");
    fprintf(out, "    lea rsi, [rsi + 8]\n");
    fprintf(out, "    mov rax, 1\n");
    fprintf(out, "    mov rdi, 1\n");
    fprintf(out, "    syscall\n");
    fprintf(out, ".L_assert_exit:\n");
    fprintf(out, "    lea rsi, [rip + algo_newline]\n");
    fprintf(out, "    mov rdx, 1\n");
    fprintf(out, "    mov rax, 1\n");
    fprintf(out, "    mov rdi, 1\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    mov rax, 60\n");
    fprintf(out, "    mov rdi, 1\n");
    fprintf(out, "    syscall\n");
    fprintf(out, ".L_assert_ok:\n");
    fprintf(out, "    ret\n\n");

    // 14. Haqiqiy son chiqarish (Float / Double printing)
    fprintf(out, "algo_chiqar_haqiqiy:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    sub rsp, 48\n");
    fprintf(out, "    movq xmm0, rax\n");
    fprintf(out, "    movsd [rbp - 8], xmm0\n");
    fprintf(out, "    xorps xmm2, xmm2\n");
    fprintf(out, "    ucomisd xmm0, xmm2\n");
    fprintf(out, "    jae .L_fp_pos\n");
    fprintf(out, "    lea rsi, [rip + algo_fp_minus]\n");
    fprintf(out, "    mov rdx, 1\n");
    fprintf(out, "    mov rax, 1\n");
    fprintf(out, "    mov rdi, 1\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    movsd xmm0, [rbp - 8]\n");
    fprintf(out, "    mulsd xmm0, [rip + algo_fp_neg_one]\n");
    fprintf(out, "    movsd [rbp - 8], xmm0\n");
    fprintf(out, ".L_fp_pos:\n");
    fprintf(out, "    movsd xmm0, [rbp - 8]\n");
    fprintf(out, "    cvttsd2si r12, xmm0\n");
    fprintf(out, "    mov rax, r12\n");
    fprintf(out, "    call algo_chiqar_son_oddiy\n");
    fprintf(out, "    lea rsi, [rip + algo_fp_dot]\n");
    fprintf(out, "    mov rdx, 1\n");
    fprintf(out, "    mov rax, 1\n");
    fprintf(out, "    mov rdi, 1\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    cvtsi2sd xmm1, r12\n");
    fprintf(out, "    movsd xmm0, [rbp - 8]\n");
    fprintf(out, "    subsd xmm0, xmm1\n");
    fprintf(out, "    mulsd xmm0, [rip + algo_fp_scale]\n");
    fprintf(out, "    cvttsd2si rax, xmm0\n");
    fprintf(out, "    call algo_chiqar_son_oddiy\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 15. Dasturdan chiqish (Exit)
    fprintf(out, "algo_chiqish:\n");
    fprintf(out, "    mov rax, 60\n"); // sys_exit
    fprintf(out, "    syscall\n\n");

    // 16. Muhit o'zgaruvchilari (Environment variables)
    fprintf(out, "algo_muhit:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    push r13\n");
    fprintf(out, "    push r14\n");
    fprintf(out, "    push r15\n");
    fprintf(out, "    push rbx\n");
    fprintf(out, "    mov r12, rdi\n");
    fprintf(out, "    test r12, r12\n");
    fprintf(out, "    jz .L_muhit_empty\n");
    fprintf(out, "    mov r13, [r12]\n");
    fprintf(out, "    lea r12, [r12 + 8]\n");
    fprintf(out, "    mov rbx, [rip + algo_envp]\n");
    fprintf(out, "    test rbx, rbx\n");
    fprintf(out, "    jz .L_muhit_empty\n");
    fprintf(out, ".L_muhit_env_loop:\n");
    fprintf(out, "    mov r14, [rbx]\n");
    fprintf(out, "    test r14, r14\n");
    fprintf(out, "    jz .L_muhit_empty\n");
    fprintf(out, "    xor rcx, rcx\n");
    fprintf(out, ".L_muhit_cmp_loop:\n");
    fprintf(out, "    cmp rcx, r13\n");
    fprintf(out, "    je .L_muhit_cmp_check_eq\n");
    fprintf(out, "    mov al, [r12 + rcx]\n");
    fprintf(out, "    cmp al, [r14 + rcx]\n");
    fprintf(out, "    jne .L_muhit_next_env\n");
    fprintf(out, "    inc rcx\n");
    fprintf(out, "    jmp .L_muhit_cmp_loop\n");
    fprintf(out, ".L_muhit_cmp_check_eq:\n");
    fprintf(out, "    cmp byte ptr [r14 + r13], 61\n"); // '='
    fprintf(out, "    jne .L_muhit_next_env\n");
    fprintf(out, "    lea r15, [r14 + r13 + 1]\n");
    fprintf(out, "    xor rcx, rcx\n");
    fprintf(out, ".L_muhit_val_len:\n");
    fprintf(out, "    cmp byte ptr [r15 + rcx], 0\n");
    fprintf(out, "    je .L_muhit_val_len_done\n");
    fprintf(out, "    inc rcx\n");
    fprintf(out, "    jmp .L_muhit_val_len\n");
    fprintf(out, ".L_muhit_val_len_done:\n");
    fprintf(out, "    push rcx\n");
    fprintf(out, "    push r15\n");
    fprintf(out, "    mov rax, rcx\n");
    fprintf(out, "    add rax, 16\n");
    fprintf(out, "    call algo_ajrat\n");
    fprintf(out, "    pop r15\n");
    fprintf(out, "    pop rcx\n");
    fprintf(out, "    mov [rax], rcx\n");
    fprintf(out, "    lea rdi, [rax + 8]\n");
    fprintf(out, "    mov rsi, r15\n");
    fprintf(out, "    push rax\n");
    fprintf(out, "    rep movsb\n");
    fprintf(out, "    mov byte ptr [rdi], 0\n");
    fprintf(out, "    pop rax\n");
    fprintf(out, "    jmp .L_muhit_done\n");
    fprintf(out, ".L_muhit_next_env:\n");
    fprintf(out, "    add rbx, 8\n");
    fprintf(out, "    jmp .L_muhit_env_loop\n");
    fprintf(out, ".L_muhit_empty:\n");
    fprintf(out, "    mov rax, 16\n");
    fprintf(out, "    call algo_ajrat\n");
    fprintf(out, "    mov qword ptr [rax], 0\n");
    fprintf(out, "    mov byte ptr [rax + 8], 0\n");
    fprintf(out, ".L_muhit_done:\n");
    fprintf(out, "    pop rbx\n");
    fprintf(out, "    pop r15\n");
    fprintf(out, "    pop r14\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 17. Butun faylni matn sifatida o'qish (Read entire file)
    fprintf(out, "algo_fayl_oqish:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    push r13\n");
    fprintf(out, "    push r14\n");
    fprintf(out, "    push rbx\n");
    fprintf(out, "    test rdi, rdi\n");
    fprintf(out, "    jz .L_file_read_err\n");
    fprintf(out, "    lea rdi, [rdi + 8]\n");
    fprintf(out, "    xor rsi, rsi\n");
    fprintf(out, "    xor rdx, rdx\n");
    fprintf(out, "    mov rax, 2\n"); // sys_open
    fprintf(out, "    syscall\n");
    fprintf(out, "    test rax, rax\n");
    fprintf(out, "    js .L_file_read_err\n");
    fprintf(out, "    mov r12, rax\n");
    fprintf(out, "    mov rax, 8\n"); // sys_lseek SEEK_END
    fprintf(out, "    mov rdi, r12\n");
    fprintf(out, "    xor rsi, rsi\n");
    fprintf(out, "    mov rdx, 2\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    test rax, rax\n");
    fprintf(out, "    js .L_file_read_close_err\n");
    fprintf(out, "    mov r13, rax\n"); // file size
    fprintf(out, "    mov rax, 8\n"); // sys_lseek SEEK_SET
    fprintf(out, "    mov rdi, r12\n");
    fprintf(out, "    xor rsi, rsi\n");
    fprintf(out, "    xor rdx, rdx\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    mov rax, r13\n");
    fprintf(out, "    add rax, 16\n");
    fprintf(out, "    call algo_ajrat\n");
    fprintf(out, "    mov r14, rax\n");
    fprintf(out, "    mov [r14], r13\n");
    fprintf(out, "    test r13, r13\n");
    fprintf(out, "    jz .L_file_read_finish\n");
    fprintf(out, "    mov rax, 0\n"); // sys_read
    fprintf(out, "    mov rdi, r12\n");
    fprintf(out, "    lea rsi, [r14 + 8]\n");
    fprintf(out, "    mov rdx, r13\n");
    fprintf(out, "    syscall\n");
    fprintf(out, ".L_file_read_finish:\n");
    fprintf(out, "    mov byte ptr [r14 + 8 + r13], 0\n");
    fprintf(out, "    mov rax, 3\n"); // sys_close
    fprintf(out, "    mov rdi, r12\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    mov rax, r14\n");
    fprintf(out, "    jmp .L_file_read_ret\n");
    fprintf(out, ".L_file_read_close_err:\n");
    fprintf(out, "    mov rax, 3\n");
    fprintf(out, "    mov rdi, r12\n");
    fprintf(out, "    syscall\n");
    fprintf(out, ".L_file_read_err:\n");
    fprintf(out, "    mov rax, 16\n");
    fprintf(out, "    call algo_ajrat\n");
    fprintf(out, "    mov qword ptr [rax], 0\n");
    fprintf(out, "    mov byte ptr [rax + 8], 0\n");
    fprintf(out, ".L_file_read_ret:\n");
    fprintf(out, "    pop rbx\n");
    fprintf(out, "    pop r14\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 18. Matnni faylga to'liq yozish (Write entire string to file)
    fprintf(out, "algo_fayl_yozish:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    push r13\n");
    fprintf(out, "    test rdi, rdi\n");
    fprintf(out, "    jz .L_file_write_err\n");
    fprintf(out, "    test rsi, rsi\n");
    fprintf(out, "    jz .L_file_write_err\n");
    fprintf(out, "    mov r13, rsi\n");
    fprintf(out, "    lea rdi, [rdi + 8]\n");
    fprintf(out, "    mov rsi, 577\n"); // O_WRONLY | O_CREAT | O_TRUNC
    fprintf(out, "    mov rdx, 420\n"); // 0644
    fprintf(out, "    mov rax, 2\n");   // sys_open
    fprintf(out, "    syscall\n");
    fprintf(out, "    test rax, rax\n");
    fprintf(out, "    js .L_file_write_err\n");
    fprintf(out, "    mov r12, rax\n");
    fprintf(out, "    mov rdx, [r13]\n");
    fprintf(out, "    lea rsi, [r13 + 8]\n");
    fprintf(out, "    mov rdi, r12\n");
    fprintf(out, "    mov rax, 1\n");   // sys_write
    fprintf(out, "    syscall\n");
    fprintf(out, "    push rax\n");
    fprintf(out, "    mov rax, 3\n");   // sys_close
    fprintf(out, "    mov rdi, r12\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    pop rax\n");
    fprintf(out, "    jmp .L_file_write_done\n");
    fprintf(out, ".L_file_write_err:\n");
    fprintf(out, "    mov rax, -1\n");
    fprintf(out, ".L_file_write_done:\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 19. Fayl mavjudligini tekshirish (Check if file exists)
    fprintf(out, "algo_fayl_mavjudmi:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    test rdi, rdi\n");
    fprintf(out, "    jz .L_fexist_no\n");
    fprintf(out, "    lea rdi, [rdi + 8]\n");
    fprintf(out, "    xor rsi, rsi\n");
    fprintf(out, "    mov rax, 21\n"); // sys_access
    fprintf(out, "    syscall\n");
    fprintf(out, "    test rax, rax\n");
    fprintf(out, "    jnz .L_fexist_no\n");
    fprintf(out, "    mov rax, 1\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n");
    fprintf(out, ".L_fexist_no:\n");
    fprintf(out, "    xor rax, rax\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 20. Faylni o'chirish (Delete file)
    fprintf(out, "algo_fayl_ochirish:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    test rdi, rdi\n");
    fprintf(out, "    jz .L_frm_err\n");
    fprintf(out, "    lea rdi, [rdi + 8]\n");
    fprintf(out, "    mov rax, 87\n"); // sys_unlink
    fprintf(out, "    syscall\n");
    fprintf(out, "    jmp .L_frm_done\n");
    fprintf(out, ".L_frm_err:\n");
    fprintf(out, "    mov rax, -1\n");
    fprintf(out, ".L_frm_done:\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 21. Papka yaratish (Make directory)
    fprintf(out, "algo_papka_yaratish:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    test rdi, rdi\n");
    fprintf(out, "    jz .L_fmkdir_err\n");
    fprintf(out, "    lea rdi, [rdi + 8]\n");
    fprintf(out, "    mov rsi, 493\n"); // 0755
    fprintf(out, "    mov rax, 83\n");  // sys_mkdir
    fprintf(out, "    syscall\n");
    fprintf(out, "    jmp .L_fmkdir_done\n");
    fprintf(out, ".L_fmkdir_err:\n");
    fprintf(out, "    mov rax, -1\n");
    fprintf(out, ".L_fmkdir_done:\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 22. Tasodifiy son hosil qilish (Random number via sys_getrandom)
    fprintf(out, "algo_tasodifiy:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    push r13\n");
    fprintf(out, "    push rbx\n");
    fprintf(out, "    mov r12, rdi\n");
    fprintf(out, "    mov r13, rsi\n");
    fprintf(out, "    cmp r13, r12\n");
    fprintf(out, "    jge .L_rnd_valid\n");
    fprintf(out, "    xchg r12, r13\n");
    fprintf(out, ".L_rnd_valid:\n");
    fprintf(out, "    mov rbx, r13\n");
    fprintf(out, "    sub rbx, r12\n");
    fprintf(out, "    inc rbx\n");
    fprintf(out, "    sub rsp, 16\n");
    fprintf(out, "    mov rax, 318\n"); // sys_getrandom
    fprintf(out, "    mov rdi, rsp\n");
    fprintf(out, "    mov rsi, 8\n");
    fprintf(out, "    xor rdx, rdx\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    mov rax, [rsp]\n");
    fprintf(out, "    add rsp, 16\n");
    fprintf(out, "    mov rdx, 0x7FFFFFFFFFFFFFFF\n");
    fprintf(out, "    and rax, rdx\n");
    fprintf(out, "    xor rdx, rdx\n");
    fprintf(out, "    div rbx\n");
    fprintf(out, "    mov rax, rdx\n");
    fprintf(out, "    add rax, r12\n");
    fprintf(out, "    pop rbx\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 23. Kvadrat ildiz (Hardware SSE square root)
    fprintf(out, "algo_ildiz:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    mov r8, rdi\n");
    fprintf(out, "    mov rax, 1000000000000000\n");
    fprintf(out, "    cmp r8, rax\n");
    fprintf(out, "    jge .L_ildiz_double\n");
    fprintf(out, "    mov rax, -1000000000000000\n");
    fprintf(out, "    cmp r8, rax\n");
    fprintf(out, "    jle .L_ildiz_double\n");
    fprintf(out, "    cvtsi2sd xmm0, rdi\n");
    fprintf(out, "    sqrtsd xmm0, xmm0\n");
    fprintf(out, "    movq rax, xmm0\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n");
    fprintf(out, ".L_ildiz_double:\n");
    fprintf(out, "    movq xmm0, rdi\n");
    fprintf(out, "    sqrtsd xmm0, xmm0\n");
    fprintf(out, "    movq rax, xmm0\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 24. Tizim buyrug'i (Process execution via fork, execve, wait4)
    fprintf(out, "algo_tizim_buyruq:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    push r13\n");
    fprintf(out, "    test rdi, rdi\n");
    fprintf(out, "    jz .L_tizim_err\n");
    fprintf(out, "    lea r12, [rdi + 8]\n");
    fprintf(out, "    mov rax, 57\n"); // sys_fork
    fprintf(out, "    syscall\n");
    fprintf(out, "    test rax, rax\n");
    fprintf(out, "    js .L_tizim_err\n");
    fprintf(out, "    jz .L_tizim_child\n");
    fprintf(out, "    mov r13, rax\n"); // child pid
    fprintf(out, "    sub rsp, 16\n");
    fprintf(out, "    mov rdi, r13\n");
    fprintf(out, "    mov rsi, rsp\n");
    fprintf(out, "    xor rdx, rdx\n");
    fprintf(out, "    xor r10, r10\n");
    fprintf(out, "    mov rax, 61\n"); // sys_wait4
    fprintf(out, "    syscall\n");
    fprintf(out, "    mov eax, dword ptr [rsp]\n");
    fprintf(out, "    shr eax, 8\n");
    fprintf(out, "    and eax, 0xFF\n");
    fprintf(out, "    add rsp, 16\n");
    fprintf(out, "    jmp .L_tizim_done\n");
    fprintf(out, ".L_tizim_child:\n");
    fprintf(out, "    lea r8, [rip + algo_str_bin_sh]\n");
    fprintf(out, "    lea r9, [rip + algo_str_sh_c]\n");
    fprintf(out, "    sub rsp, 32\n");
    fprintf(out, "    mov [rsp], r8\n");
    fprintf(out, "    mov [rsp + 8], r9\n");
    fprintf(out, "    mov [rsp + 16], r12\n");
    fprintf(out, "    mov qword ptr [rsp + 24], 0\n");
    fprintf(out, "    mov rax, 59\n"); // sys_execve
    fprintf(out, "    mov rdi, r8\n");
    fprintf(out, "    mov rsi, rsp\n");
    fprintf(out, "    mov rdx, [rip + algo_envp]\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    mov rdi, 127\n");
    fprintf(out, "    mov rax, 60\n");
    fprintf(out, "    syscall\n");
    fprintf(out, ".L_tizim_err:\n");
    fprintf(out, "    mov rax, -1\n");
    fprintf(out, ".L_tizim_done:\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 25. Massivga element qo'shish (Dynamic array push / append)
    fprintf(out, "algo_massiv_qosh:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    push r13\n");
    fprintf(out, "    push r14\n");
    fprintf(out, "    push r15\n");
    fprintf(out, "    push rbx\n");
    fprintf(out, "    mov r12, rdi\n");
    fprintf(out, "    mov r13, rsi\n");
    fprintf(out, "    test r12, r12\n");
    fprintf(out, "    jnz .L_mqosh_exist\n");
    fprintf(out, "    mov rax, 48\n"); // 16 + 4*8
    fprintf(out, "    call algo_ajrat\n");
    fprintf(out, "    mov qword ptr [rax], 0\n");
    fprintf(out, "    mov qword ptr [rax + 8], 4\n");
    fprintf(out, "    mov r12, rax\n");
    fprintf(out, ".L_mqosh_exist:\n");
    fprintf(out, "    mov r14, [r12]\n");     // length
    fprintf(out, "    mov r15, [r12 + 8]\n"); // capacity
    fprintf(out, "    cmp r14, r15\n");
    fprintf(out, "    jl .L_mqosh_have_space\n");
    fprintf(out, "    mov rbx, r15\n");
    fprintf(out, "    add rbx, 2\n");
    fprintf(out, "    shl rbx, 1\n");         // new_cap
    fprintf(out, "    mov rax, rbx\n");
    fprintf(out, "    shl rax, 3\n");
    fprintf(out, "    add rax, 16\n");
    fprintf(out, "    call algo_ajrat\n");
    fprintf(out, "    mov [rax], r14\n");
    fprintf(out, "    mov [rax + 8], rbx\n");
    fprintf(out, "    mov rcx, r14\n");
    fprintf(out, "    shl rcx, 3\n");
    fprintf(out, "    lea rsi, [r12 + 16]\n");
    fprintf(out, "    lea rdi, [rax + 16]\n");
    fprintf(out, "    push rax\n");
    fprintf(out, "    rep movsb\n");
    fprintf(out, "    pop rax\n");
    fprintf(out, "    mov r12, rax\n");
    fprintf(out, ".L_mqosh_have_space:\n");
    fprintf(out, "    mov [r12 + 16 + r14 * 8], r13\n");
    fprintf(out, "    inc r14\n");
    fprintf(out, "    mov [r12], r14\n");
    fprintf(out, "    mov rax, r12\n");
    fprintf(out, "    pop rbx\n");
    fprintf(out, "    pop r15\n");
    fprintf(out, "    pop r14\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 26. Massiv oxiridan sug'urib olish (Dynamic array pop)
    fprintf(out, "algo_massiv_sugur:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    test rdi, rdi\n");
    fprintf(out, "    jz .L_msugur_empty\n");
    fprintf(out, "    mov rcx, [rdi]\n");
    fprintf(out, "    test rcx, rcx\n");
    fprintf(out, "    jle .L_msugur_empty\n");
    fprintf(out, "    dec rcx\n");
    fprintf(out, "    mov [rdi], rcx\n");
    fprintf(out, "    mov rax, [rdi + 16 + rcx * 8]\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n");
    fprintf(out, ".L_msugur_empty:\n");
    fprintf(out, "    xor rax, rax\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 27. Matn prefiks bilan boshlanishi (String startswith)
    fprintf(out, "algo_matn_boshlanadimi:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    test rdi, rdi\n");
    fprintf(out, "    jz .L_mbosh_no\n");
    fprintf(out, "    test rsi, rsi\n");
    fprintf(out, "    jz .L_mbosh_no\n");
    fprintf(out, "    mov rax, [rdi]\n");
    fprintf(out, "    mov rcx, [rsi]\n");
    fprintf(out, "    cmp rax, rcx\n");
    fprintf(out, "    jb .L_mbosh_no\n");
    fprintf(out, "    test rcx, rcx\n");
    fprintf(out, "    jz .L_mbosh_yes\n");
    fprintf(out, "    lea rdi, [rdi + 8]\n");
    fprintf(out, "    lea rsi, [rsi + 8]\n");
    fprintf(out, ".L_mbosh_loop:\n");
    fprintf(out, "    mov al, [rdi]\n");
    fprintf(out, "    cmp al, [rsi]\n");
    fprintf(out, "    jne .L_mbosh_no\n");
    fprintf(out, "    inc rdi\n");
    fprintf(out, "    inc rsi\n");
    fprintf(out, "    dec rcx\n");
    fprintf(out, "    jnz .L_mbosh_loop\n");
    fprintf(out, ".L_mbosh_yes:\n");
    fprintf(out, "    mov rax, 1\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n");
    fprintf(out, ".L_mbosh_no:\n");
    fprintf(out, "    xor rax, rax\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 28. Matn suffiks bilan tugashi (String endswith)
    fprintf(out, "algo_matn_tugaydimi:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    test rdi, rdi\n");
    fprintf(out, "    jz .L_mtugay_no\n");
    fprintf(out, "    test rsi, rsi\n");
    fprintf(out, "    jz .L_mtugay_no\n");
    fprintf(out, "    mov rax, [rdi]\n");
    fprintf(out, "    mov rcx, [rsi]\n");
    fprintf(out, "    cmp rax, rcx\n");
    fprintf(out, "    jb .L_mtugay_no\n");
    fprintf(out, "    test rcx, rcx\n");
    fprintf(out, "    jz .L_mtugay_yes\n");
    fprintf(out, "    sub rax, rcx\n");
    fprintf(out, "    lea rdi, [rdi + 8 + rax]\n");
    fprintf(out, "    lea rsi, [rsi + 8]\n");
    fprintf(out, ".L_mtugay_loop:\n");
    fprintf(out, "    mov al, [rdi]\n");
    fprintf(out, "    cmp al, [rsi]\n");
    fprintf(out, "    jne .L_mtugay_no\n");
    fprintf(out, "    inc rdi\n");
    fprintf(out, "    inc rsi\n");
    fprintf(out, "    dec rcx\n");
    fprintf(out, "    jnz .L_mtugay_loop\n");
    fprintf(out, ".L_mtugay_yes:\n");
    fprintf(out, "    mov rax, 1\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n");
    fprintf(out, ".L_mtugay_no:\n");
    fprintf(out, "    xor rax, rax\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 29. Matn chetlarini tozalash (String strip / trim)
    fprintf(out, "algo_matn_qirqish:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    push r13\n");
    fprintf(out, "    push r14\n");
    fprintf(out, "    push r15\n");
    fprintf(out, "    test rdi, rdi\n");
    fprintf(out, "    jz .L_mqirq_empty\n");
    fprintf(out, "    mov r12, rdi\n");
    fprintf(out, "    mov r13, [r12]\n");
    fprintf(out, "    test r13, r13\n");
    fprintf(out, "    jz .L_mqirq_empty\n");
    fprintf(out, "    xor r14, r14\n");
    fprintf(out, ".L_mqirq_start_loop:\n");
    fprintf(out, "    cmp r14, r13\n");
    fprintf(out, "    jge .L_mqirq_empty\n");
    fprintf(out, "    movzx eax, byte ptr [r12 + 8 + r14]\n");
    fprintf(out, "    cmp al, 32\n");
    fprintf(out, "    ja .L_mqirq_start_found\n");
    fprintf(out, "    inc r14\n");
    fprintf(out, "    jmp .L_mqirq_start_loop\n");
    fprintf(out, ".L_mqirq_start_found:\n");
    fprintf(out, "    mov r15, r13\n");
    fprintf(out, "    dec r15\n");
    fprintf(out, ".L_mqirq_end_loop:\n");
    fprintf(out, "    cmp r15, r14\n");
    fprintf(out, "    jl .L_mqirq_empty\n");
    fprintf(out, "    movzx eax, byte ptr [r12 + 8 + r15]\n");
    fprintf(out, "    cmp al, 32\n");
    fprintf(out, "    ja .L_mqirq_end_found\n");
    fprintf(out, "    dec r15\n");
    fprintf(out, "    jmp .L_mqirq_end_loop\n");
    fprintf(out, ".L_mqirq_end_found:\n");
    fprintf(out, "    mov rax, r15\n");
    fprintf(out, "    sub rax, r14\n");
    fprintf(out, "    inc rax\n");
    fprintf(out, "    mov rdx, rax\n");
    fprintf(out, "    mov rdi, r12\n");
    fprintf(out, "    mov rsi, r14\n");
    fprintf(out, "    call algo_matn_kesish\n");
    fprintf(out, "    jmp .L_mqirq_done\n");
    fprintf(out, ".L_mqirq_empty:\n");
    fprintf(out, "    mov rax, 16\n");
    fprintf(out, "    call algo_ajrat\n");
    fprintf(out, "    mov qword ptr [rax], 0\n");
    fprintf(out, "    mov byte ptr [rax + 8], 0\n");
    fprintf(out, ".L_mqirq_done:\n");
    fprintf(out, "    pop r15\n");
    fprintf(out, "    pop r14\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 30. Matnni almashtirish (String replace)
    fprintf(out, "algo_matn_almashtir:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    push r13\n");
    fprintf(out, "    push r14\n");
    fprintf(out, "    push r15\n");
    fprintf(out, "    push rbx\n");
    fprintf(out, "    mov r12, rdi\n");
    fprintf(out, "    mov r13, rsi\n");
    fprintf(out, "    mov r14, rdx\n");
    fprintf(out, "    test r12, r12\n");
    fprintf(out, "    jz .L_malmash_empty\n");
    fprintf(out, "    test r13, r13\n");
    fprintf(out, "    jz .L_malmash_ret_orig\n");
    fprintf(out, "    mov rbx, [r13]\n");
    fprintf(out, "    test rbx, rbx\n");
    fprintf(out, "    jz .L_malmash_ret_orig\n");
    fprintf(out, "    xor r15, r15\n");
    fprintf(out, "    xor rcx, rcx\n");
    fprintf(out, "    mov r8, [r12]\n");
    fprintf(out, ".L_malmash_cnt_loop:\n");
    fprintf(out, "    mov r9, rcx\n");
    fprintf(out, "    add r9, rbx\n");
    fprintf(out, "    cmp r9, r8\n");
    fprintf(out, "    ja .L_malmash_cnt_done\n");
    fprintf(out, "    xor r10, r10\n");
    fprintf(out, ".L_malmash_cnt_cmp:\n");
    fprintf(out, "    cmp r10, rbx\n");
    fprintf(out, "    je .L_malmash_cnt_match\n");
    fprintf(out, "    lea rax, [rcx + r10]\n");
    fprintf(out, "    mov al, [r12 + 8 + rax]\n");
    fprintf(out, "    cmp al, [r13 + 8 + r10]\n");
    fprintf(out, "    jne .L_malmash_cnt_nomatch\n");
    fprintf(out, "    inc r10\n");
    fprintf(out, "    jmp .L_malmash_cnt_cmp\n");
    fprintf(out, ".L_malmash_cnt_match:\n");
    fprintf(out, "    inc r15\n");
    fprintf(out, "    add rcx, rbx\n");
    fprintf(out, "    jmp .L_malmash_cnt_loop\n");
    fprintf(out, ".L_malmash_cnt_nomatch:\n");
    fprintf(out, "    inc rcx\n");
    fprintf(out, "    jmp .L_malmash_cnt_loop\n");
    fprintf(out, ".L_malmash_cnt_done:\n");
    fprintf(out, "    test r15, r15\n");
    fprintf(out, "    jz .L_malmash_ret_orig\n");
    fprintf(out, "    mov r10, [r14]\n");
    fprintf(out, "    sub r10, rbx\n");
    fprintf(out, "    imul r10, r15\n");
    fprintf(out, "    mov rax, r8\n");
    fprintf(out, "    add rax, r10\n");
    fprintf(out, "    push rax\n");
    fprintf(out, "    add rax, 16\n");
    fprintf(out, "    call algo_ajrat\n");
    fprintf(out, "    pop r10\n");
    fprintf(out, "    mov [rax], r10\n");
    fprintf(out, "    push rax\n");
    fprintf(out, "    lea rdi, [rax + 8]\n");
    fprintf(out, "    mov r8, [r12]\n");
    fprintf(out, "    mov rbx, [r13]\n");
    fprintf(out, "    xor rcx, rcx\n");
    fprintf(out, ".L_malmash_wr_loop:\n");
    fprintf(out, "    cmp rcx, r8\n");
    fprintf(out, "    jge .L_malmash_wr_done\n");
    fprintf(out, "    mov r9, rcx\n");
    fprintf(out, "    add r9, rbx\n");
    fprintf(out, "    cmp r9, r8\n");
    fprintf(out, "    ja .L_malmash_wr_single\n");
    fprintf(out, "    xor r11, r11\n");
    fprintf(out, ".L_malmash_wr_cmp:\n");
    fprintf(out, "    cmp r11, rbx\n");
    fprintf(out, "    je .L_malmash_wr_replace\n");
    fprintf(out, "    lea rax, [rcx + r11]\n");
    fprintf(out, "    mov al, [r12 + 8 + rax]\n");
    fprintf(out, "    cmp al, [r13 + 8 + r11]\n");
    fprintf(out, "    jne .L_malmash_wr_single\n");
    fprintf(out, "    inc r11\n");
    fprintf(out, "    jmp .L_malmash_wr_cmp\n");
    fprintf(out, ".L_malmash_wr_replace:\n");
    fprintf(out, "    push rcx\n");
    fprintf(out, "    mov rcx, [r14]\n");
    fprintf(out, "    lea rsi, [r14 + 8]\n");
    fprintf(out, "    rep movsb\n");
    fprintf(out, "    pop rcx\n");
    fprintf(out, "    add rcx, rbx\n");
    fprintf(out, "    jmp .L_malmash_wr_loop\n");
    fprintf(out, ".L_malmash_wr_single:\n");
    fprintf(out, "    mov al, [r12 + 8 + rcx]\n");
    fprintf(out, "    mov [rdi], al\n");
    fprintf(out, "    inc rdi\n");
    fprintf(out, "    inc rcx\n");
    fprintf(out, "    jmp .L_malmash_wr_loop\n");
    fprintf(out, ".L_malmash_wr_done:\n");
    fprintf(out, "    mov byte ptr [rdi], 0\n");
    fprintf(out, "    pop rax\n");
    fprintf(out, "    jmp .L_malmash_done\n");
    fprintf(out, ".L_malmash_ret_orig:\n");
    fprintf(out, "    mov rax, r12\n");
    fprintf(out, "    jmp .L_malmash_done\n");
    fprintf(out, ".L_malmash_empty:\n");
    fprintf(out, "    mov rax, 16\n");
    fprintf(out, "    call algo_ajrat\n");
    fprintf(out, "    mov qword ptr [rax], 0\n");
    fprintf(out, "    mov byte ptr [rax + 8], 0\n");
    fprintf(out, ".L_malmash_done:\n");
    fprintf(out, "    pop rbx\n");
    fprintf(out, "    pop r15\n");
    fprintf(out, "    pop r14\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 31. Matnni ajratuvchi bo'yicha bo'lish (String split)
    fprintf(out, "algo_matn_bolish:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    push r13\n");
    fprintf(out, "    push r14\n");
    fprintf(out, "    push r15\n");
    fprintf(out, "    push rbx\n");
    fprintf(out, "    sub rsp, 32\n");
    fprintf(out, "    mov r12, rdi\n");
    fprintf(out, "    mov r13, rsi\n");
    fprintf(out, "    mov rax, 48\n"); // 16 header + 4*8 elements
    fprintf(out, "    call algo_ajrat\n");
    fprintf(out, "    mov qword ptr [rax], 0\n");
    fprintf(out, "    mov qword ptr [rax + 8], 4\n");
    fprintf(out, "    mov [rbp - 48], rax\n");
    fprintf(out, "    test r12, r12\n");
    fprintf(out, "    jz .L_mbol_done\n");
    fprintf(out, "    test r13, r13\n");
    fprintf(out, "    jz .L_mbol_single\n");
    fprintf(out, "    mov rbx, [r13]\n");
    fprintf(out, "    test rbx, rbx\n");
    fprintf(out, "    jz .L_mbol_single\n");
    fprintf(out, "    mov r14, [r12]\n");
    fprintf(out, "    xor r15, r15\n");
    fprintf(out, "    xor rcx, rcx\n");
    fprintf(out, ".L_mbol_loop:\n");
    fprintf(out, "    mov [rbp - 56], rcx\n");
    fprintf(out, "    mov r8, rcx\n");
    fprintf(out, "    add r8, rbx\n");
    fprintf(out, "    cmp r8, r14\n");
    fprintf(out, "    ja .L_mbol_tail\n");
    fprintf(out, "    xor r9, r9\n");
    fprintf(out, ".L_mbol_cmp:\n");
    fprintf(out, "    cmp r9, rbx\n");
    fprintf(out, "    je .L_mbol_match\n");
    fprintf(out, "    lea rax, [rcx + r9]\n");
    fprintf(out, "    mov al, [r12 + 8 + rax]\n");
    fprintf(out, "    cmp al, [r13 + 8 + r9]\n");
    fprintf(out, "    jne .L_mbol_nomatch\n");
    fprintf(out, "    inc r9\n");
    fprintf(out, "    jmp .L_mbol_cmp\n");
    fprintf(out, ".L_mbol_match:\n");
    fprintf(out, "    mov rdx, rcx\n");
    fprintf(out, "    sub rdx, r15\n");
    fprintf(out, "    mov rsi, r15\n");
    fprintf(out, "    mov rdi, r12\n");
    fprintf(out, "    call algo_matn_kesish\n");
    fprintf(out, "    mov rdi, [rbp - 48]\n");
    fprintf(out, "    mov rsi, rax\n");
    fprintf(out, "    call algo_massiv_qosh\n");
    fprintf(out, "    mov [rbp - 48], rax\n");
    fprintf(out, "    mov rcx, [rbp - 56]\n");
    fprintf(out, "    mov rbx, [r13]\n");
    fprintf(out, "    add rcx, rbx\n");
    fprintf(out, "    mov r15, rcx\n");
    fprintf(out, "    jmp .L_mbol_loop\n");
    fprintf(out, ".L_mbol_nomatch:\n");
    fprintf(out, "    mov rcx, [rbp - 56]\n");
    fprintf(out, "    inc rcx\n");
    fprintf(out, "    jmp .L_mbol_loop\n");
    fprintf(out, ".L_mbol_tail:\n");
    fprintf(out, "    mov rdx, r14\n");
    fprintf(out, "    sub rdx, r15\n");
    fprintf(out, "    mov rsi, r15\n");
    fprintf(out, "    mov rdi, r12\n");
    fprintf(out, "    call algo_matn_kesish\n");
    fprintf(out, "    mov rdi, [rbp - 48]\n");
    fprintf(out, "    mov rsi, rax\n");
    fprintf(out, "    call algo_massiv_qosh\n");
    fprintf(out, "    mov [rbp - 48], rax\n");
    fprintf(out, "    jmp .L_mbol_done\n");
    fprintf(out, ".L_mbol_single:\n");
    fprintf(out, "    mov rdi, [rbp - 48]\n");
    fprintf(out, "    mov rsi, r12\n");
    fprintf(out, "    call algo_massiv_qosh\n");
    fprintf(out, "    mov [rbp - 48], rax\n");
    fprintf(out, ".L_mbol_done:\n");
    fprintf(out, "    mov rax, [rbp - 48]\n");
    fprintf(out, "    add rsp, 32\n");
    fprintf(out, "    pop rbx\n");
    fprintf(out, "    pop r15\n");
    fprintf(out, "    pop r14\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 32. Massivni bitta matnga birlashtirish (String join)
    fprintf(out, "algo_matn_birlashtir_massiv:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    push r13\n");
    fprintf(out, "    push r14\n");
    fprintf(out, "    push r15\n");
    fprintf(out, "    push rbx\n");
    fprintf(out, "    sub rsp, 16\n");
    fprintf(out, "    mov r12, rdi\n");
    fprintf(out, "    mov r13, rsi\n");
    fprintf(out, "    test r12, r12\n");
    fprintf(out, "    jz .L_mjoin_empty\n");
    fprintf(out, "    mov r14, [r12]\n");
    fprintf(out, "    test r14, r14\n");
    fprintf(out, "    jz .L_mjoin_empty\n");
    fprintf(out, "    mov rbx, 0\n");
    fprintf(out, "    test r13, r13\n");
    fprintf(out, "    jz .L_mjoin_no_sep\n");
    fprintf(out, "    mov rbx, [r13]\n");
    fprintf(out, ".L_mjoin_no_sep:\n");
    fprintf(out, "    xor r15, r15\n");
    fprintf(out, "    xor rcx, rcx\n");
    fprintf(out, ".L_mjoin_len_loop:\n");
    fprintf(out, "    cmp rcx, r14\n");
    fprintf(out, "    jge .L_mjoin_len_done\n");
    fprintf(out, "    mov rax, [r12 + 16 + rcx * 8]\n");
    fprintf(out, "    test rax, rax\n");
    fprintf(out, "    jz .L_mjoin_len_next\n");
    fprintf(out, "    add r15, [rax]\n");
    fprintf(out, ".L_mjoin_len_next:\n");
    fprintf(out, "    inc rcx\n");
    fprintf(out, "    jmp .L_mjoin_len_loop\n");
    fprintf(out, ".L_mjoin_len_done:\n");
    fprintf(out, "    mov rax, r14\n");
    fprintf(out, "    dec rax\n");
    fprintf(out, "    imul rax, rbx\n");
    fprintf(out, "    add r15, rax\n");
    fprintf(out, "    mov [rsp], r15\n");
    fprintf(out, "    mov rax, r15\n");
    fprintf(out, "    add rax, 16\n");
    fprintf(out, "    call algo_ajrat\n");
    fprintf(out, "    mov r15, rax\n");
    fprintf(out, "    mov rax, [rsp]\n");
    fprintf(out, "    mov [r15], rax\n");
    fprintf(out, "    lea rdi, [r15 + 8]\n");
    fprintf(out, "    xor rbx, rbx\n");
    fprintf(out, ".L_mjoin_copy_loop:\n");
    fprintf(out, "    cmp rbx, r14\n");
    fprintf(out, "    jge .L_mjoin_copy_done\n");
    fprintf(out, "    test rbx, rbx\n");
    fprintf(out, "    jz .L_mjoin_cp_elem\n");
    fprintf(out, "    test r13, r13\n");
    fprintf(out, "    jz .L_mjoin_cp_elem\n");
    fprintf(out, "    mov rcx, [r13]\n");
    fprintf(out, "    test rcx, rcx\n");
    fprintf(out, "    jz .L_mjoin_cp_elem\n");
    fprintf(out, "    lea rsi, [r13 + 8]\n");
    fprintf(out, "    rep movsb\n");
    fprintf(out, ".L_mjoin_cp_elem:\n");
    fprintf(out, "    mov r8, [r12 + 16 + rbx * 8]\n");
    fprintf(out, "    test r8, r8\n");
    fprintf(out, "    jz .L_mjoin_cp_next\n");
    fprintf(out, "    mov rcx, [r8]\n");
    fprintf(out, "    test rcx, rcx\n");
    fprintf(out, "    jz .L_mjoin_cp_next\n");
    fprintf(out, "    lea rsi, [r8 + 8]\n");
    fprintf(out, "    rep movsb\n");
    fprintf(out, ".L_mjoin_cp_next:\n");
    fprintf(out, "    inc rbx\n");
    fprintf(out, "    jmp .L_mjoin_copy_loop\n");
    fprintf(out, ".L_mjoin_copy_done:\n");
    fprintf(out, "    mov byte ptr [rdi], 0\n");
    fprintf(out, "    mov rax, r15\n");
    fprintf(out, "    jmp .L_mjoin_ret\n");
    fprintf(out, ".L_mjoin_empty:\n");
    fprintf(out, "    mov rax, 16\n");
    fprintf(out, "    call algo_ajrat\n");
    fprintf(out, "    mov qword ptr [rax], 0\n");
    fprintf(out, "    mov byte ptr [rax + 8], 0\n");
    fprintf(out, ".L_mjoin_ret:\n");
    fprintf(out, "    add rsp, 16\n");
    fprintf(out, "    pop rbx\n");
    fprintf(out, "    pop r15\n");
    fprintf(out, "    pop r14\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 33. TCP Mijoz Ulanishi (TCP connect syscall)
    fprintf(out, "algo_tarmoq_ulan:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    push r13\n");
    fprintf(out, "    push r14\n");
    fprintf(out, "    push r15\n");
    fprintf(out, "    push rbx\n");
    fprintf(out, "    sub rsp, 32\n");
    fprintf(out, "    mov r12, rdi\n");
    fprintf(out, "    mov r13, rsi\n");
    fprintf(out, "    mov rax, 41\n"); // sys_socket
    fprintf(out, "    mov rdi, 2\n");
    fprintf(out, "    mov rsi, 1\n");
    fprintf(out, "    xor rdx, rdx\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    test rax, rax\n");
    fprintf(out, "    js .L_tulan_err\n");
    fprintf(out, "    mov r14, rax\n");
    fprintf(out, "    mov r15d, 0x0100007F\n"); // 127.0.0.1 default
    fprintf(out, "    test r12, r12\n");
    fprintf(out, "    jz .L_tulan_have_ip\n");
    fprintf(out, "    cmp qword ptr [r12], 0\n");
    fprintf(out, "    je .L_tulan_have_ip\n");
    fprintf(out, "    lea rsi, [r12 + 8]\n");
    fprintf(out, "    xor r15d, r15d\n");
    fprintf(out, "    xor r8, r8\n");
    fprintf(out, ".L_tulan_parse_octet:\n");
    fprintf(out, "    xor eax, eax\n");
    fprintf(out, ".L_tulan_parse_digits:\n");
    fprintf(out, "    movzx ecx, byte ptr [rsi]\n");
    fprintf(out, "    cmp cl, 48\n");
    fprintf(out, "    jb .L_tulan_digit_done\n");
    fprintf(out, "    cmp cl, 57\n");
    fprintf(out, "    ja .L_tulan_digit_done\n");
    fprintf(out, "    sub cl, 48\n");
    fprintf(out, "    imul eax, 10\n");
    fprintf(out, "    add eax, ecx\n");
    fprintf(out, "    inc rsi\n");
    fprintf(out, "    jmp .L_tulan_parse_digits\n");
    fprintf(out, ".L_tulan_digit_done:\n");
    fprintf(out, "    mov ecx, r8d\n");
    fprintf(out, "    shl ecx, 3\n");
    fprintf(out, "    shl eax, cl\n");
    fprintf(out, "    or r15d, eax\n");
    fprintf(out, "    inc r8\n");
    fprintf(out, "    cmp byte ptr [rsi], 46\n"); // '.'
    fprintf(out, "    jne .L_tulan_ip_finished\n");
    fprintf(out, "    inc rsi\n");
    fprintf(out, "    cmp r8, 4\n");
    fprintf(out, "    jl .L_tulan_parse_octet\n");
    fprintf(out, ".L_tulan_ip_finished:\n");
    fprintf(out, ".L_tulan_have_ip:\n");
    fprintf(out, "    mov word ptr [rsp], 2\n");
    fprintf(out, "    mov rax, r13\n");
    fprintf(out, "    xchg al, ah\n");
    fprintf(out, "    mov word ptr [rsp + 2], ax\n");
    fprintf(out, "    mov dword ptr [rsp + 4], r15d\n");
    fprintf(out, "    mov qword ptr [rsp + 8], 0\n");
    fprintf(out, "    mov rax, 42\n"); // sys_connect
    fprintf(out, "    mov rdi, r14\n");
    fprintf(out, "    mov rsi, rsp\n");
    fprintf(out, "    mov rdx, 16\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    test rax, rax\n");
    fprintf(out, "    js .L_tulan_close_err\n");
    fprintf(out, "    mov rax, r14\n");
    fprintf(out, "    jmp .L_tulan_ret\n");
    fprintf(out, ".L_tulan_close_err:\n");
    fprintf(out, "    mov rax, 3\n");
    fprintf(out, "    mov rdi, r14\n");
    fprintf(out, "    syscall\n");
    fprintf(out, ".L_tulan_err:\n");
    fprintf(out, "    mov rax, -1\n");
    fprintf(out, ".L_tulan_ret:\n");
    fprintf(out, "    add rsp, 32\n");
    fprintf(out, "    pop rbx\n");
    fprintf(out, "    pop r15\n");
    fprintf(out, "    pop r14\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 34. Matn ichidan qidirish va indeksini qaytarish (Find substring index, or -1)
    // rdi = str, rsi = needle
    fprintf(out, "algo_matn_topish:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    push r13\n");
    fprintf(out, "    push r14\n");
    fprintf(out, "    push r15\n");
    fprintf(out, "    push rbx\n");
    fprintf(out, "    mov r12, rdi\n");
    fprintf(out, "    mov r13, rsi\n");
    fprintf(out, "    test r12, r12\n");
    fprintf(out, "    jz .L_find_not_found\n");
    fprintf(out, "    test r13, r13\n");
    fprintf(out, "    jz .L_find_not_found\n");
    fprintf(out, "    mov r14, [r12]\n");
    fprintf(out, "    mov r15, [r13]\n");
    fprintf(out, "    cmp r15, r14\n");
    fprintf(out, "    jg .L_find_not_found\n");
    fprintf(out, "    test r15, r15\n");
    fprintf(out, "    jz .L_find_zero\n");
    fprintf(out, "    mov r8, r14\n");
    fprintf(out, "    sub r8, r15\n");
    fprintf(out, "    xor r9, r9\n");
    fprintf(out, ".L_find_outer:\n");
    fprintf(out, "    cmp r9, r8\n");
    fprintf(out, "    jg .L_find_not_found\n");
    fprintf(out, "    xor r10, r10\n");
    fprintf(out, ".L_find_inner:\n");
    fprintf(out, "    cmp r10, r15\n");
    fprintf(out, "    jge .L_find_found\n");
    fprintf(out, "    mov r11, r9\n");
    fprintf(out, "    add r11, r10\n");
    fprintf(out, "    mov al, [r12 + 8 + r11]\n");
    fprintf(out, "    mov bl, [r13 + 8 + r10]\n");
    fprintf(out, "    cmp al, bl\n");
    fprintf(out, "    jne .L_find_next\n");
    fprintf(out, "    inc r10\n");
    fprintf(out, "    jmp .L_find_inner\n");
    fprintf(out, ".L_find_next:\n");
    fprintf(out, "    inc r9\n");
    fprintf(out, "    jmp .L_find_outer\n");
    fprintf(out, ".L_find_found:\n");
    fprintf(out, "    mov rax, r9\n");
    fprintf(out, "    jmp .L_find_ret\n");
    fprintf(out, ".L_find_zero:\n");
    fprintf(out, "    xor rax, rax\n");
    fprintf(out, "    jmp .L_find_ret\n");
    fprintf(out, ".L_find_not_found:\n");
    fprintf(out, "    mov rax, -1\n");
    fprintf(out, ".L_find_ret:\n");
    fprintf(out, "    pop rbx\n");
    fprintf(out, "    pop r15\n");
    fprintf(out, "    pop r14\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 35. Belgining ASCII kodini olish (Get character ASCII code at index)
    // rdi = str, rsi = index
    fprintf(out, "algo_belgi_kodi:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    test rdi, rdi\n");
    fprintf(out, "    jz .L_char_code_err\n");
    fprintf(out, "    mov rcx, [rdi]\n");
    fprintf(out, "    cmp rsi, rcx\n");
    fprintf(out, "    jae .L_char_code_err\n");
    fprintf(out, "    cmp rsi, 0\n");
    fprintf(out, "    jl .L_char_code_err\n");
    fprintf(out, "    movzx rax, byte ptr [rdi + 8 + rsi]\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n");
    fprintf(out, ".L_char_code_err:\n");
    fprintf(out, "    xor rax, rax\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 36. Birinchi belgining ASCII kodi (ASCII code of first character)
    // rdi = str
    fprintf(out, "algo_kod:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    test rdi, rdi\n");
    fprintf(out, "    jz .L_kod_err\n");
    fprintf(out, "    cmp qword ptr [rdi], 0\n");
    fprintf(out, "    jle .L_kod_err\n");
    fprintf(out, "    movzx rax, byte ptr [rdi + 8]\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n");
    fprintf(out, ".L_kod_err:\n");
    fprintf(out, "    xor rax, rax\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 37. Belgidan matn yasash (char code to 1-char string)
    // rdi = char code
    fprintf(out, "algo_belgi:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    mov r12, rdi\n");
    fprintf(out, "    mov rax, 18\n");
    fprintf(out, "    call algo_ajrat\n");
    fprintf(out, "    mov qword ptr [rax], 1\n");
    fprintf(out, "    mov byte ptr [rax + 8], r12b\n");
    fprintf(out, "    mov byte ptr [rax + 9], 0\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 38. Fayl oxiriga qo'shish (Append string to file)
    // rdi = path, rsi = text
    fprintf(out, "algo_fayl_qoshish:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    push r12\n");
    fprintf(out, "    push r13\n");
    fprintf(out, "    test rdi, rdi\n");
    fprintf(out, "    jz .L_file_app_err\n");
    fprintf(out, "    test rsi, rsi\n");
    fprintf(out, "    jz .L_file_app_err\n");
    fprintf(out, "    mov r13, rsi\n");
    fprintf(out, "    lea rdi, [rdi + 8]\n");
    fprintf(out, "    mov rsi, 1089\n"); // O_WRONLY | O_CREAT | O_APPEND
    fprintf(out, "    mov rdx, 420\n");  // 0644
    fprintf(out, "    mov rax, 2\n");    // sys_open
    fprintf(out, "    syscall\n");
    fprintf(out, "    test rax, rax\n");
    fprintf(out, "    js .L_file_app_err\n");
    fprintf(out, "    mov r12, rax\n");
    fprintf(out, "    mov rdx, [r13]\n");
    fprintf(out, "    lea rsi, [r13 + 8]\n");
    fprintf(out, "    mov rdi, r12\n");
    fprintf(out, "    mov rax, 1\n");    // sys_write
    fprintf(out, "    syscall\n");
    fprintf(out, "    mov rax, 3\n");    // sys_close
    fprintf(out, "    mov rdi, r12\n");
    fprintf(out, "    syscall\n");
    fprintf(out, "    mov rax, 1\n");
    fprintf(out, "    jmp .L_file_app_done\n");
    fprintf(out, ".L_file_app_err:\n");
    fprintf(out, "    mov rax, -1\n");
    fprintf(out, ".L_file_app_done:\n");
    fprintf(out, "    pop r13\n");
    fprintf(out, "    pop r12\n");
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n\n");

    // 39. Joriy jarayon identifikatori (Process ID)
    fprintf(out, "algo_jarayon_id:\n");
    fprintf(out, "    mov rax, 39\n"); // sys_getpid
    fprintf(out, "    syscall\n");
    fprintf(out, "    ret\n\n");

    // Barcha foydalanuvchi funksiyalarini generatsiya qilish
    for (int i = 0; i < prog->function_count; i++) {
        gen_function(prog->functions[i], out);
    }

    // .data va .rodata bo'limi
    fprintf(out, "\n.section .data\n");
    fprintf(out, "algo_argc:       .quad 0\n");
    fprintf(out, "algo_argv:       .quad 0\n");
    fprintf(out, "algo_envp:       .quad 0\n");
    fprintf(out, "algo_heap_start: .quad 0\n");
    fprintf(out, "algo_heap_ptr:   .quad 0\n");
    fprintf(out, "algo_heap_end:   .quad 0\n");

    fprintf(out, "\n.section .rodata\n");
    fprintf(out, "algo_rodata_start:\n");
    fprintf(out, "algo_str_bin_sh:\n    .string \"/bin/sh\"\n");
    fprintf(out, "algo_str_sh_c:\n    .string \"-c\"\n");
    fprintf(out, "algo_newline:\n    .byte 10\n");
    fprintf(out, "algo_space:\n    .byte 32\n");
    fprintf(out, "algo_lbracket:\n    .byte 91\n");
    fprintf(out, "algo_rbracket:\n    .byte 93\n");
    fprintf(out, "algo_comma_space:\n    .ascii \", \"\n");
    fprintf(out, "algo_assert_msg:\n    .string \"[Tasdiq Xatosi]: \"\n");
    fprintf(out, "algo_fp_dot:\n    .byte 46\n");
    fprintf(out, "algo_fp_minus:\n    .byte 45\n");
    fprintf(out, "algo_fp_scale:\n    .double 100000.0\n");
    fprintf(out, "algo_fp_neg_one:\n    .double -1.0\n");

    for (int i = 0; i < float_count; i++) {
        fprintf(out, ".LC_f%d:\n", floats[i].id);
        fprintf(out, "    .double %.10f\n", floats[i].val);
    }

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
    fprintf(out, "algo_rodata_end:\n");
}
