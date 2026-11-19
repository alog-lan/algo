# 🚀 Algo Dasturlash Tili (`.algo`)

<p align="center">
  <b>Python kabi oson, Go dan tez, to'g'ridan-to'g'ri x86_64 Assembler ustiga qurilgan mustaqil milliy dasturlash tili.</b>
</p>

---

## 🌟 Nega aynan Algo?

1. **Mutlaq Mustaqillik (Zero Dependencies):**
   * Hech qanday LLVM, GCC runtime yoki tashqi og'ir freymvorklarga bog'liq emas.
   * Dasturlar hatto `libc` (standart C kutubxonasi)ga ham muhtoj emas — to'g'ridan-to'g'ri Linux yadrosi tizim chaqiruvlari (**Direct Linux Syscalls**) orqali ishlaydi.
   * Kompilyator hajmi bor-yo'g'i **~45 KB**, hosil bo'ladigan mustaqil binar dasturlar hajmi esa **~8.9 KB** *(Go dasturlari 2–10 MB bo'lgan bir paytda)*.

2. **Chaqmoqdek Tezlik:**
   * Og'ir Garbage Collector (axlat yig'uvchi) to'xtashlari yo'q.
   * To'g'ridan-to'g'ri protsessor registrlarida (`RAX`, `RBX`, `RCX`, `RDX`, `RSI`, `RDI`) hisoblashlar bajariladi.
   * 100 million marta aylanadigan siklni **0.26 soniyada** hisoblaydi (*Pythondan 25+ barobar tez*).

3. **To'liq Standart Ekotizim (v1.2.0):**
   * Dinamik massivlar, dinamik matnlar, lug'at (Hashmap), modullar (`yukla`), vaqt va kutish, xatoliklar va tasdiqlash (`tasdiqla`), buyruqlar qatori argumentlari (`argumentlar()`), apparat darajasidagi o'nlik sonlar (SSE Float), to'liq jonli TCP/HTTP veb serverlari, fayl tizimi va tuzilmalar (`tuzilma`).

---

## 🛠️ O'rnatish va Yig'ish

Kompilyatorni yig'ish:
```bash
make
```

Barcha 21 ta avtomatlashtirilgan testlarni sinovdan o'tkazish:
```bash
make test
# yoki
./bin/algo sinov
```

---

## 💻 CLI Buyruqlari (Algo Toolchain)

| Buyruq | Muqobili | Vazifasi |
|---|---|---|
| `./bin/algo ishga-tushir fayl.algo` | `algo run fayl.algo` | Kompilyatsiya qilib darhol ishga tushirish (Python/Go kabi) |
| `./bin/algo yig fayl.algo -o binar` | `algo build fayl.algo` | Mustaqil bajariluvchi (ELF binary) fayl yaratish |
| `./bin/algo sinov` | `algo test` | 21 ta avtomatik testni to'liq tekshirish |
| `./bin/algo versiya` | `algo version` | Til versiyasi va arxitekturasini ko'rish |
| `./bin/algo yordam` | `algo help` | Yordam ma'lumotlarini chiqarish |

---

## 📖 To'liq Imkoniyatlar Qo'llanmasi

### 1. Salom, Dunyo va Chiqarish (`chiqar`)
```algo
amal asosiy() {
    chiqar("Salom, Dunyo! Algo tiliga xush kelibsiz!")
    
    ism = "Jasurbek"
    yosh = 22
    chiqar("Salom", ism, "! Yoshingiz:", yosh)
    qaytar 0
}
```

### 2. Haqiqiy (o'nlik) Sonlar (SSE Hardware Floats)
```algo
amal asosiy() {
    pi = 3.1415
    e = 2.7182
    chiqar("Pi + E:", pi + e)
    chiqar("Pi * 2.0:", pi * 2.0)
    chiqar("10.0 / 4.0:", 10.0 / 4.0) // 2.5
    qaytar 0
}
```

### 3. Dinamik Massivlar (Ro'yxatlar)
```algo
amal asosiy() {
    sonlar = [10, 20, 30, 40, 50]
    chiqar("Massiv:", sonlar)
    chiqar("Uzunligi:", uzunlik(sonlar))
    
    sonlar[2] = 999
    chiqar("O'zgartirilgandan so'ng:", sonlar)
    qaytar 0
}
```

