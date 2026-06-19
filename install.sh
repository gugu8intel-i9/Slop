#!/usr/bin/env bash
# Slop Programming Language Installer
# Installs Slop into ~/.slop and configures the system PATH

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== [Installing the Slop Programming Language] ===${NC}"

# Define paths
SLOP_DIR="$HOME/.slop"
SLOP_BIN="$SLOP_DIR/bin"
SLOP_INCLUDE="$SLOP_DIR/include"

# Create directories
mkdir -p "$SLOP_BIN"
mkdir -p "$SLOP_INCLUDE"

# Temporary build directory
TMP_DIR=$(mktemp -d)
trap 'rm -rf "$TMP_DIR"' EXIT

echo -e "${BLUE}Cloning Slop repository...${NC}"
git clone --depth 1 https://github.com/gugu8intel-i9/Slop.git "$TMP_DIR/Slop"

cd "$TMP_DIR/Slop"

echo -e "${BLUE}Compiling the self-hosting native Slop Compiler...${NC}"

# Check for Python (used for bootstrapping)
if ! command -v python3 &> /dev/null; then
    echo -e "${RED}Error: python3 is required to bootstrap Slop.${NC}"
    exit 1
fi

# Check for GCC/Clang (used to build the native binary)
if command -v gcc &> /dev/null; then
    CC="gcc"
elif command -v clang &> /dev/null; then
    CC="clang"
else
    echo -e "${RED}Error: A C/C++ compiler (gcc or clang) is required to build Slop.${NC}"
    exit 1
fi

# Bootstrap compiler.slop -> compiler.c
python3 slop_boot.py compiler.slop compiler.c

# Compile compiler.c -> native slop-compiler executable
$CC -O3 -ffast-math -flto -march=native compiler.c -o "$SLOP_BIN/slop-compiler"

# Copy runtime headers and Python helper files
cp slop_rt.h "$SLOP_INCLUDE/"
cp slop_boot.py "$SLOP_BIN/"

# Create the beautiful high-level "slop" command runner script
# This lets users type "slop run file.slop" or "slop build file.slop"
cat << 'EOF' > "$SLOP_BIN/slop"
#!/usr/bin/env bash
set -e

SLOP_DIR="$HOME/.slop"
SLOP_BIN="$SLOP_DIR/bin"
SLOP_INCLUDE="$SLOP_DIR/include"

if [ -z "$1" ]; then
    echo "Slop Programming Language Tool"
    echo "Usage: slop <command> [arguments]"
    echo ""
    echo "Commands:"
    echo "  run <file.slop>    Transpile, compile, and execute a Slop program instantly"
    echo "  build <file.slop>  Compile a Slop program into an optimized native binary"
    echo "  lex <file.slop>    Lex/tokenize a Slop program using the self-hosted compiler"
    exit 0
fi

CMD="$1"
FILE="$2"

if [ "$CMD" = "run" ]; then
    if [ -z "$FILE" ]; then echo "Error: No file specified"; exit 1; fi
    BASE="${FILE%.slop}"
    python3 "$SLOP_BIN/slop_boot.py" "$FILE" "$BASE.c"
    gcc -O3 -ffast-math -flto -march=native -I"$SLOP_INCLUDE" "$BASE.c" -o "$BASE"
    ./"$BASE"
    rm -f "$BASE.c" "$BASE"
elif [ "$CMD" = "build" ]; then
    if [ -z "$FILE" ]; then echo "Error: No file specified"; exit 1; fi
    BASE="${FILE%.slop}"
    python3 "$SLOP_BIN/slop_boot.py" "$FILE" "$BASE.c"
    gcc -O3 -ffast-math -flto -march=native -I"$SLOP_INCLUDE" "$BASE.c" -o "$BASE"
    rm -f "$BASE.c"
    echo "Successfully built native executable: $BASE"
elif [ "$CMD" = "lex" ]; then
    if [ -z "$FILE" ]; then echo "Error: No file specified"; exit 1; fi
    "$SLOP_BIN/slop-compiler" "$FILE"
else
    echo "Error: Unknown command '$CMD'"
    exit 1
fi
EOF

chmod +x "$SLOP_BIN/slop"

echo -e "${GREEN}Slop successfully installed to $SLOP_DIR!${NC}"
echo ""
echo -e "To configure Slop in your shell, add it to your PATH by running:"
echo -e "  ${BLUE}export PATH=\"\$HOME/.slop/bin:\$PATH\"${NC}"
echo ""
echo -e "You can add that line to your ${BLUE}~/.bashrc${NC} or ${BLUE}~/.zshrc${NC} to make it permanent."
echo ""
echo -e "${GREEN}Try running your first Slop program with:${NC}"
echo -e "  ${BLUE}slop run hello.slop${NC}"
