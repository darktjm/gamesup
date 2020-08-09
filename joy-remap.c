/*
 * LD_PRELOAD shim which remaps axis and button events for gamepad event
 * devices.  Use jscal to do it for js devices.  Since it's an LD_PRELOAD,
 * it doesn't need root, and it doesn't deal with problems using uinput.
 * It's very simplistic, intended to map exactly one controller to what
 * one program expects to see.  It really doesn't support hot-plugging,
 * although it might work.  Internal support for hot-plugging, or at least
 * persistence in the face of temporary disconnects, really requires uinput,
 * since every possible use of the file descriptor would otherwise have to be
 * intercepted.  It doesn't support multiple devices, although
 * it wouldn't be hard to support later on.*  It is also unable to merge
 * multiple devices into one, such as mapping the Dualshock 3+ motion
 * sensors into the main controller.  It also doesn't support adding
 * autofire and chording.  Since it happens at the user level, js devices
 * associated with the same gamepad will not be affected; again, use jscal
 * for that (sort of) or one of the uinput-based remappers.  Since a few
 * games use the device name to decide mapping, this does support that,
 * and only that, for js devices.  Really, given that js devices have never
 * supported force feedback, and likely never will, new programs should not
 * be using them, in the first place.  Of course Linux doesn't exactly make
 * it easy to figure out which event device(s) to use, either.
 *
 * * one way to support multiple controllers right now would be to compile
 *   two copies of this shim, using different shim names and different
 *   environment variable names, and use them both.  Not guaranteed to work.
 *
 * Some messages are normally printed to stderr.  In order to catch them
 * even if the program redirects stderr, or if you just want them stored
 * elsewhere, set EV_JOY_REMAP_LOG to something else.  To hide, just set
 * to /dev/null.  To see even if redirected, set to /dev/tty.
 *
 * There are many ways an event device can be accessed.  Following the
 * open, the only methods supported are read and ioctl.  The actual
 * method of opening is expected to be open/open64, finished by close.
 * The CAP_* defines below can also be used to enable other methods:
 * openat/openat64, fopen/fclose, and syscall(open,openat).  There are
 * also many ways this entire shim can be disabled.  For example:
 * explicit symbol lookups from libc, forking after unsetting LD_PRELOAD,
 * use of an unsupported access method, and explicitly dlopen()ing libc.
 * In fact, one game I have does the latter, which can be disabled by
 * placing a dummy, empty shared object file named libc.so.6 somewhere
 * the game can find it (hopefully this needn't be in its LD_LIBRARY_PATH).
 *    gcc -shared -x c /dev/null -o libc.so.6 -Wl,--as-needed
 *
 * This uses a configuration file, with one directive per line.  Blank
 * lines and lines beginning with # are ignored, as is initial and trailing
 * whitespace.  The directives are case-insensitive keywords, followed by
 * (skipped) whitespace, followed by a parameter, where needed.  The file
 * name is specified by the environment variable EV_JOY_REMAP_CONFIG.  If
 * that environment variable is missing or blank, it uses ev_joy_remap.conf
 * in the current directory, ~/.config, or /etc, whichever is found first.
 * Failure to find any file, or failure to parse the file it found will
 * result in an error message and no attempt at device interception.
 * If the configuration contains multiple sections, the regular expression
 * in EV_JOY_REMAP_ENABLE enables a subset of sections (all are enabled
 * if not present or blank).  The last section which matches a device
 * applies for capture, and disabled devices are the intersection of all
 * enabled sections' disabled devices (i.e., if any section enables it,
 * it is enabled).
 *
 * Keywords are:
 *
 * section <name>
 *   If present, all configuration prior to this line, if any, belongs to
 *   the previous section (unnamed if no section keyword given), and any
 *   subsequent configuration, up to the next section keyword, belongs to
 *   the section with the given name (which must be non-blank).  If this
 *   name is repeated, the subsequent configuration will override
 *   previous configuration for the same-named section.
 *
 * use [<name>]
 *   Copy config from given (un)named section.  Note that future changes
 *   to the given section are not followed.  This must be the first keyword
 *   in a section, and may only be used once in a section.
 *
 * match <pattern>
 *   This mandatory keyword is a pattern to match the device name to
 *   intercept, as a POSIX case-sensitive extended regular expression
 *   (regex(7)).
 *   The pattern is matched against both the device name and a pseudo name
 *   generated from the device ID (capital 4-digit hex for all fields but the
 *   last, which is decimal):
 *     <bus type>:<vendor id>:<product id>:<version>-<event device #>
 *   Only the first accepted match is remapped.  Note that if an application
 *   supports hot-plugging, and the old device is not closed before opening
 *   the new one, the new device will not be remapped.
 *
 * filter
 *   If present, this keyword indicates that all non-matching event devices
 *   should be hidden from the application.  Unlike remapping, any matching
 *   device is unfiltered.
 *
 * reject <pattern>
 *   This regular expression specifies names to reject from "match".  If
 *   either name matches the reject pattern, it is rejected.  Otherwise, if
 *   either name matches the accept pattern, it is allowed.  Otherwise, it
 *   is rejected.
 *
 * name <name>
 *   Replace the advertised name of the device.  There is no way to change
 *   the version number right now, as I don't know of any software that
 *   depends on the version.
 *
 * jsrename <pattern>
 *   This is a match pattern for joystick device names (/dev/input/jsX).
 *   Joystick devices do not advertise the device ID, so it matches the
 *   name only.   The first device opened which matches this name will
 *   be renamed by the name directive above.  Any other changes, such
 *   as button and axis remapping, should be done using jscal.  This will
 *   likely always be a dummy pattern, such as . or just blank.
 *
 * id <idcode>
 *   Replace the advertised ID of the device.  The idcode must be of the
 *   form <bus type>:<vendor id>:<product id>:<version> with each field
 *   being hexadecimal.  If any field is blank, that portion of the ID is
 *   not replaced.  For example, 3::: forces a bus type of 3 (USB).  See
 *   /usr/include/linux/input.h for bus types.
 *
 * uniq <guid>
 *   Replace the unique string for the device (usually the UUID).
 *
 * axes <list>
 *   This remaps absolute axes.  Relative axes are not supported.  It is
 *   a comma-separated list of input axes to map (regardless of whether or
 *   not the device actually has this axis).  Just a plain number*
 *   or range of numbers (separated by -), optionally preceeded by a -
 *   to invert the values, maps to the next unmapped output axis number,
 *   starting with 0.  A blank entry skips an unmapped output for this
 *   auto-assignment.  Two numbers separated by = (wiith a an optional -
 *   after the = for inversion) indicate the output axis to the left of
 *   equals and the input to the right.  Note that hats (dpads) start at 16.
 *   Ranges are also allowed after the =, in which case the range is
 *   assigned in sequential order (unlike auto-assignment, which skips
 *   outputs already mapped).  Finally, in place of the input axis number
 *   or range in either form, a button triplet, preceeded by the letter b,
 *   separated by less-than signs, may be specified:  b<but><<but><<but>
 *   where <but> is the  button event for negative, middle, and positive
 *   values, respectively.  Each button may be preceeded by a - to indicate
 *   "when released".  Each button may also be blank (but at least one must
 *   not be blank).  If the positive is blank, then the inverted state of
 *   the negative is used (if that is not already used by the middle), and
 *   vice-versa.  If the middle is blank, the inverted state of either
 *   positive or negative is used.  If only the middle is non-blank, the
 *   negative is left unmapped and the positive uses the inverse of the
 *   middle.  Button codes are described below.  Finally, a list entry
 *   consisting of an axis number preceeded by an exclamation point (!)
 *   disables that axis for input, removing any existing mapping to that
 *   axis.  Note that in all cases, any mapping for an input overrides any
 *   preceeding (un)mapping.  If this keyword is missing, any axes not
 *   mapped to buttons are passed through as is.  Otherwise, any inputs not
 *   explicitly mapped are ignored.
 *   * numbers are C-style:  decimal, octal (0 prefix), hexadecimal (0x prefix)
 *
 * rescale <list>
 *   Change the absinfo parameters for the given output axes.  Multiple
 *   list entries are separated by commas (or separate keywords).  Each
 *   list entry is an output axis number, followed by an equals sign,
 *   followed by the minimum, maximum, fuzz, flat, and resolution
 *   parameters, respectively, separated by colons.  Missing parameters
 *   (either due to too few colons or missing numbers) are set to 0.
 *   Values are automatically scaled to the new range (before inversion,
 *   if requested by the axes keyword):
 *    (v - old_min) * (new_max - new_min) / (old_max - old_min)) + new_min
 *   It uses integer math with unisgned longs with no checks for overflow.
 *   This rescaling may not produce desired results, but it's all I've got.
 *   Note that rescaling must be specified after mapping using the axes
 *   keyword, or the scaling will be lost.
 *
 * pass_axes
 *   Normally, if there are any axes keywords at all, any inputs not
 *   explicilty mapped are ignored.  This passes through any inputs not
 *   conflicting with outputs through.
 *
 * buttons <list>
 *   This remaps buttons (key events).  It is a comma-separated list of input
 *   button (key) codes to remap (regardless of whether or not the device
 *   actually has this button).  See /usr/include/linux/input-event-codes.h
 *   for codes.  Just a plain number* or range (separated by -) of
 *   numbers, optionally preceeded by a - to invert the state, maps to the
 *   next unmapped output, starting with 0x130 (BTN_A).  Just like with axes,
 *   you can also use = to specify the output.  Axes can be mapped
 *   to buttons, as well.  Using ax<num>><num><<num> instead of an input
 *   button code says that when the axis specified by the first number has
 *   a value greater than or equal to the second, press the button, and if
 *   less than or equal to the third, release the button.  If the second is
 *   less than the third, the tests are inverted.  One plain and one inverted
 *   button map per axis is allowed.  Finally, a button ID preceeded by an
 *   exclamation point (!) will explicitly unmap the given input.  Here is
 *   the list of standard gamepad buttons:
 *     A = SOUTH = 304, B = EAST = 305, C = 306,
 *     X = NORTH = 306, Y = WEST = 308, Z = 309,
 *     TL = 310, TR = 311, TL2 = 312, TR2 = 313,
 *     SELECT = 314, START = 315, MODE = 316, THUMBL = 317, THUMBR = 318
 *   And here are the standard joystick buttons:
 *     TRIGGER = 288, THUMB = 289, THUMB2 = 290, TOP = 291, TOP2 = 292,
 *     PINKIE = 293, BASE = 294, BASE2 = 295, BASE3 = 296, BASE4 = 297,
 *     BASE5 = 298, BASE6 = 299, DEAD = 303
 *   Note that most controllers do not have C or Z buttons.  Older versions
 *   of the hid_sony driver issued joystick events (THUMB/THUMB2/TOP/TRIGGER
 *   instead of A/B/X/Y).  Also, some controllers only use axes for TL2 and
 *   TR2.  My other remappers support hex and symbolic names (using the C
 *   preprocessor with input-event-codes.h), but not this time.  As a
 *   compromise, the above-listed names are supported, case-insensitive
 *   (without the BTN_ prefix, as shown).
 *   * numbers are C-style:  decimal, octal (0 prefix), hexadecimal (0x prefix)
 *
 * pass_buttons
 *   Normally, if there are any buttons keywords at all, any inputs not
 *   explicilty mapped are ignored.  This passes through any inputs not
 *   conflicting with outputs through.
 *
 * syn_drop
 *   When dropping events, rather than just removing them from the stream,
 *   send SYN_DROP events.
 * 
 * Note that for button-to-axis and axis-to-button mappings, the button press
 * or release event will not occur unless the state changes.  All buttons
 * are assumed to be initially released, and all axes are assumed to be
 * initially at their center value.
 *
 * Note also that each input axis or button can only do one thing.  Again,
 * this is only a simple mapper.
 *
 * Once the config is set, run the command <cmd> with LD_PRELOAD set to the
 * path to this compiled shim:
 * 
 * > LD_PRELOAD="<path_to>/joy-remap.so" <cmd>
 *
 *
 * Build with: gcc -s -Wall -O2 -shared -fPIC -o joy-remap.{so,c} -ldl
 * for debug:  gcc -g -Wall -shared -fPIC -o joy-remap.{so,c} -ldl
 * Use clang instead of gcc if you prefer.  Don't bother with debug; gdb
 * has a real hard time debugging LD_PRELOADs (or maybe I'm missing some
 * special magic).  At least crashes can be debugged using the core file.
 * Crashing within gdb confuses gdb, and setting breakpoints doesn't work.
 */

