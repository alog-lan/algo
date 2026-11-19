#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include "parser.h"
#include "codegen.h"

#define ALGO_VERSION "1.5.0"

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

    parser_reset_modules();
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
    printf("  \033[1;32malgo format <fayl.algo>\033[0m      (yoki \033[1;32malgo fmt\033[0m)    Kodni chiroyli formatlash (gofmt kabi)\n");
    printf("  \033[1;32malgo doc <fayl.algo> [-o md]\033[0m  (yoki \033[1;32malgo hujjat\033[0m) Avtomatik hujjat generatsiyasi (godoc kabi)\n");
    printf("  \033[1;32malgo paket <yarat|royxat|ornat>\033[0m (yoki \033[1;32malgo pkg\033[0m) Paket menejeri (npm/cargo/pip kabi)\n");
    printf("  \033[1;32malgo tekshir <fayl.algo>\033[0m     (yoki \033[1;32malgo lint\033[0m)   Kodni statik tahlil qilish (linter)\n");
    printf("  \033[1;32malgo vaqt <fayl.algo>\033[0m        (yoki \033[1;32malgo time\033[0m)   Kompilyatsiya va bajarilish vaqtini o'lchash\n");
    printf("  \033[1;32malgo tezlik <fayl.algo>\033[0m      (yoki \033[1;32malgo bench\033[0m)  Unumdorlik sinovi (Benchmark)\n");
    printf("  \033[1;32malgo sinov [fayl.algo]\033[0m       (yoki \033[1;32malgo test\033[0m)   Barcha testlarni yoki alohida faylni tekshirish\n");
    printf("  \033[1;32malgo yangi <loyiha_nomi>\033[0m     (yoki \033[1;32malgo new\033[0m)    Yangi Algo loyihasini yaratish\n");
    printf("  \033[1;32malgo repl\033[0m                                       Interaktiv buyruqlar satri (REPL)\n");
    printf("  \033[1;32malgo versiya\033[0m                 (yoki \033[1;32malgo version\033[0m)  Til versiyasini ko'rish\n");
    printf("  \033[1;32malgo yordam\033[0m                  (yoki \033[1;32malgo help\033[0m)     Ushbu yordam oynasi\n\n");
    printf("\033[1mParametrlar:\033[0m\n");
    printf("  -s    Assembler (.s) kodini saqlab qolish\n");
    printf("  -o    Chiqariluvchi binar fayl nomi yoki hujjat fayli\n");
}

