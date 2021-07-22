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
`notify-send` from libnotify for that.

Disclaimer: I will make sweeping changes to all of these scripts and
procedures without notice.  Don't use these for yourself unless you
are willing to look through the git-log and git-diff output and
determine for yourself what changed.  I really only expect you to look
at them for inspiration, not use them directly.

lgogdownloader
==============
To download games from gog, I use `lgogdownloader`.  Check your
distro, alternate package sites, or google it to obtain it; I use
either gentoo's version or the latest git depending on my mood.  Use
it.  It's the only reasonable way to manage more than 5-10 games on
gog.  I currently patch mine to not change the shelf search order when
updating the cache, to ignore SIGPIPE during cache updates, to make
cache updates fast enough for my current limited Internet access to
finish, and a way to check orphans and what would be downloaded while
disconnected.  Look in the `lgogdownloader` subdirectory; it should be
obvious which patch is which.  I may propose those upstream some day
if I get things in order.  I also use two support scripts currently:
`gog-checkpatch` was written before the March 2017 great gog renaming,
when version numbers could be easily compared, so it no longer really
works.  Its purpose is to detect if I have to install a patch to the
latest full download. Currently, only Dragon Age: Origins Ultimate is
detected by the script (I used to have more, but don't remember
which).  `chkorph` adds full download directory orphan checks to
check-orphans.  That way, when I hide a game or gog changes the game
ID/name, I notice it.

nonet
=====
First of all, I don't let any games or anything Windows-related, for
that matter, connect to the 'net.  I used to do this by unconfiguring
the network or pulling the plug, but that became a hassle, and caused
some network connection attempts to just hang.  I now use a Linux
network namespace I call nonet.  At boot time, I run `nonet.start`,
which creates the namespace.  I also have an `/etc/nonet` directory,
containing a hosts file listing only loopback addresses; the nonet
namespace will use this instead of `/etc/hosts`.

I can't use "`ip netns exec`" to execute stuff in the new namespace,
because it needs to run as root, and doesn't drop privilege.  It also
does other things I don't need it to do.  Instead, I use a simplified
setuid wrapper that just changes the namespace and then executes the
command-line parameters as the invoking user.

I still use iproute2's code to do this, so compile (and leave
compiled) iproute2 at least until it generates namespace.o.  I
currently use iproute-4.3.0 just because I'm lazy, but I'm sure more
recent versions will work.  Compile `nonet` with:

    gcc -s -O2 -o nonet{,.c} -Wall -iiproute2/include iproute2/lib/namespace.o

Replace iproute2 above with the path to the compiled iproute2 (in my
case, iproute2-4.3.0).  Then, make it root-owned and setuid, and install:

    sudo chown 0:0 nonet
    sudo chmod +s nonet
    sudo mv nonet /usr/local/bin

Note that I also looked into `firejail`, which does way more than I want
and less conveniently.  Its only advantage (mainly for other people)
is that it comes with most distros.

gameprep
========
To install gog games, I just run the gog installer using the default
installation directory (`~/GOG Games`).  This sets the top-level
directory name for the game.  I then move that to /usr/local/games,
owned by root:games and no "other" permissions (basically how gentoo
installs games, sort of).  To do this, after I install one or more
games, I run "`gameprep *`" in `~/GOG Games`, and then remove that
directory.

I realize that gog installers can just be unzipped, but I find some
comfort in running the graphical installers, for some reason, even
though mojosetup has some serious deficiencies compared to the Loki
installer it was meant to replace, and also lacks the ads that are in
the Windows installers.

dogame
======
`dogame` grew out of a need, for some games, to turn off edge scrolling
in my window manager (FVWM).  The script still does that (although it
uses a different method), so it's dependent on a window manager
probably used by .1% of Linux users.  You'll have to figure out for
yourself what you want such a wrapper to do.  In particular, if you
use multiple monitors or other such things, I'm sure there are other
things needed.

  - I might move part of the game to an offline drive, so I first check
    if the target directory exists.  If not, notify and exit.

  - If I'm using the touchpad on my laptop, I also use `syndaemon` to try
    and turn it off while typing, normally.  I don't want this for
    games, so I kill it and restart it if it was found running.  I
    really wish somebody made a laptop with the touchpad to the side,
    rather in the center.  Or maybe use something else, like trackpoint.

  - If the game wants it, I change to its directory

  - If the game needs it, I set up two directories in `~/.local/share`:

     - `game-saves/`*gamename* - where to store writable overlay
     - `game-mnt/`*gamename* - where to mount the game + writable overlay

    I then use unionfs to mount the game writable.  Otherwise, the game
    directory will be read-only.  Note that unionfs is a fuse
    filesystem.  This makes it much easier to use than overlayfs.  It
    also means that a bug in gentoo, where `/etc/mtab` isn't a
    symbolic link as it should be, must be corrected in order for
    `fusermount -u` to work.

  - Since I use FVWM with a large virtual desktop (not multiple
    desktops!), this confuses some games.  It also sometimes causes edge
    scrolling to fail.  For both of these reasons, I send a command
    (which requires that the FvwmCommandS module be running) to fvwm to
    reduce the virtual desktop size to 1x1.  I suppress this for things
    known to work without it.

  - I also turn off X's screen saver options.  I don't, however kill
    `xscreensaver` or whater other tricks your system may need in
    addition to turn off screen savers.  Such active "screensavers"
    don't really save LCD screens, and popping processes to the front
    often kills wine full-screen programs.

  - Since setuid programs (like `nonet`) drop `LD_LIBRARY_PATH`, among
    other things, I preserve it if non-empty by prefixing the command
    with "`env LD_LIBRARY_PATH=`...".  The same is done for LD_PRELOAD,
    which is also disabled immediately and then re-enabled for the game
    only.  This avoids some (but not all) spurious errors or other
    issues with loading the preload into every command.

  - I save the game's main process ID in a known location, so that I
    can kill a game using `killgame`.  This is bound to a global key using
    `actkbd`; see below.

  - I save the game's name, if requested, so that a global key config
    can be used to run a script to save the current game.  I use this
    in some games to bypass "ironman mode" or "permadeath".

  - I pop up a notice using `xmessage` if the game wants a joystick, but
    none is found.

  - I run `xboxdrv` using `mkxpad` (see below) if the game doesn't work
    right with my controller.  For now, I expect the game script itself
    to tell me this.  I am slowly replacing this with a new method:

  - I preload `~/lib/joy-remap.so` or `~/lib/joy-remap-32.so` to do
    controller remapping.  This can be used instead of `xboxdrv` for
    a slightly less invasive remapper.

  - I create a directory in `~/.local/share` and use it as `$HOME` for
    games that insist on Windows-like save/config locations.
    Game-private stuff should remain private.  Soft links are made to
    the real `~/.local`, `~/.config` and `~/.mono` dirs.  The contents
    of the subdir the game writes to can be made to be stored in the
    created dir directly by making soft links to `.`.

