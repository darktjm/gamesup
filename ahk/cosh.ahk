#ErrorStdOut ; dialog can be hidden by game, so print to stdout
#NoEnv       ; don't auto-import variables from environment
SendMode Input
;#Warn
#include jscursor.ahk
#include autofire.ahk

#IfWinActive Cosmic Star Heroine
;swap z/x
*z::Send (x)
*x::Send (z)

/*
;WASD
*w::Send {Up Down}
*w Up::Send {Up Up}
*a::Send {Left Down}
*a Up::Send {Left Up}
*s::Send {Down Down}
*s Up::Send {Down Up}
*d::Send {Right Down}
*d Up::Send {Right Up}
*/
/*
;test y->z
;*y::Send (z)
*/


; gamepad buttons
; don't support modifiers, anyway, so * is optional
*Joy1::Send {z}    ; X
*Joy2::Send {x}    ; O
*Joy3::Send {Tab}  ; T
*Joy4::Send {Tab}  ; S
*Joy9::Send {Esc}  ; Start/Share
*Joy10::Send {Esc} ; Select/Options
*Joy11::Send {Esc} ; PS/Mode
*Joy5::Send {a}    ; L1
*Joy6::Send {s}    ; R1
*Joy7::AutoFire("Joy7", "z") ; L2

#If ; end of Cosmic Star Heroine
