# 🚀 Algo Dasturlash Tili (`.algo`)

<p align="center">
  <b>Python kabi oson, Go dan tez, to'g'ridan-to'g'ri x86_64 GNU Assembler ustiga qurilgan mutlaq mustaqil dasturlash tili.</b>
</p>

---

## 🌟 Nega aynan Algo?

1. **Mutlaq Mustaqillik (Zero External Dependencies):**
   * Hech qanday LLVM, GCC runtime, libc (standart C kutubxonasi) yoki tashqi og'ir kutubxonalarga bog'liq emas.
   * Dasturlar to'g'ridan-to'g'ri Linux yadrosi tizim chaqiruvlari (**Direct Linux Syscalls: sys_write, sys_read, sys_open, sys_socket, sys_connect, sys_mmap va h.k.**) orqali x86_64 mashina kodida ishlaydi.
   * Hosil bo'ladigan mustaqil binar dasturlar hajmi bor-yo'g'i **~8.9 KB** *(Go dasturlari 2–10 MB, Python esa gigabaytlab runtime talab qilgan bir paytda)*.

2. **Chaqmoqdek Tezlik:**
   * Og'ir Garbage Collector (axlat yig'uvchi) to'xtashlari yo'q.
   * To'g'ridan-to'g'ri protsessor registrlarida (`RAX`, `RBX`, `RCX`, `RDX`, `RSI`, `RDI`, `XMM0-XMM7`) hisoblashlar bajariladi.
   * 100 million marta aylanadigan siklni **0.04 - 0.26 soniyada** hisoblaydi (*Pythondan 25-90+ barobar tez*).

