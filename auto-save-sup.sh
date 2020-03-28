pid=
nl=`wc -l /tmp/.last-game 2>/dev/null | cut -d\  -f1`
if [ ${nl:-0} -eq 2 ]; then
  pid=`head -n 1 /tmp/.last-game`
  ps -p $pid >/dev/null || pid=
fi
if [ -z "$pid" ]; then
  notify-send "$sr Game Failed" "No save-slot game running"
  exit 1
fi
game="`tail -n 1 /tmp/.last-game`"
af="/tmp/.${game}-auto"
auto="`cat \"\$af\" 2>/dev/null`"
if [ -z "$auto" ]; then
  ls -tr "$HOME/games/saves/$game-auto"*"-sav.tar.bz2" 2>/dev/null | (
    while read -r x; do
      auto="${x##*/}"
    done
    if [ -n "$auto" -a "x$auto" != "x${game}-auto-sav.tar.bz2" ]; then
      auto="${auto##*-auto}"
      auto="${auto%-*}"
      echo "$((auto+1))" >"$af"
    fi
  )
  auto="`cat \"\$af\" 2>/dev/null`"
fi