/* Every intercept takes time, albeit usually unnoticabbly little */
/* The only ones known to work are open and open64 */
#define CAP_OPENAT  1  /* this is the syscall used by glibc, but nobody calls this */
#define CAP_FOPEN   1  /* for c++; assumes use of read() rather than fread() */
#define CAP_SYSCALL 1  /* for mono, I guess */
/* glibc also exports numerous "internal" symbols beginning with __ */
/* not going to intercept them, for now. */
/* the only other open is open_to_handle_at, which I don't want to deal with */

/* for RTLD_NEXT */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/joystick.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <regex.h>
/* why would you be scanning for devices in parallel?  Oh well, some
 * jackass will try and screw this up, so may as well support it */
#include <pthread.h>

/* These numbers are not exported, and may change in the future */
/* see linux/drivers/input/evdev.c and linux/drivers/input/joydev.c */
#define INPUT_MAJOR 13
#define EVDEV_MINOR0 64
#define EVDEV_NMINOR 32
#define JSDEV_MINOR0 0
#define JSDEV_NMINOR 16

static FILE *logf;

/* macros for accessing bits in arrays of unsigned longs */
/* stupid kernel doesn't export its bitops, so everybody has to reimplement */
/* these probably only work on little-endian, but that's ok for now */
#define ULBITS (sizeof(unsigned long)*8)
#define MINBITS(x) (((x) + ULBITS - 1)/ULBITS)
#define ULSET(bits, bit) do { \
    int _b = bit; \
    (bits)[_b/ULBITS] |= 1UL << _b % ULBITS; \
} while(0)
#define ULCLR(bits, bit) do { \
    int _b = bit; \
    (bits)[_b/ULBITS] &= ~(1UL << _b % ULBITS); \
} while(0)
#define ULISSET(bits, bit) ((bits)[(bit)/ULBITS] & (1UL << (bit) % ULBITS))

/* info for mapping an input axis to an axis or key target */
struct axmap {
    struct input_absinfo ai; /* for rescaling */
    int flags;
    int target;
    int onthresh, offthresh; /* axis->button */
    int ntarget, nonthresh, noffthresh;  /* axis->button, 2nd key */
};
#define AXFL_MAP      (1<<0)  /* does this need processing? */
#define AXFL_BUTTON   (1<<1)  /* is this a button map? else ax map */
#define AXFL_INVERT   (1<<2)  /* invert before sending on? */
                              /* onthresh is min+max if AXFL_INVERT */
#define AXFL_RESCALE  (1<<3)  /* rescale using ai? */
#define AXFL_NINVERT  (1<<4)  /* invert ntarget before sending out */
#define AXFL_PRESSED  (1<<5)  /* is the button currently pressed? */
                              /* defaults to no, even if axis says otherwise */
#define AXFL_NPRESSED (1<<6)  /* is the neg button pressed? */

/* info for mapping an input key to a key or axis target */
struct butmap {
    int flags;
    int target;
#define onax target /* this and next are for button->axis */
    int offax, onval, offval; /* val is -1 0 1 on init */
};
#define BTFL_MAP      (1<<0)  /* does this need processing? */
#define BTFL_AXIS     (1<<1)  /* is this an axis map?  else bt map */
#define BTFL_INVERT   (1<<2)  /* invert before sending on? */

/* all config combined into one structure for multiple sections */
static struct evjrconf {
    char *name;
    struct axmap *ax_map;
    struct butmap *bt_map;
    char *repl_name, *repl_id, *repl_uniq;
    /* since there is no regcopy() or equiv., need strings for KW_USE */
    char *match_str, *reject_str, *jsrename_str;
    regex_t match, reject, jsrename; /* compiled matching regexes */
    /* stuff below this is safe to copy on USE */
    int bt_low, nbt, nax; /* mapping array valid bounds */
    int max_ax, max_bt; /* mapping array sizes */
    int auto_ax, auto_bt; /* during parse: current auto-assigned inputs */
    int filter_ax, filter_bt; /* flag:  pass-through unmapped? */
    int filter_dev; /* flag:  filter non-matching devs completely? */
    int syn_drop; /* use SYN_DROP instead of deleting drops? */
} *conf;
static int nconf = 0;

/* captured fds and the config that captured them */
/* also other per-device info */
static struct evfdcap {
    struct evfdcap *next; /* linked list is less thread-unsafe */
    const struct evjrconf *conf;
    unsigned long absout[MINBITS(ABS_MAX)]; /* sent GBITS(EV_ABS) */
    int axval[ABS_MAX]; /* value for key-generated axes */
    /* FIXME:  do I need to support EVIOC[GS]KEYCODE*? */
    unsigned long keysout[MINBITS(KEY_MAX)],  /* sent GBITS(EV_KEY) */
	          keystates[MINBITS(KEY_MAX)], /* sent GKEY */
	          keystates_in[MINBITS(KEY_MAX)];  /* device GKEY */
    struct input_id repl_id_val;
    int excess_read;
    char ebuf[sizeof(struct input_event)];
    int fd;
} *ev_fd = NULL, *free_ev_fd = NULL;
static struct jsfdcap {
    struct jsfdcap *next; /* linked list is less thread-unsafe */
    const struct evjrconf *conf;
    int fd;
} *js_fd = NULL, *free_js_fd = NULL;

static char buf[1024]; /* generic large buffer to reduce stack usage */
static pthread_mutex_t buf_lock = PTHREAD_MUTEX_INITIALIZER; /* in case of threads */

/* array must be alphabetized */
static const struct bname {
    const char *nm;
    int code;
} bname[] = {
    { "a",	BTN_A },
    { "b",	BTN_B },
    { "base",	BTN_BASE },
    { "base2",	BTN_BASE2 },
    { "base3",	BTN_BASE3 },
    { "base4",	BTN_BASE4 },
    { "base5",	BTN_BASE5 },
    { "base6",	BTN_BASE6 },
    { "c",	BTN_C },
    { "dead",	BTN_DEAD },
    { "east",	BTN_EAST },
    { "mode",	BTN_MODE },
    { "north",	BTN_NORTH },
    { "pinkie",	BTN_PINKIE },
    { "select",	BTN_SELECT },
    { "south",	BTN_SOUTH },
    { "start",	BTN_START },
    { "thumb",	BTN_THUMB },
    { "thumb2",	BTN_THUMB2 },
    { "thumbl",	BTN_THUMBL },
    { "thumbr",	BTN_THUMBR },
    { "tl",	BTN_TL },
    { "tl2",	BTN_TL2 },
    { "top",	BTN_TOP },
    { "top2",	BTN_TOP2 },
    { "tr",	BTN_TR },
    { "tr2",	BTN_TR2 },
    { "trigger", BTN_TRIGGER },
    { "west",	BTN_WEST },
    { "x",	BTN_X },
    { "y",	BTN_Y },
    { "z",	BTN_Z }
};

/* a is always target; b is always bname[] entry */
static int bncmp(const void *_a, const void *_b)
{
    const struct bname *a = (const struct bname *)_a;
    const struct bname *b = _b;
    /* code in a is actually a->nm's length */
    int ret = strncasecmp(a->nm, b->nm, a->code);
    if(ret)
	return ret;
    if(b->nm[a->code])
	return -1;
    return 0;
}

/* interpret string of alnum as button code */
/* advances s if successful; returns -1 otherwise */
/* technically s could be const char **, but then compiler complains too often */
static int bnum(char **s)
{
    if(!**s)
	return -1;
    if(isdigit(**s))
	/* FIXME: restore *s and return -1 if value > KEY_MAX */
	return strtol(*s, (char **)s, 0);
    const char *e;
    for(e = *s; isalnum(*e); e++);
    struct bname str = { *s, e - *s}; /* code is length */
    struct bname *m = bsearch(&str, bname, sizeof(bname)/sizeof(bname[0]),
			      sizeof(bname[0]), bncmp);
    if(m) {
	*s += str.code;
	return m->code;
    }
    return -1;
}

/* array and enum must be alphabetized */
static const char * const kws[] = {
    "axes",
    "buttons",
    "filter",
    "id",
    "jsrename",
    "match",
    "name",
    "pass_axes",
    "pass_buttons",
    "reject",
    "rescale",
    "section",
    "syn_drop",
    "uniq",
    "use"
};

