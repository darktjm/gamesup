This is a collection of my support scripts for games.

I own way too many GOG.com games.  When I started, I still had some
other commercial games as well, but I've lost all of the installation
media in a fire, so this means all of my commercial games are GOG.com
games.  I also use Gentoo Linux; I don't like a lot of what they do,
but I do like some things, and it's really hard to change again (I
used to use Debian, and hung on to that for a few more years than I
wanted to as well, for the same reason.  The only distros that were
easy to switch were the ones I used before Debian, and not for that
long).  In any case, the point of this is that I have a way of dealing
with the games that is influenced by what software I currently use.

Some of these scripts use X notification to display messages.  I use
notify-send from libnotify for that.

nonet
=====
First of all, I don't let any games connect to the 'net, or anything
Windows-related, for that matter.  I used to do this by unconfiguring
the network or pulling the plug, but that became a hassle, and caused
some network connection attempts to just hang.  I now use a Linux
network namespace I call nonet.  At boot time, I run nonet.start,
which creates the namespace.  I also have an /etc/nonet directory,
which the namespace uses the hosts file from if it finds it.  The
hosts file only lists loopback addresses.

I can't use "ip netns exec" to execute stuff in the new namespace,
because it needs to run as root, and doesn't drop privilege.  It also
does other things I don't need it to do.  Instead, I use a simplified
setuid wrapper that just changes the namespace and then executes the
command-line parameters as the invoking user.

I still use iproute2's code to do this, so compile (and leave
compiled) iproute2 at least until it generates namespace.o.  I
currently use iproute-4.3.0 just because I'm lazy, but I'm sure more
recent versions will work.  Compile nonet with:

    gcc -s -O2 -o nonet{,.c} -Wall -iiproute2/include iproute2/lib/namespace.o

Replace iproute2 above with the path to the compiled iproute2 (in my
case, iproute2-4.3.0).  Then, make it root-owned and setuid, and install:

    sudo chown 0:0 nonet
    sudo chmod +s nonet
    sudo mv nonet /usr/local/bin

gameprep
========
To install gog games, I just run the gog installer using the default
installation directory (~/GOG Games).  This sets the top-level
directory name for the game.  I then move that to /usr/local/games,
owned by root:games and no "other" permissions (basically how gentoo
installs games, sort of).  To do this, after I install one ore more
games, I run "gameprep *" in ~/GOG Games, and then remove that
directory.

dogame
======
dogame grew out of a need, for some games, to turn off edge scrolling
in my window manager (FVWM).  The script still does that (although it
uses a different method), so it's dependent on a window manager
probably used by .1% of Linux users.  You'll have to figure out for
yourself what you want such a wrapper to do.  In particular, if you
use multiple monitors or other such things, I'm sure there are other
things needed.

  - I might move part of the game to an offline drive, so I first check
    if the target directory exists.  If not, notify and exit.

  - If I'm using the touchpad on my laptop, I also use syndaemon to try
    and turn it off while typing, normally.  I don't want this for
    games, so I kill it and restart it if it was found running.  I
    really wish somebody made a laptop with the touchpad to the side,
    rather in the center.  Or maybe use something else, like trackpoint.

  - If the game wants it, I change to its directory

  - If the game needs it, I set up two directories in ~/.local/share:

     - game-saves/<gamename> - where to store writable overlay
     - game-mnt/<gamename> - where to mount the game + writable overlay

    I then use unionfs to mount the game writable.  Otherwise, the game
    directory will be read-only.  Note that unionfs is a fuse
    filesystem.  This makes it much easier to use than overlayfs.  It
    also means that a bug in gentoo, where mtab isn't a symbolic like
    as it should be, must be corrected in order for fusermount -u to
    work.

  - Since I use FVWM with a large virtual desktop (not multiple
    desktops!), this confuses some games.  It also sometimes causes edge
    scrolling to fail.  For both of these reasons, I send a command
    (which requires that the FvwmCommandS module be running) to fvwm to
    reduce the virtual window size to 1x1.  I suppress this for things
    known to work without it.

  - I also turn off X's screen saver options.  DPMS seems to have been
    removed from my X server recently, but I still leave that in.  I
    don't, however kill xscreensaver or whater other tricks your system
    may need in addition to turn off screen savers.  Such active
    "screensavers" don't really save LCD screens, and popping
    processes to the front often kills wine full-screen programs.

  - Since setuid programs drop LD_LIBRARY_PATH, among other things, I
    preserve it if non-empty by prefixing the command with "env
    LD_LIBRARY_PATH=...".

  - I save the game's main process ID in a known location, so that I
    can kill a game using killgame.  This is bound to a global key in
    the window manager, but some games manage to override these, so
    maybe a lower-level hack will be necessary (e.g. outside of X
    entirely).

  - I save the game's name, if requested, so that a global key config
    can be used to run a script to save the current game.  I use this
    in some games to bypass "ironman mode" or "permadeath".

