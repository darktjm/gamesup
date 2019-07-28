#!/bin/sh
game="${game:-${0##*/}}"
wdir="$HOME/.local/share/game-saves/ami-$game"
groot="$HOME/.local/share/game-mnt/ami-$game"
gdir=/usr/local/games/ami/"$game"
function clean_saves() {
  # clean non-written copied files; should be done after unmount, but that's
  # not done very often here
  (
  	cd "$wdir"  # easer than stripping $wdir off of $x
	find -depth | while IFS= read -r x; do
		if [ -d "$x" ]; then
			test -d "$gdir/$x" && rmdir "$x" 2>/dev/null
		else
			cmp "$gdir/$x" "$x" >/dev/null 2>&1 && rm "$x" && echo "cleaned $x"
		fi
	done
  )
}
if [ x-u = "x$1" ]; then
  cd
  fusermount -u "$groot"
  clean_saves
  exit
fi
# small enough that there's no point in moving to external storage
#if [ ! -d "$gdir" ]; then
#  notify-send "$gdir not mounted" &
#  exit 1
#fi

if ! mount | fgrep "$groot type fuse.unionfs" >/dev/null; then
  mkdir -p "$groot" "$wdir"
  clean_saves
  unionfs -o cow,nonempty,uid=$(id -u) "$wdir=RW:$gdir=RO" "$groot"
fi
if [ x-U = "x$1" ]; then
  exit
fi
cd "$groot"

# fs-uae version; fs-uae is "game-focused" and harder to configure than it should be
function parse_args() {
  while read -r x; do
    case "$x" in
      ""|\#*) ;;
      --*\ --*) for y in $x; do echo "$y"; done ;;
      --*) echo "$x" ;;
      \<*\>)
	y="${x#\<}"; y="${y%\>}"
	for f in "$y" "/usr/local/games/ami/$y"; do
	   test -f "$f" || continue
	   parse_args "$f"
	   break
	 done
	 ;;
      *) echo "--$x" | sed -E -e 's/ += +/=/g;s/ +/ --/g;s/ --/\n--/g' ;;
    esac
  done < "$1" || exit 1
}
if [ -f fs-uae-extra.conf ]; then
  parse_args fs-uae-extra.conf >/tmp/arg.$$
  args=()
  while read -r x; do args+=("$x"); done </tmp/arg.$$
  rm /tmp/arg.$$
  set -- "${args[@]}" "$@"
fi
exec dogame fs-uae --sub_title="$game" --hard_drive_0="$groot" --hard_drive_0_priority=11 --fullscreen "$@"

# old e-uae version; e-uae is unmaintained bitrot
if [ -f uae.conf ]; then
  exec dogame e-uae -f uae.conf -s "filesystem2=rw,dh1:${game}:$groot,11"
else
  exec dogame e-uae -s "filesystem2=rw,dh1:${game}:${groot},11"
fi
