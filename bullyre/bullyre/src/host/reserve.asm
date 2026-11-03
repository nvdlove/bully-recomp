; The original game's image, 0x401000..0x20F3000, reserved as the start of the host
; exe's .text: ".text$A" sorts before the compiler's ".text$mn", and the exe is linked at
; the game's base (0x400000), so the reserve sits exactly where the game's sections go.
; It has to be code: data, even in a section named .text, is put in a section of its own
; after ours. host.cpp's HostEntry copies the game in at startup.
.686
.model flat
GAMEIMG SEGMENT ALIGN(4096) PUBLIC 'CODE' ALIAS(".text$A")
PUBLIC _g_game
_g_game LABEL BYTE
    db 01CF2000h dup (0)
GAMEIMG ENDS
END