If dogame manages to survive running the game, it will then try to
restore things as they were.  If nothing else, I can run "dogame true"
to force the cleanup items to happen:

  - It dismounts the unionfs mount, if present.  It's kind of stupid
    about detecting that, but it never hurt me.

  - I restore the FVWM 3x2 virtual screen I usually use.  Yeah,
    hard-coded.

  - I restore the screen saver parameters I usually use.  Again,
    hard-coded.

  - I restart syndaemon if it was running at the start.

  - In case the game changed screen resolution and didn't restore, I set
    the screen resolution to default.

  - Some games mess with the display gamma, and so I restore it to 1.0.

  - It deletes the saved process ID/game name.

The dogame script accepts (and eats) several options before the game
name and its arguments:

   - -c -> change to the game's parent directory
   - -d -> use next parameter as "executable name" for determination of dir
   - -g -> change to game's root directory (assumed to end in /game/)
   - -w -> make game's root directory (assumed to end in /game/)
     writable using unionfs
   - -u -> dismount writable unionfs mount if still mounted

I generally make short names for all of my games, and create a
launcher script /usr/local/bin.  During installation, I let it create
a desktop file, which I modify to point to /usr/local/games instead of
GOG Games, as well as using my script instead of start.sh.  I move the
desktop file to /usr/local/share/applications.  A typical Linux game
launch script looks like this:

    #!/bin/sh
    exec dogame -c /usr/local/games/Game\ Name/game/Game\ Executable "$@"

save-slots.sh
=============
I hate permadeath and "iron man" mode.  I tried to genericize adding
multiple save slots as much as possible, and the result is
/usr/local/share/save-slots.sh.  It just tars up a directory into
another location.  When restoring, the target directory is removed and
replaced with the tarball contents.

To use, set some variables and source the script.  The variables to
set are game (the name of the game, used in save game file names) and
dir (the directory to tar up for the save).  For example, from my
bedlam script:

    game=bedlam; dir="$HOME/games/wine/bedlam/users/`id -un`/Application Data/SkyshinesBedlam"
    . /usr/local/share/save-slots.sh

This will add -r and -s options to the game's command line.  If -r or
-s is given, the game itself won't be run at all.  Instead, it either
saves or restores game files and then exits.  The flag is followed by
an arbitrary string to describe the save slot.  The saved games are
stored in $HOME/games/saves/$game<suffix>-sav.tar.bz2, where suffix is
blank if no slot argument is given, or "-<slot>" if given.

This will also set the magic save_game environment variable to
convince dogame to add the game name to the pid file so that a global
hot key can create saves at any time.  In fact, there are two scripts
to support this: save_game (saves to slot "auto") and restore_game
(restores from slot "auto").  This should probably be changed to use
more slots, but it's still useful in an emergency, as long as the game
doesn't override the global hot keys.

Since it's too easy to accidentally overwrite saves due to habit, both
saving and restoring saves backups in /tmp/save_backups_<username>.
When saving, if the target save already exists, it is first copied to
that directory.  When restoring, the original save directory's
contents are tared up for backup before wiping it.  This is only meant
as an immediate backup; use multiple save slots to provide long-term
history.

