# Chyves Injector.

a simple, easy to use, dll injector based off.

- thank you fligger for the injector code i useto actually inject the dll.
- the ui itself and the discord rpc were made by me :D
- feel free to fork and make a pull request <3

## Build Instructions

```sh
cl /EHsc src\main.cpp /MD /link /SUBSYSTEM:WINDOWS /out:Chyjector.exe User32.lib Comdlg32.lib Shell32.lib Kernel32.lib gdi32.lib lib/discord-rpc.lib
```