If `dogame` manages to survive running the game, it will then try to
restore things as they were.  If nothing else, I can run "`dogame true`"
to force the cleanup items to happen:

  - It dismounts the unionfs mount, if present.  It's kind of stupid
    about detecting that, but it never hurt me.

  - I restore the FVWM 3x2 virtual desktop I usually use.  Yeah,
    hard-coded.  I'm not even sure I can obtain the current layout
    easily, so it's too much effort.

  - I restore the screen saver parameters I usually use.  Again,
    hard-coded.  I suppose I could run "xset q" beforehand and restore
    what was there, but I'm too lazy.

  - I restart syndaemon if it was running at the start.

  - In case the game changed screen resolution and didn't restore, I set
    the screen resolution to default.

  - Some games mess with the display gamma, and so I restore it to 1.0.

  - It deletes the saved process ID/game name.

  - It kills `xboxdrv` if it started it, using `mkxpad -r`.

The `dogame` script accepts (and eats) several options before the game
name and its arguments:

   - `-c` -> change to the game's parent directory
   - `-d` -> use next parameter as "executable name" for determination of dir
   - `-g` -> change to game's root directory (assumed to end in /game/)
   - `-w` -> make game's root directory (assumed to end in /game/)
     writable using unionfs; probably needs `-c` as well.
   - `-u` -> dismount writable unionfs mount if still mounted
   - `-j` -> notify user if /dev/input/js0 not present
   - `-J` *sec* -> use ~/lib/joy-remap.so LD_PRELOAD; set section re
     to *sec*
   - `-J32` *sec* -> use ~/lib/joy-remap-32.so LD_PRELOAD; set section
     re to *sec*
   - `-x` *args* `--` -> start up `xboxdrv` supplying extra *args* up to `--`;
     note that `--` is mandatory, even if no extra *args* given.
   - `-h` *name* *dir* -> set $HOME to $HOME/.local/share/*name* and
     link *dir* to that new $HOME.

I generally make short names for all of my games, and create a
launcher script with that name in `/usr/local/bin`.  During
installation, I let the installer create a menu entry (.desktop file
in `~/.local/share/applications`), which I modify to point to
`/usr/local/games` instead of `~/GOG Games`, as well as using my script
instead of `start.sh` (without quotes, as quotes confuse some parsers). 
I move the .desktop file to `/usr/local/share/applications`, owned by
root:games.  A typical Linux game launch script looks like this:

    #!/bin/sh
    exec dogame -c /usr/local/games/Game\ Name/game/Game\ Executable "$@"

As a side note, if you're using FVWM, like me, a recent trend in Linux
native games is to not go full-screen correctly any more.  Many games
continue to display borders, and worse yet, some games not only do
that, but also iconify themselves when they go full-screen (how does
that even make sense?  Probably due to not getting focus when it wants
focus, and thus assuming it's no longer supposed to be displayed). 
The solution is to add styling for each of these pesky games.  I use
the following:

    Style FullScreen PositionPlacement, !Title, !Borders
    Style NoIconify UseStyle FullScreen, !Iconifiable

Then, for each afflicted game, I find out the title (for those that
don't iconify, it's in the title bar, and for those that do, I use
`xwininfo -root -tree` to find it), and add styles for those games,
like so:

    Style "Fell Seal" UseStyle FullScreen
    Style "7 Billion Humans" UseStyle NoIconify

While I'm installing and trying a game for the first time, I can use
`FvwmCommmand` to temporarily issue one of the latter style commands for
testing.  i added flags in my grok database (see below) to track these
pesky games as well.  I currently have 24 games that need FullScreen
and 20 that need NoIconify.

save-slots.sh
=============
I hate permadeath and "iron man" mode.  I tried to genericize adding
multiple save slots as much as possible, and the result is
`/usr/local/share/save-slots.sh`.  It just tars up a directory or file
into another location.  When restoring, the target directory or file is
removed and replaced with the tarball contents.

To use, set some variables and source the script.  The main variable
to set is the directory name, `dir`.  The game script's file name is
used as the base name for the saves (formerly set manually using
`game`).  For example, from my bedlam script:

    dir="$HOME/games/wine/bedlam/users/`id -un`/Application Data/SkyshinesBedlam"
    . /usr/local/share/save-slots.sh

This will add `-r` and `-s` options to the game's command line.  If
`-r` or `-s` is given, the game itself won't be run at all.  Instead,
it either saves or restores game files and then exits.  The flag is
followed by an optional arbitrary string to describe the save slot. 
The saved games are stored in `$HOME/games/saves/$0`*suffix*`-sav.tar.bz2`,
where *suffix* is blank if no slot argument is given, or "`-`*slot*"
if given.

This will also set the magic `save_game` environment variable to
convince `dogame` to add the game name to the pid file so that a
global hot key can create saves at any time.  In fact, there are two
scripts to support this: `save_game` (saves to slot "auto") and
`restore_game` (restores from slot "auto").  After saving, it will
increase an internal slot number by 1.  If the slot number is more
than 0, it will append it to "auto" (e.g. "auto1" for the second
hotkey-save).  When saving for the first time, it tries to use the
most recent save plus one.  In addition, `auto_slot` applies its
argument to calucalte the next save slot (e.g. +1 or -1).  I have
provided an excerpt from my actkbd.conf to show how I use it; using
global window manager bindings or the like will probably be overridden
by the game.  Obviously you will need to edit this to suit your key
preferences, user ID, and login shell.  Note that all three of these
auto-scripts rely on `auto-save-sup.sh`, and I assume all 4 are in
`/usr/local/games/bin`.

Since it's too easy to accidentally overwrite saves due to habit, both
saving and restoring saves backups in `/tmp/save_backups_`*username*.
When saving, if the target save already exists, it is first copied to
that directory with a timestamp added to the file name.  When
restoring, the original save directory's contents are tared up for
backup before wiping it.  This is only meant as an immediate backup;
use multiple save slots to provide long-term history.

Some games (e.g. Darkest Dungeon, which I wrote this for, although I
don't play that game any more) have "multiple save slots" that are
really just multiple single-save-slot playthroughs ("profiles").  I
spent a lot of time trying to figure out how to support this easily.
The solution I came up with works well enough for the games I need it
for that have sane save slots.  To enable profile support, make the
dir be the prefix for slot files, add a plus (`+`) to the start of the
value, and add a `psuffix` variable to complete the slot name.  Slot
files are then `${dir#+}`*slot*`$psuffix`, where *slot* and `$psuffix`
can't contain slashes in the current implementation.  This adds an
optional parameter immediately following -s/-r: `+`*slot*.  If not
given, the first found directory entry matching any slot is saved, and
the restoration replaces the slot that was saved (it's not possible to
find out what slot was saved at present).  If given, the selected slot
is saved, and the restoration is renamed to the given slot.  For
example, here is what Battle Chasers: Nightwar uses:

    # Support saves in profile mode
    dir="+$HOME/games/wine/bcn/users/`id -un`/AppData/LocalLow/Airship Syndicate/BattleChasersNightwar/user/saves/Save0"
    psuffix=.sav

A typical slot-specific save or restore would be:

    # Copy slot 1 to 2
    bcn -s +1 slot1
    bcn -r +2 slot1

The profile mode is pretty stupid and can't deal with many things.
Some will never be supported.  For example, Starbound saves stuff all
over the place, including shared global state among the profiles.  It
also stores the names of available profiles in another file, rather
than just reading the directory, so creating/deleting profiles would
be diffcult.  Furthermore, profile names are hex strings with unknown
properties, so creating new profiles is impossible.  The last two
issues affect the Thea games as well.  Vambrace Cold Soul stores all
of its saves as separate variable assignments in a config file.  It
doesn't appear to be possible to even copy values from one to another
to move slots around or create new slots, and editing a file is beyond
the scope of this utility, anyway.

hacks
=====
I was going to place my original Stars in Shadow hack in a separate
project, but the devs added a similar fix in-game and I dropped it.
From that hack was born a similar hack, ig2-hack.c, for Imperium
Glactica II: Alliances.  I have decided to place that one here instead
of creating a new project for it.  Both games used GLSL features not
available in the version of GLSL advertised in the shaders.  Mesa is
strict about this, so the shaders failed to compile on Mesa.  The hack
simply edits the version before submitting to Mesa.  Compile and usage
instructions are in the top comment block.

dosbox
======
Many games I own come with DOSBox.  I generally run the GOG.com
installer, and then I delete the supplied DOSBox and ignore the config
files in favor of my own configuration.  I have a primitive script,
`dos-gameprep`, similar to `gameprep` above, to run after the game's
installer; it works in "GOG Games" or in wine above the installation
root.  In the case of wine, it also converts the icon and changes the
directory structure to put the game under data, like the Linux
installers do.  Unlike `gameprep`, it can't be run on more than one game
at a time.  Maybe I'll fix that one day.

My DOSBox configuration is the default, plus changes present in
`dosbox-tjm.conf`.  You don't have to change your global config to use
my changes; just launch with "`dosbox -userconfig -config
dosbox-tjm.conf -config `...".  In particular, I use ALSA
virtual MIDI with timidity++ attached (on 128:0).  I also use IRQ 7
for Sound Blaster, and enable GUS emulation on 5.

I currently use dosbox-SVN-4227 with the mt32 patch.  I could've sworn
I had other patches as well (at least for glide and keyboard
injection), but I haven't dealt with that in so long I've forgotten.
In fact, I can't reinstall using the dosbox-9999 ebuild on gentoo any
more, so I should probably investigate why and fix/update it.

Note that I have the GUS installation in `/usr/local/share/dosbox` (I've
no idea where to get it any more, and I'm sure I am not permitted to
mirror it here).  Good luck getting this to work.  The only games I
tried with GUS were worse than just using SB+MIDI.  It's probably best
to just turn it off and change the SB IRQ to 5.

The mt32 patch requires munt (https://github.com/munt/munt; I use
gentoo's 2.3.0 ebuilds) and MT32 ROMs (I've no idea where to get those
any more, and like the GUS stuff, it's probably not freely
redistributable).  For games that support MT32 sound, I soft link the
MT3232_CONTROL.ROM and MT32_PCM.ROM into the game's root directory,
and add the lines from `mt32.conf` to each per-game launcher config.

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
(usually named dosbox*game*.conf) is common settings, and the rest are
small ones for each supported separate launcher.  The first thing I do
is diff the common file (with `diff -wub` for context and removal of
Windows CRLF) with my config, and look for interesting changes.  The
most useful ones are memory and CPU cycles, although the latter can
sometimes be very wrong.  Some experimentation may be necessary.  In
any case, I copy only the relevant changes (similar to how I presented
my config changes above) and I copy them to each created launcher
config.

I copy the launcher configs to *gamename*[*opt*]`.conf`, where
*gamename* is the root directory name chosen by the installer (not the
shortened name used by GOG.com for their config files), and *opt* is
the flag to add to the game to run that config (e.g. `-s` for setup,
or nothing for the main game).  I usually use GOG.com's config files
verbatim, except that I remove useless IPX sections, change "`..`" to
"`.`" (and usually have to move the "`c:`" line above the `imgmount`
line if both are present) in Windows installations (I launch from the
game dir, not a subdir), add the global config changes needed, remove
CR from CRLF files, and adjust the mount command for Windows
installations (backslash to slash and case conversion).

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
run from `C:\` in Linux, so I create a soft link from `.` to `LBA2` and
change paths in the config file to launch from `C:\LBA2` instead of
`C:\`.

Similar to Linux native games, I pick a short name, and write a short
wrapper script with that name to launch the game.  This script always
sources a helper script, `dosbox-game.sh`.  The main purpose of the
helper script is to use unionfs to keep the installed game pristine.
This adds a "`-u`" argument to the game launcher script, which
unmounts the unionfs if it isn't already, and a "`-U`" argument to
just mount the unionfs and exit without launching the game.  The game
script goes in `/usr/local/bin`.  The desktop file is then modified to
use my wrapper script instead of `start.sh` and get the icon from the
new location.  For games that only have Windows installers, I just
fill in a desktop file from scratch, using the icon extracted by
`dos-gameprep`.  The desktop file is then moved to
`/usr/local/share/applications`, owned by root:games.  A typical
script looks like:

    #!/bin/sh
    game=Game\ Name
    . /usr/local/share/dosbox-game.sh

The game should be installed in `/usr/local/games/dosbox/Game\ Name`,
with the actual game in a subdirectory called `data`.  Running the
script without arguments uses `Game\ Name.conf`, and running it with a
single argument uses `Game\ Name`*arg*`.conf`; both of these are in
the `data` subdirectory with the game.

ags scummvm
===========
I use gentoo's scummvm instead of anything that gog packs for most
games that scummvm supports (not the Eye of the Beholder games,
because I think the random drops and some other random things don't
work quite right).  I used to use the gentoo-supplied ags for
Adventure Game Studio games, but recent gentoo shenanigans have made
ags unusable, so I now use scummvm's ags interpreter for them as well.
I stick scummvm games in `/usr/local/games/scummvm`, all with the same
directory structure.  The GOG.com installer's root directory name,
with the game itself usually in a subdirectory (usually `data` or
`game`).  A typical scummvm launcher looks like:

    #!/bin/sh
    cd
    exec dogame scummvm -p /usr/local/games/scummvm/Game\ Name/data gameid

In this case, *gameid* is the scummvm game ID to use to execute the
game.  Finding this out may require looking it up on the 'net, or
using the scummvm configuration GUI to add the game first, and looking
at what that picked.  The `--detect` option may help as well.  For
example, the engine used by Toonstruck is `toon`.

Note that to install, I just use the installer and install it as if it
were a native game, and then I move things around to have the correct
directory structure.  I suppose I should have a `scummvm-gameprep`,
but it's not that much trouble to just move things around.

Some games require additional work, such as Return To Zork; see the
gog forum for details.

infocom
=======
GOG.com doesn't sell many Infocom games, but I have enough that I made
some support for it.  I store the games in `/usr/local/games/inf`.
Unlike the other game types, I do not create subdirectories for each
game.  All related files are renamed to the game name, plus the
appropriate extension (e.g. Planetfall for the data file,
Planetfall.png for the icon, Planetfall.pdf for the manual).  The
script to run the game is a soft link to a common script (`inf-game`)
that just invokes zoom.  The soft link is placed in `/usr/local/bin`,
and, as with other such games, an appropriate desktop file is placed
in `/usr/local/share/applications`.  I use `dos-gameprep` for the Windows
installers to extract the icon, and then delete everything but the
game data (DATA/*.DAT), icon and manuals.  The only game I don't run
in zooom is Beyond Zork, since zoom doesn't support this game.
Instead, I use scummvm.  Technically, I could run all the others in
scummvm as well, but I have not yet made that move.

wine
====
Ah, Windows.  Bane of my (and, really, everyone's) existence.  If I
stuck to my guns and never installed Windows games, though, I'd lose
nearly half my games (I don't count Windows installers for dosbox,
ags, scummvm or Infocom games as Windows games).  I guess I should
just be happy wine lets me run most of them.

I store all of my wine games in their own prefixes, under
`~/games/wine/`*shortname*, using the short name I give to the game
and its wrapper script.  Unlike the other types of games, I store the
wrapper in `~/bin` (which is obviously also in my `PATH`), and the
icons in `~/.local/share/applications`.  This is because they are hard
to make completely system-stored.  I might work on that again some
time in the future, but for now only the bulk of the data is stored in
`/usr/local/games`.

To run an arbitrary Windows executable, I use `dowine`.  This also
supports running `winetricks` and wine-supplied commands like
`winecfg` and `regedit`.  This uses unionfs, but for different reasons
than above. At some point, wine started placing a bunch of files in
the prefix, and I ended up having gigabytes of extra crap due to my
use of separate prefixes.  My first solution was to use symbolic
links. However, some programs (especially `winetricks`) require
writable files, and symbolic links to root-owned files are obviously
read-only.  Also, when I uninstalled things, it left dangling links.
I do things differently now.  I create a "reference" prefix, which is
just an empty, initialized prefix for every version of wine I run.  I
then unionfs-mount that prefix's `windows` directory using a writable
`.windows` over the game's `windows` directory.  This keeps the
file duplication down without the read-only headaches of symbolic
links.  The management of this procedure is one of the tasks of
`dowine`.

Another thing I like to do is set some initial registry entries upon
prefix initialization.  Some of those I probably ought to remove as
they no longer do anything useful, but one thing I set is the GOG.com
installer defaults.  I never create desktop icons and I like to
install in `C:\`.  The former option is ignored by older installers, so
it's always a good idea to press the button to view/verify options
beore installing.

I now also enable gallium-nine-standalone and dxvk by default, by
setting the appropriate registry entries and soft linking the dlls
into the prefix.  For the gallium-nine-standalone, I soft link the
dll.so file directly rather than pointlessly enabling `z:` and linking
it through there, as the configurator program it comes with does.
Something is still creating `z:` even though I patch it out, but
gallium-nine-standalone works fine without it (apparently). For dxvk,
I store it in `~/games/win/dxvk` (actually a soft link to the latest
installed version) and make relative soft links to there. Obviously if
you use this you'll want it to soft link to wherever you installed it.
Note that the use of standalone gallium-nine eliminates my need (and
support) for wine-d3d9 and wine-any, which haven't worked and haven't
been updated in a while, anyway.  For some reason, some 3D games use
neither of these.  I'm not sure why.  Maybe they're OpenGL games
(which still haven't been ported to Linux).

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
the installation: the game.  As such, I make the prefix's
`dosdevices/c:` point to `../..` and make the `WINEPREFIX` actually
reside in *shortname*/windows.  This places all wine's config in the
`windows` directory, where it belongs, as well.