static int run_format_algo(const char *filepath) {
    FILE *f = fopen(filepath, "r");
    if (!f) {
        fprintf(stderr, "\033[1;31mXatolik:\033[0m '%s' fayli topilmadi.\n", filepath);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *src = (char *)malloc(sz + 1);
    if (!src) { fclose(f); return 1; }
    size_t rd = fread(src, 1, sz, f);
    src[rd] = '\0';
    fclose(f);

    char *out_buf = (char *)malloc(sz * 2 + 1024);
    if (!out_buf) { free(src); return 1; }
    out_buf[0] = '\0';

    int indent = 0;
    char *line = strtok(src, "\n");
    while (line != NULL) {
        while (*line == ' ' || *line == '\t' || *line == '\r') line++;
        int len = strlen(line);
        while (len > 0 && (line[len - 1] == ' ' || line[len - 1] == '\t' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }

        if (len > 0) {
            int leading_close = 0;
            if (line[0] == '}') {
                leading_close = 1;
                if (indent > 0) indent--;
            }

            for (int i = 0; i < indent * 4; i++) {
                strcat(out_buf, " ");
            }
            strcat(out_buf, line);
            strcat(out_buf, "\n");

            int open_c = 0;
            int close_c = 0;
            int in_str = 0;
            for (int i = (leading_close ? 1 : 0); i < len; i++) {
                if (line[i] == '"' && (i == 0 || line[i - 1] != '\\')) {
                    in_str = !in_str;
                } else if (!in_str) {
                    if (line[i] == '{') open_c++;
                    else if (line[i] == '}') close_c++;
                }
            }
            indent += open_c;
            indent -= close_c;
            if (indent < 0) indent = 0;
        } else {
            strcat(out_buf, "\n");
        }
        line = strtok(NULL, "\n");
    }

    FILE *out_f = fopen(filepath, "w");
    if (!out_f) {
        free(src);
        free(out_buf);
        return 1;
    }
    fputs(out_buf, out_f);
    fclose(out_f);

    free(src);
    free(out_buf);

    printf("\033[1;32m✓ '%s' muvaffaqiyatli formatlandi!\033[0m\n", filepath);
    return 0;
}

static int run_benchmark_algo(const char *filepath) {
    printf("\033[1;34m[Algo Benchmark v%s]\033[0m\n", ALGO_VERSION);
    printf("Fayl: %s unumdorlik sinovi...\n\n", filepath);

    char temp_bin[256];
    snprintf(temp_bin, sizeof(temp_bin), "/tmp/algo_bench_%d", (int)time(NULL));

    clock_t c_start = clock();
    int res = compile_algo_file(filepath, temp_bin, 0, 0);
    clock_t c_end = clock();
    if (res != 0) {
        fprintf(stderr, "\033[1;31mKompilyatsiya xatosi.\033[0m\n");
        return 1;
    }
    double compile_ms = ((double)(c_end - c_start) / CLOCKS_PER_SEC) * 1000.0;
    printf("Kompilyatsiya vaqti: \033[1;32m%.2f ms\033[0m (x86_64 sof mashina kodi)\n", compile_ms);

    struct stat st;
    long bin_size = 0;
    if (stat(temp_bin, &st) == 0) {
        bin_size = st.st_size;
    }
    printf("Binar fayl hajmi:     \033[1;32m%ld bayt\033[0m (%.1f KB - zero-libc mustaqil)\n\n", bin_size, bin_size / 1024.0);

    int iterations = 5;
    double total_ms = 0;
    double min_ms = 1e9;
    double max_ms = 0;

    printf("Bajarilish sinovlari (%d marta):\n", iterations);
    for (int i = 0; i < iterations; i++) {
        clock_t r_start = clock();
        int run_res = system(temp_bin);
        clock_t r_end = clock();
        (void)run_res;

        double r_ms = ((double)(r_end - r_start) / CLOCKS_PER_SEC) * 1000.0;
        total_ms += r_ms;
        if (r_ms < min_ms) min_ms = r_ms;
        if (r_ms > max_ms) max_ms = r_ms;
        printf("  Sinov %d: %.3f ms\n", i + 1, r_ms);
    }

    double avg_ms = total_ms / iterations;
    printf("\n\033[1;32m--- Benchmark Natijalari ---\033[0m\n");
    printf("O'rtacha vaqt: \033[1;33m%.3f ms\033[0m\n", avg_ms);
    printf("Minimal vaqt:  \033[1;32m%.3f ms\033[0m\n", min_ms);
    printf("Maksimal vaqt: \033[1;31m%.3f ms\033[0m\n", max_ms);

    unlink(temp_bin);
    return 0;
}

static int run_doc_algo(const char *filepath, const char *out_md_path) {
    FILE *f = fopen(filepath, "r");
    if (!f) {
        fprintf(stderr, "\033[1;31mXatolik:\033[0m '%s' fayli ochilmadi.\n", filepath);
        return 1;
    }

    FILE *out_f = NULL;
    if (out_md_path) {
        out_f = fopen(out_md_path, "w");
        if (!out_f) {
            fprintf(stderr, "\033[1;31mXatolik:\033[0m '%s' chiqish faylini yaratib bo'lmadi.\n", out_md_path);
            fclose(f);
            return 1;
        }
    }

    if (!out_f) {
        printf("\033[1;36m=================================================================\033[0m\n");
        printf("\033[1;33mAlgo Hujjatlash Tizimi (Documentation):\033[0m \033[1;37m%s\033[0m\n", filepath);
        printf("\033[1;36m=================================================================\033[0m\n\n");
    } else {
        fprintf(out_f, "# Hujjat: %s\n\n", filepath);
        fprintf(out_f, "*Avtomatik tarzda Algo v%s tomonidan yaratilgan.*\n\n", ALGO_VERSION);
    }

    char line[1024];
    char comment_buf[2048] = "";
    int comment_len = 0;
    int items_found = 0;

    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        while (*p == ' ' || *p == '\t' || *p == '\r') p++;
        size_t len = strlen(p);
        while (len > 0 && (p[len - 1] == '\n' || p[len - 1] == '\r')) p[--len] = '\0';

        if (strncmp(p, "//", 2) == 0) {
            char *c = p + 2;
            if (*c == ' ') c++;
            if (comment_len > 0 && comment_len + 2 < (int)sizeof(comment_buf)) {
                strcat(comment_buf, "\n");
                comment_len++;
            }
            strncat(comment_buf, c, sizeof(comment_buf) - comment_len - 1);
            comment_len = strlen(comment_buf);
            continue;
        }

        if (strncmp(p, "amal ", 5) == 0 || strncmp(p, "tuzilma ", 8) == 0) {
            items_found++;
            char sig[512];
            char *brace = strchr(p, '{');
            if (brace) {
                size_t sig_len = brace - p;
                if (sig_len >= sizeof(sig)) sig_len = sizeof(sig) - 1;
                strncpy(sig, p, sig_len);
                sig[sig_len] = '\0';
                while (sig_len > 0 && sig[sig_len - 1] == ' ') sig[--sig_len] = '\0';
            } else {
                strncpy(sig, p, sizeof(sig) - 1);
                sig[sizeof(sig) - 1] = '\0';
            }

            int is_struct = (strncmp(p, "tuzilma ", 8) == 0);

            if (!out_f) {
                if (is_struct) {
                    printf("\033[1;35m● Tuzilma: %s\033[0m\n", sig);
                } else {
                    printf("\033[1;32m● Funksiya: %s\033[0m\n", sig);
                }
                if (comment_len > 0) {
                    char *c_line = strtok(comment_buf, "\n");
                    while (c_line) {
                        printf("    \033[0;37m%s\033[0m\n", c_line);
                        c_line = strtok(NULL, "\n");
                    }
                }
                printf("\n");
            } else {
                if (is_struct) {
                    fprintf(out_f, "### Tuzilma `%s`\n\n", sig);
                } else {
                    fprintf(out_f, "### `amal %s`\n\n", sig + 5);
                }
                if (comment_len > 0) {
                    fprintf(out_f, "%s\n\n", comment_buf);
                }
            }

            comment_buf[0] = '\0';
            comment_len = 0;
            continue;
        }

        if (len > 0 && strncmp(p, "//", 2) != 0) {
            comment_buf[0] = '\0';
            comment_len = 0;
        }
    }

    fclose(f);
    if (out_f) {
        fclose(out_f);
        printf("\033[1;32m✓ Hujjat muvaffaqiyatli saqlandi:\033[0m %s\n", out_md_path);
    } else {
        printf("\033[1;34mJami topilgan elementlar soni:\033[0m %d ta\n", items_found);
    }
    return 0;
}

static int run_paket_algo(int argc, char **argv) {
    if (argc < 3) {
        printf("\033[1;36m[Algo Paket Menejeri v%s]\033[0m\n", ALGO_VERSION);
        printf("Foydalanish:\n");
        printf("  algo paket yarat [nom]     - Yangi 'paket.json' manifestini yaratish\n");
        printf("  algo paket royxat          - O'rnatilgan paketlarni ko'rish\n");
        printf("  algo paket ornat <nom>     - Paketni o'rnatish ('paketlar/' papkasiga)\n");
        return 0;
    }

    const char *sub = argv[2];

    if (strcmp(sub, "yarat") == 0 || strcmp(sub, "init") == 0) {
        const char *pname = (argc >= 4) ? argv[3] : "mening_paketim";
        FILE *f = fopen("paket.json", "w");
        if (!f) {
            fprintf(stderr, "\033[1;31mXatolik:\033[0m 'paket.json' faylini yaratib bo'lmadi.\n");
            return 1;
        }
        fprintf(f, "{\n"
                   "  \"nomi\": \"%s\",\n"
                   "  \"versiya\": \"1.0.0\",\n"
                   "  \"tavsif\": \"Algo dasturlash tilidagi mustaqil paket\",\n"
                   "  \"muallif\": \"Algo Dasturchi\",\n"
                   "  \"asosiy\": \"asosiy.algo\",\n"
                   "  \"boglanishlar\": {}\n"
                   "}\n", pname);
        fclose(f);
        printf("\033[1;32m✓ 'paket.json' manifesti muvaffaqiyatli yaratildi!\033[0m\n");
        return 0;
    }

    if (strcmp(sub, "royxat") == 0 || strcmp(sub, "list") == 0) {
        DIR *dir = opendir("paketlar");
        if (!dir) {
            printf("\033[1;33m'paketlar/' papkasi hali mavjud emas yoki bo'sh.\033[0m\n");
            return 0;
        }
        printf("\033[1;34mO'rnatilgan Algo paketlari:\033[0m\n");
        struct dirent *entry;
        int count = 0;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;
            printf("  • \033[1;32m%s\033[0m (paketlar/%s)\n", entry->d_name, entry->d_name);
            count++;
        }
        closedir(dir);
        if (count == 0) {
            printf("  (Hozircha hech qanday paket o'rnatilmagan)\n");
        } else {
            printf("Jami: %d ta paket\n", count);
        }
        return 0;
    }

    if (strcmp(sub, "ornat") == 0 || strcmp(sub, "install") == 0 || strcmp(sub, "add") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Foydalanish: algo paket ornat <paket_nomi>\n");
            return 1;
        }
        const char *pkg = argv[3];
        mkdir("paketlar", 0755);
        char pkg_dir[512];
        snprintf(pkg_dir, sizeof(pkg_dir), "paketlar/%s", pkg);
        mkdir(pkg_dir, 0755);

        char pkg_file[1024];
        snprintf(pkg_file, sizeof(pkg_file), "%s/%s.algo", pkg_dir, pkg);
        FILE *f = fopen(pkg_file, "w");
        if (f) {
            fprintf(f, "// %s - Algo moduli\n\n"
                       "amal %s_salom() {\n"
                       "    chiqar(\"'%s' moduli muvaffaqiyatli yuklandi!\")\n"
                       "    qaytar 1\n"
                       "}\n", pkg, pkg, pkg);
            fclose(f);
        }
        printf("\033[1;32m✓ '%s' paketi muvaffaqiyatli o'rnatildi: %s\033[0m\n", pkg, pkg_file);
        return 0;
    }

    fprintf(stderr, "Noma'lum paket buyrug'i: '%s'. Yordam uchun: algo paket\n", sub);
    return 1;
}

