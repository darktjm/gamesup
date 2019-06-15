#ErrorStdOut ; dialog can be hidden by game, so print to stdout
#NoEnv       ; don't auto-import variables from environment
SendMode Input
;#Warn
gosub SetupViewstick
XAxis := "JoyU"
YAxis := "JoyV"
ButtonMiddle := 20
#include jscursor.ahk

; the easy part: kb buttons
*Joy11::Send {Alt F4}
*Joy12::Send {Ctrl}
*Joy13::Send {c}
*Joy4::Send {Space}
*Joy3::Send {Tab}
*Joy5::Send {2}
*Joy6::Send {3}
*Joy7::Send {q}
*Joy8::Send {e}

SetupViewstick:
#include JoystickMouse.ahk

; The hard part: right stick to change view
; Partically copied from JoystickMouse.ahk at autohotkey.com

;;; Configuration
; Increase the following value to make the mouse cursor move faster:
JoyMultiplier = 0.30

; Decrease the following value to require less joystick displacement-from-center
; to start moving the mouse.  However, you may need to calibrate your joystick
; -- ensuring it's properly centered -- to avoid cursor drift. A perfectly tight
; and centered joystick could use a value of 1:
JoyThreshold = 3

; Change the following to true to invert the Y-axis, which causes the mouse to
; move vertically in the direction opposite the stick:
InvertYAxis := false
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Calculate the axis displacements that are needed to start moving the cursor:
JoyThresholdUpper := 50 + JoyThreshold
JoyThresholdLower := 50 - JoyThreshold
if InvertYAxis
	YAxisMultiplier = -1
else
	YAxisMultiplier = 1

SetTimer, WatchJoystick, 10  ; Monitor the movement of the joystick.
return

