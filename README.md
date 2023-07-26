# Minecraft DLL Injector

## Build Instructions

```sh
cl /EHsc src\main.cpp /MD /link /SUBSYSTEM:WINDOWS /out:Chyjector.exe User32.lib Comdlg32.lib Shell32.lib Kernel32.lib gdi32.lib lib/discord-rpc.lib
```