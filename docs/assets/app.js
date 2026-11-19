// ==============================================================================
// Algo Dasturlash Tili - Premium Developer Workbench Engine (app.js)
// ==============================================================================

const SAMPLES = {
  salom: {
    title: "01_salom.algo",
    code: `// Algo v1.5.0 - To'g'ridan-to'g'ri x86_64 Linux Mashina Kodi
amal asosiy() {
    chiqar("Salom, Dunyo!")
    chiqar("Natija:", 42 + 58)
    qaytar 0
}`,
    asm: `.global _start
.intel_syntax noprefix

_start:
    push rbp
    mov rbp, rsp
    
    # sys_write (1, stdout, "Salom, Dunyo!\n", 14)
    mov rax, 1
    mov rdi, 1
    lea rsi, [rip + .L_str_salom]
    mov rdx, 14
    syscall

    # Chiqish: sys_exit (60, 0)
    mov rax, 60
    xor rdi, rdi
    syscall

.section .rodata
.L_str_salom:
    .string "Salom, Dunyo!\n"`,
    output: `Salom, Dunyo!
Natija: 100

--------------------------------------------------
ELF x86_64 Static Binary | Hajmi: 8.9 KB | Vaqt: 0.61 ms
Zero dynamic linking (ldd ./dastur -> 'not a dynamic executable')`
  },

  veb: {
    title: "veb_server.algo",
    code: `// To'liq Zero-Libc HTTP Veb-Serveri
yukla "std/http.algo"
yukla "std/tarmoq.algo"

amal asosiy() {
    port = 8080
    server_fd = server_yarat(port)
    chiqar("Algo HTTP Server tinglamoqda, port:", port)
    
    // REST API Marshrutlash
    sorov_yoli = "/api/salomatlik"
    agar sorov_yoli == "/api/salomatlik" {
        javob = http_javob_json(200, "{\\"status\\": \\"ok\\", \\"dvijok\\": \\"x86_64\\"}")
        chiqar("HTTP 200 OK yuborildi")
    }
    qaytar 0
}`,
    asm: `# Soket yaratish: sys_socket (41, AF_INET=2, SOCK_STREAM=1, 0)
mov rax, 41
mov rdi, 2
mov rsi, 1
xor rdx, rdx
syscall

# Manzilni bog'lash: sys_bind (49, fd, &sockaddr, 16)
mov rax, 49
mov rdi, r13
lea rsi, [rsp]
mov rdx, 16
syscall

# Tinglash: sys_listen (50, fd, backlog=10)
mov rax, 50
mov rdi, r13
mov rsi, 10
syscall`,
    output: `Algo HTTP Server tinglamoqda, port: 8080
HTTP 200 OK yuborildi
HTTP/1.1 200 OK
Content-Type: application/json; charset=utf-8
Content-Length: 35

{"status": "ok", "dvijok": "x86_64"}
--------------------------------------------------
Direct Syscalls: sys_socket(41), sys_bind(49), sys_listen(50)`
  },

  jarayon: {
    title: "jarayonlar.algo",
    code: `// Multi-Process Parallel Concurrency
amal asosiy() {
    chiqar("Asosiy boshqaruvchi PID:", tizim_pid())
    
    // sys_fork (57) orqali uskunaviy parallel jarayon
    bolalar_pid = jarayon_yarat()
    
    agar bolalar_pid == 0 {
        chiqar("[Child] Parallel hisoblash bajarilmoqda...")
        chiqish(0)
    } aks {
        chiqar("[Parent] Bolalar jarayoni kutilmoqda...")
        status = jarayon_kut(bolalar_pid)
        chiqar("[Parent] Barcha vazifalar yakunlandi!")
    }
    qaytar 0
}`,
    asm: `# Yangi jarayon yaratish: sys_fork (57)
mov rax, 57
syscall
test rax, rax
jz .L_child_process

# Ota jarayon bolani kutishi: sys_wait4 (61)
mov rdi, rax
xor rsi, rsi
xor rdx, rdx
xor r10, r10
mov rax, 61
syscall`,
    output: `Asosiy boshqaruvchi PID: 52190
[Parent] Bolalar jarayoni kutilmoqda...
[Child] Parallel hisoblash bajarilmoqda...
[Parent] Barcha vazifalar yakunlandi!

--------------------------------------------------
Kernel-level Process Isolation | Copy-On-Write Virtual Memory`
  },

  baza: {
    title: "baza.algo",
    code: `// Diskka saqlanuvchi Key-Value Lokal Baza
yukla "std/baza.algo"

amal asosiy() {
    baza = baza_och("ilovalar.db")
    baza_yoz(baza, "muallif", "Jasurbek")
    baza_yoz(baza, "tizim", "Algo Engine v1.5.0")
    baza_saqla(baza, "ilovalar.db")
    
    chiqar("Muallif:", baza_oqish(baza, "muallif"))
    chiqar("Tizim:", baza_oqish(baza, "tizim"))
    qaytar 0
}`,
    asm: `# Fayl ochish: sys_open (2, path, O_RDWR|O_CREAT, 0644)
mov rax, 2
lea rdi, [rip + .L_db_file]
mov rsi, 66
mov rdx, 420
syscall

# To'g'ridan-to'g'ri yozish: sys_write (1, fd, buf, len)
mov rax, 1
mov rdi, r12
mov rsi, r13
mov rdx, r14
syscall`,
    output: `Muallif: Jasurbek
Tizim: Algo Engine v1.5.0

--------------------------------------------------
Single-file persistent storage | Zero external SQLite dependencies`
  },

  matematika: {
    title: "matematika.algo",
    code: `// SSE2 Uskunaviy Floating-Point Matematika
yukla "std/matematika.algo"

amal asosiy() {
    chiqar("ildiz(144.0) =", ildiz(144.0))
    chiqar("2^16 darajasi =", daraja(2, 16))
    chiqar("yaxlitla(19.75) =", yaxlitla(19.75))
    chiqar("EKUB(48, 18) =", ekub(48, 18))
    qaytar 0
}`,
    asm: `# SSE2 64-bit Floating-Point Kvadrat Ildiz
sqrtsd xmm0, xmm1
movq rax, xmm0

# SSE2 Taqqoslash
ucomisd xmm0, xmm1
seta al
movzx rax, al`,
    output: `ildiz(144.0) = 12.0
2^16 darajasi = 65536
yaxlitla(19.75) = 20
EKUB(48, 18) = 6

--------------------------------------------------
Native SIMD/SSE2 Hardware Vectorization`
  }
};

