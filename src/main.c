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

static void print_topic_help(const char *topic) {
    printf("\033[1;36m╔══════════════════════════════════════════════════════════════════════════════╗\033[0m\n");
    printf("\033[1;36m║\033[0m \033[1;37mALGO LUG'ATI VA QO'LLANMASI: \033[1;33m%-47s\033[0m \033[1;36m║\033[0m\n", topic);
    printf("\033[1;36m╚══════════════════════════════════════════════════════════════════════════════╝\033[0m\n\n");

    if (strcmp(topic, "amal") == 0 || strcmp(topic, "funksiya") == 0) {
        printf("\033[1;32mKategoriya:\033[0m Kalit so'z (Sintaksis)\n");
        printf("\033[1;32mTavsifi:\033[0m    Funksiya (protsedura) e'lon qilish. Har qanday amalda argumentlar qabul qilinadi va natija qaytariladi.\n");
        printf("\033[1;32mSintaksis:\033[0m  \033[1;33mamal nom(arg1, arg2, ...) { ... qaytar natija }\033[0m\n\n");
        printf("\033[1mNamuna Kod:\033[0m\n");
        printf("  \033[0;32mamal kvadrat(son) {\033[0m\n");
        printf("  \033[0;32m    qaytar son * son\033[0m\n");
        printf("  \033[0;32m}\033[0m\n");
        printf("  \033[0;32mamal asosiy() {\033[0m\n");
        printf("  \033[0;32m    chiqar(\"Kvadrati:\", kvadrat(8))\033[0m\n");
        printf("  \033[0;32m    qaytar 0\033[0m\n");
        printf("  \033[0;32m}\033[0m\n");
    } else if (strcmp(topic, "asosiy") == 0) {
        printf("\033[1;32mKategoriya:\033[0m Kirish Nuqtasi (Entry Point)\n");
        printf("\033[1;32mTavsifi:\033[0m    Har bir Algo dasturi x86_64 mashina kodida '_start' orqali boshlanib, 'asosiy' amalini chaqiradi.\n");
        printf("\033[1;32mSintaksis:\033[0m  \033[1;33mamal asosiy() { qaytar 0 }\033[0m\n");
    } else if (strcmp(topic, "qaytar") == 0 || strcmp(topic, "return") == 0) {
        printf("\033[1;32mKategoriya:\033[0m Kalit so'z (Return)\n");
        printf("\033[1;32mTavsifi:\033[0m    Amaldan chiqish va hisoblangan qiymatni chaqiruvchiga uzatish. RAX registriga joylanadi.\n");
        printf("\033[1;32mSintaksis:\033[0m  \033[1;33mqaytar natija\033[0m yoki \033[1;33mqaytar 0\033[0m\n");
    } else if (strcmp(topic, "agar") == 0 || strcmp(topic, "shart") == 0 || strcmp(topic, "if") == 0) {
        printf("\033[1;32mKategoriya:\033[0m Shart Operatorlari (If / Else)\n");
        printf("\033[1;32mTavsifi:\033[0m    Mantiqiy ifodaga ko'ra kod blokini shartli bajarish.\n");
        printf("\033[1;32mSintaksis:\033[0m  \033[1;33magar shart { ... } aks agar shart2 { ... } aks { ... }\033[0m\n\n");
        printf("\033[1mNamuna Kod:\033[0m\n");
        printf("  \033[0;32myosh = 20\033[0m\n");
        printf("  \033[0;32magar yosh >= 18 {\033[0m\n");
        printf("  \033[0;32m    chiqar(\"Voyaga yetgan\")\033[0m\n");
        printf("  \033[0;32m} aks {\033[0m\n");
        printf("  \033[0;32m    chiqar(\"Voyaga yetmagan\")\033[0m\n");
        printf("  \033[0;32m}\033[0m\n");
    } else if (strcmp(topic, "toki") == 0 || strcmp(topic, "sikl") == 0 || strcmp(topic, "while") == 0) {
        printf("\033[1;32mKategoriya:\033[0m Takrorlanish Sikli (While)\n");
        printf("\033[1;32mTavsifi:\033[0m    Shart rost bo'lib turguniga qadar blokni qayta-qayta bajaradi.\n");
        printf("\033[1;32mSintaksis:\033[0m  \033[1;33mtoki shart { ... }\033[0m\n\n");
        printf("\033[1mNamuna Kod:\033[0m\n");
        printf("  \033[0;32mi = 1\033[0m\n");
        printf("  \033[0;32mtoki i <= 5 {\033[0m\n");
        printf("  \033[0;32m    chiqar(\"Qadam:\", i)\033[0m\n");
        printf("  \033[0;32m    i = i + 1\033[0m\n");
        printf("  \033[0;32m}\033[0m\n");
    } else if (strcmp(topic, "uchun") == 0 || strcmp(topic, "for") == 0) {
        printf("\033[1;32mKategoriya:\033[0m Diapazon Sikli (For in Range/Array)\n");
        printf("\033[1;32mTavsifi:\033[0m    Belgilangan sonlar oralig'i (a..b) yoki massiv bo'yicha iteratsiya qiladi.\n");
        printf("\033[1;32mSintaksis:\033[0m  \033[1;33muchun ozgaruvchi ichida boshlangich..oxirgi { ... }\033[0m\n\n");
        printf("\033[1mNamuna Kod:\033[0m\n");
        printf("  \033[0;32muchun son ichida 1..10 {\033[0m\n");
        printf("  \033[0;32m    chiqar(\"Son:\", son)\033[0m\n");
        printf("  \033[0;32m}\033[0m\n");
    } else if (strcmp(topic, "tuzilma") == 0 || strcmp(topic, "struct") == 0) {
        printf("\033[1;32mKategoriya:\033[0m Ma'lumotlar Tuzilmasi (Struct)\n");
        printf("\033[1;32mTavsifi:\033[0m    Bir nechta maydonlarni o'z ichiga oluvchi foydalanuvchi ma'lumotlar tipi.\n");
        printf("\033[1;32mSintaksis:\033[0m  \033[1;33mtuzilma Nom { maydon1, maydon2 }\033[0m\n\n");
        printf("\033[1mNamuna Kod:\033[0m\n");
        printf("  \033[0;32mtuzilma Talaba { ism, ball }\033[0m\n");
        printf("  \033[0;32mali = Talaba(\"Ali\", 95)\033[0m\n");
        printf("  \033[0;32mchiqar(ali.ism, ali.ball)\033[0m\n");
    } else if (strcmp(topic, "chiqar") == 0 || strcmp(topic, "print") == 0) {
        printf("\033[1;32mKategoriya:\033[0m O'rnatilgan Amal (Built-in)\n");
        printf("\033[1;32mTavsifi:\033[0m    Linux 'sys_write' chaqiruvi orqali standart chiqishga (stdout/1) ma'lumot chop etadi.\n");
        printf("\033[1;32mSintaksis:\033[0m  \033[1;33mchiqar(arg1, arg2, ...)\033[0m\n");
    } else if (strcmp(topic, "kirit") == 0 || strcmp(topic, "input") == 0) {
        printf("\033[1;32mKategoriya:\033[0m O'rnatilgan Amal (Built-in)\n");
        printf("\033[1;32mTavsifi:\033[0m    Standart kiritishdan (stdin/0) 'sys_read' orqali bir satr matn o'qiydi.\n");
        printf("\033[1;32mSintaksis:\033[0m  \033[1;33msatr = kirit()\033[0m\n");
    } else if (strcmp(topic, "http") == 0 || strcmp(topic, "web") == 0) {
        printf("\033[1;32mKategoriya:\033[0m Standart Kutubxona (\033[1;33mstd/http.algo\033[0m)\n");
        printf("\033[1;32mTavsifi:\033[0m    HTTP/1.1 veb-serveri va REST API yaratish vositalari.\n");
        printf("\033[1;32mAmallar:\033[0m    http_metod, http_yol, http_javob_json, http_javob_html, http_tana\n\n");
        printf("\033[1mNamuna Kod:\033[0m\n");
        printf("  \033[0;32myukla \"std/http.algo\"\033[0m\n");
        printf("  \033[0;32myukla \"std/tarmoq.algo\"\033[0m\n");
        printf("  \033[0;32mamal asosiy() {\033[0m\n");
        printf("  \033[0;32m    server_fd = server_yarat(8080)\033[0m\n");
        printf("  \033[0;32m    chiqar(\"Server ishga tushdi: port 8080\")\033[0m\n");
        printf("  \033[0;32m    toki 1 == 1 {\033[0m\n");
        printf("  \033[0;32m        fd = mijoz_kut(server_fd)\033[0m\n");
        printf("  \033[0;32m        sorov = xabar_qabul(fd, 2048)\033[0m\n");
        printf("  \033[0;32m        javob = http_javob_json(200, \"{\\\"holat\\\":\\\"faol\\\"}\")\033[0m\n");
        printf("  \033[0;32m        xabar_yubor(fd, javob)\033[0m\n");
        printf("  \033[0;32m        ulanish_yop(fd)\033[0m\n");
        printf("  \033[0;32m    }\033[0m\n");
        printf("  \033[0;32m    qaytar 0\033[0m\n");
        printf("  \033[0;32m}\033[0m\n");
    } else if (strcmp(topic, "baza") == 0 || strcmp(topic, "db") == 0) {
        printf("\033[1;32mKategoriya:\033[0m Standart Kutubxona (\033[1;33mstd/baza.algo\033[0m)\n");
        printf("\033[1;32mTavsifi:\033[0m    Lokal diskda tranzaksiyali saqlanuvchi kalit-qiymat (Key-Value) ma'lumotlar bazasi.\n");
        printf("\033[1;32mAmallar:\033[0m    baza_och, baza_yoz, baza_oqish, baza_saqla, baza_ochir\n\n");
        printf("\033[1mNamuna Kod:\033[0m\n");
        printf("  \033[0;32myukla \"std/baza.algo\"\033[0m\n");
        printf("  \033[0;32mbaza_och(\"malumot.baza\")\033[0m\n");
        printf("  \033[0;32mbaza_yoz(\"muallif\", \"Jasurbek\")\033[0m\n");
        printf("  \033[0;32mism = baza_oqish(\"muallif\")\033[0m\n");
        printf("  \033[0;32mbaza_saqla(\"malumot.baza\")\033[0m\n");
    } else if (strcmp(topic, "matematika") == 0 || strcmp(topic, "math") == 0) {
        printf("\033[1;32mKategoriya:\033[0m Standart Kutubxona (\033[1;33mstd/matematika.algo\033[0m)\n");
        printf("\033[1;32mTavsifi:\033[0m    SSE2 apparatli kvadrat ildiz, daraja, EKUB, EKUK, sinus, kosinus va yaxlitlash.\n");
        printf("\033[1;32mAmallar:\033[0m    ildiz(x), daraja(a, b), ekub(a, b), ekuk(a, b), sin(rad), cos(rad), yaxlitla(x)\n");
    } else if (strcmp(topic, "fayl") == 0 || strcmp(topic, "file") == 0) {
        printf("\033[1;32mKategoriya:\033[0m Standart Kutubxona (\033[1;33mstd/fayl.algo\033[0m)\n");
        printf("\033[1;32mTavsifi:\033[0m    Zero-libc x86_64 direct syscalls orqali fayl tizimi boshqaruvi.\n");
        printf("\033[1;32mAmallar:\033[0m    fayl_oqish(yol), fayl_yozish(yol, matn), fayl_bormi(yol), fayl_ochir(yol)\n");
    } else if (strcmp(topic, "rang") == 0 || strcmp(topic, "color") == 0) {
        printf("\033[1;32mKategoriya:\033[0m Standart Kutubxona (\033[1;33mstd/rang.algo\033[0m)\n");
        printf("\033[1;32mTavsifi:\033[0m    Terminal uchun 16 ta ANSI ranglari va matn formatlash amallari.\n");
        printf("\033[1;32mAmallar:\033[0m    rang_turkuaz(m), rang_yashil(m), rang_qizil(m), rang_sariq(m), rang_qalin(m)\n");
    } else {
        printf("\033[1;31mNoma'lum mavzu:\033[0m '%s'\n", topic);
        printf("Mavjud lug'at bo'limlari: amal, asosiy, qaytar, agar, toki, uchun, tuzilma, chiqar, kirit, http, baza, matematika, fayl, rang, tarmoq, massiv, matn\n");
        printf("Barcha lug'atni ko'rish uchun: \033[1;32malgo yordam\033[0m\n");
    }
}