I also may want key rebinding.  If you create a standalone executable
from AutoHotKey, you can call it `wine-keys.exe` and `dowine` will
execute it before the game, and kill it afterwards.  I have included a
few scripts as examples.  `cosh.ahk` is what I use when playing Cosmic
Space Heroine (the game I added this feature for), and `bge.ahk` is
what I use for Beyond Good & Evil (although the latter is mostly
untested).  Both add joystick support (cosh has built-in joystick
support, but it uses wine's mostly broken (at least in 4.0 and before)
SDL joystick support and doesn't support remapping; it's best to just
disable it by adding "`Enable SDL`"=(`REG_DWORD`)`0` to
`HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\WineBus`).  The
samples use several "library" scripts.  `JoystickMouse.ahk` was
downloaded from autohotkey.com and modified to support other axes and
to disable the d-pad mouse wheel. `jscursor.ahk` is an adaptation of
the joystick-to-cursor code on autohotkey.com.  My modifications merge
the left stick and d-pad code into one, and issue multiple keypresses
for diagonals.  Finally, `autofire.ahk` provides a function to map a
single key/button to a single key/button that autofires.  It might be
better to do that with a chord, but I only need it for one key to skip
long conversations in cosh.  Compile in the Compiler subdirectory of
the autohotkey distribution (whose installer doesn't work in wine, but
you can just extract it with 7z) using e.g. "`dowine Ahk2Exe.exe /in
cosh.ahk`".  

Since gentoo has slotted wine, I have multiple wine vesions installed
at once. The `dowine` script supports simple options for version
selection, but they hard-code version numbers.  It should also support
an arbitrary wine executable/version number, but I don't test it, and
it probably doesn't work.  As mentioned above, a separate reference
prefix is created for each version of wine run (32-bit or 64-bit
prefix), and never deleted (except manually).

For a long time, I couldn't get 64-bit prefixes to work at all.  It
tunred out to be because I use the multilib-portage overlay, which
requires an environment override; see `wine-env` (rename this to
`/etc/portage/env/app-emulation/wine` if want to use it; also soft
link `wine` to `wine-staging` and `wine-vanilla` if you use those
ebuilds).

Along the same lines, my use of `windows` as the prefix name triggered a
bug in wine, which has been fixed in 4.6, I think.  Prior to that, I
have to apply `wine-dir64.patch`.  Otherwise, any attempt to run a
GOG.com installer in a 64-bit prefix will fail (as will many other
programs).

As to gecko and mono, I install them in the reference directories, but
do not normally replicate that bit to the games' prefix.  In fact, I
have not found one game that benefits from either of these, and all
mono does is prevent installation of (working) .NET from MS for the
games that need it.  I should probably just stop building wine with
support for them.  In fact, gstreamer seems to cause more problems
than it fixes, so I should probably drop that as well.  Why can't wine
play movies correctly?

Usage:  `dowine` [*options*] *exefile* *args*

To create/use a 64-bit prefix, always set `WINEARCH=win64` in the
environment.  This is done automatically if the prefix is already
initialized.

  - Version selection:
      - `-s` = `-s6` = current stable (6.0.x), which is also the default wine
      - `-s5` = 5.0.x (must be manually linked as wine-5)
      - `-s4` = 4.0.x (must be manually linked as wine-4)
      - `-s3` = 3.0.x (must be manually linked as wine-3)
      - `-s2` = 2.0.x (must be manually linked as wine-2)
      - `-d` = wine-vanilla:  latest vanilla wine
      - `-S` = wine-staging:  latest with staging
      - `-V` *exe* = use exe as the wine executable; probably doesn't work
      - `-D` = use winedbg for debugging; probably doesn't work right
      - `-c` = use wineconsole instead of wine
   - Other:
      - `-k` = kill wine in this prefix
      - `-m` = mount the windows overlay
      - `-M` = dismount the windows overlay
      - `-w` = assume exe will exit before program finishes; use wine-wait
      - `-n` = allow mono/.net install/usage (rarely works)
      - `-g` = allow built-in gecko install/usage (never works)
      - `-b` = change what's installed based on -n/-g flags

Note that `-s`, `-d`, and `-S` rely on using `eselect wine` to set the
system default wine, wine-vanilla (`--vanilla`) and wine-staging
(`--staging`).  The others need symbolic links to `wine-`*major* and
`wine64-`*major* to exist and be manually updated.  After my next
round of testing, I will probably remove versions below current stable
which are no longer used by any installed game, even if an uninstalled
game depended on it.

Note that `wine-wait` just waits for wine to finish instead of killing
wine as soon as the main executable finishes.  It takes an optional
"`-e `*ext*" argument in case you're running non-default wine and the
wineserver has an associated extension.

To install a wine game, I just create `~/games/wine/`*shortname*, soft
link the installer files (`*.{exe,bin}`) from my usual download drive
into that directory, and run `dowine` on all the .exe files, optionally
setting `WINEARCH=win64` first (if it was necessary and I forgot, I
rerun the installation in a fresh directory again with it set).  I
realize that, like the Linux installers, I can extract the game
without using wine (using innoextract).  In addition to feeling better
about using the official installers, there is also something important
missing from innoextract: all the stuff that isn't actual game files.
For example, registry entries and additional commands executed by the
installer.  These are not always necesary, but I'd prefer keeping
them.  This is one of the reasons I try to find out what registry
changes are made by each invocation of `dowine` by saving a .diff file.
Wine is flaky, though, and it often doesn't work.

A wine game uses an additional wrapper, `wine-game.sh`.  This uses an
additional unionfs mount for the game root itself.  The game root is
moved to `/usr/local/games/wine` and owned by root:games; it is then
union-mounted in the wine prefix on `Game\ Name` using `.Game\ Name`
as the writable overlay.  `wine-gameprep` moves the root into place,
and extracts an icon using ImageMagick's `convert`.  This icon is
copied into the prefix game root so that even when it's not mounted
it's in the same place.  Since I have a hard time knowing what version
of a wine game is installed, I also copy the installer links into the
prefix.  In this case, the installers are always symbolic links into
my download directory, so they don't take up any space, and I can tell
by their listing color whether or not they are up-to-date (they are
red if not present in my download directory, and cyan otherwise).
Like `dos-gameprep`, it only runs on one game at a time (and more than
one wouldn't make sense anyway, since I store each game in a separate
root and the script expects the root to be in the current directory).

A typical wine game launcher looks like this:

    #!/bin/sh
    . ${0%/*}/wine-game.sh
    exec dogame dowine -s Game\ Exe.exe

Note that as mentioned above, this script name has the same name as
the wine directory I installed it into; wine-game.sh depends on it,
but you can set groot manually before invoking the script to make it
something else.  I used to install exactly one pair of games in the
same directory, with the second one changing groot, but I don't do
that any more.  Actually, I do this with some other games as well,
but they all launch from the same script with different command-line
parameters, which does not necessitate setting groot.

I used to require setting the `game` variable to the installation
directory name, but that is usually no longer necessary, as the script
will use the only capitalized directory name that doesn't start with
`Program`.  Of course it's perfectly safe to manually set, and if two
installation directories exist, the script will ignore both and die,
so the directory must be set manually.

I always give the wine version flag, even though `-s` is always
optional.  I end up having to retest all my wine games every version
update, and having the version coded like that can help.

Some games have extra parts, like editors.   To deal with that, I just
use a shell case statement on "$1" and exec the appropriate command.

The wine-game.sh script adds a few command-line options to all that use it:

   - `-u` = unmount the game unionfs; it never gets unmounted otherwise
   - `-U` = just mount the game unionfs and exit

grok
====
I used to keep a plain text file, `gog-status`, with notes about all
of my game installs.  When the number of games became large, and I
also added more information to the file, I decided a database would be
better.  To that end, I experimented with some tools and ended up
making a Qt port of `grok`, available [here as well](
https://bitbucket.org/darktjm/grok).  I have included my grok database
definition and templates under the `grok` directory; if you want to
use them or try them out, copy them to `~/.grok` first.  I have also
included my current database, woefully in need of updating. (I am in
the process of once again checking/updating all games' entries).  Note
that I may have a few changes to grok I have yet to check in at the time
of this writing, and have discovered new bugs that I have yet to even
document.  Use at your own risk.  Maybe I'll switch to
libreoffice-base or kexi and abandon my work on grok some day, but
it's all just too much trouble.

A quick note on the templates: `gcs` outputs to a native GCStar
database I used before I switched to grok.  `sql-gcs` exported to the
same format exported by GCStar's SQL exporter, although I'm not sure
what the point was, since GCStar can't import that format.  Both of
these are broken now due to my switching of DLC info to a separate
databse, and of course missing new fields.  Finally, `summary` is an
attempt at maintaining `gog_status`' global information more easily,
and has the advantage of having some of its info read from the
database itself.  This is my current output from `summary`; judge for
yourself if I have too many games (I do):

    258 Linux games (10 nonfunctional; 2 converted to wine; 2 uninstalled)

    111 DOS games (1 uninstalled, 11 w/ Amiga alternates)
      note:  bt1-3 are hidden in a Linux game; this accounts for 3 more Amiga games

    485 Wine games (56 nonfunctional; 2 converted from Linux; 2 uninstalled)
      4 non-GOG

    57 hidden games (20 Linux, 30 Windows, 7 DOS)
      15 because they are superceded by later games
      26 because I don't want to play them
      16 because they are demos and I either got the main game or don't want to
           note that some demos I keep because they are prologues

    Currently 57 unusable Wine-only games:
       bg2 bg1 bgtutu cmbo cons crt1 crt2 crt3 crt4 crys  deusex dist fench
       fenchlh fallh fo3 gciv inst idng lohtocs legr mh mirror mi2 pc phantd
       prod ppin rtk smbtas smstw shaw civ3 spra smb smfffd smsoc smp tgr hom
       lohtocs2 tb tl txex tr6 tron2 2w2 underrail ven vikw wh40kcg wh40krow
       wots4 x2 xnext xcom2
      [note: in my recent game retest, I skipped 5 or so games and just marked
       them broken for now, to be processed later]
      [note: I am way behind in retesting again especially given wine-5.x]

    Currently 2 unusable Linux games: (at least barely usable in wine)
       trine2 trine3
    Currently 8 games unusable in both Linux and Wine
       halcyon hofate mable obs bt4 unmech vran
       werewolf_the_apocalypse_heart_of_the_forest_demo

    Currently 26 games with serious movie playback issues
       anb bloodrayne rayne ctp2 cors mfc2 darkstone ga gmast gothic inq kq8
       konung2 konung msn mh nwn omikron sacred silver kotor txex tr6 2w u9
       xnext
    Currently 5 games with no music (not sure if Konung ever had any)
       bast konung2 konung se4 war3
    Currently 16 games with broken editors
       aowp divos dao elvenlegacy etherlords2 eu2 grimd homm5 homm5-toe ja2ub
       eisen nwn2 spellforce witcher2 titq torch

    Note: the following have 3rd party Linux ports, so no point in messing
    with Windows (except nwn editor, and ctp2 until movies work):
       aoc2 dk diablo eadorg ehtb expconq fbk fs2 ja2 nwn oriente po morrowind
       ultima_iv exult exult xcomtftd xcomud

The `list` template is used by the `chklst` tool to verify that I have
updated the database after mass updates/installs.  The tool's `-t`
option generates a grok template, which can be executed by copying the
template to the database template directory (`~/.grok/gog_games.tm/`)
and then exporting using that template while in the GUI.  Doing so on
the command line discards all changes.  After executing the template
this way, save the result.  Keep in mind that the tool requires manual
intervention for version number changes, since it extracts them from
the file name (rather than the `gameinfo` file(s)).  In fact, it is a
good idea to manually edit the result to remove spurious errors (e.g.
due to stupid version number extraction).  The tool obviously also
expects my own way of storing games in `/usr/local/games`, with soft
links to the installers in `/mnt/usb3f/gog`.  The `chklst` tool also
checks some other things, like missing executables and icons.  To help
with building those missing icons, the `wicon` template, combined with
the `mkwi` tool generates them from the database and tries to set the
icon path correctly, at least according to my way of arranging things.
I fix up the generated desktop files for other game types as well, if
the installer didn't generate one itself.  The `fixcat` tool extracts
category errors from the `chklst` output and updates the desktop files
as well.  Both tools expect me to write the results of `chklst` to
`~/c-t`, as well as relying on my way of storing things.

ds4
===
I use a DualShock 4 controller now.  I used to use various cheap
knockoffs, but they tend not to support vibration, LEDS, etc. at all
on Linux, and have become harder and harder to reverse engineer.  I
just wish I had thought of that earlier, when there were still
DualShock 3s around, since I prefer not to have the touchpad, which
makes the Start and Select (renamed to Share and Options) buttons hard
to press.  The main issue I've found so far with it are the countless
games that make assumptions about button mappings and the fact that
Unity3D games don't like the extra devices created by Linux for the
motion sensors (and maybe touchpad).  For the motion sensors only, I
run the `ds4-nomo` command before the game, manually.  I'll probably end
up adding that to `dogame` as well.  It's not like I use the motion
sensors for anything, anyway.  If the pad really causes issues,
there's also `ds4-nopad`.

I also use a script to disconnect the ds4 immediately when I'm done
with it: `ds4-down`.  I also wrote a script to use with xfce4's generic
monitor, which just displays text output of a command repeatedly:
`ds4-power`.  This displays the current charging status.

I made my own uinput-based pad remapper for this, because my old
`input-kbd` based evdev remappings didn't work on the ds4 for some
reason (but they did work on all my old controllers, and seem to work
again now), and tracking down that reason seemed harder than just
writing a uinput driver.  However, as I started adding more features, I
felt that an LD_PRELOAD-based driver would be more appropriate, so I
decided to abandon it for now.  In fact, I found two alternatives that
don't require that I actually maintain them myself: `MoltenGamePad`
and `xboxdrv`.  Maybe some day I'll resume work on an LD_PRELOAD-based
remapper, but for now, I'm out of energy and either of these will
probably do.

Note that I have recently restarted an LD_PRELOAD-based remapper
(although not exactly what I envisiaged when I wrote the line above).
It's in this project as joy-remap.c, and the top documentation block
describes its usage.  I have replaced all of my uses of `xboxdrv` with
this except for one (a Java-based game which mysteriously crashes with
joy-remap.so, and, as usual, there's nobody I can ask about why it's
broken, and the crash can't even be caught by gdb).  It's easy enough
to configure and may be faster than any uinput-based solution since it
doesn't go though an extra driver layer.  It's only real advantages
are that it's temporary (only applies to the program it's preloaded
into) and that it doesn't require root (which uinput doesn't, either,
if configured via udev to be that way, but I don't want a user to be
able to create arbitrary input devices).  It does nothing to prevent
occasional disconnects mid-game like my ds4 does; that will require
uinput.  Rather than use one of the below uinput drivers, I will
probalby write a simplified one that does nothing but make devices
persistent, since Linux apparently has no other way of doing it.  Then
again, maybe I'll just live with it the way it is.  Maybe LD_PRELOAD
wasn't the right path, after all, although it's the only way I'll ever
make a remapper that can also do macros based on audiovisual changes
in the program (part of my original vision, which, as I said, isn't
even remotely there yet).

[MoltenGamePad](https://github.com/jgeumlek/MoltenGamepad/) seemed
pretty nice on first look, but its only advantage over `xboxdrv` is
that it supports hot plugging for event devices.  In exchange, it's
harder to configure, build and use.  It also hasn't been updated in a
year, making me wonder if it's abandoned.  I will not be attempting to
use this any more.

[xboxdrv](htts://xboxdrv.gitlab.io/xboxdrv.html) does support autofire
and macros, is packaged by my distro, and may be a better choice
overall.  Its main deficiencies are lack of support for multiple input
devices on one virtual controller and rumble support for generic evdev
devices.  The latter is easy enough to patch in: see
`xboxdrv_evff.patch`.  It also doesn't do anything to remove existing
device nodes, so that has to be done manually in a wrapper.  I have
included my ds4 configuration (`xboxdrv-ds4.conf`); used as follows:

    sudo xboxdrv -s --evdev /dev/input/event13 -c /etc/xboxdrv-ds4.conf &

I have also written a small wrapper script, `mkxpad`, which isn't
nearly as generic as I'd like.  I originally made it as a wrapper that
started a passed-in command and restored things when done, but I
decided to instead run the entire script as root and call it once to
set up and once to restore.  To set up, pass additional `xboxdrv`
arguments (e.g. `--trigger-as-button`) to `mkxpad`.  To restore, pass
only `-r`.  The device nodes are physically removed on startup, and restored
with `-r`.  I'd prefer to use `udevadm trigger` to restore the device
nodes, but I've never gotten that to work.  Instead, I use `udevadm
test` to try and fake it.  I originally just changed ownership to
prevent reading, but AER, the game I originally started this remapping
mess for, crashes silently if it can't read an event device.  As a
side note, `MoltenGamePad` "auto-removes" devices via permissions
changes, as well, so it probably isn't compatible with AER, either.

For a while, my ds4 was extremely flaky.  It wouldn't connect just
by pressing the button; I had to set it to pairing mode and use a
manual connect (assuming it's already been paired).  The new and
improved bluez only logs info if run via systemd now, though (I think;
I haven't looked at it long enough to tell), so I can't figure out
what's wrong.  Pressing the reset button does nothing, at the very
least.  I used the `ds4-connect` script while the ds4 is in
pairing mode to do the connection; it's better than nothing.  In the
mean time, it's started working correctly again.  Why?  Who knows?
Whom can I even ask about such things?  Most people couldn't even get
this far.

Emulation
=========

Emulation is a tricky subject, and one I'd rather avoid.  Recently,
though, I accidentally deleted all my Amiga-related scripts and
reference installs (the only stuff on my machine I explicitly never
backed up, being exlcuded by both my "no games" pattern and my "no
unfinished games" patterns, but luckily I still had some old versions
from when I didn't make it part of my whole game archive), so I
decided to move some of that stuff here. Don't even ask me where to
get ROMs or disk images or whatever. For DreamCast games I just use
`reicast`'s built-in GUI.  The same applies to `dolphin` and GameCube
games.  The only thing I add to either of these is to run them under
`dogame` (since both do untoward network accesses).  I'm still working
on making other emulators more convenient for me as well, but I
haven't touched that stuff in over a decade.

Amiga
-----

For the Amiga, I use `fs-uae`.  It works well enough without much
hassle, although the documentation is on-line only and awful.  Much of
my time getting things set up involved figuring out how to configure
the damn thing.  At one point Gentoo just decided to drop GUI support
(and therefore the ability to e.g. change floppies) for `e-uae`, so I
switched immediately to `fs-uae`.  Years later, Gentoo finally switched
as well.  Note that I'm just describing my standalone games; I also
use UAE/`fs-uae` for emulating my old Amiga pretty much as I was using
it back in the early 90s.

I use the `fs-uae.conf` here as my base config for games.  Note that
ROMs are someting you'll have to figure out for yourself.  Likewise,
`min-wb` is not something I'm providing here:  It's a minimal
Workbench that just boots to CLI and allows me to do basic tasks.  It
also contains UAEQuit in `c`.  This is how I end games.  Where to
get it is not entirely clear, but it's basically part of UAE, which
just calls the UAE resident resource to shut down the emulator.  The
configuration always mounts min-wb as dh1:, bootable at priority 10.

I use `amiga-floppy` *images* to run games on ADF floppy images.  This
is mainly to test if a game image I have works at all.  I use
`amiga-load` *drive-name* *images* to boot the min-wb drive as dh1:
with floppies pre-mounted. This is what I use to install games.  The
basic procedure I usually use is `copy df0: dh0: all clone quiet` and
repeat for each floppy.  Then I exit the emulator, and edit
`s/startup-sequence` on the new drive to add `dh1:c/UAEQuit` at the
end and remove any running in background or text printed to the
initial console.  I might also have to use `dh1:c/assign` to assign
the floppy names (gotten from `info` or the prompts asking to insert a
floppy) to dh0:.

An Amiga game then uses `amiga-game.sh` to load.  This is almost
identical to `dos-game.sh`; see above.  I should probably merge the
two, but I'm too lazy right now.  The main differences are:

  - `$game` defaults to the script name; I will probably remove this soon.
  - The overlay directories are prefixed with ami-
  - Launching the game is obviously different.

Games are stored in `/usr/local/games/ami/$game`, which is the root of
a filesystem-style hard drive image.  The script just sets dh0: to the
game root, bootable at higher priority (11) than dh1: (10), and runs
the emulator full-screen.  If there is a file in the game root called
`fs-uae-extra.conf`, its contents are added to the `fs-uae` command
line, with a few extras.  Blank lines or lines whose inital
non-whitespace character is `#` are ignored.  If a line is bracketed
by `<` and `>`, it is replaced by the contents of the file named in
the brackets.  The file is first looked for relative to the drive
root, and then to `/usr/local/games/ami` (see `a500.conf`
`16-bit.conf` and `68010.conf` for examples).  The initial `--` of an
option may be skipped, and if it is skipped, the equals sign may also
be separated by whitespace.  Multiple options are permitted on a line,
but not mixing of skipped `--` and unskipped `--`.  Whitespace on
lines with multiple words starting with `--` (or with no leading `--`)
are assumed to be separate arguments; use a single argument on a line by
itself to preserve whitespace in the argument.

