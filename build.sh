set -e

LINUX_BUILD="build-linux"
WIN_BUILD="build-mingw"

echo "==> Configuring nxsd (Linux)..."
cmake -S . -B $LINUX_BUILD -G Ninja -DCMAKE_BUILD_TYPE=Release

echo "==> Building nxsd for Linux..."
cmake --build $LINUX_BUILD --config Release

echo "==> Running unit tests for nxsd (Linux)..."
./build-linux/nxsd/tests/nxsd_tests

# echo "==> Installing nxsd (Linux)..."
# cmake --install $LINUX_BUILD --prefix release/linux/nxsd

# echo "==> Configuring nxsd (Windows)..."
# cmake -S . -B $WIN_BUILD -G Ninja \
#   -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw64.cmake \
#   -DCMAKE_BUILD_TYPE=Release

# echo "==> Building nxsd for Windows..."
# cmake --build $WIN_BUILD --config Release

# echo "==> Running unit tests for nxsd (Windows)..."
# wine build-mingw/nxsd/tests/nxsd_tests | cat



# echo "==> Bundling MinGW runtime DLLs..."

# DLLSRC="/usr/x86_64-w64-mingw32/lib"

# cp "$DLLSRC/libwinpthread-1.dll" release/windows/nxsd/bin/ || true
# cp "$DLLSRC/libgcc_s_seh-1.dll" release/windows/nxsd/bin/ || true
# cp "$DLLSRC/libstdc++-6.dll" release/windows/nxsd/bin/ || true

# echo "==> Checking formatting..."
# tools/format_check.sh

# echo "==> Running static analysis (cert)..."
# tools/lint_cert.sh build

# echo "==> Running nxsd (Windows via WINE)..."
# ./build/examples/minimal_app/project_minimal
# wine build-mingw/examples/minimal_app/project_minimal