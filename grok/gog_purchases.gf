grok
name       gog_purchases
dbase      gog_purchases
cdelim     ,
syncable   0
size       434 124
query_s    0
query_n    Unused
query_q    {Z="x";I=_title;@"gog_games":foreach("{_bought==I}","Z=''");@"gog_dlc":foreach("{_bought==I}","Z=''");Z}
query_s    0
query_n    Unusedf
query_q    (!referenced)

item
type       Input
name       title
pos        12 12
size       408 28
mid        52 28
sumwid     60
column     0
search     1
label      Title
ljust      1
lfont      0
maxlen     1000
ifont      0

item
type       Time
name       pdate
pos        12 48
size       152 28
mid        52 28
sumwid     8
sumcol     1
column     1
defsort    1
timewidget 2
label      Date
ljust      1
lfont      0
ifont      0

item
type       Number
name       price
pos        172 48
size       136 28
mid        52 28
sumwid     8
sumcol     2
column     2
search     1
timewidget 2
label      Price
ljust      1
lfont      0
range      -1000 1000
digits     2
ifont      0

item
type       Referers
name       item3
pos        316 48
size       56 28
mid        52 28
sumwid     60
column     0
timewidget 2
label      Games
ljust      1
lfont      0
maxlen     1000
ifont      0
fk_db      gog_games
nfkey      1
fkey       bought
_fk_key    1

item
type       Referers
name       item3
pos        376 48
size       40 28
mid        40 28
sumwid     60
column     0
timewidget 2
label      DLC
ljust      1
lfont      0
maxlen     1000
ifont      0
fk_db      gog_dlc
nfkey      1
fkey       bought
_fk_key    1

item
type       Input
name       orderno
pos        12 84
size       408 28
mid        68 28
column     3
search     1
label      Order #
ljust      1
lfont      0
maxlen     16
ifont      0
