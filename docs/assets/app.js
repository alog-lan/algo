// ==============================================================================
// Algo Dasturlash Tili - Rasmiy Portal Interaktiv Dvijogi (app.js)
// ==============================================================================

// Namunaviy dasturlar bazasi
const CODE_SAMPLES = {
  salom: {
    name: "01_salom.algo",
    code: `// Algo Dasturlash Tili - Salom Dunyo
amal asosiy() {
    chiqar("Salom, Dunyo!")
    chiqar("Algo dasturlash tili - x86_64 mashina kodida!")
    
    // Matematik hisob-kitob
    son1 = 42
    son2 = 58
    yigindi = son1 + son2
    chiqar("Natija:", yigindi)
    
    qaytar 0
}`,
    output: `[Kompilyatsiya: x86_64 ELF direct syscalls]
Salom, Dunyo!
Algo dasturlash tili - x86_64 mashina kodida!
Natija: 100

----------------------------------------
✓ Jarayon muvaffaqiyatli yakunlandi!
Vaqt: 0.61 ms | Binar hajmi: 8.9 KB | Xotira: 1.2 MB`
  },

  veb: {
    name: "veb_server.algo",
    code: `// Algo Tilida To'liq Mustaqil HTTP Backend Server
yukla "std/http.algo"
yukla "std/tarmoq.algo"

amal asosiy() {
    port = 8080
    chiqar("Algo HTTP Veb-serveri ishga tushmoqda...")
    server_fd = server_yarat(port)
    
    // REST API Marshrutlash
    sorov_yoli = "/api/salomatlik"
    agar sorov_yoli == "/api/salomatlik" {
        javob = http_javob_json(200, "{\\"holat\\": \\"faol\\", \\"dvijok\\": \\"x86_64\\"}")
        chiqar("Mijozga javob yuborildi: 200 OK")
    }
    
    qaytar 0
}`,
    output: `Algo HTTP Veb-serveri ishga tushmoqda...
Server muvaffaqiyatli ishga tushdi, port: 8080
Server 8080-portda ulanishlarni tinglashga tayyor!
Mijozga javob yuborildi: 200 OK
HTTP/1.1 200 OK
Content-Type: application/json; charset=utf-8
Content-Length: 38
{"holat": "faol", "dvijok": "x86_64"}

----------------------------------------
✓ 100% Zero-Libc (sys_socket: 41, sys_bind: 49)
Vaqt: 0.74 ms | Rejasi: Non-blocking Event Loop`
  },

  jarayon: {
    name: "jarayonlar.algo",
    code: `// Ko'p Jarayonli Parallellik (Multi-Process Concurrency)
amal asosiy() {
    chiqar("Asosiy boshqaruvchi jarayon:", tizim_pid())
    
    // Tizim darajasida yangi parallel jarayon (sys_fork)
    bolalar_pid = jarayon_yarat()
    
    agar bolalar_pid == 0 {
        chiqar("[Yordamchi Jarayon] Og'ir hisoblash boshlandi...")
        kutish(50)
        chiqar("[Yordamchi Jarayon] Hisob-kitob yakunlandi!")
        chiqish(0)
    } aks {
        chiqar("[Asosiy Jarayon] Bolalar jarayoni kutilmoqda...")
        status = jarayon_kut(bolalar_pid)
        chiqar("[Asosiy Jarayon] Barcha vazifalar bajarildi!")
    }
    
    qaytar 0
}`,
    output: `Asosiy boshqaruvchi jarayon: 41208
[Asosiy Jarayon] Bolalar jarayoni kutilmoqda...
[Yordamchi Jarayon] Og'ir hisoblash boshlandi...
[Yordamchi Jarayon] Hisob-kitob yakunlandi!
[Asosiy Jarayon] Barcha vazifalar bajarildi!

----------------------------------------
✓ Hardware Parallelizm (sys_fork 57, sys_wait4 61)
Vaqt: 1.12 ms | Xotira izolyatsiyasi: 100% Copy-on-Write`
  },

  baza: {
    name: "kv_baza.algo",
    code: `// Tezkor Kalit-Qiymat Ma'lumotlar Bazasi
yukla "std/baza.algo"

amal asosiy() {
    baza_fayl = "foydalanuvchilar.db"
    baza = baza_och(baza_fayl)
    
    // Ma'lumotlarni yozish
    baza_yoz(baza, "foydalanuvchi_1", "Jasurbek")
    baza_yoz(baza, "rol", "Bosh Tizim Arxitektori")
    baza_saqla(baza, baza_fayl)
    
    // Qayta o'qish
    chiqar("Baza muvaffaqiyatli saqlandi!")
    chiqar("Foydalanuvchi:", baza_oqish(baza, "foydalanuvchi_1"))
    chiqar("Lavozim:", baza_oqish(baza, "rol"))
    
    qaytar 0
}`,
    output: `Baza muvaffaqiyatli saqlandi!
Foydalanuvchi: Jasurbek
Lavozim: Bosh Tizim Arxitektori

----------------------------------------
✓ O'rnatilgan KV Dasturiy Baza
Bajarilish: 0.82 ms | I/O: To'g'ridan-to'g'ri x86_64 disk syscall`
  },

  matematika: {
    name: "matematika.algo",
    code: `// Ilmiy Matematika va SSE2 Uskunaviy Tezlashtirish
yukla "std/matematika.algo"

amal asosiy() {
    // SSE Uskunaviy kvadrat ildiz
    ildiz_144 = ildiz(144.0)
    chiqar("ildiz(144.0) =", ildiz_144)
    
    // Darajaga ko'tarish va Yaxlitlash
    chiqar("2^16 darajasi =", daraja(2, 16))
    chiqar("yaxlitla(19.75) =", yaxlitla(19.75))
    
    // EKUB va EKUK
    chiqar("EKUB(48, 18) =", ekub(48, 18))
    chiqar("EKUK(12, 15) =", ekuk(12, 15))
    
    qaytar 0
}`,
    output: `ildiz(144.0) = 12.0
2^16 darajasi = 65536
yaxlitla(19.75) = 20
EKUB(48, 18) = 6
EKUK(12, 15) = 60

----------------------------------------
✓ SSE2 sqrtsd / ucomisd Mashina Ko'rsatmalari
Vaqt: 0.54 ms | Aniqlik: 64-bit IEEE 754 Floating Point`
  }
};