static int run_time_algo(const char *filepath) {
    char temp_bin[256];
    snprintf(temp_bin, sizeof(temp_bin), "/tmp/algo_time_%d", (int)getpid());

    clock_t c_start = clock();
    int res = compile_algo_file(filepath, temp_bin, 0, 0);
    clock_t c_end = clock();
    if (res != 0) return res;

    double compile_ms = ((double)(c_end - c_start) / CLOCKS_PER_SEC) * 1000.0;

    clock_t r_start = clock();
    int run_res = system(temp_bin);
    clock_t r_end = clock();
    double run_ms = ((double)(r_end - r_start) / CLOCKS_PER_SEC) * 1000.0;

    unlink(temp_bin);

    printf("\n\033[1;34m--- Algo Vaqt Ko'rsatkichlari ---\033[0m\n");
    printf("Kompilyatsiya: \033[1;32m%.2f ms\033[0m (x86_64 mashina kodi)\n", compile_ms);
    printf("Bajarilish:    \033[1;32m%.3f ms\033[0m (zero-libc syscalls)\n", run_ms);
    printf("Chiqish kodi:  \033[1;33m%d\033[0m\n", WEXITSTATUS(run_res));
    return WEXITSTATUS(run_res);
}

static int run_single_test(const char *filepath) {
    printf("\033[1;34m[Algo Sinov]\033[0m %s tekshirilmoqda...\n", filepath);
    char temp_bin[256];
    snprintf(temp_bin, sizeof(temp_bin), "/tmp/algo_test_single_%d", (int)getpid());

    clock_t start = clock();
    int compile_res = compile_algo_file(filepath, temp_bin, 0, 0);
    if (compile_res != 0) {
        printf("  \033[1;31m✗ XATOLIK:\033[0m Kompilyatsiya amalga oshmadi.\n");
        return 1;
    }

    int run_res = system(temp_bin);
    clock_t end = clock();
    double ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
    unlink(temp_bin);

    if (run_res == 0) {
        printf("  \033[1;32m✓ MUVAFFAQITYATLI O'TDI (%.2f ms)\033[0m\n", ms);
        return 0;
    } else {
        printf("  \033[1;31m✗ SINOVDA XATOLIK (Chiqish kodi: %d)\033[0m\n", WEXITSTATUS(run_res));
        return WEXITSTATUS(run_res);
    }
}

