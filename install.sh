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

echo -e "${BLUE}Installing native Slop compiler...${NC}"

# Prefer checked-in prebuilt stage0 binaries so normal installs do not need Python or GCC.
# If this platform is not prebuilt yet, fall back to the transparent bootstrap path.
OS_NAME="$(uname -s)"
ARCH_NAME="$(uname -m)"
PREBUILT_DIR=""
if [ "$OS_NAME" = "Linux" ] && { [ "$ARCH_NAME" = "x86_64" ] || [ "$ARCH_NAME" = "amd64" ]; }; then
    PREBUILT_DIR="bootstrap/prebuilt/linux-x86_64"
fi

if [ -n "$PREBUILT_DIR" ] && [ -x "$PREBUILT_DIR/slop-compiler" ] && [ -x "$PREBUILT_DIR/slop-native-backend" ] && [ -x "$PREBUILT_DIR/slop-pipeline" ]; then
    echo -e "${GREEN}Using prebuilt Slop compiler for $OS_NAME/$ARCH_NAME.${NC}"
    cp "$PREBUILT_DIR/slop-compiler" "$SLOP_BIN/slop-compiler"
    cp "$PREBUILT_DIR/slop-native-backend" "$SLOP_BIN/slop-native-backend"
    cp "$PREBUILT_DIR/slop-pipeline" "$SLOP_BIN/slop-pipeline"
    chmod +x "$SLOP_BIN/slop-compiler" "$SLOP_BIN/slop-native-backend" "$SLOP_BIN/slop-pipeline"
    if command -v sha256sum >/dev/null 2>&1 && [ -f "$PREBUILT_DIR/SHA256SUMS" ]; then
        (cd "$PREBUILT_DIR" && sha256sum -c SHA256SUMS)
    fi
else
    echo -e "${BLUE}No prebuilt compiler for $OS_NAME/$ARCH_NAME; falling back to bootstrap.${NC}"

    # Check for Python (used only for first bootstrap fallback)
    if ! command -v python3 &> /dev/null; then
        echo -e "${RED}Error: python3 is required only when no prebuilt Slop compiler exists for this platform.${NC}"
        exit 1
    fi

    # Check for GCC/Clang (used only to build the fallback native binaries)
    if command -v gcc &> /dev/null; then
        CC="gcc"
    elif command -v clang &> /dev/null; then
        CC="clang"
    else
        echo -e "${RED}Error: gcc or clang is required only when no prebuilt Slop compiler exists for this platform.${NC}"
        exit 1
    fi

    # Stage 0: one-time Python bootstrap compiler.slop -> compiler_boot.c
    python3 slop_boot.py compiler.slop compiler_boot.c
    $CC -O3 -std=gnu11 -ffast-math -flto -march=native compiler_boot.c -o "$TMP_DIR/slop-compiler-bootstrap"

    # Stage 1: native Slop compiler compiles the Slop compiler itself
    "$TMP_DIR/slop-compiler-bootstrap" compiler.slop compiler_self.c
    $CC -O3 -std=gnu11 -ffast-math -flto -march=native compiler_self.c -o "$SLOP_BIN/slop-compiler"

    # Stage 2: verify the self-hosted compiler reaches a stable C fixpoint
    "$SLOP_BIN/slop-compiler" compiler.slop compiler_self2.c
    if cmp -s compiler_self.c compiler_self2.c; then
        echo -e "${GREEN}Self-hosting fixpoint verified.${NC}"
    else
        echo -e "${RED}Error: self-hosting fixpoint check failed.${NC}"
        exit 1
    fi

    # Build the direct native backend MVP
    $CC -O3 -std=gnu11 slop_native_backend.c -o "$SLOP_BIN/slop-native-backend"
    $CC -O3 -std=gnu11 slop_pipeline_cli.c -o "$SLOP_BIN/slop-pipeline"
fi