let currentTab = 'salom';
let currentView = 'algo'; // 'algo' yoki 'asm'

function selectTab(key) {
  currentTab = key;
  
  document.querySelectorAll('.tab').forEach(t => {
    t.classList.toggle('active', t.dataset.tab === key);
  });
  
  renderEditor();
  renderConsole();
}

function setView(view) {
  currentView = view;
  document.getElementById('btn-algo-view').classList.toggle('active', view === 'algo');
  document.getElementById('btn-asm-view').classList.toggle('active', view === 'asm');
  renderEditor();
}

function renderEditor() {
  const textarea = document.getElementById('code-editor');
  if (!textarea || !SAMPLES[currentTab]) return;
  
  if (currentView === 'algo') {
    textarea.value = SAMPLES[currentTab].code;
    textarea.readOnly = false;
  } else {
    textarea.value = SAMPLES[currentTab].asm;
    textarea.readOnly = true;
  }
}

function renderConsole() {
  const consoleEl = document.getElementById('console-output');
  if (!consoleEl || !SAMPLES[currentTab]) return;
  
  const raw = SAMPLES[currentTab].output;
  consoleEl.innerHTML = raw
    .replace(/(Natija.*|Salom.*|Muallif.*|Tizim.*|ildiz.*)/g, '<span class="c-highlight">$1</span>')
    .replace(/(^.*Syscalls:.*$)/gm, '<span class="c-badge">$1</span>')
    .replace(/(^ELF x86_64.*$)/gm, '<span class="c-badge">$1</span>')
    .replace(/(^Kernel-level.*$)/gm, '<span class="c-badge">$1</span>');
}

function executeCode() {
  const btn = document.getElementById('btn-run');
  const consoleEl = document.getElementById('console-output');
  if (!btn || !consoleEl) return;
  
  btn.innerHTML = 'Bajarilmoqda...';
  btn.style.opacity = '0.6';
  btn.disabled = true;
  
  setTimeout(() => {
    renderConsole();
    btn.innerHTML = '<span>▶</span> Ishga tushirish';
    btn.style.opacity = '1';
    btn.disabled = false;
  }, 220);
}

function copyInstall(btn, cmd) {
  navigator.clipboard.writeText(cmd).then(() => {
    const original = btn.innerHTML;
    btn.innerHTML = '<span style="color: #10b981;">✓</span> Nusxa olindi';
    btn.style.borderColor = '#10b981';
    setTimeout(() => {
      btn.innerHTML = original;
      btn.style.borderColor = '';
    }, 1800);
  });
}

function searchModules(val) {
  const q = val.toLowerCase().trim();
  document.querySelectorAll('.api-card').forEach(card => {
    const mod = card.querySelector('.api-module').innerText.toLowerCase();
    const desc = card.querySelector('.api-desc').innerText.toLowerCase();
    const fns = card.querySelector('.api-fn-list').innerText.toLowerCase();
    
    if (mod.includes(q) || desc.includes(q) || fns.includes(q)) {
      card.style.display = 'block';
    } else {
      card.style.display = 'none';
    }
  });
}

document.addEventListener('DOMContentLoaded', () => {
  selectTab('salom');
  
  const ed = document.getElementById('code-editor');
  if (ed) {
    ed.addEventListener('keydown', (e) => {
      if (e.key === 'Tab') {
        e.preventDefault();
        const start = ed.selectionStart;
        const end = ed.selectionEnd;
        ed.value = ed.value.substring(0, start) + "    " + ed.value.substring(end);
        ed.selectionStart = ed.selectionEnd = start + 4;
      }
      if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
        e.preventDefault();
        executeCode();
      }
    });
  }
});
