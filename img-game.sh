#!/bin/sh

# Find/list game images; intended to be used as part of emu game launcher
# parses/shifts over cmd line : [-l-g] <gamepat>
# gamepat may be substring or glob pattern
# if more than one match, pops up selection dialog or just lists matches if -l
# if no gamepat, lists all games or pops up selection dialog if -g

# The driver sets the parameters:
# gamesdir=() image directories
# ext=() image file extensions

# exits if no specific image selected

# otherwise, sets variables:
# df=path to game
# bb=base name of image without sequence (-1, -2, etc. for multi-image games)

zcmd() {
    zenity --width=800 --height=600 --list --hide-header --column=game --text= --title=Select\ Game
}

gcat() {
    type -p zenity >/dev/null || exec cat
    exec "$0" "^$(zcmd 2>/dev/null)\$"
}
catcmd=gcat
listall=y
case "$1" in
    -l) catcmd=cat; shift ;;
    -g) listall=; shift ;;
esac

game="$1"
shift

if [ -z "$game" ]; then
    test -n "$listall" && catcmd=cat
    for d in "${gamesdir[@]}"; do (
        cd "$d"
	for e in "${ext[@]}"; do
	    ls *."$e"
	done
    ) done | sed 's/\.[^.]*$//' | sort | $catcmd
    exit 0
fi

b="${game##*/}"
# This ensures that dual-ext images are (usually) stripped correctly
for x in "${ext[@]}" \*; do
   test "x${b%.$x}" = "x$x" && continue
   b="${b%.$x}"
done
bb="${b%-[1-9]}"
if [ -f "$game" ]; then
    df="$game"
else
    # try to find given name directly
    for d in "${gamesdir[@]}"; do
	df="$d/$b"
     	test -f "$df" && break
     	for x in "${ext[@]}"; do
       	    test -f "$df".$x && break
     	done
     	df="$df".$x
     	test -f "$df" && break
    done
    if [ ! -f "$df" ]; then
	# and if that fails, try pattern matching
	bop=\*; eop=\*
	case "$game" in
	    '^'*'$') bop=; eop=; game="${game#^}"; game="${game%\$}" ;;
	    '^'*) bop=; game="${game#^}" ;;
	    *'$') eop=; game="${game%\$}" ;;
	esac
	g=()
	cwd="$PWD"
	for d in "${gamesdir[@]}"; do
	    cd "$d"
	    for e in "${ext[@]}"; do
		for f in *."$e"; do
		    case "$f" in
	        	$bop$game$eop".$e") g+=("${f%.$e}") ;;
	            esac
		done
	    done
	    cd "$cwd"
	done
	test -z "$g" && echo "No match" && exit 1
	if [ "${#g[*]}" -gt 1 ]; then
	    echo "More than one match"
	    for n in "${g[@]}"; do
		echo "$n"
	    done | sort | $catcmd
	    exit 0
	fi
	bb="${g%-[1-9]}"
	for d in "${gamesdir[@]}"; do
	    df="$d/$g"
	    for x in "${ext[@]}"; do
		test -f "$df".$x && break
	    done
	    df="$df".$x
	    test -f "$df" && break
	done
    fi
    test -f "$df" || exit 1
fi