### 4. Lug'at (Hashmap / Dictionary)
```algo
amal asosiy() {
    lugat = lugat_yarat()
    lugat["shahar"] = "Toshkent"
    lugat["aholi"] = 3000000

    chiqar("Poytaxt:", lugat["shahar"])
    chiqar("Aholi:", lugat["aholi"])

    agar lugat_bormi(lugat, "shahar") {
        chiqar("'shahar' kaliti mavjud!")
    }
    qaytar 0
}
```

### 5. Buyruqlar Qatori Argumentlari (`argumentlar()`)
```algo
amal asosiy() {
    args = argumentlar()
    chiqar("Argumentlar soni:", uzunlik(args))
    uchun i = 0; i < uzunlik(args); i = i + 1 {
        chiqar("Arg", i, ":", args[i])
    }
    qaytar 0
}
```

### 6. Xatoliklarni Tekshirish va Tasdiqlash (`tasdiqla`)
```algo
amal asosiy() {
    yosh = 20
    tasdiqla(yosh >= 18, "Yosh 18 dan kichik bo'lmasligi kerak!")
    chiqar("Tasdiq muvaffaqiyatli o'tdi!")
    qaytar 0
}
```

### 7. Modullar Tizimi (`yukla`)
```algo
yukla "std/matematika.algo"

amal asosiy() {
    m = modul(-50)
    dar = daraja(2, 10)
    chiqar("Modul:", m)       // 50
    chiqar("2^10:", dar)      // 1024
    qaytar 0
}
```

### 8. Vaqt va Kutish (`vaqt`, `kutish`)
```algo
amal asosiy() {
    boshlandi = vaqt()
    kutish(200) // 200 millisoniya kechiktirish
    tugadi = vaqt()
    chiqar("O'tgan vaqt (ms):", tugadi - boshlandi)
    qaytar 0
}
```

### 9. Jonli HTTP Veb Server (Networking)
```algo
amal asosiy() {
    server_fd = tarmoq_server_och(9090)
    agar server_fd > 0 {
        chiqar("HTTP server 9090-portda ishga tushdi...")
        mijoz_fd = tarmoq_mijoz_qabul(server_fd)
        agar mijoz_fd > 0 {
            javob = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Salom! Algo veb-serveri!</h1>"
            tarmoq_yubor(mijoz_fd, javob)
            tarmoq_yop(mijoz_fd)
        }
        tarmoq_yop(server_fd)
    }
    qaytar 0
}
```

### 10. Tuzilmalar (Structs: `tuzilma` va `yangi`)
```algo
tuzilma Foydalanuvchi {
    ism,
    yosh,
    kasb
}

amal asosiy() {
    user = yangi Foydalanuvchi
    user.ism = "Jasurbek"
    user.yosh = 22
    user.kasb = "Arxitektor"

    chiqar("Foydalanuvchi:", user.ism, "Kasbi:", user.kasb)
    qaytar 0
}
```

---

## 📁 Ekotizim Strukturasi

```
algo/
├── bin/
│   ├── algo             # Asosiy CLI vositasi (Run, Build, Test, Help)
│   └── algoc            # Kompilyator
├── src/
│   ├── token.h / .c     # Leksik tokenlar va kalit so'zlar
│   ├── lexer.h / .c     # UTF-8 leksik tahlilchi
│   ├── ast.h / .c       # Abstrakt Sintaktik Daraxt (AST)
│   ├── parser.h / .c    # Sintaktik tahlilchi va Modul yuklovchi
│   ├── codegen.h / .c   # x86_64 Assembler generatori (Zero-libc, SSE, Syscalls)
│   └── main.c           # CLI boshqaruv moduli
├── std/                 # Standart kutubxona
│   ├── matematika.algo  # Matematik amallar (modul, daraja, max, min)
│   ├── massiv.algo      # Massiv amallari (teskari, yig'indi, max)
│   └── tarmoq.algo      # Tarmoq amallari (server_yarat, server_yop)
├── examples/            # 22 ta namunaviy dastur
├── editors/
│   └── vscode/          # VS Code & Antigravity IDE uchun sintaksis paketi
├── Makefile             # Avtomatlashtirilgan yig'ish va sinov ssenariysi
└── README.md            # To'liq hujjatlar
```

---

## ⚡ Samaradorlik (Benchmark)

100,000,000 marta takrorlanuvchi sikl sinovi:
* **Python 3:** ~5.80 soniya
* **Algo:** **0.26 soniya** (~25 barobar tezroq)
* **Binar dastur hajmi:** ~8.9 KB (Go da ~2.5 MB)