enum kw {
    KW_AXES, KW_BUTTONS, KW_FILTER, KW_ID, KW_JSRENAME, KW_MATCH, KW_NAME,
    KW_PASS_AX, KW_PASS_BT, KW_REJECT, KW_RESCALE, KW_SECTION, KW_SYN_DROP,
    KW_UNIQ, KW_USE
};

static int kwcmp(const void *_a, const void *_b)
{
    const char *a = _a, * const *b = _b;
    return strcasecmp(a, *b);
}

static void free_conf(struct evjrconf *sec);

#if CAP_SYSCALL
static long (*real_syscall)(long number, ...);
#endif

/* parse config file */
/* is this too early for file I/O?  apparently not */
/* dlopen() docs say this must be exported, but again, apparently not */
/* note that this attribute works with clang as well */
__attribute__((constructor))
static void init(void)
{
#if CAP_SYSCALL
    real_syscall = dlsym(RTLD_NEXT, "syscall");  /* needs to be first! */
#endif
    const char *fname = getenv("EV_JOY_REMAP_CONFIG"),
	       *logn = getenv("EV_JOY_REMAP_LOG");
    FILE *f;
    long fsize;
    char *cfg;
    struct evjrconf *sec;

    if(logn) {
	logf = fopen(logn, "w");
	if(!logf)
	    perror(logn);
	else
	    setbuf(logf, NULL);
    }
    if(!logf)
	logf = stderr;
    if(fname && *fname)
	f = fopen(fname, "r");
    else if(!(f = fopen((fname = "ev_joy_remap.conf"), "r"))) {
	if((fname = getenv("HOME")) && *fname) {
	    sprintf(buf, "%.200s/.config/ev_joy_remap.conf", fname);
	    f = fopen((fname = buf), "r");
	}
	if(!f)
	    f = fopen("/etc/ev_joy_remap.conf", "r");
    }
    if(!f) {
	fprintf(logf, "%s: %s\n", fname, strerror(errno));
	return;
    }
    /* config should be short enough to fit in memory */
    /* this eliminates the need for line read gymnastics */
    if(fseek(f, 0, SEEK_END) || (fsize = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) ||
       !(cfg = malloc(fsize + 1))) {
	fprintf(logf, "%s: %s\n", fname, strerror(errno));
	fclose(f);
	return;
    }
    if(fread(cfg, fsize, 1, f) != 1) {
	fprintf(logf, "%s: %s\n", fname, strerror(errno));
	fclose(f);
	free(cfg);
	return;
    }
    fclose(f);
    sec = conf = calloc(sizeof(*conf), (nconf = 1));
    if(!conf) {
	fprintf(logf, "%s: %s\n", "conf", strerror(errno));
	free(cfg);
	nconf = 0;
	return;
    }
    cfg[fsize] = 0;
    char *ln = cfg, *e, c;
    sec->ax_map = calloc((sec->max_ax = 18), sizeof(*sec->ax_map));
    sec->bt_map = calloc((sec->max_bt = 15), sizeof(*sec->bt_map));
    if(!sec->ax_map || !sec->bt_map) {
	fprintf(logf, "%s: %s\n", "mapping", strerror(errno));
	free(cfg);
	if(sec->ax_map)
	    free(sec->ax_map);
	if(sec->bt_map)
	    free(sec->bt_map);
	return;
    }
    sec->auto_ax = -1;
    sec->auto_bt = BTN_A - 1;
    int lno = 1;
#define abort_parse(msg) do { \
    fprintf(logf, "error parsing map on line %d: " msg "\n", lno); \
    goto err; \
} while(0)
#define map_resize(what, sz) do { \
    if(sec->max_##what < sz) { \
	int old_max = sec->max_##what; \
	void *old_map = sec->what##_map; \
	while(sec->max_##what < sz) \
	    sec->max_##what *= 2; \
	sec->what##_map = realloc(sec->what##_map, sec->max_##what * sizeof(*sec->what##_map)); \
	if(!sec->what##_map) { \
	    sec->what##_map = old_map; \
	    abort_parse(#what " map too large"); \
	} \
	memset(sec->what##_map + old_max, 0, (sec->max_##what - old_max) * sizeof(*sec->what##_map)); \
    } \
} while(0)
    int i, ret;
    int has_conf = 0;
    while(1) {
	while(isspace(*ln)) {
	    if(*ln == '\n')
		lno++;
	    ln++;
	}
	if(!*ln)
	    break;
	if(*ln == '#') {
	    while(*++ln && *ln != '\n');
	    continue;
	}
	for(e = ln; *e && !isspace(*e); e++);
	c = *e;
	*e = 0;
	const char **kw = bsearch(ln, kws, sizeof(kws)/sizeof(kws[0]),
				 sizeof(ln), kwcmp);
	*e = c;
	if(!kw)
	    abort_parse("unknown keyword");
	has_conf++;
	for(ln = e; isspace(*ln) && *ln != '\n'; ln++);
	for(e = ln; *e && *e != '\n'; e++);
	while(e > ln && isspace(e[-1]))
	    e--;
	c = *e;
	*e = 0;
	switch(kw - kws) {
	  case KW_SECTION:
	    /* is it a continuation? */
	    for(i = 0; i < nconf; i++)
		if((!*ln && !conf[i].name) ||
		   (conf[i].name && !strcmp(ln, conf[i].name)))
		    break;
	    if(i < nconf) {
		/* yes */
		sec = &conf[i];
		has_conf = 2; /* don't allow USE */
	    } else if(i == 1 && !sec->name && has_conf == 1) {
		/* no, and it's the first section w/ no unnamed entries */
		/* so replace the 1st section */
		if(*ln) {
		    sec->name = strdup(ln);
		    if(!sec->name) {
			fprintf(logf, "%s: %s\n", "sec name", strerror(errno));
			goto err;
		    }
		}
	    } else {
		/* no, and it doesn't replace the first unnamed */
		/* yeah, one at a time, rather than in blocks.  too lazy */
		sec = conf;
		conf = realloc(conf, ++nconf * sizeof(*conf));
		if(!conf) {
		    conf = sec;
		    fprintf(logf, "%s: %s\n", "expand conf", strerror(errno));
		    goto err;
		}
		sec = &conf[nconf - 1];
		memset(sec, 0, sizeof(*sec));
		if(*ln) {
		    sec->name = strdup(ln);
		    if(!sec->name) {
			fprintf(logf, "%s: %s\n", "sec name", strerror(errno));
			goto err;
		    }
		}
		has_conf = 1;
	    }
	    break;
	  case KW_USE:
	    if(has_conf != 2)
		abort_parse("use must be first in a section");
	    for(i = 0; i < nconf; i++)
		if((!*ln && !conf[i].name) ||
		   (conf[i].name && !strcmp(ln, conf[i].name)))
		    break;
	    if(i == nconf)
		abort_parse("unknown section");
	    if(i == sec - conf)
		abort_parse("can't include self");
	    free(sec->ax_map);
	    free(sec->bt_map);
	    memcpy((char *)sec + offsetof(struct evjrconf, bt_low),
		   (char *)&conf[i] + offsetof(struct evjrconf, bt_low),
		   sizeof(*sec) - offsetof(struct evjrconf, bt_low));
	    sec->ax_map = malloc(sec->max_ax * sizeof(*sec->ax_map));
	    sec->bt_map = malloc(sec->max_bt * sizeof(*sec->bt_map));
	    if(!sec->ax_map || !sec->bt_map)
		abort_parse("no mem");
	    memcpy(sec->ax_map, conf[i].ax_map, sec->max_ax * sizeof(*sec->ax_map));
	    memcpy(sec->bt_map, conf[i].bt_map, sec->max_bt * sizeof(*sec->bt_map));
#define dupstr(s) do { \
    if(conf[i].s) { \
	sec->s = strdup(conf[i].s); \
	if(!sec->s) \
	    abort_parse("no mem"); \
    } \
} while(0)
	    dupstr(repl_name);
	    dupstr(repl_id);
	    dupstr(repl_uniq);
#define comp_regex(s, type) do { \
    if((ret = regcomp(&sec->type, s, REG_EXTENDED | REG_NOSUB))) { \
	regerror(ret, &sec->type, buf, sizeof(buf)); \
	fprintf(logf, #type " pattern error: %.*s\n", (int)sizeof(buf), buf); \
	regfree(&sec->type); \
	goto err; \
    } \
    sec->type##_str = strdup(s); \
    if(!sec->type##_str) { \
	fprintf(logf, "%s: %s\n", s, strerror(errno)); \
	regfree(&sec->type); \
	goto err; \
    } \
} while(0)
#define dupre(r) do { \
    if(conf[i].r##_str) \
	comp_regex(conf[i].r##_str, r); \
} while(0)
	    dupre(match);
	    dupre(reject);
	    dupre(jsrename);
	    break;
	  case KW_MATCH:
#define parse_regex(type) do { \
    if(sec->type##_str) { \
	free(sec->type##_str); \
	sec->type##_str = NULL; \
	regfree(&sec->type); \
    } \
    comp_regex(ln, type); \
} while(0)
	    parse_regex(match);
	    break;
	  case KW_REJECT:
	    parse_regex(reject);
	    break;
	  case KW_FILTER:
	    if(*ln)
		abort_parse("filter takes no parameter");
	    sec->filter_dev = 1;
	    break;
	  case KW_NAME:
#define store_repl(w) do { \
    if(sec->repl_##w) \
	free(sec->repl_##w); \
    sec->repl_##w = strdup(ln); \
    if(!sec->repl_##w) \
	abort_parse(#w); \
} while(0)
	    store_repl(name);
	    break;
	  case KW_JSRENAME:
	    parse_regex(jsrename);
	    break;
	  case KW_ID:
	    store_repl(id);
	    break;
	  case KW_UNIQ:
	    store_repl(uniq);
	    break;
	  case KW_AXES:
	    /* I guess duplicates are OK here */
	    /* blank list just skips an auto */
	    if(!*ln) {
		int t;
#define next_auto_axis do { \
    sec->auto_ax++; \
    for(t = 0; t < sec->nax; t++) \
	if((sec->ax_map[t].flags & (AXFL_MAP | AXFL_BUTTON)) == AXFL_MAP && \
	   sec->ax_map[t].target == sec->auto_ax) { \
	    sec->auto_ax++; \
	    t = -1; \
	} \
    for(t = 0; t < sec->nbt; t++) \
	if((sec->bt_map[t].flags & (BTFL_MAP | BTFL_AXIS)) == (BTFL_MAP | BTFL_AXIS) && \
	   (sec->bt_map[t].onax == sec->auto_ax || sec->bt_map[t].offax == sec->auto_ax)) { \
	    sec->auto_ax++; \
	    t = -1; \
	} \
    t = -1; \
} while(0)
		next_auto_axis;
		break;
	    }
	    sec->filter_ax |= 1;
	    while(*ln) {
		int a = -1, t = -1;
		if(*ln == '!' && isdigit(ln[1])) {
		    /* FIXME: abort if value > ABS_MAX */
		    a = strtol(ln + 1, &ln, 0);
		    if(*ln && *ln != ',')
			abort_parse("invalid !");
		    if(sec->nax <= a)
			sec->nax = a + 1;
		    map_resize(ax, sec->nax);
		    sec->ax_map[a].flags = AXFL_MAP;
		    sec->ax_map[a].target = -1;
		    if(*ln == ',')
			ln++;
		    continue;
		}
		if(isdigit(*ln))
		    /* FIXME: abort if value > ABS_MAX */
		    a = t = strtol(ln, (char **)&ln, 0);
		if(*ln != '=') {
		    /* skip if target already used */
		    next_auto_axis;
		} else {
		    if(t < 0 || ln[1] == ',')
			abort_parse("unexpected =");
		    ln++;
		    a = -1;
		}
		if(a < 0 && t < 0 && *ln == ',') {
		    ++ln;
		    continue;
		}
		int invert = a < 0 && *ln == '-';
		if(invert) {
		    ln++;
		    if(!isdigit(*ln))
			abort_parse("unexpected -");
		    invert = AXFL_INVERT;
		}
		if(isdigit(*ln))
		    /* FIXME: abort if value > ABS_MAX */
		    a = strtol(ln, (char **)&ln, 0);
		if(a >= 0) {
		    if(sec->nax <= a)
			sec->nax = a + 1;
		    map_resize(ax, sec->nax);
		    memset(sec->ax_map + a, 0, sizeof(*sec->ax_map));
		    sec->ax_map[a].flags = AXFL_MAP | invert;
		    sec->ax_map[a].target = t < 0 ? sec->auto_ax : t;
		    if(*ln == '-' && isdigit(ln[1])) {
			/* FIXME: abort if value > ABS_MAX */
			int b = strtol(ln + 1, (char **)&ln, 0);
			if(b < a)
			    abort_parse("invalid range");
			if(sec->nax <= b)
			    sec->nax = b + 1;
			map_resize(ax, sec->nax);
			for(;++a <= b;) {
			    if(t < 0)
				next_auto_axis;
			    else
				t++;
			    memset(&sec->ax_map[a], 0, sizeof(*sec->ax_map));
			    sec->ax_map[a].flags = AXFL_MAP | invert;
			    sec->ax_map[a].target = t < 0 ? sec->auto_ax : t;
			}
		    }
		} else if(tolower(*ln) == 'b') {
		    ln++;
		    if(t < 0) /* we don't need this flag any more as there are no ranges */
			t = sec->auto_ax;
		    int l, m, h, li, mi, hi;
		    if((li = *ln == '-'))
			ln++;
		    l = bnum(&ln);
		    if(*ln++ != '<')
			abort_parse("invalid axis button");
		    if((mi = *ln == '-'))
			ln++;
		    m = bnum(&ln);
		    if(*ln++ != '<')
			abort_parse("invalid axis button");
		    if((hi = *ln == '-'))
			ln++;
		    h = bnum(&ln);
		    if(l < 0 && m < 0 && h < 0)
			abort_parse("invalid axis button");
#define expand_bt(n) do { \
    if(!sec->nbt) { \
	if(n < BTN_A || n >= BTN_A + sec->max_bt) \
	    sec->bt_low = n; \
	else \
	    sec->bt_low = BTN_A; \
    } \
    if(n < sec->bt_low) { \
	map_resize(bt, sec->nbt + sec->bt_low - n); \
	memmove(sec->bt_map + sec->bt_low - n, sec->bt_map, sec->nbt * sizeof(*sec->bt_map)); \
	memset(sec->bt_map, 0, (sec->bt_low - n) * sizeof(*sec->bt_map)); \
	sec->nbt += sec->bt_low - n; \
	sec->bt_low = n; \
    } \
    if(sec->nbt <= n - sec->bt_low) { \
	sec->nbt = n - sec->bt_low + 1; \
	map_resize(bt, sec->nbt); \
    } \
} while(0)
		    if(l >= 0) {
			expand_bt(l);
			sec->bt_map[l - sec->bt_low].offax = sec->bt_map[l - sec->bt_low].onax = -1;
		    }
		    if(m >= 0) {
			expand_bt(m);
			sec->bt_map[m - sec->bt_low].offax = sec->bt_map[m - sec->bt_low].onax = -1;
		    }
		    if(h >= 0) {
			expand_bt(h);
			sec->bt_map[h - sec->bt_low].offax = sec->bt_map[h - sec->bt_low].onax = -1;
		    }
		    if((l >= 0 && ((l == h && l == m) || (l == h && li == hi) ||
				   (l == m && li == mi))) ||
		       (m >= 0 && m == h && mi == hi))
			abort_parse("same key event gives different axis events");
		    if(l < 0 && h >= 0 && h != m) {
			l = h;
			li = !hi;
		    }
		    if(h < 0 && l >= 0 && l != m) {
			h = l;
			hi = !li;
		    }

#define doaxbt(w, v) do { \
    if(w >= 0) { \
	int wa = w - sec->bt_low; \
	sec->bt_map[wa].flags = BTFL_MAP | BTFL_AXIS; \
	if(w##i) { \
	    sec->bt_map[wa].offax = t; \
	    sec->bt_map[wa].offval = v; \
	} else { \
	    sec->bt_map[wa].onax = t; \
	    sec->bt_map[wa].onval = v; \
	} \
    } \
} while(0)

		    doaxbt(l, -1);
		    doaxbt(m, 0);
		    doaxbt(h, 1);
		    if(m < 0 && l != h) {
			li = !li;
			hi = !hi;
			doaxbt(l, 0);
			doaxbt(h, 0);
		    }
		} else
		    abort_parse("invalid mapping entry");
		if(*ln == ',') {
		    if(!*++ln)
			next_auto_axis;
		} else if(*ln)
		    abort_parse("garbage at end of mapping");
	    }
	    break;
	  case KW_RESCALE:
	    while(*ln) {
		if(!isdigit(*ln))
		    abort_parse("invalid rescale axis");
		/* FIXME: abort if value > ABS_MAX */
		int t = strtol(ln, &ln, 0), a;
		if(*ln++ != '=')
		    abort_parse("rescale w/o =");
		for(a = 0; a < sec->nax; a++)
		    if(sec->ax_map[a].target == t && (sec->ax_map[a].flags & AXFL_MAP))
			break;
		if(a == sec->nax) {
		    a = t;
		    if(a >= sec->nax) {
			sec->nax = a + 1;
			map_resize(ax, sec->nax);
		    }
		    if(sec->ax_map[a].flags & AXFL_MAP)
			abort_parse("rescale target unavailable");
		    sec->ax_map[a].flags = AXFL_MAP;
		    sec->ax_map[a].target = a;
		}
		sec->ax_map[a].flags |= AXFL_RESCALE;
		memset(&sec->ax_map[a].ai, 0, sizeof(sec->ax_map[a].ai));
		sec->ax_map[a].ai.minimum = strtol(ln, &ln, 0);
		if(*ln == ':')
		    sec->ax_map[a].ai.maximum = strtol(ln + 1, &ln, 0);
		if(*ln == ':')
		    sec->ax_map[a].ai.fuzz = strtol(ln + 1, &ln, 0);
		if(*ln == ':')
		    sec->ax_map[a].ai.flat = strtol(ln + 1, &ln, 0);
		if(*ln == ':')
		    sec->ax_map[a].ai.resolution = strtol(ln + 1, &ln, 0);
		if(sec->ax_map[a].ai.maximum <= sec->ax_map[a].ai.minimum)
		    abort_parse("invalid rescale range");
		/* since I don't entirely understand fuzz & flat, I won't check */
		if(*ln && *ln != ',')
		    abort_parse("invalid rescale entry");
		if(*ln)
		    ln++;
	    }
	    break;
	  case KW_PASS_AX:
	    if(*ln)
		abort_parse("pass_ax takes no parameter");
	    sec->filter_ax = -1;
	    break;
	  case KW_BUTTONS:
	    /* I guess duplicates are OK here */
	    sec->filter_bt |= 1;
	    /* blank list just skips an auto */
	    if(!*ln) {
		int t;
#define next_auto_bt do { \
    sec->auto_bt++; \
    for(t = 0; t < sec->nbt; t++) \
	if((sec->bt_map[t].flags & (BTFL_MAP | BTFL_AXIS)) == BTFL_MAP && \
	   sec->bt_map[t].target == sec->auto_bt) { \
	    sec->auto_bt++; \
	    t = -1; \
	} \
    for(t = 0; t < sec->nbt; t++) \
	if((sec->ax_map[t].flags & (AXFL_MAP | AXFL_BUTTON)) == (AXFL_MAP | AXFL_BUTTON) && \
	   (sec->ax_map[t].target == sec->auto_bt || sec->ax_map[t].ntarget == sec->auto_bt)) { \
	    sec->auto_bt++; \
	    t = -1; \
	} \
    t = -1; \
} while(0)
		next_auto_bt;
		break;
	    }
	    while(*ln) {
		int a = -1, t = -1;
		if(*ln == '!' && isalnum(ln[1])) {
		    ++ln;
		    a = bnum(&ln);
		    if(*ln && *ln != ',')
			abort_parse("invalid !");
		    expand_bt(a);
		    a -= sec->bt_low;
		    sec->bt_map[a].flags = BTFL_MAP;
		    sec->bt_map[a].target = -1;
		    if(*ln == ',')
			ln++;
		    continue;
		}
		if(isalnum(*ln))
		    a = t = bnum(&ln); /* should fail and not move for ax... */
		if(*ln != '=') {
		    /* skip if target already used */
		    next_auto_bt;
		} else {
		    if(t < 0 || ln[1] == ',')
			abort_parse("unexpected =");
		    ln++;
		    a = -1;
		}
		if(a < 0 && t < 0 && *ln == ',') {
		    ++ln;
		    continue;
		}
		int invert = a < 0 && *ln == '-';
		if(invert) {
		    ln++;
		    if(!isalnum(*ln) ||
		       ((a = bnum(&ln)) < 0 &&
			   (tolower(*ln) != 'a' || tolower(ln[1]) != 'x' ||
			       !isdigit(ln[2]))))
			abort_parse("unexpected -");
		    invert = BTFL_INVERT;
		} else if(isalnum(*ln))
			a = bnum(&ln);
		if(a >= 0) {
		    expand_bt(a);
		    a -= sec->bt_low;
		    memset(sec->bt_map + a, 0, sizeof(*sec->bt_map));
		    sec->bt_map[a].flags = BTFL_MAP | invert;
		    sec->bt_map[a].target = t < 0 ? sec->auto_bt : t;
		    if(*ln == '-' && isalnum(ln[1])) {
			++ln;
			a += sec->bt_low;
			int b = bnum(&ln);
			if(b < a)
			    abort_parse("invalid range");
			expand_bt(b);
			for(;++a <= b;) {
			    if(t < 0)
				next_auto_bt;
			    else
				t++;
			    memset(&sec->bt_map[a - sec->bt_low], 0, sizeof(*sec->bt_map));
			    sec->bt_map[a - sec->bt_low].flags = BTFL_MAP | invert;
			    sec->bt_map[a - sec->bt_low].target = t < 0 ? sec->auto_bt : t;
			}
		    }
		} else if(tolower(*ln) == 'a' && tolower(ln[1]) == 'x' && isdigit(ln[2])) {
		    if(t < 0) /* we don't need this flag any more as there are no ranges */
			t = sec->auto_bt;
		    /* FIXME: abort if value > ABS_MAX */
		    a = strtol(ln + 2, (char **)&ln, 0);
		    /* FIXME: support % instead of values */
		    int l, h;
		    if(*ln != '>' || !isdigit(ln[1]))
			abort_parse("invalid axis-to-button");
		    l = strtol(ln + 1, (char **)&ln, 0);
		    if(*ln != '<' || !isdigit(ln[1]))
			abort_parse("invalid axis-to-button");
		    h = strtol(ln + 1, (char **)&ln, 0);
		    if(l == h)
			abort_parse("invalid axis-to-button thresholds");
		    if(a >= sec->nax)
			sec->nax = a + 1;
		    map_resize(ax, sec->nax);
		    if(!(sec->ax_map[a].flags & AXFL_BUTTON)) {
			memset(sec->ax_map + a, 0, sizeof(*sec->ax_map));
			sec->ax_map[a].target = sec->ax_map[a].ntarget = -1;
			sec->ax_map[a].flags = AXFL_MAP | AXFL_BUTTON;
		    }
		    if(l < h) {
			sec->ax_map[a].target = t;
			sec->ax_map[a].onthresh = l;
			sec->ax_map[a].offthresh = h;
			sec->ax_map[a].flags |= invert;
		    } else {
			sec->ax_map[a].ntarget = t;
			sec->ax_map[a].nonthresh = l;
			sec->ax_map[a].noffthresh = h;
			sec->ax_map[a].flags |= invert ? AXFL_NINVERT : 0;
		    }
		} else
		    abort_parse("invalid mapping entry");
		if(*ln == ',')
		    ln++;
		else if(*ln)
		    abort_parse("garbage at end of mapping");
	    }
	    break;
	  case KW_PASS_BT:
	    if(*ln)
		abort_parse("pass_bt takes no parameter");
	    sec->filter_bt = -1;
	    break;
	  case KW_SYN_DROP:
	    if(*ln)
		abort_parse("syn_drop takes no parameter");
	    sec->syn_drop = 1;
	    break;
	}
	*e = c;
	ln = e;
    }
    for(sec = conf; sec < conf + nconf; sec++) {
	if(sec->filter_ax < 0)
	    sec->filter_ax = 0;
	if(sec->filter_bt < 0)
	    sec->filter_bt = 0;
	/* disable individual passthrough for mapped outputs */
#define dis_ax(axno) do { \
    if(axno >= 0) { \
	if(sec->nax <= axno) \
	    sec->nax = axno + 1; \
	map_resize(ax, sec->nax); \
	if(!(sec->ax_map[axno].flags & AXFL_MAP)) { \
	    sec->ax_map[axno].flags = AXFL_MAP; \
	    sec->ax_map[axno].target = -1; \
	} \
    } \
} while(0)
#define dis_bt(btno) do { \
    int bno = btno; \
    if(bno >= 0) { \
	expand_bt(bno); \
	bno -= sec->bt_low; \
	if(!(sec->bt_map[bno].flags & BTFL_MAP)) { \
	    sec->bt_map[bno].flags = BTFL_MAP; \
	    sec->bt_map[bno].target = -1; \
	} \
    } \
} while(0)
	for(i = 0; i < sec->nbt; i++) {
	    if((sec->bt_map[i].flags & (BTFL_MAP | BTFL_AXIS)) == BTFL_MAP) {
		int ol = sec->bt_low;
		dis_bt(sec->bt_map[i].target);
		if(ol != sec->bt_low)
		    i += ol - sec->bt_low;
	    } else if(sec->bt_map[i].flags & BTFL_MAP) {
		dis_ax(sec->bt_map[i].onax);
		dis_ax(sec->bt_map[i].offax);
	    }
	}
	for(i = 0; i < sec->nax; i++) {
	    if((sec->ax_map[i].flags & (AXFL_MAP | AXFL_BUTTON)) == AXFL_MAP)
		dis_ax(sec->ax_map[i].target);
	    else if(sec->ax_map[i].flags & AXFL_MAP) {
		dis_bt(sec->ax_map[i].target);
		dis_bt(sec->ax_map[i].ntarget);
	    }
	}
	/* even though technically the first device may be a gamepad due to
	 * permissions, I'm forcing you to have a pattern */
	/* if this is just used to rename joysticks, it still needs a dummy pattern */
	if(!sec->match_str) {
	    fprintf(logf, "section %s: ", sec->name ? sec->name : "[unnamed]");
	    abort_parse("match pattern required");
	}
    }
    free(cfg);
    cfg = NULL;
    const char *ensec_s = getenv("EV_JOY_REMAP_ENABLE");
    if(ensec_s && *ensec_s) {
	regex_t re;
	if((ret = regcomp(&re, ensec_s, REG_EXTENDED | REG_NOSUB))) {
	    regerror(ret, &re, buf, sizeof(buf));
	    fprintf(logf, "EV_JOY_REMAP_ENABLE pattern error: %.*s\n", (int)sizeof(buf), buf);
	    regfree(&re);
	    goto err;
	}
	for(i = 0; i < nconf; i++)
	    if(regexec(&re, conf[i].name ? conf[i].name : "", 0, NULL, 0)) {
		free_conf(&conf[i]);
		memmove(conf + i, conf + i + 1, (nconf - i - 1) * sizeof(*conf));
		--i;
		--nconf;
	    }
	regfree(&re);
	if(!nconf) {
	    fputs("No sections enabled for remapper; disabled\n", logf);
	    return;
	}
    }
    fputs("Installed event device remapper\n", logf);
    return;
