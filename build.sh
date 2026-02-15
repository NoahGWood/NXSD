set -e

LINUX_BUILD="build-linux"
WIN_BUILD="build-mingw"

echo "==> Configuring NTemplate (Linux)..."
cmake -S . -B $LINUX_BUILD -G Ninja -DCMAKE_BUILD_TYPE=Release

echo "==> Building NTemplate for Linux..."
cmake --build $LINUX_BUILD --config Release

echo "==> Running unit tests for NTemplate (Linux)..."
./build-linux/NTemplate/tests/NTemplate_tests

echo "==> Running NTemplate (Linux)..."
./build-linux/NTemplate/NTemplate

# echo "==> Installing NTemplate (Linux)..."
# cmake --install $LINUX_BUILD --prefix release/linux/NTemplate

# echo "==> Configuring NTemplate (Windows)..."
# cmake -S . -B $WIN_BUILD -G Ninja \
#   -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw64.cmake \
#   -DCMAKE_BUILD_TYPE=Release

# echo "==> Building NTemplate for Windows..."
# cmake --build $WIN_BUILD --config Release

# echo "==> Running unit tests for NTemplate (Windows)..."
# wine build-mingw/NTemplate/tests/NTemplate_tests | cat



# echo "==> Bundling MinGW runtime DLLs..."

# DLLSRC="/usr/x86_64-w64-mingw32/lib"

# cp "$DLLSRC/libwinpthread-1.dll" release/windows/NTemplate/bin/ || true
# cp "$DLLSRC/libgcc_s_seh-1.dll" release/windows/NTemplate/bin/ || true
# cp "$DLLSRC/libstdc++-6.dll" release/windows/NTemplate/bin/ || true

# echo "==> Checking formatting..."
# tools/format_check.sh

# echo "==> Running static analysis (cert)..."
# tools/lint_cert.sh build

# echo "==> Running NTemplate (Windows via WINE)..."
# ./build/examples/minimal_app/project_minimal
# wine build-mingw/examples/minimal_app/project_minimal