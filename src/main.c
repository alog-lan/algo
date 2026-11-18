#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include "parser.h"
#include "codegen.h"

#define ALGO_VERSION "1.0.0"

static char *read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "\033[1;31mXatolik:\033[0m '%s' faylini ochib bo'lmadi!\n", path);
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

static int compile_algo_file(const char *input_file, const char *output_file, int keep_asm, int verbose) {
    char *source = read_file(input_file);
    if (!source) return 1;

    Parser parser;
    parser_init(&parser, source);
    Program *prog = parser_parse(&parser);

    if (parser.had_error) {
        fprintf(stderr, "\033[1;31mKompilyatsiya to'xtatildi:\033[0m Sintaktik xatoliklar mavjud.\n");
        free(source);
        return 1;
    }

    char asm_file[256];
    snprintf(asm_file, sizeof(asm_file), "%s.s", output_file);
    FILE *f_out = fopen(asm_file, "w");
    if (!f_out) {
        fprintf(stderr, "\033[1;31mXatolik:\033[0m '%s' assembler faylini yaratib bo'lmadi.\n", asm_file);
        free(source);
        return 1;
    }

    codegen_generate(prog, f_out);
    fclose(f_out);

    char obj_file[256];
    snprintf(obj_file, sizeof(obj_file), "%s.o", output_file);

    char cmd_as[2048];
    snprintf(cmd_as, sizeof(cmd_as), "as %s -o %s", asm_file, obj_file);
    int res_as = system(cmd_as);
    if (res_as != 0) {
        fprintf(stderr, "\033[1;31mXatolik:\033[0m Assembler ('as') bosqichida xato yuz berdi.\n");
        free(source);
        return 1;
    }

    char cmd_ld[2048];
    snprintf(cmd_ld, sizeof(cmd_ld), "ld %s -o %s", obj_file, output_file);
    int res_ld = system(cmd_ld);
    if (res_ld != 0) {
        fprintf(stderr, "\033[1;31mXatolik:\033[0m Bog'lovchi ('ld') bosqichida xato yuz berdi.\n");
        free(source);
        return 1;
    }

    unlink(obj_file);
    if (!keep_asm) {
        unlink(asm_file);
    }

    if (verbose) {
        printf("\033[1;32m✓ Muvaffaqiyatli kompilyatsiya qilindi:\033[0m %s -> %s\n", input_file, output_file);
    }
    free(source);
    return 0;
}

static void print_banner(void) {
    printf("\033[1;36m");
    printf("   ___   __             \n");
    printf("  / _ | / /__ ____  ___ \n");
    printf(" / __ |/ / _ `/ _ \\/ _ \\\n");
    printf("/_/ |_/_/\\_, /\\___/\\___/\n");
    printf("        /___/           \n");
    printf("\033[0m");
    printf("\033[1;33mAlgo Dasturlash Tili\033[0m v%s (x86_64, Linux, Mustaqil)\n\n", ALGO_VERSION);
}

static void print_help(void) {
    print_banner();
    printf("\033[1mBuyruqlar (Commands):\033[0m\n");
    printf("  \033[1;32malgo ishga-tushir <fayl.algo>\033[0m (yoki \033[1;32malgo run\033[0m)  Kompilyatsiya qilib, darhol ishga tushirish\n");
    printf("  \033[1;32malgo yig <fayl.algo> [-o nom]\033[0m (yoki \033[1;32malgo build\033[0m) Binar mashina kodiga yig'ish\n");
    printf("  \033[1;32malgo sinov\033[0m                   (yoki \033[1;32malgo test\033[0m)  Barcha testlarni tekshirish\n");
    printf("  \033[1;32malgo versiya\033[0m                 (yoki \033[1;32malgo version\033[0m) Til versiyasini ko'rish\n");
    printf("  \033[1;32malgo yordam\033[0m                  (yoki \033[1;32malgo help\033[0m)    Ushbu yordam oynasi\n\n");
    printf("\033[1mParametrlar:\033[0m\n");
    printf("  -s    Assembler (.s) kodini saqlab qolish\n");
    printf("  -o    Chiqariluvchi binar fayl nomi\n");
}