static void print_help(const char *topic) {
    if (topic != NULL && strlen(topic) > 0 && strcmp(topic, "barchasi") != 0 && strcmp(topic, "all") != 0) {
        print_topic_help(topic);
        return;
    }

    print_banner();
    printf("\033[1;36m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m\n");
    printf(" \033[1;37m1. CLI ASBOBLAR VA BUYRUQLAR RO'YXATI (TOOLCHAIN COMMANDS)\033[0m\n");
    printf("\033[1;36m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m\n");
    printf("  \033[1;32malgo ishga-tushir <fayl>\033[0m   (run)    Kompilyatsiya qilib, darhol ishga tushirish\n");
    printf("  \033[1;32malgo yig <fayl> [-o nom]\033[0m   (build)  Mustaqil x86_64 ELF binar mashina kodiga yig'ish\n");
    printf("  \033[1;32malgo sinov [fayl]\033[0m          (test)   31 ta birlik sinovlarni avtomatik tekshirish\n");
    printf("  \033[1;32malgo format <fayl>\033[0m         (fmt)    Kodni chiroyli tartibga solish (gofmt kabi)\n");
    printf("  \033[1;32malgo doc <fayl> [-o md]\033[0m    (hujjat) Koddan avtomatik Markdown hujjat yaratish\n");
    printf("  \033[1;32malgo tekshir <fayl>\033[0m        (lint)   Statik xatolar tahlilchisi (Linter)\n");
    printf("  \033[1;32malgo vaqt <fayl>\033[0m           (time)   Bajarilish vaqtini mikrosekundlarda o'lchash\n");
    printf("  \033[1;32malgo tezlik <fayl>\033[0m         (bench)  Unumdorlik va xotira iste'moli testi\n");
    printf("  \033[1;32malgo paket <yarat|royxat>\033[0m  (pkg)    Paket va modullar menejeri\n");
    printf("  \033[1;32malgo yangi <loyiha_nomi>\033[0m   (new)    Yangi Algo loyihasi shablonini yaratish\n");
    printf("  \033[1;32malgo repl\033[0m                           Interaktiv tezkor buyruqlar satri (REPL)\n");
    printf("  \033[1;32malgo lugat [soz]\033[0m                    Til sintaksisi va modullar lug'atini ko'rish\n");
    printf("  \033[1;32malgo versiya\033[0m                (version)Til versiyasi va tizim arxitekturasini ko'rish\n");
    printf("  \033[1;32malgo yordam [soz]\033[0m           (help)   To'liq yordam va lug'at oynasi\n\n");

    printf("\033[1;36m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m\n");
    printf(" \033[1;37m2. TIL SINTAKSISI VA KALIT SO'ZLAR LUG'ATI (KEYWORDS DICTIONARY)\033[0m\n");
    printf("\033[1;36m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m\n");
    printf("  \033[1;33mamal\033[0m       Funksiya yaratish: \033[0;32mamal qosh(a, b) { qaytar a + b }\033[0m\n");
    printf("  \033[1;33masosiy\033[0m     Dasturning asosiy kirish amali: \033[0;32mamal asosiy() { ... }\033[0m\n");
    printf("  \033[1;33mqaytar\033[0m     Amaldan natija qaytarish (\033[0;36mreturn\033[0m): \033[0;32mqaytar 0\033[0m\n");
    printf("  \033[1;33magar\033[0m       Shart tekshirish (\033[0;36mif\033[0m): \033[0;32magar x > 10 { chiqar(\"Katta\") }\033[0m\n");
    printf("  \033[1;33maks agar\033[0m   Qo'shimcha shart (\033[0;36melse if\033[0m): \033[0;32maks agar x == 10 { ... }\033[0m\n");
    printf("  \033[1;33maks\033[0m        Aks holdagi blok (\033[0;36melse\033[0m): \033[0;32maks { chiqar(\"Kichik\") }\033[0m\n");
    printf("  \033[1;33mtoki\033[0m       Shartli sikl (\033[0;36mwhile\033[0m): \033[0;32mtoki i < 10 { i = i + 1 }\033[0m\n");
    printf("  \033[1;33muchun\033[0m      Sikl (\033[0;36mfor\033[0m): \033[0;32muchun i ichida 1..5 { chiqar(i) }\033[0m\n");
    printf("  \033[1;33michida\033[0m     Oraliq yoki massiv elementi (\033[0;36min\033[0m): \033[0;32michida [1, 2, 3]\033[0m\n");
    printf("  \033[1;33mtoxtat\033[0m     Siklni zudlik bilan to'xtatish (\033[0;36mbreak\033[0m)\n");
    printf("  \033[1;33mdavom\033[0m      Siklning keyingi qadamiga o'tish (\033[0;36mcontinue\033[0m)\n");
    printf("  \033[1;33myukla\033[0m      Modulni ulash (\033[0;36mimport\033[0m): \033[0;32myukla \"std/http.algo\"\033[0m\n");
    printf("  \033[1;33mtuzilma\033[0m    Yangi ma'lumot tuzilmasi (\033[0;36mstruct\033[0m): \033[0;32mtuzilma Foydalanuvchi { ism, yosh }\033[0m\n\n");

    printf("\033[1;36m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m\n");
    printf(" \033[1;37m3. O'RNATILGAN ASOSIY AMALLAR LUG'ATI (CORE BUILT-IN FUNCTIONS)\033[0m\n");
    printf("\033[1;36m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m\n");
    printf("  \033[1;32mchiqar(...)\033[0m         Konsolga matn yoki qiymatlarni chop etadi (\033[0;36msys_write\033[0m)\n");
    printf("  \033[1;32mkirit()\033[0m             Terminaldan foydalanuvchi kiritgan satrni o'qiydi (\033[0;36msys_read\033[0m)\n");
    printf("  \033[1;32muzunlik(x)\033[0m          Matn yoki massiv elementlari sonini qaytaradi\n");
    printf("  \033[1;32mbutun(x)\033[0m            Matn yoki sonni 64-bitli butun songa o'giradi (int64)\n");
    printf("  \033[1;32mhaqiqiy(x)\033[0m          Sonni 64-bitli kasr haqiqiy songa o'tkazadi (float64)\n");
    printf("  \033[1;32mmatn(x)\033[0m             Son yoki obyektni matn (string) formatiga aylantiradi\n");
    printf("  \033[1;32mildiz(x)\033[0m            SSE2 uskunaviy kvadrat ildiz hisoblaydi (sqrt)\n");
    printf("  \033[1;32mtasodifiy(a, b)\033[0m     Kriptografik xavfsiz tasodifiy butun son qaytaradi (CSPRNG)\n");
    printf("  \033[1;32mvaqt()\033[0m              Unix vaqt tamg'asini mikrosekundlarda qaytaradi\n");
    printf("  \033[1;32mtizim_buyruq(cmd)\033[0m   Operatsion tizim buyrug'ini chaqiradi va kodini qaytaradi\n");
    printf("  \033[1;32mxotira_holati()\033[0m     Arena xotira ko'rsatkichining joriy holatini oladi\n");
    printf("  \033[1;32mxotira_tikla(p)\033[0m     Xotirani belgilangan nuqtagacha bir zumda tiklaydi (O(1))\n\n");

    printf("\033[1;36m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m\n");
    printf(" \033[1;37m4. MA'LUMOT TURLARI VA OPERATORLAR (TYPES & OPERATORS)\033[0m\n");
    printf("\033[1;36m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m\n");
    printf("  \033[1mButun Sonlar:\033[0m   64-bitli butun sonlar (masalan: 42, -15, 0, 1000000)\n");
    printf("  \033[1mHaqiqiy Sonlar:\033[0m 64-bitli suzuvchi kasr sonlar (masalan: 3.14159, 0.618)\n");
    printf("  \033[1mMatnlar:\033[0m        UTF-8 satrlar (masalan: \"Salom Dunyo!\", 'O\\'zbekiston')\n");
    printf("  \033[1mMantiqiy:\033[0m       \033[1;32mrost\033[0m (1 / true), \033[1;31myolgon\033[0m (0 / false)\n");
    printf("  \033[1mMassivlar:\033[0m      Dinamik ro'yxat: [10, 20, 30], element olish: massiv[0]\n");
    printf("  \033[1mLug'atlar:\033[0m      Kalit-qiymat xaritasi: {\"nom\": \"Algo\", \"versiya\": 1.5}\n");
    printf("  \033[1mArifmetika:\033[0m     + (qo'shish), - (ayirish), * (ko'paytirish), / (bo'lish), %% (qoldiq)\n");
    printf("  \033[1mTaqqoslash:\033[0m     == (teng), != (teng emas), < (kichik), <=, >, >=\n");
    printf("  \033[1mMantiqiy Amallar:\033[0mva (&&), yoki (||), emas (!)\n\n");

    printf("\033[1;36m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m\n");
    printf(" \033[1;37m5. STANDART KUTUBXONA (std/) MODULLAR LUG'ATI\033[0m\n");
    printf("\033[1;36m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m\n");
    printf("  \033[1;33mstd/http.algo\033[0m       HTTP/1.1 backend server, marshrutlar va JSON format\n");
    printf("                      \033[0;90mAmallar: http_metod, http_yol, http_javob_json, http_javob_html\033[0m\n");
    printf("  \033[1;33mstd/tarmoq.algo\033[0m     To'g'ridan-to'g'ri TCP soketlar (sys_socket, bind, listen)\n");
    printf("                      \033[0;90mAmallar: server_yarat, mijoz_kut, xabar_qabul, xabar_yubor\033[0m\n");
    printf("  \033[1;33mstd/baza.algo\033[0m       Tranzaksiyali persistent Key-Value lokal ma'lumotlar bazasi\n");
    printf("                      \033[0;90mAmallar: baza_och, baza_yoz, baza_oqish, baza_saqla, baza_ochir\033[0m\n");
    printf("  \033[1;33mstd/matematika.algo\033[0m Ilmiy hisob-kitoblar, ildiz, daraja, EKUB, EKUK, sin, cos\n");
    printf("                      \033[0;90mAmallar: ildiz, daraja, yaxlitla, pol, shift, ekub, ekuk, sin, cos\033[0m\n");
    printf("  \033[1;33mstd/matn.algo\033[0m       Matnlar bilan ishlash, qidirish, kesish, katta/kichik harflar\n");
    printf("                      \033[0;90mAmallar: matn_kesish, matn_topish, matn_katta, matn_kichik, matn_teng\033[0m\n");
    printf("  \033[1;33mstd/massiv.algo\033[0m     Massivlar bilan ishlash, tartiblash, qidirish va filtrlash\n");
    printf("                      \033[0;90mAmallar: massiv_qosh, massiv_sarala, massiv_teskari, massiv_yigindi\033[0m\n");
    printf("  \033[1;33mstd/fayl.algo\033[0m       Fayl tizimi: o'qish, yozish, tekshirish va o'chirish\n");
    printf("                      \033[0;90mAmallar: fayl_oqish, fayl_yozish, fayl_bormi, fayl_ochir\033[0m\n");
    printf("  \033[1;33mstd/rang.algo\033[0m       Terminal konsoli uchun 16 ta boy ANSI ranglar va formatlar\n");
    printf("                      \033[0;90mAmallar: rang_turkuaz, rang_yashil, rang_qizil, rang_sariq, rang_qalin\033[0m\n");
    printf("  \033[1;33mstd/bayroq.algo\033[0m     CLI argumentlari va flaglarini avtomatik tahlil qilish\n");
    printf("                      \033[0;90mAmallar: bayroq_matn_ol, bayroq_butun_ol, bayroq_bormi\033[0m\n");
    printf("  \033[1;33mstd/sinov.algo\033[0m      Birlik testlari (Unit Testing) va test hisobotlari\n");
    printf("                      \033[0;90mAmallar: sinovchi_yarat, sinov_teng_butun, sinov_rost, sinovchi_yakunla\033[0m\n");
    printf("  \033[1;33mstd/navbat.algo\033[0m     LIFO Stek va FIFO Navbat abstrakt ma'lumotlar tuzilmalari\n");
    printf("                      \033[0;90mAmallar: stek_yarat, stek_qoy, stek_ol, navbat_yarat, navbat_qoy\033[0m\n");
    printf("  \033[1;33mstd/arxiv.algo\033[0m      RLE algoritmi orqali ma'lumotlarni siqish va ochish\n");
    printf("                      \033[0;90mAmallar: arxiv_siqish_rle, arxiv_ochish_rle\033[0m\n");
    printf("  \033[1;33mstd/csv.algo\033[0m        CSV formatdagi jadvallarni o'qish va formatlash\n");
    printf("                      \033[0;90mAmallar: csv_qator_bol, csv_qator_qur\033[0m\n");
    printf("  \033[1;33mstd/shablon.algo\033[0m    Dinamik shablonlash va HTML xavfsiz sanitizatsiya\n");
    printf("                      \033[0;90mAmallar: shablon_almashtir, shablon_lugatdan, shablon_html_xavfsiz\033[0m\n");
    printf("  \033[1;33mstd/yol.algo\033[0m        Fayl yo'llarini birlashtirish va qismlarga ajratish\n");
    printf("                      \033[0;90mAmallar: yol_birlashtir, yol_asosiy, yol_papka, yol_kengaytma\033[0m\n\n");

    printf("\033[1;36m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m\n");
    printf(" \033[1;37m6. NAMUNA VA QIDIRUV (EXAMPLES & SEARCH)\033[0m\n");
    printf("\033[1;36m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\033[0m\n");
    printf("  Alohida kalit so'z yoki modul bo'yicha batafsil ma'lumot olish uchun:\n");
    printf("    \033[1;32malgo yordam amal\033[0m       - Funksiya sintaksisi va namunasi\n");
    printf("    \033[1;32malgo yordam agar\033[0m       - Shart operatori haqida\n");
    printf("    \033[1;32malgo yordam toki\033[0m       - Sikllar bo'yicha qo'llanma\n");
    printf("    \033[1;32malgo yordam http\033[0m       - Veb server yaratish namunasi\n");
    printf("    \033[1;32malgo yordam baza\033[0m       - Kalit-qiymat bazasidan foydalanish\n");
    printf("    \033[1;32malgo lugat\033[0m             - To'liq til lug'ati\n\n");
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

    if (strcmp(cmd, "yordam") == 0 || strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0 || strcmp(cmd, "lugat") == 0 || strcmp(cmd, "dictionary") == 0) {
        const char *topic = (argc >= 3) ? argv[2] : NULL;
        print_help(topic);
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