# Copy compiler source, runtime headers, REPL, and helper files
cp compiler.slop "$SLOP_DIR/"
cp slop_rt.h "$SLOP_INCLUDE/"
cp slop_lowlevel.h "$SLOP_INCLUDE/"
cp slop_ir.h "$SLOP_INCLUDE/"
cp slop_lowering.h "$SLOP_INCLUDE/"
cp slop_ir_tools.h "$SLOP_INCLUDE/"
cp slop_sir_c_backend.h "$SLOP_INCLUDE/"
cp slop_sir_optimizer.h "$SLOP_INCLUDE/"
cp slop_elf64_x86_64.h "$SLOP_INCLUDE/"
cp slop_native_codegen.h "$SLOP_INCLUDE/"
cp slop_object_link.h "$SLOP_INCLUDE/"
cp slop_runtime_abi.h "$SLOP_INCLUDE/"
cp slop_phase4_7.h "$SLOP_INCLUDE/"
cp slop_pipeline_cli.c "$SLOP_INCLUDE/"
cp slop_pipeline.h "$SLOP_INCLUDE/"
cp slop_diagnostics.h "$SLOP_INCLUDE/"
cp slop_boot.py "$SLOP_BIN/"
cp slop_repl.py "$SLOP_BIN/"
cp slop_convert.py "$SLOP_BIN/"
cp slop_translate.py "$SLOP_BIN/"
cp slop_builder.py "$SLOP_BIN/"
cp slop_fmt.py "$SLOP_BIN/"
cp tools/slop_test.py "$SLOP_BIN/"

