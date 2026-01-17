set -eu

# prepare
for arg in "$@"; do declare $arg='1'; done

if [ ! -v release ]; then debug=1; fi

# command types
compiler_libs='-Lvendor/raylib/src/ -lraylib -lm'
compiler_common='-std=c23 -Wall -Wextra -Wpedantic -Wno-missing-braces -Wno-unused-function -Wno-unused-value -Wno-unused-variable -Wno-unused-local-typedef -Wno-unused-but-set-variable -Wno-initializer-overrides'
compiler_debug="clang -O0 -g -DBUILD_DEBUG=1 $compiler_common"
compiler_release="clang -O2 -Werror -DBUILD_DEBUG=0 $compiler_common"

if [ -v debug ]; then compiler="$compiler_debug"; fi
if [ -v release ]; then compiler="$compiler_release"; fi

# compile
$compiler orthography.c $compiler_libs -o orthography