err:
    for(sec = conf; nconf; nconf--, sec++)
	free_conf(sec);
    free(conf);
    if(cfg)
	free(cfg);
}

static void free_conf(struct evjrconf *sec)
{
    if(sec->ax_map)
	free(sec->ax_map);
    if(sec->bt_map)
	free(sec->bt_map);
#define free_re(r) do { \
    if(sec->r##_str) { \
	regfree(&sec->r); \
	free(sec->r##_str); \
    } \
} while(0)
    free_re(match);
    free_re(reject);
    free_re(jsrename);
    if(sec->repl_uniq)
	free(sec->repl_uniq);
    if(sec->repl_id)
	free(sec->repl_id);
    if(sec->repl_name)
	free(sec->repl_name);
}

/* I use ioctl() a bit here, so bypass intercept */
static int real_ioctl(int fd, unsigned long request, ...)
{
    static int (*next)(int, unsigned long, ...) = NULL;
    if(!next)
	next = dlsym(RTLD_NEXT, "ioctl");
    void *argp = NULL;
    if(_IOC_SIZE(request)) {
	va_list va;
	va_start(va, request);
	argp = va_arg(va, void *);
	va_end(va);
    }
    return next(fd, request, argp);
}

/* I also use close() sometimes, so bypass intercept */
static int real_close(int fd)
{
    static int (*next)(int) = NULL;
    if(!next)
	next = dlsym(RTLD_NEXT, "close");
    return next(fd);
}

