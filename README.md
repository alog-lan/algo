# Algo Dasturlash Tili (`.algo`)

> **Python kabi oson, Go dan tez, to'g'ridan-to'g'ri x86_64 Assembler ustiga qurilgan mustaqil milliy dasturlash tili.**

---

## 🌟 Asosiy Xususiyatlari

1. **Hech qanday og'ir freymvorklarsiz (Zero Dependencies):**
   * LLVM yoki begona gigabaytlab kutubxonalarga muhtoj emas.
   * Kompilyator (`algoc`) hajmi bor-yo'g'i **~39 KB**.
2. **x86_64 Mashina Kodiga To'g'ridan-to'g'ri Kompilyatsiya:**
   * Linux yadrosi chaqiruvlari (**Direct Linux Syscalls**: `sys_write`, `sys_exit`, `sys_mmap`).
   * Hatto `libc` (standart C kutubxonasi) talab qilinmaydi! Toza mashina kodi.
   * Dastur binar hajmi bor-yo'g'i **~8 KB** (Go dasturlarining hajmi 2-10 MB bo'lgan bir paytda).
3. **Chaqmoqdek Tezlik:**
   * Og'ir Garbage Collector (axlat yig'uvchi) yo'q.
   * 100 million marta takrorlanuvchi siklni 0.2 soniyada hisoblaydi (Pythondan 25+ barobar tez).
4. **O'zbekcha Tabiiy Sintaksis:**
   * `amal`, `asosiy`, `chiqar`, `qaytar`, `agar`, `aks`, `toki`.

---

## 🚀 O'rnatish va Kompilyatsiya qilish

Kompilyatorni yig'ish:
```bash
make
```
Natijada `bin/algoc` binar fayli yaratiladi.

---

## 📝 Misollar

### 1. Salom, Dunyo (`examples/01_salom.algo`)
```algo
amal asosiy() {
    chiqar("Salom, Dunyo! Algo tiliga xush kelibsiz!")
    qaytar 0
}
```

Kompilyatsiya va ishga tushirish:
```bash
./bin/algoc examples/01_salom.algo -o salom
./salom
```

### 2. O'zgaruvchilar va Arifmetika (`examples/02_arifmetika.algo`)
```algo
amal asosiy() {
    a = 15
    b = 27
    yigindi = a + b
    kopaytma = a * b
    chiqar(yigindi)
    chiqar(kopaytma)
    qaytar 0
}
```

### 3. Shartlar va Sikllar (`examples/03_shart_va_sikl.algo`)
```algo
amal asosiy() {
    son = 1
    toki son <= 10 {
        agar son % 2 == 0 {
            chiqar(son)
        }
        son = son + 1
    }
    qaytar 0
}
```

### 4. Funksiyalar va Rekursiya (`examples/04_funksiyalar.algo`)
```algo
amal faktorial(n) {
    agar n <= 1 {
        qaytar 1
    }
    qaytar n * faktorial(n - 1)
}

amal asosiy() {
    f = faktorial(6)
    chiqar(f)
    qaytar 0
}
```

---

## 📂 Loyiha Strukturasi

* `src/token.h`, `src/token.c` — Leksik tokenlar va o'zbekcha kalit so'zlar
* `src/lexer.h`, `src/lexer.c` — UTF-8 qo'llab-quvvatlovchi leksik tahlilchi
* `src/ast.h`, `src/ast.c` — Abstrakt Sintaktik Daraxt (AST)
* `src/parser.h`, `src/parser.c` — Sintaktik tahlilchi (Recursive Descent Parser)
* `src/codegen.h`, `src/codegen.c` — x86_64 Assembler generatori (Zero-libc)
* `src/main.c` — Kompilyator CLI drayveri (`algoc`)
* `examples/` — Namunaviy dasturlar
