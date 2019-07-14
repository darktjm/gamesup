groot=$HOME/.local/share/game-mnt/"$game"
wdir=$HOME/.local/share/game-saves/"$game"
gdir=/usr/local/games/dosbox/"$game"
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
if [ ! -d "$gdir/data" ]; then
  notify-send "$gdir/data not mounted" &
  exit 1
fi

if ! mount | fgrep "$groot type fuse.unionfs" >/dev/null; then
  mkdir -p "$groot" "$wdir"
  clean_saves
  unionfs -o cow,nonempty,uid=$(id -u) "$wdir=RW:$gdir/data=RO" "$groot"
fi
if [ x-U = "x$1" ]; then
  exit
fi
cd "$groot"
test -z "$noexec" && exec dogame dosbox -userconf -conf "$groot/$game$1.conf"
dogame dosbox -userconf -conf "$groot/$game$1.conf"
