; Convert gamepad 1st stick/POV to cursor keys
; Adapted from autohotkey docs on remapping joystick axes & POV
; My changes merge the two and support diagonals (2 keys at once)
; If you use both at once, it'll act weird.  Why are you doing that, anyway?

#Persistent  ; Keep this script running until the user explicitly exits it.
             ; Not technically necessary if there are hotkeys in the script
SetTimer, WatchAxis, 10 ; was 5, but 10 is enough and may be rounded up anyway
return

WatchAxis:
JoyX := GetKeyState("JoyX")  ; Get position of X axis.
JoyY := GetKeyState("JoyY")  ; Get position of Y axis.
POV := GetKeyState("JoyPOV")  ; Get position of the POV control.
LRKeyPrev := LRKey  ; Prev now holds the L/R key that was down before (if any).
UDKeyPrev := UDKey  ; Ditto for U/D key

if (JoyX > 70)
    LRKey := "Right"
else if (JoyX < 30)
    LRKey := "Left"
; note that I extended POV angles 22.5 degrees each direction for diagonals
else if POV between 2250 and 15750
    LRKey := "Right"
else if POV between 20250 and 33750
    LRKey := "Left"
else
    LRKey := ""

if (JoyY > 70)
    UDKey := "Down"
else if (JoyY < 30)
    UDKey := "Up"
else if (POV > 29250)
    UDKey := "Up"
else if POV < 0
    UDKey := ""
else if POV between 0 and 6750
    UDKey := "Up"
else if POV between 11250 and 24750
    UDKey := "Down"
else
    UDKey := ""

SetKeyDelay -1  ; Avoid delays between keystrokes.
if (UDKey != UDKeyPrev) {
    if (UDKeyPrev)   ; There is a previous key to release.
        Send {%UDKeyPrev% Up}  ; Release it.
    if (UDKey)   ; There is a key to press down.
        Send {%UDKey% Down}  ; Press it down.
}
if (LRKey != LRKeyPrev) {
    if (LRKeyPrev)   ; There is a previous key to release.
        Send {%LRKeyPrev% Up}  ; Release it.
    if (LRKey)   ; There is a key to press down.
        Send {%LRKey% Down}  ; Press it down.
}
;SetKeyDelay 10  ; Presumably gets reset after completion -- don't care
return
