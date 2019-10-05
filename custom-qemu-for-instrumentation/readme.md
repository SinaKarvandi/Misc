# Custom QEMU for Instrumentation

If you need a fast way to instrument user/kernel/hypervisor then you have custom-qemu-for-instrumentation as a fast and light option. As the TCG plugins are much slower than using this method, so I prefer to have a custom QEMU. This project might not work on the future versions of QEMU but in such case, it'll be updated. You can also save r/e flags and general purpose registers based on your needs.

**This project only works for x86 and AMD64 emulator version of QEMU.**

![QEMU Instrumentation](https://github.com/SinaKarvandi/misc/raw/master/Imgs/custom-qemu-1.jpg)



Instructions (and optionally gp registers and r/e flags) will be saved into files.

![QEMU Saved Instr. with GP and flags](https://github.com/SinaKarvandi/misc/raw/master/Imgs/custom-qemu-3.PNG)

## How to use
Copy the `translate.c` file into your QEMU Source path and replace this file with `/qemu/target/i386/translate.c`.

Make sure to see the below section about configuration.

## Configuratioon
You have to change the path to save the logs of instrumentation in the `translate.c`.

Take a look at this picture :
![QEMU Configuration](https://github.com/SinaKarvandi/misc/raw/master/Imgs/custom-qemu-4.png)

Set the `save_path` to the path you want to save the instrumentation results. (you have to use `%d` in your path as an index to add into each instrumentation log.)

* You can also use `packet_capacity`, modify this constant will increase/decrease the amount of instructions to be saved.
* If you don't need hex assemlies the undefine `save_assembly_hex_bytes`.
* If you don't need general purpose and r/e flags to be saved then undefine `save_gp_registers`.
* If you don't wanna see debug messages then undefine `my_debug`.
* If you undefine `modify_qemu` all the modifications to qemu will be ignored.

## Build QEMU
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
*   Comment out (#) Capstone (line 508) in qemu\\Makefile (Instead of commenting out capstone line, you can add --disable-capstone to configure arguments in the case if it didn't work.
*   Build QEMU:
    *   ./configure --enable-gtk --enable-sdl --target-list=x86\_64-softmmu --disable-werror --disable-stack-protector
    *   make
*   Run in qemu/x86\_64-softmmu ./qemu-system-x86\_64 -L ./../pc-bios
*   Optional (for better performance): Install HAXM according to this guide: [https://www.qemu.org/2017/11/22/haxm-usage-windows/](https://www.qemu.org/2017/11/22/haxm-usage-windows/) and start QEMU with option -accel hax

## QEMU Architecture
![QEMU Architecture](https://github.com/SinaKarvandi/misc/raw/master/Imgs/custom-qemu-2.jpg)