Some games (e.g. Darkest Dungeon, which I wrote this for, although I
don't play that game any more) have "multiple save slots" that are
really just multiple single-save-slot playthroughs ("profiles").  I
spent a lot of time trying to figure out how to support this easily.
The solution I came up with is still complicated, but better than
nothing.  To enable profile support, make the dir be the slot to save
to or restore, and tack on "++".  Then, set a third variable,
extract_to. If non-blank, the last element of the path is replaced
with $extract_to when restoring.  For example, here is the code that
itbr (Into the Breach) uses:

    game=itbr; dir="$HOME/games/wine/itbr/users/`id -un`/My Documents/My Games/Into The Breach/profile_"
    if [ x-p = "x$1" ]; then
      dir="$dir$2"; shift 2
      extract_to="${dir##*/}"
    else
      extract_to=
      for x in "$dir"*; do break; done
      if [ -d "$x" ]; then
        dir="$x"
      else
        dir="$dir"Alpha
      fi
    fi
    dir="${dir}++" # profile mode

Note that it allows the first two arguments to be "-p <profile>".  If
specified, both saves and restores will go to that profile.
Otherwise, since extract_to is blank, restores will always go to the
same profile they were saved from.  Saves will automatically select
the first (alphabetically) slot, or Alpha if none is found (which
should never happen unless there's nothing to save).

dosbox
======
Many games I own come with DOSBox.  I generally run the GOG.com
installer, and then I delete the supplied DOSBox and ignore the config
files in favor of my own configuration.  I have a primitive script,
dos-gameprep, similar to gameprep above to run after the game's
installer; it works in "GOG Games" or in wine above the installation
root.  In the case of wine, it also converts the icon and changes the
directory structure to put the game under data, like the Linux
installers do.  Unlike gameprep, it can't be run on more than one game
at a time.  Maybe I'll fix that one day.

My DOSBox configuration is the default, plus changes present in
dosbox-tjm.conf.  You don't have to change your global config to use
my changes; just launch with "dosbox -userconfig -config
dosbox-tjm.conf -config ...".  In particular, I use ALSA
virtual MIDI with timidity++ attached (on 128:0).  I also use IRQ 7
for Sound Blaster, and enable GUS emulation on 5.

I currently use dosbox-SVN-4227 with the mt32 patch.  I could've sworn
I had other patches as well (at least for glide and keyboard
injection), but I haven't dealt with that in so long I've forgotten.
In fact, I can't reinstall using the dosbox-9999 ebuld on gentoo any
more, so I should probably investigate why and fix/update it.

Note that I have the GUS installation in /usr/local/share/dosbox (I've
no idea where to get it any more, and I'm sure I am not permitted to
mirror it here).  Good luck getting this to work.  The only games I
tried with GUS were worse than just using SB+MIDI.  It's probably best
to just turn it off and change the SB IRQ to 5.

The mt32 patch requires munt (https://github.com/munt/munt; I use
gentoo's 2.3.0 ebuilds) and MT32 ROMs (I've no idea where to get those
any more, and like the GUS stuff, it's probably not freely
redistributable).  For games that support MT32 sound, I soft link the
MT3232_CONTROL.ROM and MT32_PCM.ROM into the game's root directory,
and add the lines from mt32.conf to each per-game launcher config.

I use gentoo's openglide 0.09_rc9_p20160913 ebuild
(http://openglide.sourceforge.net) for glide support.  I have no idea
where the glide2x.ovl file I use came from, but I soft link that from
its central location into the root dir of games that support glide,
and just add the following to each launcher config:

    [glide]
    glide=true
    ifb=none

GOG.com may do things differently; there is no harm in trying their
config first (usually in the common config, rather than the per-game
config).

GOG.com uses multiple dosbox configuration files.  One of those
(usually named dosbox<game>.conf) is common settings, and the rest are
small ones for each supported separate launcher.  The first thing I do
is diff the common file (with diff -wub for context and removal of
Windows CRLF) with my config, and look for interesting changes.  The
most useful ones are memory and CPU cycles, although the latter can
sometimes be very wrong.  Some experimentation may be necessary.  In
any case, I copy only the relevant changes (similar to how I presented
my config changes above) and I copy them to each created launcher
config.

I copy the launcher configs to <gamename><opt>.conf, where gamename is
the root directory name chosen by the installer (not the shortened
name used by GOG.com for their config files), and <opt> is the flag to
add to the game to run that config (e.g. -s for setup).  I usually use
GOG.com's config files verbatim, except that I remove useless IPX
sections, change ".." to "." (and usually have to move the "c:" line
above the imgmount line if both are present) in Windows installations
(I launch from the game dir, not a subdir), add the global config
changes needed, remove CR from CRLF files, and adjust the mount
command for Windows installations (backslash to slash and case
conversion).

In addition, if the game came from a Windows installer, and the game
has a virtual CD, the CD descriptor file (the one referenced by the
mount command) usually needs editing for case-sensitivity and
converting backslashes to slashes.  The only game I recall needing
more effort was Tomb Raider, which used mp3 files.  DOOSBox doesn't
support track times and time seeking for mp3, so I converted them all
to ogg (and adjusted the CD descriptor file accordingly).

I then run the sound config for the game to determine if better sound
options are available.  When possible, I use SB16+MT32.  Since the IRQ
for SB is 7, and some games assume 5, at least that must be changed by
the sound config program.

There may be other things that need to be done, but only if the game
is "special".  For example, Little Big Adventure 2 doesn't like being
run for C:\ in Linux, so I create a soft link from . to LBA2 and
change paths in the config file to launch from C:\LBA2 instead of C:\.

After that, I launch the game using a wrapper script, dosbox-game.sh. 
The main purpose of this wrapper is to use unionfs to keep the
installed game pristine.  This adds a "-u" argument to the game
launcher script, which unmounts the unionfs if it isn't already, and a
"-U" argument to just mount the unionfs and exit without launching the
game.  Similar to the Linux games, I create a script with a short
name, and use that in the launcher desktop file (which comes from
enabling a menu entry in Linux installers, or just filled in from
scratch for Windows installers, using the icon extracted by
dos-gameprep).  The game script goes in /usr/local/bin and the icon in
/usr/local/share/applications.  A typical script looks like:

    #!/bin/sh
    game=Game\ Name
    . /usr/local/share/dosbox-game.sh

The game should be installed in /usr/local/games/dosbox/Game\ Name,
with the actual game in a subdirectory called data.  Running the
script without arguments uses Game\ Name.conf, and running it with a
single argument uses Game\ Name<arg>.conf.

ags scummvm
===========
For games that use Adventure Game Studio and scummvm, I use the
gentoo-supplied ebuilds and system binaries and delete the supplied
interpreters.   For AGS, I also delete the supplied acsetup.cfg, since
that seems to cure full-screen mode and sound issues.  I stick AGS
games in /usr/local/games/ags and sscummvm games in
/usr/local/games/scummvm, all with the same directory structure.  The
GOG.com installer's root directory name, with the game itself in a
subdirectory (game for ags and data for scummvm).  A typical ags
launcher looks like:

    #!/bin/sh
    cd
    exec dogame ags /usr/local/games/ags/Game\ Name/game

A typical scummvm launcher looks like:

    #!/bin/sh
    cd
    exec dogame scummvm -p /usr/local/games/scummvm/Game\ Name/data engname

In this case, engname is the name of the engine to use to execute the
game.  Finding this out may require looking it up on the 'net, or
using the scummvm configuration GUI to add the game first, and looking
at what that picked.  For example, the engine used by King's Quest is
kq1.

Note that to install, I just use the installer and install it as if it
were a native game, and then I move things around to have the correct
directory structure.  I suppose I should have an ags-gameprep and
scummvm-gameprep, but it's not that much trouble to just move things
around.

infocom
=======
GOG.com doesn't sell many Infocom games, but I have enough that I made
some support for it.  I store the games in /usr/local/games/inf.  All
related files are renamed to the game name.  The script to run the
game is a soft link to a common script (inf-game) that just invokes
zoom.  The soft link is placed in /usr/local/bin, and, as with other
such games, an appropriate disktop file is placed in
/usr/local/share/applications.  I use dos-gameprep for the Windows
installers to extract the icon, and then delete everything but the
game data (DATA/*.DAT), icon and manuals.  The only game for which I
retain the DOSBox version is Beyond Zork, since zoom doesn't support
this game.

wine
====
Ah, Windows.  Bane of my (and, really, everyone's) existence.  If I
stuck to my guns and never installed Windows games, though, I'd lose
nearly half my games (I don't count Windows installers for dosbox,
ags, scummvm or Infocom games as Windows games).  I guess I should
just be happy wine lets me run most of them.

I store all of my wine games in their own prefixes, under
~/games/wine/<prefix>.  The prefix name is the short name I give the
game, and is also the name of the wrapper script that executes the
game.  Unlike the other types of games, I store the binary in ~/bin
(which is obviously also in my PATH), and the icons in
~/.local/share/applications.  This is because they are hard to make
completely system-stored.  I might work on that again some time in the
future, but for now only the bulk of the data is stored in
/usr/local/games.

To run an arbitrary Windows executable, I use dowine.  This also
supports running winetricks and wine-supplied commands like winecfg
and regedit.  This uses unionfs, but for different reasons than above.
At some point, wine started placing a bunch of files in the prefix,
and I ended up having gigabytes of extra crap due to my use of
separate prefixes.  My first solution was to use symbolic links.
However, some programs (especially winetricks) require writable files,
and symbolic links were still read-only.  Also, when I uninstalled
things, it left dangling links.  I do things differently now.  I
create a "reference" prefix, which is just an empty, initialized
prefix for every version of wine I run.  I then unionfs-mount that
prefix's windows directory using a writable .windows over the game
prefix's windows directory.  This keeps the file duplication down
without the read-only headaches of symbolic links.  The management of
this procedure is one of the tasks of dowine.

Another thing I like to do is set some initial registry entries upon
prefix initialization.  Some of those I probably ought to remove as
they no longer do anything useful, but one thing I set is the GOG.com
installer defaults.  I never create desktop icons and I like to
install in C:\.  The former option is ignored by older installers, so
it's always a good idea to press the button to view/verify options
beore installing.

Another thing I like to do is not have symbolic links in the user
directory to my home directory.  Windows programs throw their crap all
over the place, and I'd rather just keep it under the prefix.  For
similar reasons, I don't allow z: to be created.  Both of these need
to be done by patching wine.  I'm not sure that's actually deleting z:
any more, but I don't really care enough to fix it.  After all, it's
possible to access the entire UNIX filesystem using other means, so
it's not a very good security measure, anyway.

Along the same lines, I don't want wine creating mostly invalid icons
and mime type entries every time it runs, but that can be done by
explicitly disabling winemenubuilder.exe.  I should probably do that
in the registry, instead, for cleaner operation.

I also don't want to dive into subdirectories to get at the meat of
the installation:  the game.  As such, I make the prefix's
dosdevices/c: point to ../.. and make the prefix actually reside in
<prefix_name>/windows.  This places all wine's config in the windows
directory, where it belongs, as well.

I also may want key rebinding.  If you create a standalone executable
from AutoHotKey, you can call it wine-keys.exe and dowine will execute
it before the game, and kill it afterwards.

Since gentoo has slotted wine, I have multiple wine vesions at once.
The script supports simple options for version selection, but they
hard-code version numbers.  It should also support an arbitrary wine
executable/version number, but I don't test it, and it probably
doesn't work.  As mentioned above, a separate reference prefix is
created for each version of wine run (32-bit or 64-bit prefix), and
never deleted (except manually).

For a long time, I couldn't get 64-bit prefixes to work at all.  It
tunred out to be because I use the multilib-portage overlay, which
requires an environment override; see wine-env (place this in
/etc/portage/env/app-emulation/wine if want to use it; also soft link
it to wine-any, wine-vanilla, and wine-d3d9 if you use those ebuilds).

Along the same lines, my use of windows as the prefix name triggered a
bug in wine, which has been fixed in 4.6, I think.  Prior to that, I
have to apply wine-dir64.patch.  Otherwise, any attempt to run a
GOG.com installer in a 64-bit prefix will fail.

As to gecko and mono, I install them in the reference directories, but 

Usage:  dowine [options] exefile args

To create/use a 64-bit prefix, always set WINEARCH=win64 in the
environment.  I may fix this in the future to set it if the prefix is
already 64-bit.

    Version selection:
      -s = current stable, which is also the default wine
      -s3 = 3.0.x (must be manually linked as wine-3)
      -s2 = 2.0.x (must be manually linked as wine-2)
      -d = wine-d3d9: latest with gallium-d3d9
      -a = wine-staging: latest with staging+gallium-d3d9 (was wine-any, thus the a)
      -v = wine-vanilla; latest vanilla wine
      -V <exe> = use exe as the wine executable; probably doesn't work
      -D = use winedbg for debugging; probably doesn't work right
    Other:
      -k = kill wine in this prefix
      -m = mount the windows overlay
      -M = dismount the windows overlay
      -w = assume exe will exit before program finishes; use wine-wait
      -n = allow built-in mono/.net install/usage (never works)
      -g = allow built-in gecko install/usage (never works)
      -b = change what's installed based on -n/-g flags

A wine game uses an additional wrapper, wine-game.sh.  This uses an
additional unionfs mount for the game root itself.  The game root is
moved to /usr/local/games/wine and owned by root:games; it is then
union-mounted in the wine prefix on Game\ Name using .Game\ Name as
the writable overlay.  wine-gameprep moves the root into place, and
extracts an icon.  This icon is copied into the prefix game root so
that even when it's not mounted it's in the same place.  Since I have
a hard time knowing what version of a wine game is installed, I also
copy the installers into the prefix.  In this caes, the installers are
always symbolic links into my download directory, so they don't take
up any space, and I can tell by their listing color whether or not
they are up-to-date (they are red if not present in my download
directory, and cyan otherwise).  Like dos-gameprep, it only runs on
one game at a time (and more than one wouldn't make sense anyway,
since I store each game in a separate root and the script expects the
root to be in the current directory).

A typical wine game launcher looks like this:

    #!/bin/sh
    game=Game\ Name
    . ${0%/*}/wine-game.sh
    exec dogame dowine -s Game\ Exe.exe

I always give the wine version flag, even though -s is always
optional.  I end up having to retest all my wine games every version
update, and having the version coded like that can help.

Some games have extra parts, like editors.   To deal with that, I just
use a shell case statement on "$1" and exec the appropriate command.

The wine-game.sh script adds a few command-line options to all that use it:

   -u = unmount the game fusermount; it never gets unmounted otherwise
   -U = just mount the game fusermount and exit

Ignore the joystick button functions in there; I'll remove them soon
and they do no harm.