Playstation
-----------

I use wrappers for Playstation 1 and 2 games that provide per-game
configuration and per-game memory cards (since the emulators don't,
really).  They also provide a convenient (for me) way to select a game
to play.  To list all images, run the script.  To select one from a
GUI (requires `zenity`), run with just `-g` as an argument.  To run an
image, run the script with a (quoted, obviously) shell pattern (with
optional `^` or `$` to anchor beginning/end).  If more than one
matches, the selection GUI (again, using `zenity`) will pop up to
choose which you meant; use `-l` before the pattern to just get a
listing instead. To change global configuration, just run the emulator
itself.  To change per-game configuration, append `-cfg` to the
command to normally run the game.  You have to be careful not to edit
the configuration of games while running them, since that may or may
not actually edit the global config.  Since both scripts use the same
code to do the image file selection, I extracted that bit to
`/usr/local/share/img-game.sh`.

The PS1 script is `psx` for which I use `pcsxr` (PCSX Reloaded).  I
used to use my own custom version with improved JIT and other fixes,
but I don't care any more now and just run Gentoo's current ebuild
(1.1.94, I guess).  Among other things, the JIT in that version
doesn't work at all any more.  Luckily the performance of my own
machine has improved since the time I cared, so JIT isn't vital any
more.  There are still slowdowns in some places, but whatever.