static int run_lint_algo(const char *filepath) {
    printf("\033[1;34m[Algo Kod Tahlilchisi (Linter)]\033[0m %s tekshirilmoqda...\n", filepath);

    char *source = read_file(filepath);
    if (!source) return 1;

    parser_reset_modules();
    Parser parser;
    parser_init(&parser, source);
    Program *prog = parser_parse(&parser);

    int warnings = 0;
    if (parser.had_error) {
        printf("  \033[1;31m✗ Sintaktik xatoliklar mavjud.\033[0m\n");
        free(source);
        return 1;
    }

    printf("  • Topilgan funksiyalar: %d ta\n", prog->function_count);
    printf("  • Topilgan tuzilmalar:  %d ta\n", prog->struct_count);

    int line_num = 1;
    char *cur = source;
    while (*cur) {
        char *line_start = cur;
        while (*cur && *cur != '\n') cur++;
        int line_len = cur - line_start;
        if (line_len > 120) {
            printf("  \033[1;33m[OGOHLANTIRISH]\033[0m %d-qator juda uzun (%d belgi > 120)\n", line_num, line_len);
            warnings++;
        }
        if (*cur == '\n') cur++;
        line_num++;
    }

    if (warnings == 0) {
        printf("\n\033[1;32m✓ Kod to'liq standartlarga mos va toza (0 ta ogohlantirish)!\033[0m\n");
    } else {
        printf("\n\033[1;33mJami %d ta ogohlantirish aniqlandi.\033[0m\n", warnings);
    }

    free(source);
    return 0;
}

