# piece of shit gentoo claims to only support /proc/mounts link now, but
# yet somehow it keeps reverting to a file
# having as a file ignores fusermounts
test -h /etc/mtab || sudo ln -sf /proc/mounts /etc/mtab

# note: following assumptions are made by users of this script:
#  1) groot is empty when dir==$0 (i.e., groot not in environment!)

groot=${groot:-$HOME/games/wine/${0##*/}}
export WINEPREFIX=${groot}/windows
# if $game is blank or not present in $groot, select one "automatically"
if [ -z "$game" -o ! -d "$groot/$game" ]; then
   ngame=
   # god damn utf8 locale is case-insensitive
   # this has always bothered me with sorts, but now bash case-folds patterns
   # and file names (I didn't notice because zsh doesn't do that)
   # who knows how much other crap is now broken thanks to this "feature"
   l="$LANG"
   export LANG=C
   for x in "$groot"/[A-Z]*; do
     case "${x##*/}" in
       Program*) ;;
       # checking for dir enables leaving LAVFilters & such around
       *) if [ -d "$x" -a -z "$ngame" ]; then
            ngame="${x##*/}"
	  else
	    ngame=bad
	  fi ;;
     esac
   done
   LANG="$l"
   if [ -z "$ngame" -o bad = "$ngame" ]; then
     notify-send "Game directory for ${0##*/} (${game:-unspecified}) not found" &
     exit 1
   fi
   game="$ngame"
fi
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
