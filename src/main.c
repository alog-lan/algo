#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "parser.h"
#include "codegen.h"

static char *read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Xatolik: '%s' faylini ochib bo'lmadi!\n", path);
        return NULL;
    }
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

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Algo Dasturlash Tili Kompilyatori (algoc)\n");
        printf("Foydalanish: algoc <fayl.algo> [-o chiqish_nomi] [-s]\n");
        printf("  -o <nom>   Binar fayl nomi\n");
        printf("  -s         Hosil bo'lgan .s (Assembler) faylni saqlab qolish\n");
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = "dastur";
    int keep_asm = 0;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-s") == 0) {
            keep_asm = 1;
        }
    }

    char *source = read_file(input_file);
    if (!source) return 1;

    Parser parser;
    parser_init(&parser, source);
    Program *prog = parser_parse(&parser);

    if (parser.had_error) {
        fprintf(stderr, "Kompilyatsiya to'xtatildi: sintaktik xatoliklar mavjud.\n");
        free(source);
        return 1;
    }

    char asm_file[256];
    snprintf(asm_file, sizeof(asm_file), "%s.s", output_file);
    FILE *f_out = fopen(asm_file, "w");
    if (!f_out) {
        fprintf(stderr, "Xatolik: '%s' assembler faylini yaratib bo'lmadi.\n", asm_file);
        free(source);
        return 1;
    }

    codegen_generate(prog, f_out);
    fclose(f_out);

    // as va ld orqali mashina kodiga o'girish
    char obj_file[256];
    snprintf(obj_file, sizeof(obj_file), "%s.o", output_file);

    char cmd_as[1024];
    snprintf(cmd_as, sizeof(cmd_as), "as %s -o %s", asm_file, obj_file);
    int res_as = system(cmd_as);
    if (res_as != 0) {
        fprintf(stderr, "Xatolik: Assembler ('as') bosqichida xato yuz berdi.\n");
        return 1;
    }

    char cmd_ld[1024];
    snprintf(cmd_ld, sizeof(cmd_ld), "ld %s -o %s", obj_file, output_file);
    int res_ld = system(cmd_ld);
    if (res_ld != 0) {
        fprintf(stderr, "Xatolik: Bog'lovchi ('ld') bosqichida xato yuz berdi.\n");
        return 1;
    }

    // Tozalash
    unlink(obj_file);
    if (!keep_asm) {
        unlink(asm_file);
    }

    printf("Muvaffaqiyatli kompilyatsiya qilindi: %s -> %s\n", input_file, output_file);
    free(source);
    return 0;
}
