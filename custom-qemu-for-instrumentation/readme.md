#custom-qemu-for-instrumentation

If you need a fast way to instrument user/kernel/hypervisor then you have custom-qemu-for-instrumentation as a fast and light option.

## Build 
The build instructions come from : (https://stackoverflow.com/questions/53084815/compile-qemu-under-windows-10-64-bit-for-windows-10-64-bit)

Here's a complete step-by-step guide for compiling qemu-system-x86\_64.exe:


OS: Microsoft Windows 10 Home 64-bit

Guide based on: [https://wiki.qemu.org/Hosts/W32#Native\_builds\_with\_MSYS2](https://wiki.qemu.org/Hosts/W32#Native_builds_with_MSYS2)

*   Download and install msys2 to C:\\msys64: [http://repo.msys2.org/distrib/x86\_64/msys2-x86\_64-20180531.exe](http://repo.msys2.org/distrib/x86_64/msys2-x86_64-20180531.exe)
*   Start C:\\msys64\\mingw64.exe
*   Updates (then close window and restart mingw64.exe): pacman -Syu
*   Updates: pacman -Su
*   Install basic packets: pacman -S base-devel mingw-w64-x86\_64-toolchain git python
*   Install QEMU specific packets: pacman -S mingw-w64-x86\_64-glib2 mingw-w64-x86\_64-gtk3 mingw-w64-x86\_64-SDL2
*   Get QEMU sources:
    *   git clone git://git.qemu-project.org/qemu.git
    *   cd qemu
    *   git submodule update --init ui/keycodemapdb
    *   git submodule update --init capstone
    *   git submodule update --init dtc
*   Insert void \_\_stack\_chk\_fail(void); void \_\_stack\_chk\_fail(void) { } to qemu\\util\\oslib-win32.c e.g. at line 44
*   Comment out (#) Capstone (line 508) in qemu\\Makefile
*   Build QEMU:
    *   ./configure --enable-gtk --enable-sdl --target-list=x86\_64-softmmu --disable-werror --disable-stack-protector
    *   make
*   Run in qemu/x86\_64-softmmu ./qemu-system-x86\_64 -L ./../pc-bios
*   Optional (for better performance): Install HAXM according to this guide: [https://www.qemu.org/2017/11/22/haxm-usage-windows/](https://www.qemu.org/2017/11/22/haxm-usage-windows/) and start QEMU with option -accel hax