3. **To'liq Mukammal Ekotizim (v1.5.0):**
   * **Ko'p Jarayonli Parallel Ishlash (Multi-Process Concurrency):** To'g'ridan-to'g'ri Linux `sys_fork` (57) va `sys_wait4` (61) orqali `jarayon_yarat()` va `jarayon_kut()` — CPU yadrolarida parallel hisoblash.
   * **Xotira Boshqaruvi va Arena Checkpoint:** `xotira_holati()` va `xotira_tikla()` orqali serverlarda xotirani to'liq boshqarish va qayta tiklash.
   * **Ko'p Qatorli Izohlar (`/* ... */`):** C, Go, Rust kabi ko'p qatorli kod bloklarini izohlash.
   * **Terminal Ranglari va UI (`std/rang.algo`):** `rang_qizil`, `rang_yashil`, `rang_sariq`, `rang_kok`, `rang_binafsha`, `rang_moviy`, `rang_oq`, `rang_qalin`, `rang_tagchiziq`.
   * **CLI Bayroqlar Tahlilchisi (`std/bayroq.algo`):** `bayroq_matn_ol`, `bayroq_butun_ol`, `bayroq_bormi` (Go `flag` / Python `argparse` kabi).
   * **Shablonlashtirish Tizimi (`std/shablon.algo`):** `shablon_almashtir`, `shablon_lugatdan`, `shablon_html_xavfsiz` (Jinja2 / Go template kabi).
   * **Ma'lumotlarni Siqish (`std/arxiv.algo`):** Run-Length Encoding algoritmi (`arxiv_siqish_rle`, `arxiv_ochish_rle`).
   * **Birlik Sinovlari Freymvorki (`std/sinov.algo`):** Rasmiy test tizimi (`sinovchi_yarat`, `sinov_teng_butun`, `sinov_teng_matn`, `sinov_rost`, `sinov_yolgon`, `sinovchi_yakunla`).
   * **Avtomatik Hujjatlash Tizimi (`algo doc`):** Kod izohlaridan avtomatik chiroyli konsol va Markdown API hujjatlari generatsiyasi (`godoc` kabi).
   * **Paket Menejeri (`algo paket` / `algo ornat`):** To'liq o'rnatilgan paket boshqaruvi (`paket.json`, o'rnatish, ro'yxat).
   * **Statik Kod Tahlilchisi (`algo tekshir` / `algo lint`):** Kod tozaligi, funksiyalar va tuzilmalar hisoboti.
   * **Fayl Yo'llari Kutubxonasi (`std/yol.algo`):** `yol_birlashtir`, `yol_asosiy` (basename), `yol_papka` (dirname), `yol_kengaytma`, `yol_mutlaqmi`.
   * **CSV Tahlil va Yozish (`std/csv.algo`):** `csv_qator_bol`, `csv_qator_qur`, `csv_matn_bol`, `csv_matn_qur`.
   * **Tizimli Loglash (`std/log.algo`):** `log_malumot`, `log_muvaffaqiyat`, `log_ogoh`, `log_xato`, `log_faylga`.
   * **Kengaytirilgan Matematika (`std/matematika.algo`):** `pol` (floor), `shift` (ceil), `yaxlitla` (round), `tasodifiy_oraliq`, `eksponenta`, `natural_logarifm`, `sinus`, `kosinus`, `tangens`.
   * **Yangi Built-in Tizim Chaqiruvlari:** `joriy_papka()` (`sys_getcwd`), `foydalanuvchi_id()` (`sys_getuid`), `fayl_hajmi()` (`sys_stat`), `papka_bormi()` (`sys_stat`), `butun()` (float->int), `haqiqiy()` (int->float), `yoqsa` (sintaktik sinonim).
   * **Doimiy NoSQL Baza (`std/baza.algo`):** Faylga saqlanuvchi kalit-qiymat ma'lumotlar bazasi.
   * **Stek va Navbat (`std/navbat.algo`):** LIFO va FIFO xotira tuzilmalari.
   * **Bit amallari (Bitwise):** `&`, `|`, `^`, `~`, `<<`, `>>`
   * **Dinamik massivlar:** avtomatik kengayuvchi amortizatsiyalangan O(1) `massiv_qosh` (push/append), `massiv_sugur` (pop), `massiv_birlashtir`, `massiv_kesib_ol`, `massiv_mavjudmi`, `massiv_nusxa`, saralash, qidirish.
   * **Kengaytirilgan matn amallari:** `matn_bolish` (split), `matn_birlashtir_massiv` (join), `matn_almashtir` (replace), `matn_qirqish` (trim), `matn_boshlanadimi` (startswith), `matn_tugaydimi` (endswith), `matn_topish` (indexOf), `belgi_kodi` (ord/ASCII), `belgi` (chr), `matn_takrorla`, `matn_chap_toldir`, `matn_ong_toldir`.
   * **Veb Backend va REST API (`std/http.algo`):** HTTP so'rovlar marshrutizatsiyasi (Routing), metodlar (`GET`, `POST`, `PUT`, `DELETE`), tana va sarlavhalarni ajratish, JSON/HTML/Matn javoblari, status kodlari (`200 OK`, `201 Created`, `404 Not Found`, `500 Server Error`), CORS qo'llab-quvvatlash.
   * **JSON Serialization & Deserialization (`std/json.algo`):** To'liq Algo tilida yozilgan xavfsiz JSON serializer va kalit bo'yicha qiymat ajratuvchi parser.
   * **Kriptografiya va Xesh (`std/kripto.algo`):** Base64 kodlash/yechish, FNV-1a 64-bit xeshlash, XOR oqimli shifrlash, 16-lik (HEX) sanoq sistemasiga o'tkazish.
   * **Sana va Vaqt (`std/vaqt.algo`):** Unix timestamp (ms va sekund), ISO 8601 formatlash (`YYYY-MM-DD HH:MM:SS`), yuqori aniqlikdagi kutish.
   * **Fayl Tizimi (`std/fayl.algo`):** Faylni to'liq o'qish, yozish, oxiriga qo'shish (append), mavjudligini tekshirish, o'chirish va yangi papkalar ochish.

---

## 🛠️ O'rnatish va Yig'ish

Kompilyatorni yig'ish:
```bash
make
```

Barcha **31 ta avtomatlashtirilgan testlarni** sinovdan o'tkazish:
```bash
make test
# yoki
./bin/algo sinov
```

---

## 💻 CLI Buyruqlari (Algo Toolchain)

