# support -s and -r for side-saving
# set $game to the name of the game's tar files
# set $dir to the dir containing the save games

# some games have multiple save slots, but they represent multiple "runs", so
# you're still stuck with the one save slot for the whole game

# I had several ideas on how to implement this completely within this file,
# but they were not really what I wanted:
#   dir=slotted  (or: dir=-${base_dir})
#   slots=3
#   slot_1=profile_0
#   slot_2=profile_1
#   slot_3=profile_2
# Or:
#   dir=slotted  (or: dir=-${base_dir}, or: dir-$(base_dir)/$(slot_prefix))
#   slot_prefix=profile_
#   slot_suffix=
#   default_slot=0
# Or:
#   dir=slotted  (or: dir=-${base_dir}, or: dir-$(base_dir)/$(default_name))
#   default_name=profile_0
#   fn: slot_dir $(slot_Name) -> name of dir to archive
# then, the parameter after -r/-s is the slot to save/restore, prefixed with -

# Instead, it's super simple:
#    dir=${dir}++  -> enable profile code; dir is default dir
#       The ++ is used instead of directly setting a variable so that
#       non-profile games don't have to do anything
#    extract_to=... -> If non-blank, replace target dir with ...
#       This is separate from the target dir because target dir should
#       be ignored if user didn't explicitly ask for a profile
# this means that the game script has to handle getting the profile name:
# example from itbr:
#game=itbr; dir="$HOME/games/wine/itbr/users/`id -un`/My Documents/My Games/Into The Breach/profile_"
#if [ x-p = "x$1" ]; then
#  dir="$dir$2"; shift 2
#  extract_to="${dir##*/}"
#else
#  extract_to=
#  for x in "$dir"*; do break; done
#  if [ -d "$x" ]; then
#    dir="$x"
#  else
#    dir="$dir"Alpha
#  fi
#fi
#dir="${dir}++" # profile mode

test -z "$game" -o -z "$dir" && exit 1
do_profile=
case "$dir" in
	*++) do_profile=y; dir="${dir%++}" ;;
esac
case "$1" in
  -s|-r)
    cd "${dir%/*}"
    sdir="${dir##*/}"
    sg="$HOME/games/saves/${game}-$2${2:+-}sav.tar.bz2"
    # both saves and restores are backed up to this directory
    # tmpreaper or wipe-on-reboot will hopefully keep them from filling space
    bdir="/tmp/save_backups_$(id -un)"
    if [ x-r = x$1 ]; then
      if [ ! -f "$sg" ]; then
        echo "$sg does not exist"
	echo "Known save games listed below:"
	ls ${sg%/*} | sed -n -e "s/${game}-//;T;s/sav.tar.bz2//;s/-\$//;s/^\$/<Unnamed>/;p"
	exit 1
      fi
      if [ -n "$do_profile" ]; then
        extract_from="`tar tjf \"\$sg\" | head -n 1`" # should always be dir
	extract_from="${extract_from%/}"
	test -z "$extract_to" && extract_to="$extract_from"
	sdir="$extract_to"
      fi
      bdir="$bdir/$sdir"
      echo "Restoring${2:+ }$2${do_profile:+ to ${extract_to}} (backup to $bdir)"
      if [ -e "$sdir" ]; then
        mkdir -p ${bdir%/*}
        rm -rf "$bdir"
        cp -ra "$sdir" "${bdir%/*}"
        rm -rf "$sdir"
      fi
      if [ "$do_profile" -a "x$extract_to" != "x$extract_from" ]; then
        mkdir sd$$.$$
	cd sd$$.$$
      fi
      tar xjf "$sg"
      if [ "$do_profile" -a "x$extract_to" != "x$extract_from" ]; then
        cd ..
        mv "sd$$.$$/$extract_from" "$extract_to"
        rmdir sd$$.$$
      fi
    else
      if [ ! -e "$sdir" ]; then
        echo "$sdir does not exist; skipping save"
	exit 1
      fi
      echo "Saving ${do_profile:+${dir##*/}${2+ in }}$2"
      mkdir -p "$HOME/games/saves"
      if [ -f "$sg" ]; then
        mkdir -p "$bdir"
	mv "$sg" "$bdir/${sg##*/}-"`date +%F@%T`
      fi
      tar cjf "$sg" "$sdir"
    fi
    exit 0
    ;;
esac
# enable hotkeys; assumes exec
export save_game="$$:$0"