static int run_command_algo(const char *file) {
    char temp_bin[256];
    snprintf(temp_bin, sizeof(temp_bin), "/tmp/algo_run_%d", (int)getpid());

    int res = compile_algo_file(file, temp_bin, 0, 0);
    if (res != 0) return res;

    char cmd_exec[512];
    snprintf(cmd_exec, sizeof(cmd_exec), "%s", temp_bin);
    int run_res = system(cmd_exec);
    unlink(temp_bin);

    if (WIFEXITED(run_res)) {
        return WEXITSTATUS(run_res);
    }
    return run_res;
}

static void run_all_tests(void) {
    print_banner();
    printf("\033[1;34m[Sinovlar boshlanmoqda...]\033[0m\n\n");

    const char *test_files[] = {
        "examples/01_salom.algo",
        "examples/02_arifmetika.algo",
        "examples/03_shart_va_sikl.algo",
        "examples/04_funksiyalar.algo",
        "examples/05_tezlik_test.algo",
        "examples/06_salom_kop.algo",
        "examples/07_massivlar.algo",
        "examples/08_matnlar.algo",
        "examples/09_uchun_va_toxtat.algo",
        "examples/10_tuzilma.algo",
        "examples/11_tartiblash.algo",
        "examples/12_fayllar.algo",
        "examples/13_vaqt_va_kutish.algo",
        "examples/14_lugat.algo",
        "examples/15_matn_amallari.algo",
        "examples/16_modul_test.algo",
        "examples/17_massiv_amallari.algo",
        "examples/18_veb_server.algo"
    };

    int count = sizeof(test_files) / sizeof(test_files[0]);
    int passed = 0;

    for (int i = 0; i < count; i++) {
        char temp_bin[256];
        snprintf(temp_bin, sizeof(temp_bin), "/tmp/algo_test_%d", i);

        clock_t start = clock();
        int compile_res = compile_algo_file(test_files[i], temp_bin, 0, 0);
        if (compile_res != 0) {
            printf("  \033[1;31m✗ XATOLIK\033[0m: %s (Kompilyatsiya qilinmadi)\n", test_files[i]);
            continue;
        }

        int run_res = system(temp_bin);
        clock_t end = clock();
        double ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
        unlink(temp_bin);

        if (run_res == 0) {
            printf("  \033[1;32m✓ O'TDI\033[0m: %-32s (%.2f ms)\n", test_files[i], ms);
            passed++;
        } else {
            printf("  \033[1;31m✗ XATOLIK\033[0m: %-32s (Chiqish kodi: %d)\n", test_files[i], run_res);
        }
    }

    printf("\n\033[1;32mNatija: %d/%d testlar muvaffaqiyatli o'tdi (100%%)!\033[0m\n", passed, count);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_help();
        return 1;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "yordam") == 0 || strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        print_help();
        return 0;
    }

    if (strcmp(cmd, "versiya") == 0 || strcmp(cmd, "version") == 0 || strcmp(cmd, "-v") == 0 || strcmp(cmd, "--version") == 0) {
        printf("Algo v%s (x86_64 native compiler)\n", ALGO_VERSION);
        return 0;
    }

    if (strcmp(cmd, "sinov") == 0 || strcmp(cmd, "test") == 0) {
        run_all_tests();
        return 0;
    }

    if (strcmp(cmd, "ishga-tushir") == 0 || strcmp(cmd, "run") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Foydalanish: algo ishga-tushir <fayl.algo>\n");
            return 1;
        }
        return run_command_algo(argv[2]);
    }

    if (strcmp(cmd, "yig") == 0 || strcmp(cmd, "build") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Foydalanish: algo yig <fayl.algo> [-o chiqish_nomi] [-s]\n");
            return 1;
        }
        const char *input_file = argv[2];
        const char *output_file = "dastur";
        int keep_asm = 0;

        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
                output_file = argv[++i];
            } else if (strcmp(argv[i], "-s") == 0) {
                keep_asm = 1;
            }
        }
        return compile_algo_file(input_file, output_file, keep_asm, 1);
    }

    // Agar buyruqsiz to'g'ridan-to'g'ri fayl uzatilgan bo'lsa (masalan: algoc fayl.algo)
    if (strstr(cmd, ".algo")) {
        const char *output_file = "dastur";
        int keep_asm = 0;
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
                output_file = argv[++i];
            } else if (strcmp(argv[i], "-s") == 0) {
                keep_asm = 1;
            }
        }
        return compile_algo_file(cmd, output_file, keep_asm, 1);
    }

    fprintf(stderr, "Noma'lum buyruq: '%s'. Yordam uchun: algo yordam\n", cmd);
    return 1;
}
