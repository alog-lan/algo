#!/usr/bin/env bash
# ==============================================================================
# Algo Dasturlash Tili - Universal O'rnatish Skripti (Linux, macOS, BSD)
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
echo -e "${QALIN}Algo Dasturlash Tili v1.5.0 (Ko'p Platformali O'rnatuvchi)${ODDIY}"
echo -e "${KOK}Rasmiy ombor:${ODDIY} https://github.com/alog-lan/algo\n"

# 1. Tizim va arxitektura tekshiruvi
OS="$(uname -s)"
ARCH="$(uname -m)"

echo -e "${TURKUAZ}==>${ODDIY} Operatsion tizim aniqlandi: ${QALIN}${OS} (${ARCH})${ODDIY}"

case "$OS" in
    Linux)
        echo -e "${YASHIL}✓ Linux x86_64 tizimi tasdiqlandi.${ODDIY}"
        ;;
    Darwin)
        echo -e "${YASHIL}✓ macOS tizimi aniqlandi.${ODDIY}"
        if [ "$ARCH" = "arm64" ]; then
            echo -e "${TURKUAZ}ℹ Apple Silicon (M1/M2/M3/M4) aniqlandi. Rosetta 2 va mahalliy toolchain tekshirilmoqda...${ODDIY}"
        fi
        ;;
    FreeBSD|OpenBSD|NetBSD)
        echo -e "${YASHIL}✓ BSD tizimi aniqlandi.${ODDIY}"
        ;;
    MINGW*|MSYS*|CYGWIN*)
        echo -e "${YASHIL}✓ Windows POSIX muhiti aniqlandi.${ODDIY}"
        ;;
    *)
        echo -e "${SARIQ}! Ogohlantirish: Noma'lum operatsion tizim (${OS}). Standart POSIX o'rnatish davom ettiriladi...${ODDIY}"
        ;;
esac

# 2. Asboblar tekshiruvi (make, gcc yoki clang)
echo -e "${TURKUAZ}==>${ODDIY} Tizim kompilyatorlari va asboblari tekshirilmoqda..."

COMPILER=""
if command -v gcc >/dev/null 2>&1; then
    COMPILER="gcc"
elif command -v clang >/dev/null 2>&1; then
    COMPILER="clang"
elif command -v cc >/dev/null 2>&1; then
    COMPILER="cc"
else
    echo -e "${QIZIL}Xatolik: C kompilyatori (gcc yoki clang) topilmadi.${ODDIY}"
    if [ "$OS" = "Darwin" ]; then
        echo -e "Iltimos Xcode Command Line Tools o'rnating: ${TURKUAZ}xcode-select --install${ODDIY}"
    elif [ "$OS" = "Linux" ]; then
        echo -e "Iltimos o'rnating: ${TURKUAZ}sudo apt install build-essential${ODDIY} yoki ${TURKUAZ}sudo pacman -S base-devel${ODDIY}"
    fi
    exit 1
fi

echo -e "${YASHIL}✓ Kompilyator: ${COMPILER}${ODDIY}"

# 3. O'rnatish yo'llari
INSTALL_DIR="${HOME}/.algo"
BIN_DIR="${HOME}/.local/bin"
mkdir -p "$BIN_DIR"
mkdir -p "$INSTALL_DIR"

# 4. Manba kodini kompilyatsiya qilish
if [ -f "Makefile" ] && [ -d "src" ] && [ -d "std" ]; then
    echo -e "${TURKUAZ}==>${ODDIY} Joriy papkadan kompilyatsiya qilinmoqda..."
    CC="$COMPILER" make -s clean all
    cp -r bin std "$INSTALL_DIR/"
else
    echo -e "${TURKUAZ}==>${ODDIY} GitHub rasmiy omboridan yuklab olinmoqda..."
    TMP_DIR=$(mktemp -d 2>/dev/null || mktemp -d -t 'algo_install')
    git clone --depth 1 https://github.com/alog-lan/algo.git "$TMP_DIR"
    (
        cd "$TMP_DIR"
        CC="$COMPILER" make -s clean all
        cp -r bin std "$INSTALL_DIR/"
    )
    rm -rf "$TMP_DIR"
fi

# 5. Binarni PATH ga ulash
ln -sf "${INSTALL_DIR}/bin/algo" "${BIN_DIR}/algo"
if [ -f "${INSTALL_DIR}/bin/algoc" ]; then
    ln -sf "${INSTALL_DIR}/bin/algoc" "${BIN_DIR}/algoc"
fi

echo -e "\n${YASHIL}✓ Algo v1.5.0 muvaffaqiyatli o'rnatildi!${ODDIY}"
echo -e "Binar fayl yo'li: ${TURKUAZ}${BIN_DIR}/algo${ODDIY}"
echo -e "Standart kutubxonalar: ${TURKUAZ}${INSTALL_DIR}/std${ODDIY}"

# 6. PATH tekshiruvi va avtomatik qo'shish
SHELL_NAME="$(basename "$SHELL")"
RC_FILE=""

case "$SHELL_NAME" in
    bash)
        RC_FILE="${HOME}/.bashrc"
        ;;
    zsh)
        RC_FILE="${HOME}/.zshrc"
        ;;
    fish)
        RC_FILE="${HOME}/.config/fish/config.fish"
        ;;
    *)
        RC_FILE="${HOME}/.profile"
        ;;
esac

if [[ ":$PATH:" != *":$BIN_DIR:"* ]]; then
    echo -e "\n${SARIQ}Eslatma:${ODDIY} '${BIN_DIR}' hali PATH muhitiga qo'shilmagan."
    if [ -n "$RC_FILE" ] && [ -f "$RC_FILE" ]; then
        if ! grep -q "algo" "$RC_FILE" 2>/dev/null; then
            echo -e "\n# Algo Dasturlash Tili PATH" >> "$RC_FILE"
            echo "export PATH=\"\$HOME/.local/bin:\$PATH\"" >> "$RC_FILE"
            echo -e "${YASHIL}✓ ${RC_FILE} fayliga PATH avtomatik kiritildi!${ODDIY}"
            echo -e "O'zgarish kuchga kirishi uchun yangi terminal oching yoki quyidagini bajaring:"
            echo -e "  ${TURKUAZ}source ${RC_FILE}${ODDIY}\n"
        fi
    else
        echo -e "Quyidagi qatorni terminal sozlamangizga qo'shing:"
        echo -e "  ${TURKUAZ}export PATH=\"\$HOME/.local/bin:\$PATH\"${ODDIY}\n"
    fi
fi

# 7. Versiyani tekshirish
if [ -x "${BIN_DIR}/algo" ]; then
    echo -e "${TURKUAZ}Tekshiruv:${ODDIY}"
    "${BIN_DIR}/algo" versiya || true
fi

echo -e "\n${YASHIL}Ishga tushirish uchun:${ODDIY}"
echo -e "  ${TURKUAZ}algo yordam${ODDIY}              - Barcha buyruqlar ro'yxati"
echo -e "  ${TURKUAZ}algo yangi mening_loyiham${ODDIY} - Yangi loyiha yaratish"
echo -e "  ${TURKUAZ}algo ishga-tushir fayl.algo${ODDIY} - Dasturni yurgizish\n"
