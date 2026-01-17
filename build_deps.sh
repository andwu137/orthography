set -eu

# prepare
for arg in "$@"; do declare $arg='1'; done

# raylib
# if doesnt exist:
    # mkdir vendor
    # cd vendor
    # git clone --depth 1 https://github.com/raysan5/raylib.git raylib
pushd vendor/raylib/src/

if [ -v clean ]; then make clean; fi
raylib_wayland_switch=''
if [ -v wayland ]; then raylib_wayland_switch='GLFW_LINUX_ENABLE_WAYLAND=TRUE'; fi
make PLATFORM=PLATFORM_DESKTOP $raylib_wayland_switch

popd
