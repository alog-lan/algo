#!/usr/bin/env bash
# Algo Dasturlash Tili VS Code Kengaytmasini o'rnatish va yangilash skripti

set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TARGET_EXT="$HOME/.vscode/extensions/alog-lan.algo-lang-1.5.0"
MATERIAL_ICONS="$HOME/.vscode/extensions/pkief.material-icon-theme-5.40.0/icons"

echo "=================================================="
echo "⚡ Algo Dasturlash Tili VS Code Kengaytmasi"
echo "=================================================="

# 1. VS Code kengaytmasini o'rnatish / yangilash
mkdir -p "$TARGET_EXT"
cp -rf "$DIR"/* "$TARGET_EXT/"
rm -f "$TARGET_EXT/ornatish.sh"

echo "✅ Algo kengaytmasi yangilandi: $TARGET_EXT"

# 2. Material Icon Theme ga Algo 3D rasmiy logotipini integratsiya qilish
if [ -d "$MATERIAL_ICONS" ]; then
    cp -f "$DIR/icons/algo.svg" "$MATERIAL_ICONS/algo.svg"
    cp -f "$DIR/icons/algo_hex.svg" "$MATERIAL_ICONS/algo_hex.svg"
    echo "✅ Material Icon Theme uchun 3D Cyan Algo ikonkasi muvaffaqiyatli yangilandi!"
fi

# 3. Eski jasurbek.algo-lang-1.0.0 mavjud bo'lsa, uni ham yangilab qo'yish
OLD_EXT="$HOME/.vscode/extensions/jasurbek.algo-lang-1.0.0"
if [ -d "$OLD_EXT" ]; then
    cp -f "$DIR/icon.png" "$OLD_EXT/icon.png" 2>/dev/null || true
    cp -f "$DIR/icons/algo.png" "$OLD_EXT/icons/algo.png" 2>/dev/null || true
    cp -f "$DIR/icons/algo.svg" "$OLD_EXT/icons/algo.svg" 2>/dev/null || true
    cp -f "$DIR/icons/algo_hex.svg" "$OLD_EXT/icons/algo_hex.svg" 2>/dev/null || true
fi

# 4. OSS Code mavjud bo'lsa
if [ -d "$HOME/.vscode-oss/extensions" ]; then
    mkdir -p "$HOME/.vscode-oss/extensions/alog-lan.algo-lang-1.5.0"
    cp -rf "$DIR"/* "$HOME/.vscode-oss/extensions/alog-lan.algo-lang-1.5.0/"
fi

echo "--------------------------------------------------"
echo "🎉 O'rnatish yakunlandi!"
echo "Iltimos, VS Code oynasida [Ctrl+Shift+P] bosib:"
echo "  'Developer: Reload Window' ni tanlang."
echo "Barcha .algo fayllari va Algo kengaytmasi yangi 3D Cyan logotip bilan charaqlaydi! 🚀"
echo "=================================================="