| Buyruq | Muqobili | Vazifasi |
|---|---|---|
| `./bin/algo` yoki `./bin/algo repl` | `algo repl` | Interaktiv konsol (Read-Eval-Print Loop) qobig'i |
| `./bin/algo format <fayl.algo>` | `algo fmt` | Kodni avtomatik chiroyli formatlash (`gofmt` kabi) |
| `./bin/algo doc <fayl.algo> [-o md]` | `algo hujjat` | Avtomatik hujjat generatsiyasi (`godoc` kabi) |
| `./bin/algo paket <yarat\|royxat\|ornat>` | `algo pkg` | Paket menejeri (`cargo`/`npm` kabi) |
| `./bin/algo tekshir <fayl.algo>` | `algo lint` | Kodni statik tahlil qilish (Linter) |
| `./bin/algo vaqt <fayl.algo>` | `algo time` | Kompilyatsiya va bajarilish vaqtini o'lchash |
| `./bin/algo tezlik <fayl.algo>` | `algo bench` | Benchmark va unumdorlik sinovi (ms, KB, iterations) |
| `./bin/algo yangi <nomi>` | `algo new <nomi>` | Yangi Algo loyihasini andoza bilan yaratish (scaffolding) |
| `./bin/algo ishga-tushir fayl.algo` | `algo run fayl.algo` | Kompilyatsiya qilib darhol ishga tushirish |
| `./bin/algo yig fayl.algo -o binar` | `algo build fayl.algo` | Mustaqil bajariluvchi (ELF binary) fayl yaratish |
| `./bin/algo sinov [fayl.algo]` | `algo test` | 31 ta avtomatik testni to'liq tekshirish (100% muvaffaqiyat) |
| `./bin/algo versiya` | `algo version` | Til versiyasi va arxitekturasini ko'rish |
| `./bin/algo yordam` | `algo help` | Yordam ma'lumotlarini chiqarish |

---

## 📖 To'liq Imkoniyatlar va Namunalar

### 1. Interaktiv REPL Shell
```bash
$ ./bin/algo repl
   ___   __             
  / _ | / /__ ____  ___ 
 / __ |/ / _ `/ _ \/ _ \
/_/ |_/_/\_, /\___/\___/
        /___/           
Algo Dasturlash Tili v1.4.0 (x86_64, Linux, Mustaqil)
Interaktiv REPL qobig'i. Chiqish uchun 'chiqish' deb yozing yoki Ctrl+C bosing.

algo> chiqar("Salom, Dunyo!")
Salom, Dunyo!
algo> chiqar(12 & 10, 1 << 4)
8 16
```

### 2. Veb Backend va REST API (`std/http.algo`, `std/json.algo`)
Algo tilida to'liq x86_64 mashina kodida ishlaydigan yuqori tezlikdagi backend server qurish mumkin:
```algo
yukla "std/tarmoq.algo"
yukla "std/http.algo"
yukla "std/json.algo"

amal marshrutla(sorov) {
    metod = http_metod(sorov)
    yol = http_yol(sorov)
    tana = http_tana(sorov)

    // GET /api/foydalanuvchilar
    agar yol == "/api/foydalanuvchilar" va metod == "GET" {
        json_javob = "{\"maqom\": \"muvaffaqiyatli\", \"foydalanuvchi\": \"Jasurbek\"}"
        qaytar http_javob_json(200, json_javob)
    }

    // POST /api/xabar
    agar yol == "/api/xabar" va metod == "POST" {
        qaytar http_javob_201("{\"xabar\": \"qabul qilindi\", \"hajm\": " + son_matnga(uzunlik(tana)) + "}")
    }

    qaytar http_javob_404()
}

amal asosiy() {
    server = server_yarat(8080)
    chiqar("Algo REST API Serveri 8080-portda ishlamoqda...")

    toki 1 {
        mijoz = mijoz_qabul(server)
        agar mijoz >= 0 {
            sorov = xabar_qabul(mijoz)
            agar uzunlik(sorov) > 0 {
                javob = marshrutla(sorov)
                xabar_yubor(mijoz, javob)
            }
            ulanish_yop(mijoz)
        }
    }
    qaytar 0
}
```

### 3. Kriptografiya va Xesh (`std/kripto.algo`)
```algo
yukla "std/kripto.algo"

amal asosiy() {
    // 1. Base64 kodlash va qayta yechish
    b64 = baza64_kodla("Algo - eng tezkor til!")
    chiqar("Base64:", b64) // QWxnbyAtIGVuZyB0ZXprb3IgdGlsIQ==
    chiqar("Asli:", baza64_yech(b64))

    // 2. FNV-1a 64-bit xeshlash
    xesh = xesh_fnv1a("parol_maxfiy")
    chiqar("Xesh qiymati:", xesh)

    // 3. XOR oqimli shifrlash
    shifrlangan = shifr_xor("Maxfiy xabar", "kalit123")
    chiqar("Asliga qaytish:", shifr_xor(shifrlangan, "kalit123"))

    // 4. O'n oltilik (Hexadecimal) formatlash
    chiqar("255 ning HEX shakli:", son_onolti(255)) // FF
    qaytar 0
}
```

### 4. Sana va Vaqt (`std/vaqt.algo`)
```algo
yukla "std/vaqt.algo"

