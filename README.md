# 🚀 Algo Dasturlash Tili (`.algo`)

<p align="center">
  <b>Python kabi oson, Go dan tez, to'g'ridan-to'g'ri x86_64 Assembler ustiga qurilgan mustaqil milliy dasturlash tili.</b>
</p>

---

## 🌟 Nega aynan Algo?

1. **Mutlaq Mustaqillik (Zero Dependencies):**
   * Hech qanday LLVM, GCC runtime yoki tashqi og'ir freymvorklarga bog'liq emas.
   * Dasturlar hatto `libc` (standart C kutubxonasi)ga ham muhtoj emas — to'g'ridan-to'g'ri Linux yadrosi tizim chaqiruvlari (**Direct Linux Syscalls**) orqali ishlaydi.
   * Kompilyator hajmi bor-yo'g'i **~43 KB**, hosil bo'ladigan mustaqil binar dasturlar hajmi esa **~8.9 KB** *(Go dasturlari 2–10 MB bo'lgan bir paytda)*.

2. **Chaqmoqdek Tezlik:**
   * Og'ir Garbage Collector (axlat yig'uvchi) yo'q.
   * To'g'ridan-to'g'ri protsessor registrlarida (`RAX`, `RBX`, `RCX`, `RDX`, `RSI`, `RDI`) hisoblashlar bajariladi.
   * 100 million marta aylanadigan siklni **0.26 soniyada** hisoblaydi (*Pythondan 25+ barobar tez*).

3. **O'zbek Tilidagi Tabiiy Sintaksis:**
   * O'rganish va kod yozish xuddi ona tilida fikrlagandek oson va ravon.

---

## 🛠️ O'rnatish va Yig'ish

Kompilyatorni yig'ish juda oddiy (hech qanday qo'shimcha paketlar talab etilmaydi):
```bash
make
```

Barcha testlarni sinovdan o'tkazish:
```bash
make test
# yoki
./bin/algo sinov
```

---

## 💻 CLI Buyruqlari (Algo Toolchain)

`algo` buyrug'i barcha kerakli qulayliklarni o'z ichiga oladi:

| Buyruq | Inglizcha muqobili | Vazifasi |
|---|---|---|
| `./bin/algo ishga-tushir fayl.algo` | `algo run fayl.algo` | Dasturni kompilyatsiya qilib darhol ishga tushirish (Python/Go kabi) |
| `./bin/algo yig fayl.algo -o binar` | `algo build fayl.algo` | Mustaqil bajariluvchi (ELF binary) fayl yaratish |
| `./bin/algo sinov` | `algo test` | Ekotizimdagi barcha testlarni avtomatik ishga tushirish |
| `./bin/algo versiya` | `algo version` | Til versiyasi va arxitekturasini ko'rish |
| `./bin/algo yordam` | `algo help` | Yordam ma'lumotlarini chiqarish |

---

## 📖 Sintaksis va Imkoniyatlar Qo'llanmasi

### 1. Salom, Dunyo va Chiqarish (`chiqar`)
```algo
amal asosiy() {
    chiqar("Salom, Dunyo! Algo tiliga xush kelibsiz!")
    
    // Bir nechta argumentlarni avtomatik bo'sh joy bilan chiqarish:
    ism = "Jasurbek"
    yosh = 22
    chiqar("Salom", ism, "! Yoshingiz:", yosh)
    
    qaytar 0
}
```

### 2. O'zgaruvchilar va Arifmetika
```algo
amal asosiy() {
    a = 15
    b = 27
    yigindi = a + b
    kopaytma = a * b
    bolinma = b / a
    qoldiq = b % a
    
    chiqar("Yig'indi:", yigindi)
    chiqar("Ko'paytma:", kopaytma)
    qaytar 0
}
```

### 3. Shartlar (`agar`, `aks`) va Mantiqiy operatorlar (`va`, `yoki`, `inkor`)
```algo
amal asosiy() {
    bal = 85
    agar bal >= 90 va bal <= 100 {
        chiqar("A'lo baho!")
    } aks agar bal >= 70 {
        chiqar("Yaxshi baho!")
    } aks {
        chiqar("Qoniqarsiz!")
    }
    qaytar 0
}
```