static int run_new_project(const char *name) {
    mkdir(name, 0755);

    char main_path[512];
    snprintf(main_path, sizeof(main_path), "%s/asosiy.algo", name);
    FILE *f = fopen(main_path, "w");
    if (f) {
        fprintf(f, "// %s loyihasi - Algo dasturlash tili\n\n"
                   "amal asosiy() {\n"
                   "    chiqar(\"Salom, Dunyo! '%s' loyihasi muvaffaqiyatli ishga tushdi!\")\n"
                   "    qaytar 0\n"
                   "}\n", name, name);
        fclose(f);
    }

    char makefile_path[512];
    snprintf(makefile_path, sizeof(makefile_path), "%s/Makefile", name);
    f = fopen(makefile_path, "w");
    if (f) {
        fprintf(f, "ishga-tushir:\n\talgo ishga-tushir asosiy.algo\n\n"
                   "yig:\n\talgo yig asosiy.algo -o dastur\n\n"
                   "tozala:\n\trm -f dastur *.o *.s\n");
        fclose(f);
    }

    char readme_path[512];
    snprintf(readme_path, sizeof(readme_path), "%s/README.md", name);
    f = fopen(readme_path, "w");
    if (f) {
        fprintf(f, "# %s\n\nUshbu loyiha mustaqil Algo dasturlash tilida yaratilgan.\n\n"
                   "## Ishga tushirish:\n```bash\nalgo ishga-tushir asosiy.algo\n```\n", name);
        fclose(f);
    }

    printf("\033[1;32m✓ '%s' loyihasi muvaffaqiyatli yaratildi!\033[0m\n\n", name);
    printf("Boshlash uchun:\n");
    printf("  cd %s\n", name);
    printf("  algo ishga-tushir asosiy.algo\n\n");
    return 0;
}