amal asosiy() {
    hozir = vaqt_ms()
    chiqar("Unix vaqt (ms):", hozir)

    // ISO 8601 UTC formatida chiqarish
    chiqar("Sana va vaqt:", vaqt_format_iso(hozir)) // 2026-11-19 01:03:08
    qaytar 0
}
```

### 5. Bit Darajasidagi Amallar (Bitwise Operators)
```algo
amal asosiy() {
    a = 12 // 1100 bin
    b = 10 // 1010 bin

    chiqar("a & b =", a & b)   // 8
    chiqar("a | b =", a | b)   // 14
    chiqar("a ^ b =", a ^ b)   // 6
    chiqar("1 << 4 =", 1 << 4) // 16
    chiqar("64 >> 2 =", 64 >> 2) // 16
    chiqar("~0 =", ~0)         // -1
    qaytar 0
}
```

### 6. Dinamik Massivlar (Append, Pop, Concat, Slice)
```algo
yukla "std/massiv.algo"

amal asosiy() {
    m = [10, 20]
    m = massiv_qosh(m, 30) // append
    m = massiv_qosh(m, 40)
    chiqar("Massiv:", m)    // [10, 20, 30, 40]

    oxirgi = massiv_sugur(m) // pop
    chiqar("Sug'urilgan element:", oxirgi) // 40
    chiqar("Qolgan massiv:", m)            // [10, 20, 30]

    boshqa = [90, 100]
    birlashgan = massiv_birlashtir(m, boshqa)
    chiqar("Birlashgan:", birlashgan)       // [10, 20, 30, 90, 100]
    qaytar 0
}
```

### 7. Kengaytirilgan Matn Amallari (Split, Join, Replace, Trim)
```algo
yukla "std/matn.algo"

amal asosiy() {
    // Split va Join
    matn = "olma,anor,nok,shaftoli"
    mevalar = matn_bolish(matn, ",")
    chiqar("Bo'lingan:", mevalar) // [olma, anor, nok, shaftoli]
    
    yigilgan = matn_birlashtir_massiv(mevalar, " - ")
    chiqar("Yig'ilgan:", yigilgan) // olma - anor - nok - shaftoli

    // Qirqish (trim) va almashtirish (replace)
    tozalangan = matn_tozala("   Algo tili   \n")
    chiqar(matn_almashtirish(tozalangan, "Algo", "Super Algo"))

    // ASCII kod va belgilar
    chiqar("A kodi:", belgi_kodi("Algo", 0)) // 65
    chiqar("65 belgisi:", belgi(65))         // A
    qaytar 0
}
```

### 8. Matematika va Sonlar Nazariyasi (`std/matematika.algo`)
```algo
yukla "std/matematika.algo"

amal asosiy() {
    chiqar("EKUB(48, 18):", ekub(48, 18))       // 6
    chiqar("EKUK(12, 15):", ekuk(12, 15))       // 60
    chiqar("7! (Faktorial):", faktorial(7))     // 5040
    chiqar("29 tubmi:", tubmi(29))               // 1 (ha)
    chiqar("Pi:", pi(), "Ildiz(144):", kvadrat_ildiz(144.0))
    qaytar 0
}
```

### 9. Fayl Tizimi va Append Rejimi (`std/fayl.algo`)
```algo
yukla "std/fayl.algo"

amal asosiy() {
    faylga_yozish("kundalik.log", "1-qator: Tizim ishga tushdi\n")
    faylga_qoshish("kundalik.log", "2-qator: Foydalanuvchi kirdi\n") // append

    agar fayl_bormi("kundalik.log") == 1 {
        matn = faylni_oqish("kundalik.log")
        chiqar("Fayl tarkibi:\n" + matn)
    }
    qaytar 0
}
```

### 10. Tizim va Jarayonlar (`std/tizim.algo`)
```algo
yukla "std/tizim.algo"

amal asosiy() {
    chiqar("Joriy PID:", jarayon_raqami())
    chiqar("Foydalanuvchi:", muhit_ol("USER"))
    chiqar("CSPRNG Tasodifiy son:", tasodifiy_son(1000, 9999))
    buyruq_bajar("uname -a")
    qaytar 0
}
```

### 11. Kalit-Qiymat Ma'lumotlar Bazasi (`std/baza.algo`)
Tashqi serverlarsiz (SQLite yoki Redis kabi) doimiy diskda saqlanuvchi tezkor NoSQL baza:
```algo
yukla "std/baza.algo"

