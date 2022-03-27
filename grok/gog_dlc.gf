grok
name       gog_dlc
dbase      gog_dlc
comment    My GOG.com DLC
cdelim     ,
syncable   0
size       457 206

item
type       Input
name       title
pos        8 12
size       436 28
mid        68 28
sumwid     40
sumcol     2
column     0
search     1
label      Title
lfont      0
ifont      0

item
type       Input
name       id
pos        8 48
size       260 28
mid        68 28
column     4
search     1
defsort    1
timewidget 2
label      Full ID
lfont      0
ifont      0

item
type       Input
name       ver
pos        276 48
size       168 28
mid        60 28
sumwid     10
sumcol     3
column     2
search     1
timewidget 2
label      Version
lfont      0
ijust      1
ifont      0

item
type       Reference
name       game
pos        8 84
size       260 28
mid        68 28
sumwid     20
sumcol     1
column     5
search     1
timewidget 2
label      Game
lfont      0
ifont      0
fk_db      gog_games
nfkey      2
fkey       id
_fk_key    1
fkey       name
_fk_disp   1

item
type       Number
name       dlsize
pos        276 84
size       168 28
mid        60 28
column     3
search     1
timewidget 2
label      DL Size
lfont      0
range      0 9007199254740991
ijust      1
ifont      0

item
type       Reference
name       bought
pos        8 120
size       296 76
mid        68 28
column     1
search     1
timewidget 2
label      Bought
lfont      0
ifont      0
fk_db      gog_purchases
fk_header  1
nfkey      3
fkey       title
_fk_key    1
_fk_disp   1
fkey       pdate
_fk_disp   1
fkey       price
_fk_disp   1

item
type       Radio
name       type
pos        308 120
size       132 76
mid        60 28
sumwid     6
sumcol     4
column     6
search     1
timewidget 2
lfont      0
nmenu      3
menu       Expansion
_m_code    X
_m_codetxt Expansion
menu       Extras
_m_code    G
_m_codetxt Extras
menu       Both
_m_code    GX
_m_codetxt Both
ijust      1
ifont      0