The PS2 script is `ps2` for which I use `PCSX2`.  I have very little
experience using this, as the emulator only recently has become usable
for me.  My main issue with it at the moment is that it crashes
randomly, especially when it slows down; for now, the only "fix" is to
press F1 (save state) frequently, and use `ps2-rec` to recover from
the backup when it crashes while saving the state.  Then again, the
GUI does provide an alternate way to load the backup, so I haven't run
`ps2-rec` in a long time.

Nintendo Handhelds
------------------

Like the Playstation games above, I use `gb` and `ds` to launch games
for the Nintendo GBA, GBC, GB, and DS.  The scripts don't support
per-game configuration or memory cards, but they do support using
patterns to list and select games.  I use `VBA-M` for non-DS games and
`desmume` for DS games.  I haven't played any in many years, so the
scripts are probably broken.  In particular, I used my own wx port of
`VBA-M` (and have disassociated myself from that project for a number
of reasons) which doesn't work any more now that I lost most of my
patches and can't rebuild, and the upstream version doesn't work the
way I want it to any more, either.  I'll probably just switch to
`mednafen` or something like that if I ever play a game again.  It's
not like any of my save states are any good any more, anyway.  I'd
rather just rewrite my own emulators from scratch (that goes for the
Playstation emulators as well), but I don't have the motivation for that
any more.

License
=======

Everything that came from me is in the public domain.  This includes
my changes to GPL'ed code, as I am not distributing the GPL'ed code
itself.  I used to care about accreditation, but now I only ask (but
have no way of enforcing) that you do not accredit to me something I
did not do (i.e., if you change it, it's yours) and that you do not
accredit to yourself something you didn't do.  I'd rather you just not
accredit to anyone at all.  Do this as a courtesy, not because I force
you to.
