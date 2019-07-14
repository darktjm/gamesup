# support -s and -r for side-saving
# set $dir to the dir containing the save games
# the tar files will get $0 as their base name

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

# Here's what I have now, which works well enough:
#    dir=+${dir}  -> enable profile code; dir is default dir or prefix
#       The + is used instead of directly setting a variable so that
#       non-profile games don't have to do anything.  I used to use a ++
#       suffix to make it more obvious, but now I prefer a prefix.
#    psuffix=.sav -> a suffix to add to prefix if dir is a prefix.  Actual
#       slot file/dir name is ${dir#+}${slot}${psuffix}
#    I used to also have extract_to, but now I process the profile arg
#    internally:  +<slot> after -s/r selects the slot to save/load to.
# Basically I no longer try to ensure that the user has a consistent
# experience (i.e., instead of always numbering the slots, I require that
# the user know what the slot names are and not select invalid names).
# If the user doesn't specify a slot on save, it saves the first found entry
# matching ${dir#+}*${psuffix}; the default slot isn't taken into account
# at all.  For restoring, if a slot is not specified, it restores to the
# slot that was saved.
# I also don't support weird slot types; those will have to be implemented
# manually as the need arises (or this code might be extended to support that)

test -z "$dir" && exit 1
game="${0##*/}"
do_profile=
case "$dir" in
	+*)  do_profile=y; dir="${dir#+}" ;;
esac
case "$1" in
  -s|-r)
    save="${1#-r}"
    if [ -n "$do_profile" ]; then
        # process cmd-line supplied profile
	# I used to expect scripts to do this themselves, setting extract_to
	# if needed, but they all ended up looking the same, anyway
	extract_to=
	case "$2" in
		+*)  dir="$dir${2#+}${psuffix}"; shift
		     extract_to="${dir##*/}"
		     ;;
		*)   for x in "$dir"*"${psuffix}"; do break; done
		     if [ -e "$x" ]; then
		     	dir="$x"
		     else
		        dir="${dir%/*}/A saved game slot"
		     fi
		     ;;
	esac
    fi
    cd "${dir%/*}"
    sdir="${dir##*/}"
    sg="$HOME/games/saves/${game}-$2${2:+-}sav.tar.bz2"
    # both saves and restores are backed up to this directory
    # tmpreaper or wipe-on-reboot will hopefully keep them from filling space
    bdir="/tmp/save_backups_$(id -un)"
    if [ -z "$save" ]; then
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