/* capture event device and prepare ioctl returns */
static void init_joy(int fd, const struct evjrconf *sec)
{
    /* could use local lock, but it needs to be shared with close() */
    pthread_mutex_lock(&buf_lock);
    struct evfdcap *cap;
#define alloc_fd(r, t, f, s) do { \
    if(!(r = free_##t##_fd)) { \
	r = malloc(sizeof(*r)); \
	if(!r) { \
	    fprintf(logf, "%s: %s\n", "fd tracker", strerror(errno)); \
	    nconf = 0; \
	} \
    } else { \
	/* critical section w/ r assignment, protected by buf_lock */ \
	free_##t##_fd = r->next; \
    } \
    if(r) { \
	memset(r, 0, sizeof(*r)); \
	r->fd = f; \
	r->conf = s; \
    } \
} while(0)
    alloc_fd(cap, ev, fd, sec);
    if(!cap) {
	pthread_mutex_unlock(&buf_lock);
	return;
    }
    /* set up ID from string */
    if(sec->repl_id) {
	real_ioctl(fd, EVIOCGID, &cap->repl_id_val);
	char *s = sec->repl_id;
	if(*s && *s != ':')
	    cap->repl_id_val.bustype = strtol(s, &s, 16);
	/* I guess it's OK if fields are missing */
	if(*s == ':') {
	    if(*++s && *s != ':')
		cap->repl_id_val.vendor = strtol(s, &s, 16);
	    if(*s == ':') {
		if(*++s && *s != ':')
		    cap->repl_id_val.product = strtol(s, &s, 16);
		if(*s == ':' && *++s && *s != ':')
		    cap->repl_id_val.version = strtol(s, &s, 16);
	    }
	}
	if(*s) {
	    fprintf(logf, "joy-remap:  invalid id @ %s\n", s);
	    /* really, this should set ncconf to 0 and abort all remapping */
	    /* just like all other parse errors */
	    /* maybe I should parse it more in init() */
#if 0
	    free(sec->repl_id);
	    sec->repl_id = NULL;
#endif
	    goto err;
	}
    }
    /* adjust button/axis mappings */
    static unsigned long absin[MINBITS(ABS_MAX)];
    memset(absin, 0, sizeof(absin));
    memset(cap->keysout, 0, sizeof(cap->keysout));
    /* use cap->keystates as temp buffer for keysin */
    memset(cap->keystates, 0, sizeof(cap->keystates));
    if(real_ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(cap->keystates)), cap->keystates) < 0 ||
       real_ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absin)), absin) < 0) {
	fprintf(logf, "%s: %s\n", "init_joy", strerror(errno));
	goto err;
    }
    int i;
    /* add keys that are mapped if src is present */
    for(i = 0; i < sec->nbt; i++) {
	if((sec->bt_map[i].flags & (BTFL_MAP | BTFL_AXIS)) != BTFL_MAP)
	    continue;
	if(sec->bt_map[i].target < 0) { /* passthrough disable */
	    ULCLR(cap->keystates, sec->bt_low + i);
	    continue;
	}
	if(!ULISSET(cap->keystates, sec->bt_low + i)) {
	    fprintf(logf, "warning: disabling button %d due to missing %d\n",
		    sec->bt_map[i].target, sec->bt_low + i);
	    continue;
	}
	ULSET(cap->keysout, sec->bt_map[i].target);
	ULCLR(cap->keystates, sec->bt_low + i);
    }
    for(i = 0; i < sec->nax; i++) {
	if((sec->ax_map[i].flags & (AXFL_MAP | AXFL_BUTTON)) != (AXFL_MAP | AXFL_BUTTON))
	    continue;
	if(!ULISSET(absin, i)) {
	    fprintf(logf, "warning: disabling button %d/%d due to missing axis %d\n",
		    sec->ax_map[i].target, sec->ax_map[i].ntarget, i);
	    continue;
	}
	if(sec->ax_map[i].target >= 0)
	    ULSET(cap->keysout, sec->ax_map[i].target);
	if(sec->ax_map[i].ntarget >= 0)
	    ULSET(cap->keysout, sec->ax_map[i].ntarget);
	ULCLR(absin, i);
    }
    /* add axes that are mapped if src is present */
    for(i = 0; i < sec->nax; i++) {
	if((sec->ax_map[i].flags & (AXFL_MAP | AXFL_BUTTON)) != BTFL_MAP)
	    continue;
	if(sec->ax_map[i].target < 0) { /* passthrough disable */
	    ULCLR(absin, i);
	    continue;
	}
	if(!ULISSET(absin, i)) {
	    fprintf(logf, "warning: disabling axis %d due to missing %d\n",
		    sec->ax_map[i].target, i);
	    continue;
	}
	ULSET(cap->absout, sec->ax_map[i].target);
	ULCLR(absin, i);
	/* we only need absinfo for INVERT and RESCALE */
	/* and axis->key if I ever support % */
	if(sec->ax_map[i].flags & (AXFL_INVERT | AXFL_RESCALE)) {
	    struct input_absinfo ai;
	    if(real_ioctl(fd, EVIOCGABS(i), &ai) < 0) {
		fprintf(logf, "%s: %s\n", "init_joy", strerror(errno));
		goto err;
	    }
	    sec->ax_map[i].offthresh = ai.minimum;
	    sec->ax_map[i].onthresh = ai.minimum + ai.maximum;
	}
    }
    for(i = 0; i < sec->nbt; i++) {
	if((sec->bt_map[i].flags & (AXFL_MAP | AXFL_BUTTON)) != (AXFL_MAP | AXFL_BUTTON))
	    continue;
	if(!ULISSET(cap->keystates, sec->bt_low + i)) {
	    fprintf(logf, "warning: disabling axis %d/%d due to missing button %d\n",
		    sec->bt_map[i].onax, sec->bt_map[i].offax, i);
	    continue;
	}
	if(sec->bt_map[i].onax >= 0) {
	    ULSET(cap->absout, sec->bt_map[i].onax);
	    cap->axval[sec->bt_map[i].onax] = 0;
	}
	if(sec->bt_map[i].offax >= 0) {
	    ULSET(cap->absout, sec->bt_map[i].offax);
	    cap->axval[sec->bt_map[i].offax] = 0;
	}
	ULCLR(cap->keystates, sec->bt_low + i);
    }
    /* add all remaining keys & axes if no mapping given */
    /* note that if already set above, the raw input will still be dropped
     * if it's not used by a mapping */
    if(!sec->filter_bt)
	for(i = 0; i < MINBITS(KEY_MAX); i++)
	    cap->keysout[i] |= cap->keystates[i];
    if(!sec->filter_ax)
	for(i = 0; i < MINBITS(ABS_MAX); i++)
	    cap->absout[i] |= absin[i];
    /* critical section, protected well enough by buf_lock */
    cap->next = ev_fd;
    ev_fd = cap;
    pthread_mutex_unlock(&buf_lock);
    return;