amal asosiy() {
    db = baza_och("ilovamiz.db")
    baza_qoy(db, "foydalanuvchi_1", "Jasurbek")
    baza_qoy(db, "rol", "Bosh Dasturchi")
    baza_saqla(db) // Diskka doimiy saqlash

    // Boshqa sessiyada ochib o'qish
    db2 = baza_och("ilovamiz.db")
    chiqar("Foydalanuvchi:", baza_ol(db2, "foydalanuvchi_1"))
    qaytar 0
}
```

### 12. Stek va Navbat Tuzilmalari (`std/navbat.algo`)
```algo
yukla "std/navbat.algo"

amal asosiy() {
    // Stek (LIFO)
    s = stek_yarat()
    s = stek_tiq(s, 100)
    s = stek_tiq(s, 200)
    chiqar("Stek cho'qqisi:", stek_choqqi(s)) // 200

    // Navbat (FIFO)
    q = navbat_yarat()
    q = navbat_qosh(q, "birinchi")
    q = navbat_qosh(q, "ikkinchi")
    chiqar("Navbat boshi:", navbat_boshi(q)) // birinchi
    qaytar 0
}
```

---

## 📁 Ekotizim Tuzilishi

```
algo/
├── bin/
│   ├── algo             # Asosiy CLI vositasi (Run, Build, Test, REPL, New, Format, Bench)
│   └── algoc            # Kompilyator simvollar havolasi
├── src/
│   ├── token.h / .c     # Leksik tokenlar va bitwise operatorlar
│   ├── lexer.h / .c     # UTF-8 leksik tahlilchi
│   ├── ast.h / .c       # Abstrakt Sintaktik Daraxt (AST)
│   ├── parser.h / .c    # Sintaktik tahlilchi va operatorlar ierarxiyasi
│   ├── codegen.h / .c   # x86_64 GNU Assembler generatori (Zero-libc syscalls)
│   └── main.c           # CLI boshqaruv moduli, REPL, Formatter va Benchmark
├── std/                 # Standart kutubxona
│   ├── baza.algo        # Doimiy saqlanuvchi kalit-qiymat NoSQL ma'lumotlar bazasi
│   ├── navbat.algo      # Stek (LIFO) va Navbat (FIFO) ma'lumotlar tuzilmalari
│   ├── matematika.algo  # EKUB, EKUK, faktorial, tub sonlar, sinus, kosinus, daraja
│   ├── massiv.algo      # teskari, yigindi, sarala, qidir, birlashtir, kesib_ol, nusxa
│   ├── matn.algo        # bolish, yigish, tozalash, almashtirish, indeks, belgi, kod
│   ├── json.algo        # JSON serializer va kalit bo'yicha parser
│   ├── http.algo        # HTTP va REST API backend framework (marshrutlash, parametrlar)
│   ├── kripto.algo      # Base64 kodlash/yechish, FNV-1a xesh, XOR shifr, Hex
│   ├── vaqt.algo        # Unix timestamp, ISO 8601 UTC formatlash, kutish
│   ├── fayl.algo        # oqish, yozish, qoshish (append), mavjudlik, ochirish
│   ├── tizim.algo       # muhit, jarayon_id, buyruq, tasodifiy sonlar
│   └── tarmoq.algo      # TCP server va TCP mijoz ulanishi
├── examples/            # 31 ta to'liq namunaviy va sinov dasturlari
├── editors/
│   └── vscode/          # VS Code & Antigravity IDE uchun to'liq sintaksis paketi
├── Makefile             # Avtomatlashtirilgan yig'ish va sinov ssenariysi
└── README.md            # To'liq hujjatlar va texnik ma'lumotnoma
```

---

## ⚡ Samaradorlik (Benchmark)

100,000,000 marta takrorlanuvchi sikl sinovi:
* **Python 3:** ~5.80 soniya
* **Go:** ~0.05 soniya
* **Algo (v1.4.0):** **0.04 - 0.26 soniya** (*Pythondan ~25-90+ barobar tez*)
* **Binar dastur hajmi:** **~8.9 KB** *(Go dasturlari ~2.5 MB, C++ binar ~16 KB)*
* **Xotira ishlatilishi:** ~1 MB *(Minimal direct mmap heap allocator)*
