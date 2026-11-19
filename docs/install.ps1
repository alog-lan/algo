# ==============================================================================
# Algo Dasturlash Tili - Windows Rasmiy O'rnatish Skripti (PowerShell)
# PowerShell 5.1 / PowerShell 7+ (Windows 10 / Windows 11)
# ==============================================================================
$ErrorActionPreference = "Stop"

[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

Write-Host @"
    █████╗   ██╗      ██████╗   ██████╗ 
   ██╔══██╗  ██║     ██╔════╝  ██╔═══██╗
   ███████║  ██║     ██║  ███╗ ██║   ██║
   ██╔══██║  ██║     ██║   ██║ ██║   ██║
   ██║  ██║  ███████╗╚██████╔╝ ╚██████╔╝
   ╚═╝  ╚═╝  ╚══════╝ ╚═════╝   ╚═════╝ 
   ⚡ Algo Dasturlash Tili v1.5.0 (Windows / x86_64)
   Rasmiy ombor: https://github.com/alog-lan/algo
"@ -ForegroundColor Cyan

# 1. Arxitekturani tekshirish
$arch = [System.Environment]::GetEnvironmentVariable("PROCESSOR_ARCHITECTURE")
if ($arch -ne "AMD64" -and $arch -ne "x86_64") {
    Write-Host "[XATOLIK] Algo hozircha faqat 64-bitli (x86_64 / AMD64) tizimlarni qo'llab-quvvatlaydi." -ForegroundColor Red
    exit 1
}

$InstallDir = Join-Path $HOME ".algo"
$BinDir = Join-Path $InstallDir "bin"
$StdDir = Join-Path $InstallDir "std"

if (-not (Test-Path $BinDir)) {
    New-Item -ItemType Directory -Force -Path $BinDir | Out-Null
}
if (-not (Test-Path $StdDir)) {
    New-Item -ItemType Directory -Force -Path $StdDir | Out-Null
}

Write-Host "==> Windows muhiti tekshirilmoqda..." -ForegroundColor Cyan

# 2. Windows muhiti (WSL2, Git Bash, MSYS2 yoki Standalone)
$wsl = Get-Command wsl.exe -ErrorAction SilentlyContinue

if ($wsl) {
    Write-Host "[WSL2 Aniqlandi] Algo Linux x86_64 to'g'ridan-to'g'ri WSL2 yordamida ulanadi." -ForegroundColor Green
    
    # WSL ichida install.sh ni chaqiramiz
    wsl bash -c "curl -fsSL https://raw.githubusercontent.com/alog-lan/algo/master/install.sh | bash"
    
    # Windows CMD va PowerShell uchun wrapper scriptlar yaratish
    $cmdContent = @"
@echo off
wsl ~/.local/bin/algo %*
"@
    Set-Content -Path (Join-Path $BinDir "algo.cmd") -Value $cmdContent -Encoding ASCII
    Set-Content -Path (Join-Path $BinDir "algoc.cmd") -Value $cmdContent -Encoding ASCII
} else {
    Write-Host "[Standalone Rejim] Standart kutubxonalar va manbalar yuklab olinmoqda..." -ForegroundColor Yellow
    
    $zipUrl = "https://github.com/alog-lan/algo/archive/refs/heads/master.zip"
    $tempZip = Join-Path $env:TEMP "algo-master.zip"
    $tempExt = Join-Path $env:TEMP "algo-extract"
    
    Invoke-WebRequest -Uri $zipUrl -OutFile $tempZip -UseBasicParsing
    Expand-Archive -Path $tempZip -DestinationPath $tempExt -Force
    
    Copy-Item -Path (Join-Path $tempExt "algo-master\std\*") -Destination $StdDir -Recurse -Force
    Remove-Item $tempZip -Force
    Remove-Item $tempExt -Recurse -Force
    
    $cmdContent = @"
@echo off
echo [Algo Windows Toolchain v1.5.0]
echo To'liq x86_64 ELF mashina kodini kompilyatsiya qilish uchun WSL2 tavsiya etiladi.
echo O'rnatish uchun Windows Terminalida: wsl --install
"@
    Set-Content -Path (Join-Path $BinDir "algo.cmd") -Value $cmdContent -Encoding ASCII
}

# 3. Windows PATH tizimiga qo'shish
$userPath = [System.Environment]::GetEnvironmentVariable("Path", "User")
if ($userPath -notlike "*$BinDir*") {
    [System.Environment]::SetEnvironmentVariable("Path", "$userPath;$BinDir", "User")
    $env:Path = "$env:Path;$BinDir"
    Write-Host "[MUVAFFAQIYAT] $BinDir Windows foydalanuvchi PATH o'zgaruvchisiga kiritildi!" -ForegroundColor Green
}

Write-Host @"

============================================================
✓ Algo v1.5.0 muvaffaqiyatli sozlandi!
O'rnatish katalogi: $BinDir
Sinash uchun yangi PowerShell yoki CMD oching:
  algo versiya
  algo yordam
============================================================
"@ -ForegroundColor Green