err:
    /* critical section, protected by buf_lock */
    cap->next = free_ev_fd;
    free_ev_fd = cap;
    pthread_mutex_unlock(&buf_lock);
}

/* return NULL if nothing allows fd */
/* otherwise, return last section which matches */
static struct evjrconf *allowed_sec(int fd, int evno)
{
    /* this is small enough to be local, but we're locking for buf anyway */
    static struct input_id id;
    static char ibuf[25];
    struct evjrconf *sec;

    /* the lock is for buf & id */
    pthread_mutex_lock(&buf_lock);
    if(real_ioctl(fd, EVIOCGNAME(sizeof(buf)), buf) < 0)
	strcpy(buf, "ERROR: Device name unavailable");
    if(real_ioctl(fd, EVIOCGID, &id) < 0)
	memset(&id, 0, sizeof(id));
    sprintf(ibuf, "%04X-%04X-%04X-%04X-%d", (int)id.bustype,
	    (int)id.vendor, (int)id.product, (int)id.version, evno);
    for(sec = conf + nconf - 1; sec >= conf; sec--) {
	int rej = sec->reject_str && !regexec(&sec->reject, buf, 0, NULL, 0),
	    nmok = !rej && !regexec(&sec->match, buf, 0, NULL, 0);
	if(!rej) {
	    rej = sec->reject_str && !regexec(&sec->reject, ibuf, 0, NULL, 0);
	    if(rej)
		nmok = 0;
	    else if(!nmok)
		nmok = !regexec(&sec->match, ibuf, 0, NULL, 0);
	}
	if(nmok) {
	    pthread_mutex_unlock(&buf_lock);
	    return sec;
	}
    }
    pthread_mutex_unlock(&buf_lock);
    return NULL;
}

/* common code for multiple nearly identical open() functions */
static int ev_open(const char *fn, const char *pathname, int fd)
{
    struct stat st;
    if(fd < 0 || fstat(fd, &st) || !S_ISCHR(st.st_mode) || major(st.st_rdev) != INPUT_MAJOR)
	return fd;
    const struct evjrconf *sec;
    if(minor(st.st_rdev) >= JSDEV_MINOR0 &&
       minor(st.st_rdev) < JSDEV_MINOR0 + JSDEV_NMINOR) {
	for(sec = conf + nconf - 1; sec >= conf; sec--)
	    if(sec->jsrename_str && sec->repl_name)
		break;
	if(sec >= conf) {
	    pthread_mutex_lock(&buf_lock);
	    if(real_ioctl(fd, JSIOCGNAME(sizeof(buf)), buf) >= 0) {
		for(; sec >= conf; sec--)
		    if(sec->jsrename_str && sec->repl_name &&
		       !regexec(&sec->jsrename, buf, 0, NULL, 0)) {
			struct jsfdcap *jn;
			alloc_fd(jn, js, fd, sec);
			if(jn) { /* else error/aborting cap */
			    fprintf(logf, "[%s/%d] renaming %s (%s to %s)\n", fn, fd, pathname, buf, sec->repl_name);
			    /* critical section, protected well enough by buf_lock */
			    jn->next = js_fd;
			    js_fd = jn;
			}
			break;
		    }
	    } else
		fprintf(logf, "%s: %s\n", "jsname", strerror(errno));
	    pthread_mutex_unlock(&buf_lock);
	}
	return fd;
    }
    if(minor(st.st_rdev) < EVDEV_MINOR0 || minor(st.st_rdev) >= EVDEV_MINOR0 + EVDEV_NMINOR)
	return fd;
    sec = NULL;
    if(nconf && !(sec = allowed_sec(fd, minor(st.st_rdev) - EVDEV_MINOR0))) {
	/* if any enabled sections filter, filter. */
	for(sec = conf; sec < conf + nconf; sec++)
	    if(sec->filter_dev)
		break;
	if(sec == conf + nconf)
	    return fd;
/*    fprintf(logf, "[%s/%d] Rejecting open of %s\n", fn, fd, pathname); */
	real_close(fd);
	errno = EPERM;
	return -1;
    }
    if(sec) {
	fprintf(logf, "[%s/%d] Intercept %s\n", fn, fd, pathname);
	init_joy(fd, sec);
    }
    return fd;
}

#if CAP_SYSCALL
/* FIXME: this is not thread-safe */
/* it's really only for open(), though, so it's not likely to trigger */
volatile static int syscalling = 0;
#define DIS_SYSCALL syscalling = 1
#define EN_SYSCALL  syscalling = 0
#else
#define DIS_SYSCALL
#define EN_SYSCALL
#endif

int open(const char *pathname, int flags, ...)
{
    static int (*next)(const char *, int, ...) = NULL;
    if(!next)
	next = dlsym(RTLD_NEXT, "open");
    mode_t mode = 0;
    if(flags & O_CREAT) {
	va_list va;
	va_start(va, flags);
	mode = va_arg(va, mode_t);
	va_end(va);
    }
    DIS_SYSCALL;
    int ret = ev_open("open", pathname, next(pathname, flags, mode));
    EN_SYSCALL;
    return ret;
}