### 4. Takrorlash (`toki`, `uchun`, `to'xtat`, `davom`)
```algo
amal asosiy() {
    // 1 dan 10 gacha bo'lgan sonlar sikli:
    uchun i = 1; i <= 10; i = i + 1 {
        agar i == 5 {
            davom // 5-qadamni tashlab o'tish
        }
        agar i == 9 {
            toxtat // siklni to'xtatish
        }
        chiqar(i)
    }
    qaytar 0
}
```

### 5. Dinamik Massivlar (Ro'yxatlar)
```algo
amal asosiy() {
    sonlar = [10, 20, 30, 40, 50]
    chiqar("Massiv:", sonlar)
    chiqar("Uzunligi:", uzunlik(sonlar))
    
    // Indeks bo'yicha murojaat va qiymat o'zgartirish:
    sonlar[2] = 999
    chiqar("O'zgartirilgandan so'ng:", sonlar)
    qaytar 0
}
```

### 6. Matnlar bilan ishlash va Matnlarni Ulash (`+`)
```algo
amal asosiy() {
    salom = "Salom, "
    dunyo = "O'zbekiston!"
    birlashgan = salom + dunyo
    
    chiqar(birlashgan)
    chiqar("Matn uzunligi:", uzunlik(birlashgan))
    qaytar 0
}
```

### 7. Tuzilmalar (Structs: `tuzilma` va `yangi`)
```algo
tuzilma Odam {
    ism,
    yosh,
    kasb
}

amal asosiy() {
    dasturchi = yangi Odam
    dasturchi.ism = "Jasurbek"
    dasturchi.yosh = 22
    dasturchi.kasb = "Arxitektor"

    chiqar("Dasturchi ismi:", dasturchi.ism)
    chiqar("Yoshi:", dasturchi.yosh)
    qaytar 0
}
```

### 8. Funksiyalar va Rekursiya
```algo
amal faktorial(n) {
    agar n <= 1 {
        qaytar 1
    }
    qaytar n * faktorial(n - 1)
}

amal asosiy() {
    chiqar("6 faktoriali:", faktorial(6)) // 720
    qaytar 0
}
```

### 9. Fayllar bilan ishlash (File I/O)
```algo
amal asosiy() {
    nomi = "ma'lumot.txt"
    matn = "Algo tili x86_64 toza mashina kodiga kompilyatsiya qilinadi!\n"
    
    // 577 = O_WRONLY | O_CREAT | O_TRUNC
    fd = fayl_och(nomi, 577)
    agar fd > 0 {
        fayl_yoz(fd, matn)
        fayl_yop(fd)
        chiqar("Fayl muvaffaqiyatli saqlandi!")
    }
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
│   ├── token.h / .c     # Leksik tokenlar va o'zbekcha kalit so'zlar
│   ├── lexer.h / .c     # UTF-8 leksik tahlilchi
│   ├── ast.h / .c       # Abstrakt Sintaktik Daraxt (AST)
│   ├── parser.h / .c    # Sintaktik tahlilchi (Recursive Descent)
│   ├── codegen.h / .c   # x86_64 Assembler generatori (Zero-libc, Syscalls)
│   └── main.c           # CLI boshqaruv moduli
├── examples/            # 12 ta namunaviy dastur
├── editors/
│   └── vscode/          # VS Code & Antigravity IDE uchun sintaksis ranglash
├── Makefile             # Avtomatlashtirilgan yig'ish va sinov ssenariysi
└── README.md            # To'liq qo'llanma
```

---

## ⚡ Ishlash Samaradorligi (Benchmark)

100,000,000 marta takrorlanuvchi sikl sinovi:
* **Python 3:** ~5.80 soniya
* **Algo:** **0.26 soniya** (~25 barobar tezroq)
* **Xotira sarfi:** ~8 KB binar o'lchami (Go da ~2.5 MB)
