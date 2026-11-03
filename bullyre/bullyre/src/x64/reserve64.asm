; The original game's image range, 0x401000..0x20F3000, reserved as the start of
; bullyre_x64.exe's .text (as src/host/reserve.asm does for the 32-bit host): ".text$A" sorts
; before the compiler's ".text$mn" and the exe is linked at the game's base (0x400000), so
; nothing else (kernel32 lands in this range otherwise) can be mapped there. src/x64/dispatch.cpp
; writes the jump stubs at the game's function addresses and maps the game's data after them.
GAMEIMG SEGMENT ALIGN(4096) 'CODE' ALIAS(".text$A")
PUBLIC g_game
g_game LABEL BYTE
    db 01CF2000h dup (0)
GAMEIMG ENDS
END