let currentTab = 'salom';

// Tab almashtirish
function switchTab(tabKey) {
  currentTab = tabKey;
  
  // Tab tugmalarini yangilash
  document.querySelectorAll('.tab-btn').forEach(btn => {
    btn.classList.toggle('active', btn.dataset.tab === tabKey);
  });
  
  // Muharrir matnini o'rnatish
  const editor = document.getElementById('code-editor');
  if (editor && CODE_SAMPLES[tabKey]) {
    editor.value = CODE_SAMPLES[tabKey].code;
  }
  
  // Chiqish panelini yangilash
  const terminal = document.getElementById('terminal-output');
  if (terminal && CODE_SAMPLES[tabKey]) {
    terminal.innerHTML = formatOutput(CODE_SAMPLES[tabKey].output);
  }
}

// Chiqishni chiroyli formatlash
function formatOutput(text) {
  return text
    .replace(/^✓ (.*)$/gm, '<span class="success">✓ $1</span>')
    .replace(/^(Salom.*|Natija.*|Server.*|Foydalanuvchi.*|ildiz.*)$/gm, '<span class="highlight">$1</span>')
    .replace(/^(Vaqt: .*)$/gm, '<span class="time-badge">$1</span>');
}

// Simulyatsiya qilingan komplyator ishga tushirish
function runCode() {
  const terminal = document.getElementById('terminal-output');
  const runBtn = document.getElementById('run-btn');
  const editor = document.getElementById('code-editor');
  
  if (!terminal || !runBtn || !editor) return;
  
  runBtn.innerHTML = '<span>⚡</span> Bajarilmoqda...';
  runBtn.style.opacity = '0.7';
  runBtn.disabled = true;
  
  terminal.innerHTML = '<span style="color: #94a3b8;">[Kompilyatsiya qilinmoqda: x86_64 GNU Assembler / Direct Syscalls...]</span>';
  
  setTimeout(() => {
    const randomMs = (0.55 + Math.random() * 0.3).toFixed(2);
    let sampleOut = CODE_SAMPLES[currentTab] ? CODE_SAMPLES[currentTab].output : '';
    sampleOut = sampleOut.replace(/Vaqt: [\d.]+ ms/, `Vaqt: ${randomMs} ms`);
    
    terminal.innerHTML = formatOutput(sampleOut);
    
    runBtn.innerHTML = '<span>▶</span> Ishga tushirish';
    runBtn.style.opacity = '1';
    runBtn.disabled = false;
  }, 320);
}

// Buyruqdan nusxa olish
function copyCommand(btn, text) {
  navigator.clipboard.writeText(text).then(() => {
    const originalText = btn.innerHTML;
    btn.innerHTML = '<span style="color: #10b981;">✓</span> Nusxa olindi!';
    btn.style.borderColor = '#10b981';
    
    setTimeout(() => {
      btn.innerHTML = originalText;
      btn.style.borderColor = '';
    }, 2000);
  });
}

// Kutubxonalarni qidirish/filtrlash
function filterLibraries(query) {
  const q = query.toLowerCase().trim();
  const cards = document.querySelectorAll('.std-card');
  
  cards.forEach(card => {
    const name = card.querySelector('.std-name').innerText.toLowerCase();
    const desc = card.querySelector('.std-desc').innerText.toLowerCase();
    
    if (name.includes(q) || desc.includes(q)) {
      card.style.display = 'block';
    } else {
      card.style.display = 'none';
    }
  });
}

// Sahifa yuklanganda ishga tushirish
document.addEventListener('DOMContentLoaded', () => {
  switchTab('salom');
  
  const editor = document.getElementById('code-editor');
  if (editor) {
    editor.addEventListener('keydown', (e) => {
      // Tab tugmasini qo'llab-quvvatlash
      if (e.key === 'Tab') {
        e.preventDefault();
        const start = editor.selectionStart;
        const end = editor.selectionEnd;
        editor.value = editor.value.substring(0, start) + "    " + editor.value.substring(end);
        editor.selectionStart = editor.selectionEnd = start + 4;
      }
      // Ctrl + Enter orqali kodni ishga tushirish
      if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
        e.preventDefault();
        runCode();
      }
    });
  }
});
