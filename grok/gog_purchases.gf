grok
name       gog_purchases
dbase      gog_purchases
cdelim     ,
syncable   0
size       443 89
query_s    0
query_n    Unused
query_q    {Z="x";I=_title;@"gog_games":foreach("{_bought==I}","Z=''");@"gog_dlc":foreach("{_bought==I}","Z=''");Z}

item
type       Input
name       title
pos        12 12
size       424 28
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
name       date
pos        12 48
size       136 28
mid        52 28
sumwid     8
sumcol     1
column     1
timewidget 2
label      Date
ljust      1
lfont      0
ifont      0

item
type       Number
name       price
pos        156 48
size       136 28
mid        52 28
sumwid     8
sumcol     2
column     2
timewidget 2
label      Price
ljust      1
lfont      0
range      0 inf
digits     2
ifont      0

item
type       Referers
name       item3
pos        300 48
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
fk_db      gog_games.gf
nfkey      1
fkey       bought
_fk_key    1

item
type       Referers
name       item3
pos        364 48
size       56 28
mid        52 28
sumwid     60
column     0
timewidget 2
label      DLC
ljust      1
lfont      0
maxlen     1000
ifont      0
fk_db      gog_dlc.gf
nfkey      1
fkey       bought
_fk_key    1
