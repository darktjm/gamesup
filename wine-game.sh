# here are some js button-swapping support functions
js_swapbut() {
  js=`js-ev`
  test -z "$js" && return
  test x-J = x"$1" && js-unswapbut && return
  input-kbd $js | while read x y z a b; do
    test = = "$y" || continue
    case $b in
      BTN_TRIGGER) echo 0x90004 = $z ;;
      BTN_THUMB)   echo 0x90003 = $z ;;
      BTN_THUMB2)  echo 0x90002 = $z ;;
      BTN_TOP)     echo 0x90001 = $z ;;
    esac
  done | input-kbd -f - $js
}

js_unswapbut() {
  js=`js-ev`
  test -z "$js" && return
  input-kbd $js | while read x y z a b; do
    test = = "$y" || continue
    case $b in
      BTN_TRIGGER) echo 0x90001 = $z ;;
      BTN_THUMB)   echo 0x90002 = $z ;;
      BTN_THUMB2)  echo 0x90003 = $z ;;
      BTN_TOP)     echo 0x90004 = $z ;;
    esac
  done | input-kbd -f - $js
}

# piece of shit gentoo claims to only support /proc/mounts link now, but
# yet somehow it keeps reverting to a file
# having as a file ignores fusermounts
test -h /etc/mtab || sudo ln -sf /proc/mounts /etc/mtab

# note: following assumptions are made by users of this script:
#  1) groot is empty when dir==$0 (i.e., groot not in environment!)

groot=${groot:-$HOME/games/wine/${0##*/}}
export WINEPREFIX=${groot}/windows
# this used to add fixme-all @ end w/ optional , if already set
# but that's harder to override
export WINEDEBUG="${WINEDEBUG-fixme-all}"
# dowine won't get pulled in from desktop icon sometimes
p="${0%/*}"
case "$p" in
  /*) ;;
  *) p="$PWD/$p" ;;
esac
PATH="$p:${PATH}"

if [ "x-u" = "x$1" ]; then
  cd
  # note that fusermount -u won't work if /etc/mtab isn't symlinked to /proc/mounts
  fusermount -u "$groot/$game" 2>/dev/null && echo "game unmounted"
  fusermount -u "$groot/windows" 2>/dev/null && echo "prefix unmounted"
  # this is flaky at best, but better than nothing
  # the printf is to interpret \-escapes; hopefully there are no %s
  printf "`fgrep unionfs /proc/mounts | cut -d' '  -f2`" | fgrep "$groot/$game" >/dev/null && echo "!!!game still mounted"
  test -f "$groot/windows/user.reg" && echo "!!!prefix still mounted"
  exit
fi

#for x in "${groot}/$game"/*.ico; do
#  test -h "$x" -a -e "$x" && break
#  notify-send "${groot}/$game not mounted" &
#  exit 1
#done

if [ -z "$game" ]; then
  echo "Invalid startup script"
  exit 1
fi

if [ ! -d /usr/local/games/wine/"$game" ]; then
  notify-send "$game not mounted" &
  exit 1
fi

if ! mount | fgrep "$groot/$game type fuse.unionfs" >/dev/null; then
  mkdir -p "$groot/.$game"
  unionfs -o cow,nonempty,uid=$(id -u) \
     "$groot/.$game=RW:/usr/local/games/wine/$game=RO" \
     "$groot/$game"
fi
if [ x-U = "x$1" ]; then
  exit 0
fi
cd "$groot/$game"
