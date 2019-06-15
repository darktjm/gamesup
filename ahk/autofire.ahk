; Autofire function: bind a joy button/key to autofire a joy/key
; e.g.
;*Joy5::AutoFire("Joy5", "z")

AutoFire(j, k, t := 30)
{
	Send {%k% Down}
	fn := Func("FireAgain").Bind(j, k)
	SetTimer, % fn, %t%
}

FireAgain(j, k)
{
	; A square wave might be cleaner, but harder to do.  Tracking the
	; state would require a 3rd parm since GetKeyState doesn't track
	; virtual key presses, which means a full functor probably
	Send {%k% Up}
	if (GetKeyState(j))
		Send {%k% Down}
	else
		SetTimer,,Off
}