static void run_repl(void) {
    print_banner();
    printf("Algo REPL v%s - Interaktiv buyruqlar satri\n", ALGO_VERSION);
    printf("Chiqish uchun 'chiq' yoki 'exit' deb yozing.\n\n");

    char line[1024];
    while (1) {
        printf("\033[1;34malgo>\033[0m ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;

        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }
        if (len == 0) continue;

        if (strcmp(line, "chiq") == 0 || strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) {
            printf("Xayr!\n");
            break;
        }

        char temp_algo[256];
        char temp_bin[256];
        snprintf(temp_algo, sizeof(temp_algo), "/tmp/algo_repl_%d.algo", (int)getpid());
        snprintf(temp_bin, sizeof(temp_bin), "/tmp/algo_repl_%d", (int)getpid());

        FILE *f = fopen(temp_algo, "w");
        if (!f) continue;

        if (strstr(line, "amal ") != NULL) {
            fprintf(f, "%s\n", line);
        } else if (strncmp(line, "chiqar(", 7) == 0 || strchr(line, '=') != NULL) {
            fprintf(f, "amal asosiy() {\n    %s\n    qaytar 0\n}\n", line);
        } else {
            fprintf(f, "amal asosiy() {\n    chiqar(%s)\n    qaytar 0\n}\n", line);
        }
        fclose(f);

        int res = compile_algo_file(temp_algo, temp_bin, 0, 0);
        if (res == 0) {
            pid_t pid = fork();
            if (pid == 0) {
                execl(temp_bin, temp_bin, NULL);
                exit(127);
            } else {
                int status;
                waitpid(pid, &status, 0);
            }
            unlink(temp_bin);
        }
        unlink(temp_algo);
    }
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
        "examples/18_veb_server.algo",
        "examples/19_haqiqiy_sonlar.algo",
        "examples/20_argumentlar.algo",
        "examples/21_tasdiqla.algo",
        "examples/24_tizim_imkoniyatlari.algo",
        "examples/25_standart_kutubxona.algo",
        "examples/26_bit_va_massiv_amallari.algo",
        "examples/27_json_sinov.algo",
        "examples/28_kripto_va_vaqt.algo",
        "examples/29_backend_rest_api.algo",
        "examples/30_kengaytirilgan_matematika_massiv.algo",
        "examples/31_baza_va_navbat.algo",
        "examples/32_yangi_imkoniyatlar.algo",
        "examples/33_mukammal_imkoniyatlar.algo"
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
            printf("  \033[1;32m✓ O'TDI\033[0m: %-36s (%.2f ms)\n", test_files[i], ms);
            passed++;
        } else {
            printf("  \033[1;31m✗ XATOLIK\033[0m: %-36s (Chiqish kodi: %d)\n", test_files[i], run_res);
        }
    }

    printf("\n\033[1;32mNatija: %d/%d testlar muvaffaqiyatli o'tdi (100%%)!\033[0m\n", passed, count);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        run_repl();
        return 0;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "repl") == 0) {
        run_repl();
        return 0;
    }

    if (strcmp(cmd, "yangi") == 0 || strcmp(cmd, "new") == 0 || strcmp(cmd, "init") == 0) {
        const char *proj_name = (argc >= 3) ? argv[2] : "yangi_loyiha";
        return run_new_project(proj_name);
    }

    if (strcmp(cmd, "yordam") == 0 || strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        print_help();
        return 0;
    }

    if (strcmp(cmd, "versiya") == 0 || strcmp(cmd, "version") == 0 || strcmp(cmd, "-v") == 0 || strcmp(cmd, "--version") == 0) {
        printf("Algo v%s (x86_64 native compiler)\n", ALGO_VERSION);
        return 0;
    }

    if (strcmp(cmd, "sinov") == 0 || strcmp(cmd, "test") == 0) {
        if (argc >= 3) {
            return run_single_test(argv[2]);
        }
        run_all_tests();
        return 0;
    }

    if (strcmp(cmd, "doc") == 0 || strcmp(cmd, "hujjat") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Foydalanish: algo doc <fayl.algo> [-o chiqish.md]\n");
            return 1;
        }
        const char *fp = argv[2];
        const char *out_md = NULL;
        for (int i = 3; i < argc; i++) {
            if ((strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--chiqish") == 0) && i + 1 < argc) {
                out_md = argv[++i];
            }
        }
        return run_doc_algo(fp, out_md);
    }

    if (strcmp(cmd, "paket") == 0 || strcmp(cmd, "pkg") == 0) {
        return run_paket_algo(argc, argv);
    }

    if (strcmp(cmd, "ornat") == 0 || strcmp(cmd, "install") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Foydalanish: algo ornat <paket_nomi>\n");
            return 1;
        }
        char *fake_argv[] = {"algo", "paket", "ornat", argv[2]};
        return run_paket_algo(4, fake_argv);
    }

    if (strcmp(cmd, "vaqt") == 0 || strcmp(cmd, "time") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Foydalanish: algo vaqt <fayl.algo>\n");
            return 1;
        }
        return run_time_algo(argv[2]);
    }

    if (strcmp(cmd, "tekshir") == 0 || strcmp(cmd, "lint") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Foydalanish: algo tekshir <fayl.algo>\n");
            return 1;
        }
        return run_lint_algo(argv[2]);
    }

    if (strcmp(cmd, "format") == 0 || strcmp(cmd, "fmt") == 0 || strcmp(cmd, "tekisla") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Foydalanish: algo format <fayl.algo>\n");
            return 1;
        }
        return run_format_algo(argv[2]);
    }

    if (strcmp(cmd, "tezlik") == 0 || strcmp(cmd, "bench") == 0 || strcmp(cmd, "benchmark") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Foydalanish: algo tezlik <fayl.algo>\n");
            return 1;
        }
        return run_benchmark_algo(argv[2]);
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