/* identical to above, for programs that call this alias */
int open64(const char *pathname, int flags, ...)
{
    static int (*next)(const char *, int, ...) = NULL;
    if(!next)
	next = dlsym(RTLD_NEXT, "open64");
    mode_t mode = 0;
    if(flags & O_CREAT) {
	va_list va;
	va_start(va, flags);
	mode = va_arg(va, mode_t);
	va_end(va);
    }
    DIS_SYSCALL;
    int ret = ev_open("open64", pathname, next(pathname, flags, mode));
    EN_SYSCALL;
    return ret;
}

/* mono seems to use syscall().  That is pretty much impossible to intercept */
#if CAP_SYSCALL
/* at least not without arch-dependent asm */
/* and that's assuming mono even uses glibc's syscall() so I can override */
#include <sys/syscall.h>

static long opensys(long scno, const char *path, int flags, mode_t mode)
{
    DIS_SYSCALL;
    int ret = ev_open("opensys", path, syscall(SYS_open, path, flags, mode));
    EN_SYSCALL;
    return ret;
}

static long openatsys(long scno, int dirfd, const char *path, int flags, mode_t mode)
{
    DIS_SYSCALL;
    int ret = ev_open("openatsys", path, syscall(SYS_openat, dirfd, path, flags, mode));
    EN_SYSCALL;
    return ret;
}

long  __attribute((naked)) syscall(long number, ...)
{
asm volatile (
"       cmpl $0, %[syscalling]\n"
"       jne dosys\n"
#ifdef __amd64
"       cmpq %[open_sc], %%rdi\n"
"       je %P[opensys]\n"
"       cmpq %[openat_sc], %%rdi\n"
"       je %P[openatsys]\n"	      
"dosys:\n"
"       mov %P[real_syscall](%%rip), %%rax\n"
"       jmp *%%rax\n"
#else
#ifdef __i386
"       mov    4(%%esp),%%eax\n"
"       cmpl %[open_sc], %%eax\n"
"       je opensys /* can't use %[opensys] or %p[opensys] */\n"
"       cmpl %[openat_sc], %%eax\n"
"       je openatsys /* can't use %[openatsys] or %p[openatsys] */\n"
"dosys:\n"
"       mov %[real_syscall], %%eax\n"
"       jmp *%%eax\n"
#else
#error "unknown arch"
#endif
#endif
    : : [syscalling]"m"(syscalling), [real_syscall]"m"(real_syscall),
	[open_sc]"i"(SYS_open), [opensys]"m"(opensys),
	[openat_sc]"i"(SYS_openat), [openatsys]"m"(openatsys)
#ifdef __i386
    : "eax"
#endif
    );
}
#endif

/* path-relative opens */
#if CAP_OPENAT
/* not really necessary, as no known program uses them */
/* they are used indirectly by fopen(), though; but fopen always uses libc */
int openat(int dirfd, const char *pathname, int flags, ...)
{
    static int (*next)(int, const char *, int, ...) = NULL;
    if(!next)
	next = dlsym(RTLD_NEXT, "openat");
    mode_t mode = 0;
    if(flags & O_CREAT) {
	va_list va;
	va_start(va, flags);
	mode = va_arg(va, mode_t);
	va_end(va);
    }
    DIS_SYSCALL;
    int ret = ev_open("openat", pathname, next(dirfd, pathname, flags, mode));
    EN_SYSCALL;
    return ret;
}

int openat64(int dirfd, const char *pathname, int flags, ...)
{
    static int (*next)(int, const char *, int, ...) = NULL;
    if(!next)
	next = dlsym(RTLD_NEXT, "openat64");
    mode_t mode = 0;
    if(flags & O_CREAT) {
	va_list va;
	va_start(va, flags);
	mode = va_arg(va, mode_t);
	va_end(va);
    }
    DIS_SYSCALL;
    int ret = ev_open("openat64", pathname, next(dirfd, pathname, flags, mode));
    EN_SYSCALL;
    return ret;
}
#endif

static void ev_close(int fd)
{
    pthread_mutex_lock(&buf_lock);
#define close_fd(t) do { \
    struct t##fdcap **p; \
    for(p = &t##_fd; *p; p = &(*p)->next) \
	if((*p)->fd == fd) { \
	    struct t##fdcap *c = *p; \
	    /* critical section, protected by buf_lock */ \
	    *p = c->next; \
	    /* critical section, protected by buf_lock */ \
	    c->next = free_##t##_fd; \
	    free_##t##_fd = c; \
	    fprintf(logf, "closing %d\n", fd); \
	    /* skip js search if this is ev */ \
	    pthread_mutex_unlock(&buf_lock); \
	    return; \
	} \
} while(0)
    close_fd(ev);
    close_fd(js);
    pthread_mutex_unlock(&buf_lock);
}

int close(int fd)
{
    ev_close(fd);
    return real_close(fd);
}

/* fopen seems to be what c++ uses */
#if CAP_FOPEN
/* requires fread, fclose intercept as well, and probably more */
/* however, gcc uses read(), so only fclose() for now */
FILE *fopen(const char *pathname, const char *mode)
{
    static FILE * (*next)(const char *, const char *) = NULL;
    if(!next)
	next = dlsym(RTLD_NEXT, "fopen");
    DIS_SYSCALL;
    FILE *res = next(pathname, mode);
    EN_SYSCALL;
    if(!res)
	return res;
    int fd = fileno(res);
    if(fd < 0)
	return res;
    fd = ev_open("fopen", pathname, fd);
    return res;
}

FILE *fopen64(const char *pathname, const char *mode)
{
    static FILE * (*next)(const char *, const char *) = NULL;
    if(!next)
	next = dlsym(RTLD_NEXT, "fopen64");
    DIS_SYSCALL;
    FILE *res = next(pathname, mode);
    EN_SYSCALL;
    if(!res)
	return res;
    int fd = fileno(res);
    if(fd < 0)
	return res;
    fd = ev_open("fopen64", pathname, fd);
    return res;
}

int fclose(FILE *f)
{
    static int (*next)(FILE *) = NULL;
    if(!next)
	next = dlsym(RTLD_NEXT, "fclose");
    if(!f)
	return next(f);
    int fd = fileno(f);
    if(fd < 0)
	return next(f);
    ev_close(fd);
    return next(f);
}
#endif

static struct evfdcap *cap_of(int fd)
{
    for(struct evfdcap *cap = ev_fd; cap; cap = cap->next)
	if(cap->fd == fd)
	    return cap;
    return NULL;
}

/* this is where most of the translation takes place:  modify read events */
/* note that I do not intercept other forms of read as no known program uses them */
/* e.g. readv, pread, preadv, aio_read, fread, fscanf, getc/fgetc, fgets */
ssize_t read(int fd, void *buf, size_t count)
{
    static ssize_t (*next)(int, void *, size_t) = NULL;
    if(!next)
	next = dlsym(RTLD_NEXT, "read");
    /* this is complicated if the caller read less than even multiple of
     * sizeof(ev).  Need to force a read of even multiple from device and
     * keep the excess read for future returns */
    /* very unlikely to ever happen */
    struct input_event ev;
    int ret_adj = 0;
    struct evfdcap *cap = cap_of(fd);
    if(cap && cap->excess_read) {
	ret_adj = count < cap->excess_read ? count : cap->excess_read;
	memcpy(buf, cap->ebuf, ret_adj);
	if(ret_adj < cap->excess_read)
	    memmove(cap->ebuf, cap->ebuf + ret_adj, cap->excess_read - ret_adj);
	cap->excess_read -= ret_adj;
	count -= ret_adj;
	if(!count)
	    return ret_adj;
	buf += ret_adj;
    }
    ssize_t ret = next(fd, buf, count);
    if(ret < 0 || !cap)
	return ret;
    const struct evjrconf *sec = cap->conf;
    int nread = ret;
    while(nread > 0) {
	if(nread < sizeof(ev)) {
	    int todo = cap->excess_read = sizeof(ev) - nread;
	    while(todo > 0) {
		int r = next(fd, cap->ebuf + cap->excess_read - todo, todo);
		if(r < 0 && errno != EINTR && errno != EAGAIN) {
		    cap->excess_read = 0;
		    return r;
		}
		if(r > 0)
		    todo -= r;
	    }
	    memcpy(&ev, buf, nread);
	    memcpy(((char *)&ev) + nread, cap->ebuf, cap->excess_read);
	} else
	    memcpy(&ev, buf, sizeof(ev));
	/* now ev has an event. */
	int drop = 0, mod = 0; /* drop it?  copy it back? */
	if(ev.type == EV_KEY) {
	    drop = sec->filter_bt;
	    const struct butmap *m = &sec->bt_map[ev.code - sec->bt_low];
	    if(ev.code >= sec->bt_low && ev.code < sec->bt_low + sec->nbt &&
	       (m->flags & BTFL_MAP)) {
		if((drop = m->target == -1))
		    ;
		else if(!(m->flags & BTFL_AXIS)) {
		    mod = ev.code != m->target || (m->flags & BTFL_INVERT);
		    ev.code = m->target;
		    if(m->flags & BTFL_INVERT)
			ev.value = 1 - ev.value;
		} else {
		    int pressed = ev.value;
		    int ax = pressed ? m->onax : m->offax;
		    if(ax < 0)
			drop = 1;
		    else {
			ev.type = EV_ABS;
			ev.code = ax;
			cap->axval[ax] = ev.value = pressed ? m->onval : m->offval;
			mod = 1;
		    }
		}
	    }
	} else if(ev.type == EV_ABS) {
	    drop = sec->filter_ax;
	    struct axmap *m = &sec->ax_map[ev.code];
	    if(ev.code >= 0 && ev.code < sec->nax && (m->flags & AXFL_MAP)) {
		if((drop = m->target == -1))
		    ;
		else if(!(m->flags & AXFL_BUTTON)) {
		    mod = ev.code != m->target || (m->flags & (AXFL_INVERT | AXFL_RESCALE));
		    ev.code = m->target;
		    if(m->flags & AXFL_RESCALE) {
			ev.value = (ev.value - m->offthresh) * ((long)m->ai.maximum - m->ai.minimum + 1) / ((long)m->onthresh - 2 * m->offthresh + 1) + m->ai.minimum;
			if(m->flags & AXFL_INVERT)
			    ev.value = m->ai.minimum + m->ai.maximum - ev.value;
		    } else if(m->flags & AXFL_INVERT)
			ev.value = m->onthresh - ev.value;
		} else {
		    mod = 1;
		    ev.type = EV_KEY;

		    int pressed = !!(m->flags & AXFL_PRESSED);
		    int npressed = !(m->flags & AXFL_NPRESSED);
		    int tog, ntog; /* did the target/ntarget state change? */
		    if(ev.value >= m->onthresh)
			tog = !pressed;
		    else if(ev.value < m->offthresh)
			tog = pressed;
		    else
			tog = 0;
		    if(ev.value <= m->nonthresh)
			ntog = !npressed;
		    else if(ev.value > m->noffthresh)
			ntog = npressed;
		    else
			ntog = 0;
		    /* can't send multiple events right now, so if tog and
		     * ntog, only tog is honored */
		    /* sending multiple events will require intercepting
		     * poll(2), select(2), and probably others in the
		     * same family (ppoll, pselect, epoll?, aliases) */
		    /* the only time it's easy is if the buffer size is
		     * big enough to just insert them, or if we can rely
		     * on the program looping until 0 returned */
		    /* most devices send lots of axis events, so probably
		     * ntog will eventually be honored */
		    if(tog) {
			ev.code = m->target;
			ev.value = !pressed;
			m->flags ^= AXFL_PRESSED;
		    } else if(ntog) {
			ev.code = m->ntarget;
			ev.value = !npressed;
			m->flags ^= AXFL_NPRESSED;
		    } else
			drop = 1;
		}
	    }
	}
	/* the best way to drop the event would be to remove it entirely.
	 * Is this safe?  Maybe.  If the program expects data, and insists
	 * on it, it may crash.  Also, if removing an event reduces the
	 * return length to 0, another read() should be done on the device.
	 * It probably doesn't matter if the device is blocking or not,
	 * since the behavior will mostly match what the program expects.
	 * Well, except for the now superfluous SYN_REPORT events. */
	/* I used to instead convert to SYN_DROPPED.  Is this safe?  not
	 * if SYN is dropped via EVIOCSMASK, or if the program has special
	 * behavior on seeing SYN_DROPPED events. */
	/* Now I allow a choice */
	/* FIXME:  add option to drop SYN_REPORT if all prior events dropped */
	if(drop) {
	    if(sec->syn_drop) {
		ev.code = SYN_DROPPED;
		ev.type = EV_SYN;
		ev.value = 0;
		mod = 1;
	    } else {
		mod = 0;
		ret -= sizeof(ev);
		if(nread > sizeof(ev)) {
		    memmove(buf, buf + sizeof(ev), nread - sizeof(ev));
		    buf -= sizeof(ev);
		}
		if(ret <= 0) {
		    cap->excess_read = 0;
		    return read(fd, buf, count);
		}
	    }
	}
	if(mod) {
	    memcpy(buf, &ev, cap->excess_read ? nread : sizeof(ev));
	    if(cap->excess_read)
		memcpy(cap->ebuf, (char *)&ev + nread, cap->excess_read);
	}
	nread -= sizeof(ev);
	buf += sizeof(ev);
    }
    return ret + ret_adj;
}

