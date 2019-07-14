grok
name       gog_games
dbase      gog_games
comment    My GOG Games
cdelim     ;
syncable   0
size       732 679
autoq      0
help	'Here's some bloody help text.
help	'Enjoy it.
query_s    0
query_n    Visible
query_q    (!#_hidden)
query_s    0
query_n    TODO
query_q    (#_todo>0)
query_s    0
query_n    Installed
query_q    (!#_hidden && !_sn_uninstalled)

item
type       Input
name       name
pos        12 12
size       700 28
mid        92 28
sumwid     55
sumcol     5
column     1
search     1
defsort    1
label      Name
lfont      0
ifont      0

item
type       Input
name       cmd
pos        12 48
size       224 28
mid        92 28
sumwid     15
sumcol     4
column     3
search     1
label      Command
lfont      0
maxlen     30
ifont      0

item
type       Input
name       id
pos        244 48
size       468 28
mid        72 28
column     0
search     1
label      GOG ID
lfont      0
skip       (len(_id)>0)
maxlen     70
ifont      0

item
type       Input
name       dir
pos        12 84
size       460 28
mid        92 28
sumcol     1
column     2
search     1
label      Install Dir
lfont      0
ifont      0

item
type       Number
name       inst_size
pos        476 84
size       236 28
mid        108 8
column     5
label      Install Size (K)
lfont      0
gray       (_sn_uninstalled)
ifont      0

item
type       Input
name       ver
pos        12 120
size       224 28
mid        92 8
column     7
label      Version
lfont      0
maxlen     40
ifont      0

item
type       Menu
name       cmdtype
pos        248 120
size       192 28
mid        44 28
column     4
search     1
label      Type
lfont      0
nmenu      6
menu       Linux
_m_code    Linux
menu       Windows
_m_code    Windows
menu       DOS
_m_code    DOS
menu       ScummVM
_m_code    ScummVM
menu       AGS
_m_code    AGS
menu       Infocom
_m_code    Infocom
ifont      0

item
type       Input
name       cat
pos        448 120
size       260 28
mid        68 8
column     39
label      Category
lfont      0
dcombo     2
maxlen     40
ifont      0

item
type       Number
name       setup_size
pos        12 156
size       228 28
mid        92 8
column     6
label      Downld Size
lfont      0
maxlen     32
ifont      0

item
type       Input
name       hidden
pos        244 156
size       464 28
mid        60 8
sumwid     1
sumcol     3
column     8
search     1
label      Hidden
lfont      0
nmenu      3
menu       WONTPLAY
menu       OBSOLETE
menu       DEMO
dcombo     2
ifont      0

item
type       Note
name       dlc
pos        12 192
size       340 112
mid        80 24
column     9
search     1
label      DLC
lfont      0
maxlen     999
ifont      0

item
type       Note
name       todo
pos        356 192
size       348 112
mid        80 24
column     10
search     1
label      TODO
lfont      0
maxlen     4096
ifont      0

item
type       Note
name       inst_notes
pos        12 312
size       340 112
mid        80 24
column     11
search     1
label      Install Notes
lfont      0
maxlen     999
ifont      0

item
type       Note
name       game_notes
pos        356 312
size       348 112
mid        80 24
column     12
search     1
label      Game Notes
lfont      0
maxlen     999
ifont      0

item
type       Flags
name       
pos        12 428
size       692 108
mid        108 28
column     0
nosort     1
label      Short Notes
lfont      2
mcol       1
nmenu      19
menu       3rd Party Linux
_m_code    1
_m_name    sn_3plinux
_m_column  16
menu       Amiga
_m_code    1
_m_name    sn_amiga
_m_column  17
menu       Broken
_m_code    1
_m_codetxt *
_m_name    sn_broken
_m_column  18
_m_sumwid  1
menu       Editor
_m_code    1
_m_name    sn_editor
_m_column  19
menu       Finished
_m_code    1
_m_name    sn_finished
_m_column  20
menu       Open Source
_m_code    1
_m_name    sn_source
_m_column  26
menu       Linux Broken
_m_code    1
_m_name    sn_linuxbroken
_m_column  21
menu       Mods
_m_code    1
_m_name    sn_mods
_m_column  22
menu       Bad Movies
_m_code    1
_m_name    sn_nomovies
_m_column  23
menu       Slow
_m_code    1
_m_name    sn_slow
_m_column  25
menu       Uninstalled
_m_code    1
_m_codetxt U
_m_name    sn_uninstalled
_m_column  27
_m_sumcol  1
_m_sumwid  1
menu       Editor Broken
_m_code    1
_m_name    sn_editorbroken
_m_column  31
menu       Bad Music
_m_code    1
_m_name    sn_nomusic
_m_column  32
menu       Brittle
_m_code    1
_m_name    sn_brittle
_m_column  33
menu       Win Editor
_m_code    1
_m_name    sn_wineditor
_m_column  35
menu       Needs FullScreen
_m_code    1
_m_name    sn_badfullscreen
_m_column  36
menu       Iconifies FullScreen
_m_code    1
_m_name    sn_iconifies
_m_column  37
menu       Controller
_m_code    1
_m_name    sn_controller
_m_column  38
menu       Vibration
_m_code    1
_m_name    sn_vibration
_m_column  24
ifont      0

item
type       Input
name       wine_ver
pos        12 540
size       316 28
mid        104 24
column     13
search     1
label      Wine Version
lfont      0
invis      (({"Windows"!=_cmdtype})&&!_sn_wineditor&&!_sn_linuxbroken)
dcombo     1
maxlen     32
ifont      0

item
type       Flag
name       ws_dxvk
pos        328 540
size       120 28
mid        108 28
column     28
nosort     1
code       1
label      DX11/dxvk
lfont      0
invis      (({"Windows"!=_cmdtype})&&!_sn_wineditor&&!_sn_linuxbroken)
ifont      0

item
type       Flag
name       ws_dx11_disable
pos        448 540
size       136 28
mid        108 28
column     29
nosort     1
code       1
label      DX11 Disabled
lfont      0
invis      (({"Windows"!=_cmdtype})&&!_sn_wineditor&&!_sn_linuxbroken)
ifont      0

item
type       Flag
name       ws_64bit
pos        584 540
size       120 28
mid        108 28
column     30
nosort     1
code       1
label      64-Bit
lfont      0
invis      (({"Windows"!=_cmdtype})&&!_sn_wineditor&&!_sn_linuxbroken)
ifont      0

item
type       Flag
name       ws_g9
pos        328 572
size       120 28
mid        108 28
column     34
nosort     1
code       1
label      Gallium-9
lfont      0
invis      (({"Windows"!=_cmdtype})&&!_sn_wineditor&&!_sn_linuxbroken)
ifont      0

item
type       Flag
name       ws_nogallium9
pos        448 572
size       136 28
mid        108 28
column     40
nosort     1
code       1
label      G9 Disabled
lfont      0
invis      (({"Windows"!=_cmdtype})&&!_sn_wineditor&&!_sn_linuxbroken)
ifont      0

item
type       Flag
name       ws_dotnet
pos        584 572
size       120 28
mid        108 28
column     41
nosort     1
code       1
label      .net/mono
lfont      0
invis      (({"Windows"!=_cmdtype})&&!_sn_wineditor&&!_sn_linuxbroken)
ifont      0

item
type       Input
name       config_loc
pos        72 604
size       632 28
mid        132 24
column     14
search     1
label      Config Location
lfont      0
invis      (({"Windows"!=_cmdtype})&&!_sn_wineditor&&!_sn_linuxbroken)
dcombo     1
maxlen     128
ifont      0

item
type       Input
name       save_loc
pos        72 635
size       632 28
mid        132 24
column     15
search     1
label      Save Location
lfont      0
invis      (({"Windows"!=_cmdtype})&&!_sn_wineditor&&!_sn_linuxbroken)
dcombo     1
maxlen     128
ifont      0
