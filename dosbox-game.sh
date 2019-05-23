groot=$HOME/.local/share/game-mnt/"$game"
wdir=$HOME/.local/share/game-saves/"$game"
gdir=/usr/local/games/dosbox/"$game"
if [ x-u = "x$1" ]; then
  cd
  fusermount -u "$groot"
  exit
fi
if [ ! -d "$gdir/data" ]; then
  xmessage "$gdir/data not mounted" &
  exit 1
fi

if ! mount | fgrep "$groot type fuse.unionfs" >/dev/null; then
  mkdir -p "$groot" "$wdir"
  unionfs -o cow,nonempty,uid=$(id -u) "$wdir=RW:$gdir/data=RO" "$groot"
fi
if [ x-U = "x$1" ]; then
  exit
fi
cd "$groot"
test -z "$noexec" && exec dogame dosbox -userconf -conf "$groot/$game$1.conf"
dogame dosbox -userconf -conf "$groot/$game$1.conf"