/* The rest of the translation takes place here: modifying ioctl returns */
int ioctl(int fd, unsigned long request, ...)
{
    int ret, i, len;
    void *argp = NULL;
    if(_IOC_SIZE(request)) {
	va_list va;
	va_start(va, request);
	argp = va_arg(va, void *);
	va_end(va);
    }
    
    struct evfdcap *cap = cap_of(fd);
    if(cap) {
	const struct evjrconf *sec = cap->conf;
#define cpstr(n, s) do { \
    if(!s) \
	return real_ioctl(fd, request, argp); \
    fprintf(logf, "%d: EVIOC" n " -> %s\n", fd, s); \
    len = strlen(s); \
    if(++len < _IOC_SIZE(request)) \
	memcpy(argp, s, len); \
    else \
	memcpy(argp, s, (len = _IOC_SIZE(request))); \
} while(0)
#define cpmem(n, m) do { \
    fprintf(logf, "%d: altered EVIOC" n "\n", fd); \
    len = _IOC_SIZE(request); \
    if(len > sizeof(m)) { \
	memset((char *)argp + sizeof(m), 0, len - sizeof(m)); \
	len = sizeof(m); \
    } \
    memcpy(argp, &m, len); \
} while(0)
	switch(_IOC_NR(request)) {
	  case _IOC_NR(EVIOCGNAME(0)):
	    cpstr("GNAME", sec->repl_name);
	    return len;
	  case _IOC_NR(EVIOCGID):
	    if(!sec->repl_id)
		break;
	    fprintf(logf, "%d: altered ID\n", fd);
	    /* cap->repl_id_val was filled in by init_joy() */
	    memcpy(argp, &cap->repl_id_val, sizeof(cap->repl_id_val));
	    return 0;
	  case _IOC_NR(EVIOCGUNIQ(0)):
	    cpstr("GUNIQ", sec->repl_uniq);
	    return len;
	  case _IOC_NR(EVIOCGKEY(0)):
	    /* this code mostly matches init_joy()'s GKEY mask initializer */
	    /* except that it has to handle INVERT as needed */
	    memset(&cap->keystates, 0, sizeof(cap->keystates));
	    ret = real_ioctl(fd, EVIOCGKEY(sizeof(cap->keystates_in)), cap->keystates_in);
	    if(ret < 0)
		return ret;
	    for(i = 0; i < sec->nbt; i++) {
		if((sec->bt_map[i].flags & (BTFL_MAP | BTFL_AXIS)) != BTFL_MAP)
		    continue;
		if(!ULISSET(cap->keystates_in, sec->bt_low + i) == !(sec->bt_map[i].flags & BTFL_INVERT)) {
		    if(sec->bt_map[i].flags & BTFL_INVERT)
			ULCLR(cap->keystates_in, sec->bt_low + i);
		    continue;
		}
		if(sec->bt_map[i].target >= 0)
		    ULSET(cap->keystates, sec->bt_map[i].target);
		ULCLR(cap->keystates_in, sec->bt_low + i);
	    }
	    for(i = 0; i < sec->nax; i++)
		if((sec->ax_map[i].flags & (AXFL_MAP | AXFL_BUTTON)) == (AXFL_MAP | AXFL_BUTTON)) {
		    if(sec->ax_map[i].target >= 0)
			ULCLR(cap->keystates_in, sec->ax_map[i].target);
		    if(sec->ax_map[i].ntarget >= 0)
			ULCLR(cap->keystates_in, sec->ax_map[i].ntarget);
		    if(sec->ax_map[i].flags & AXFL_PRESSED)
			ULSET(cap->keystates, sec->ax_map[i].target);
		    if(sec->ax_map[i].flags & AXFL_NPRESSED)
			ULSET(cap->keystates, sec->ax_map[i].ntarget);
		}
	    if(!sec->filter_bt)
		for(i = 0; i < MINBITS(KEY_MAX); i++)
		    cap->keystates[i] |= cap->keystates_in[i];
	    cpmem("GKEY()", cap->keystates);
	    return len;
	  case _IOC_NR(EVIOCGBIT(EV_ABS, 0)):
	    /* filled in by init_joy() */
	    cpmem("GBIT(EV_ABS)", cap->absout);
	    return len;
	  case _IOC_NR(EVIOCGBIT(EV_KEY, 0)):
	    /* filled in by init_joy() */
	    cpmem("GBIT(EV_KEY)", cap->keysout);
	    return len;
	  default:
	    if(_IOC_NR(request) >= _IOC_NR(EVIOCGABS(0)) &&
	       _IOC_NR(request) < _IOC_NR(EVIOCGABS(ABS_MAX))) {
		/* return absinfo for *target*, unlike read which uses index */
		/* also rescale and invert as needed */
		int ax = _IOC_NR(request) - _IOC_NR(EVIOCGABS(0));
		for(i = 0; i < sec->nax; i++)
		    if((sec->ax_map[i].flags & (AXFL_MAP | AXFL_BUTTON)) == AXFL_MAP &&
		       sec->ax_map[i].target == ax) {
			int ret = real_ioctl(fd, EVIOCGABS(i), argp);
			if(ret >= 0 && (sec->ax_map[i].flags & AXFL_RESCALE)) {
			    const struct axmap *m = &sec->ax_map[i];
			    long value = ((struct input_absinfo *)argp)->value;
			    memcpy(argp, &m->ai, sizeof(m->ai));
			    value = (value - m->offthresh) * ((long)m->ai.maximum - m->ai.minimum + 1) / ((long)m->onthresh - 2 * m->offthresh + 1) + m->ai.minimum;
			    if(m->flags & AXFL_INVERT)
				value = m->ai.minimum + m->ai.maximum - value;
			    ((struct input_absinfo *)argp)->value = value;
			} else if(ret >= 0 && (sec->ax_map[i].flags & AXFL_INVERT)) {
			    struct input_absinfo *ai = argp;
			    ai->value = sec->ax_map[i].onthresh - ai->value;
			}
			return ret;
		    }
		for(i = 0; i < sec->nbt; i++)
		    if((sec->bt_map[i].flags & (BTFL_MAP | BTFL_AXIS)) == (BTFL_MAP | BTFL_AXIS) &&
		       (sec->bt_map[i].onax == ax || sec->bt_map[i].offax == ax)) {
			/* FIXME:  support rescaling? */
			struct input_absinfo ai = {
			    .minimum = -1, .maximum = 1, .value = cap->axval[i]
			    /* resolution? */
			};
			cpmem("GABS", ai);
			return 0;
		    }
		if(!sec->filter_ax && (ax >= sec->nax || !(sec->ax_map[ax].flags & AXFL_MAP)))
		    return real_ioctl(fd, request, argp);
		errno = EINVAL;
		return -1;
	    }
	}
    } else if(_IOC_NR(request) == _IOC_NR(JSIOCGNAME(0))) {
	for(struct jsfdcap *jn = js_fd; jn; jn = jn->next)
	    if(jn->fd == fd) {
		/* yeah, JSIOC not EVIOC, but it's just a message */
		cpstr("JGNAME", jn->conf->repl_name);
		return len;
	    }
    }
    return real_ioctl(fd, request, argp);
}
