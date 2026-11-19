#!/usr/bin/env bash
# ==============================================================================
# Algo Dasturlash Tili - Rasmiy O'rnatish Skripti (Linux x86_64)
# ==============================================================================
set -e

# Ranglar
YASHIL='\033[0;32m'
TURKUAZ='\033[0;36m'
KOK='\033[0;34m'
SARIQ='\033[1;33m'
QIZIL='\033[0;31m'
ODDIY='\033[0m'
QALIN='\033[1m'

echo -e "${TURKUAZ}"
cat << "EOF"
   ___   __             
  / _ | / /__ ____  ___ 
 / __ |/ / _ `/ _ \/ _ \
/_/ |_/_/\_, /\___/\___/
        /___/           
EOF
echo -e "${ODDIY}"
echo -e "${QALIN}Algo Dasturlash Tili v1.5.0 (Linux x86_64, Zero-Libc)${ODDIY}"
echo -e "${KOK}Rasmiy ombor:${ODDIY} https://github.com/alog-lan/algo\n"

# 1. Tizim va arxitektura tekshiruvi
OS="$(uname -s)"
ARCH="$(uname -m)"

if [ "$OS" != "Linux" ]; then
    echo -e "${QIZIL}Xatolik: Algo hozircha faqat Linux operatsion tizimini qo'llab-quvvatlaydi.${ODDIY}"
    exit 1
fi

if [ "$ARCH" != "x86_64" ]; then
    echo -e "${QIZIL}Xatolik: Algo x86_64 (amd64) mashina kodini ishlab chiqaradi.${ODDIY}"
    echo -e "Sizning arxitekturangiz: $ARCH"
    exit 1
fi

# 2. Asboblar tekshiruvi (make, gcc yoki as/ld)
echo -e "${TURKUAZ}==>${ODDIY} Tizim asboblari tekshirilmoqda..."
if ! command -v make >/dev/null 2>&1; then
    echo -e "${QIZIL}Xatolik: 'make' dasturi topilmadi. Iltimos o'rnating: sudo apt install make / sudo pacman -S make${ODDIY}"
    exit 1
fi

if ! command -v gcc >/dev/null 2>&1 && ! command -v as >/dev/null 2>&1; then
    echo -e "${QIZIL}Xatolik: 'gcc' yoki GNU Assembler (as) topilmadi.${ODDIY}"
    exit 1
fi

# 3. Vaqtinchalik papkaga klonlash yoki joriy papkadan o'rnatish
INSTALL_DIR="${HOME}/.algo"
BIN_DIR="${HOME}/.local/bin"
mkdir -p "$BIN_DIR"

if [ -f "Makefile" ] && [ -d "src" ] && [ -d "std" ]; then
    echo -e "${TURKUAZ}==>${ODDIY} Joriy loyiha papkasidan kompilyatsiya qilinmoqda..."
    make -s
    mkdir -p "$INSTALL_DIR"
    cp -r bin std "$INSTALL_DIR/"
else
    echo -e "${TURKUAZ}==>${ODDIY} GitHub omboridan yuklab olinmoqda..."
    TMP_DIR=$(mktemp -d)
    git clone --depth 1 https://github.com/alog-lan/algo.git "$TMP_DIR"
    cd "$TMP_DIR"
    make -s
    mkdir -p "$INSTALL_DIR"
    cp -r bin std "$INSTALL_DIR/"
    rm -rf "$TMP_DIR"
fi

# 4. Binarni PATH ga ulash
ln -sf "${INSTALL_DIR}/bin/algo" "${BIN_DIR}/algo"

echo -e "\n${YASHIL}✓ Algo muvaffaqiyatli o'rnatildi!${ODDIY}"
echo -e "O'rnatilgan yo'l: ${TURKUAZ}${BIN_DIR}/algo${ODDIY}"

# 5. PATH tekshiruvi
if [[ ":$PATH:" != *":$BIN_DIR:"* ]]; then
    echo -e "\n${SARIQ}Eslatma:${ODDIY} '${BIN_DIR}' sizning PATH muhitingizda mavjud emas."
    echo -e "Terminalingiz sozlama fayliga (~/.bashrc yoki ~/.zshrc) quyidagini qo'shing:"
    echo -e "${TURKUAZ}export PATH=\"\$HOME/.local/bin:\$PATH\"${ODDIY}\n"
fi

# 6. Versiyani tekshirish
"${BIN_DIR}/algo" --versiya
echo -e "\n${YASHIL}Dasturni sinash:${ODDIY} algo bajar std/matematika.algo"