# Copy example/demo Slop programs so users can run them immediately
mkdir -p "$SLOP_DIR/examples"
mkdir -p "$SLOP_DIR/std"
cp -r std/* "$SLOP_DIR/std/" 2>/dev/null || true
cp hello.slop easy_start.slop low_level_demo.slop native_backend_demo.slop complex_syntax.slop parallel_processing.slop gpu_compute.slop \
   unified_parallel.slop benchmark_seq.slop benchmark_par.slop "$SLOP_DIR/examples/" 2>/dev/null || true

# Create the beautiful high-level "slop" command runner script
cat << 'EOF' > "$SLOP_BIN/slop"
#!/usr/bin/env bash
set -e

SLOP_DIR="$HOME/.slop"
SLOP_BIN="$SLOP_DIR/bin"
SLOP_INCLUDE="$SLOP_DIR/include"

SLOP_CC=""
require_c_compiler() {
    if command -v gcc >/dev/null 2>&1; then
        SLOP_CC="gcc"
    elif command -v clang >/dev/null 2>&1; then
        SLOP_CC="clang"
    else
        echo "Error: gcc or clang is required for the portable C backend. Try 'slop native <file.slop>' for the native-backend subset."
        exit 1
    fi
}

if [ -z "$1" ]; then
    echo "Slop Programming Language Tool"
    echo "Usage: slop <command> [arguments]"
    echo ""
    echo "Commands:"
    echo "  new <project_name> Initialize a new high-performance Slop project with slop.toml"
    echo "  build              Build current project using slop.toml manifest"
    echo "  run                Build and execute current project instantly"
    echo "  run <file.slop>    Transpile, compile, and execute a standalone Slop file"
    echo "  fmt <file.slop>    Automatically format Slop code to standard clean style"
    echo "  convert <file>     Automatically turn C/C++ (.h), Rust (.rs) or Python (.py) to Slop!"
    echo "  lex <file.slop>    Lex/tokenize a Slop program using the self-hosted compiler"
    echo "  native <file.slop> [target] Compile MVP native subset directly to assembly/ELF"
    echo "  emit-ir <file.slop> Emit SIR for native-backend subset"
    echo "  emit-asm <file.slop> [target] Emit target assembly for native-backend subset"
    echo "  sir-to-c <file.sir> <out.c> Compile SIR to C through production pipeline"
    echo "  sir-to-elf <file.sir> <out> Compile SIR to direct x86_64 ELF"
    echo "  targets            List native backend targets"
    echo "  test [files...]    Run Slop example/test files"
    echo "  repl               Launch the interactive native compiling REPL shell"
    exit 0
fi

CMD="$1"
FILE="$2"

if [ "$CMD" = "run" ]; then
    if [ -z "$FILE" ]; then
        # Project mode: run project via slop_builder.py
        python3 "$SLOP_BIN/slop_builder.py" "run"
    else
        # Script mode: run single file
        if [ "$FILE" = "${FILE#/}" ]; then
            FILE="$(pwd)/$FILE"
        fi
        if [ ! -f "$FILE" ]; then
            echo "Error: File not found '$FILE'"; exit 1
        fi
        BASE="${FILE%.slop}"
        "$SLOP_BIN/slop-compiler" "$FILE" "$BASE.c"
        require_c_compiler
        "$SLOP_CC" -O3 -std=gnu11 -ffast-math -flto -march=native -I"$SLOP_INCLUDE" "$BASE.c" -o "$BASE"
        "$BASE"
        rm -f "$BASE.c" "$BASE"
    fi
elif [ "$CMD" = "build" ]; then
    if [ -z "$FILE" ]; then
        # Project mode: build project via slop_builder.py
        python3 "$SLOP_BIN/slop_builder.py" "build"
    else
        # Script mode: build single file
        if [ "$FILE" = "${FILE#/}" ]; then
            FILE="$(pwd)/$FILE"
        fi
        if [ ! -f "$FILE" ]; then
            echo "Error: File not found '$FILE'"; exit 1
        fi
        BASE="${FILE%.slop}"
        "$SLOP_BIN/slop-compiler" "$FILE" "$BASE.c"
        require_c_compiler
        "$SLOP_CC" -O3 -std=gnu11 -ffast-math -flto -march=native -I"$SLOP_INCLUDE" "$BASE.c" -o "$BASE"
        rm -f "$BASE.c"
        echo "Successfully built native executable: $BASE"
    fi
elif [ "$CMD" = "new" ]; then
    if [ -z "$FILE" ]; then echo "Error: Please specify a project name"; exit 1; fi
    python3 "$SLOP_BIN/slop_builder.py" "new" "$FILE"
elif [ "$CMD" = "fmt" ]; then
    if [ -z "$FILE" ]; then echo "Error: No file specified"; exit 1; fi
    python3 "$SLOP_BIN/slop_fmt.py" "$FILE" "$3"
elif [ "$CMD" = "lex" ]; then
    if [ -z "$FILE" ]; then echo "Error: No file specified"; exit 1; fi
    "$SLOP_BIN/slop-compiler" "$FILE"
elif [ "$CMD" = "targets" ]; then
    echo "x86_64-linux"
    echo "x86_64-linux-elf"
    echo "aarch64-linux"
    echo "armv7-linux"
    echo "riscv64-linux"
elif [ "$CMD" = "emit-ir" ]; then
    if [ -z "$FILE" ]; then echo "Error: No file specified"; exit 1; fi
    if [ "$FILE" = "${FILE#/}" ]; then FILE="$(pwd)/$FILE"; fi
    BASE="${FILE%.slop}"
    "$SLOP_BIN/slop-native-backend" "$FILE" "$BASE.sir" sir
    echo "Wrote Slop IR: $BASE.sir"
elif [ "$CMD" = "emit-asm" ]; then
    if [ -z "$FILE" ]; then echo "Error: No file specified"; exit 1; fi
    if [ "$FILE" = "${FILE#/}" ]; then FILE="$(pwd)/$FILE"; fi
    BASE="${FILE%.slop}"
    TARGET="${3:-x86_64-linux}"
    "$SLOP_BIN/slop-native-backend" "$FILE" "$BASE.s" "$TARGET"
    echo "Wrote target assembly: $BASE.s"
elif [ "$CMD" = "sir-to-c" ]; then
    if [ -z "$FILE" ] || [ -z "$3" ]; then echo "Usage: slop sir-to-c <file.sir> <out.c>"; exit 1; fi
    "$SLOP_BIN/slop-pipeline" "$FILE" "$3" c
elif [ "$CMD" = "sir-to-elf" ]; then
    if [ -z "$FILE" ] || [ -z "$3" ]; then echo "Usage: slop sir-to-elf <file.sir> <out>"; exit 1; fi
    "$SLOP_BIN/slop-pipeline" "$FILE" "$3" elf x86_64-linux-elf
elif [ "$CMD" = "native" ]; then
    if [ -z "$FILE" ]; then echo "Error: No file specified"; exit 1; fi
    if [ "$FILE" = "${FILE#/}" ]; then
        FILE="$(pwd)/$FILE"
    fi
    if [ ! -f "$FILE" ]; then
        echo "Error: File not found '$FILE'"; exit 1
    fi
    BASE="${FILE%.slop}"
    TARGET="${3:-x86_64-linux}"
    if [ "$TARGET" = "sir" ] || [ "$TARGET" = "--emit-ir" ]; then
        "$SLOP_BIN/slop-native-backend" "$FILE" "$BASE.sir" sir
        echo "Wrote Slop IR: $BASE.sir"
        exit 0
    fi
    if [ "$TARGET" = "x86_64-linux-elf" ] || [ "$TARGET" = "amd64-linux-elf" ] || [ "$TARGET" = "elf64-x86_64" ]; then
        "$SLOP_BIN/slop-native-backend" "$FILE" "$BASE" "$TARGET"
        chmod +x "$BASE"
        echo "Successfully built direct native ELF executable without assembler/linker: $BASE"
        exit 0
    fi

    "$SLOP_BIN/slop-native-backend" "$FILE" "$BASE.s" "$TARGET"

    AS_CMD=""
    LD_CMD=""
    AS_FLAGS=""
    case "$TARGET" in
        x86_64-linux|amd64-linux) AS_CMD="as"; LD_CMD="ld"; AS_FLAGS="--64" ;;
        aarch64-linux|arm64-linux) AS_CMD="aarch64-linux-gnu-as"; LD_CMD="aarch64-linux-gnu-ld" ;;
        armv7-linux|arm-linux|armhf-linux) AS_CMD="arm-linux-gnueabihf-as"; LD_CMD="arm-linux-gnueabihf-ld" ;;
        riscv64-linux|rv64-linux) AS_CMD="riscv64-linux-gnu-as"; LD_CMD="riscv64-linux-gnu-ld" ;;
        *) echo "Unknown target '$TARGET'"; exit 1 ;;
    esac

    if command -v "$AS_CMD" >/dev/null 2>&1 && command -v "$LD_CMD" >/dev/null 2>&1; then
        "$AS_CMD" $AS_FLAGS "$BASE.s" -o "$BASE.o"
        "$LD_CMD" -o "$BASE" "$BASE.o"
        rm -f "$BASE.o"
        echo "Successfully built direct native executable: $BASE ($TARGET)"
    else
        echo "Wrote $TARGET assembly: $BASE.s"
        echo "Install $AS_CMD and $LD_CMD to assemble/link this target on this machine."
    fi
elif [ "$CMD" = "convert" ]; then
    if [ -z "$FILE" ]; then echo "Error: No file specified"; exit 1; fi
    python3 "$SLOP_BIN/slop_convert.py" "$FILE" "$3"
elif [ "$CMD" = "test" ]; then
    shift
    python3 "$SLOP_BIN/slop_test.py" "$@"
elif [ "$CMD" = "repl" ]; then
    python3 "$SLOP_BIN/slop_repl.py"
else
    echo "Error: Unknown command '$CMD'"
    exit 1
fi
EOF

chmod +x "$SLOP_BIN/slop"

# Automatically configure user shell PATH
SHELL_CONFIGS=("$HOME/.zshrc" "$HOME/.bashrc" "$HOME/.profile" "$HOME/.zprofile")
EXPORT_LINE="export PATH=\"\$HOME/.slop/bin:\$PATH\""
CONFIGURED=false

for CONFIG in "${SHELL_CONFIGS[@]}"; do
    if [ -f "$CONFIG" ]; then
        if ! grep -q "slop/bin" "$CONFIG"; then
            echo "" >> "$CONFIG"
            echo "# Slop Programming Language Path Configuration" >> "$CONFIG"
            echo "$EXPORT_LINE" >> "$CONFIG"
            echo -e "${BLUE}Automatically added Slop PATH configuration to $CONFIG${NC}"
        fi
        CONFIGURED=true
    fi
done

echo -e "${GREEN}Slop successfully installed to $SLOP_DIR!${NC}"
echo ""

if [ "$CONFIGURED" = true ]; then
    echo -e "${GREEN}PATH configuration has been added to your shell profiles.${NC}"
    echo -e "To start using Slop immediately in this terminal session, run:"
    echo -e "  ${BLUE}export PATH=\"\$HOME/.slop/bin:\$PATH\"${NC}"
    echo -e "Or simply reload your terminal."
else
    echo -e "Please configure Slop manually in your shell by running:"
    echo -e "  ${BLUE}export PATH=\"\$HOME/.slop/bin:\$PATH\"${NC}"
fi
echo ""
echo -e "${GREEN}Try running your first Slop program with:${NC}"
echo -e "  ${BLUE}slop run \$HOME/.slop/examples/hello.slop${NC}"
echo -e "${GREEN}Or try the new parallel compute demo:${NC}"
echo -e "  ${BLUE}slop run \$HOME/.slop/examples/unified_parallel.slop${NC}"
