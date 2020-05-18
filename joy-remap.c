/*
 * LD_PRELOAD shim which remaps axis and button events for gamepad event
 * devices.  Use jscal to do it for js devices.  Since it's an LD_PRELOAD,
 * it doesn't need root, and it doesn't deal with problems using uinput.
 * It's very simplistic, intended to map exactly one controller to what
 * one program expects to see.  It really doesn't support hot-plugging,
 * although it might work.  It doesn't support multiple devices, although
 * it wouldn't be hard to support later on.*  It is also unable to merge
 * multiple devices into one, such as mapping the Dualshock 3+ motion
 * sensors into the main controller.  It also doesn't support adding
 * autofire and chording.  Since it happens at the user level, js devices
 * associated with the same gamepad will not be affected; again, use jscal
 * for that (sort of) or one of the uinput-based remappers.
 *
 * * one way to support multiple controllers right now would be to compile
 *   two copies of this shim, using different shim names and different
 *   environment variable names, and use them both.  Not guaranteed to work.
 *
 * This uses a configuration file, with one directive per line.  Blank
 * lines and lines beginning with # are ignored, as is initial and trailing
 * whitespace.  The directives are case-sensitive keywords, followed by
 * (skipped) whitespace, followed by a parameter, where needed.  The file
 * name is specified by the mandatory environment variable EV_JOY_REMAP_CONFIG.
 * Keywords are:
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
 *   not the device actually has this axis).  Just a plain decimal number
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
 *   for codes.  Just a plain decimal number or range (separated by -) of
 *   numbers, optionally preceeded by a - to invert the state, maps to the
 *   next unmapped output, starting with 286 (BTN_A).  Just like with axes,
 *   you can also use = to specify the output.  Axes can be mapped
 *   to buttons, as well.  Using ax<num>><num><<num> instead of an input
 *   button code says that when the axis specified by the first number has
 *   a value greater than or equal to the second, press the button, and if
 *   less than or equal to the third, release the button.  If the second is
 *   less than the third, the tests are inverted.  One plain and one inverted
 *   button map per axis is allowed.  Finally, a button ID preceeded by an
 *   exclamation point (!) will explicitly unmap the given input.  Here is
 *   the list of standard gamepad buttons:
 *     A = SOUTH = 286, B = EAST = 287, C = 288,
 *     X = NORTH = 289, Y = WEST = 290, Z = 291,
 *     TL = 292, TR = 293, TL2 = 294, TR2 = 295,
 *     SELECT = 296, START = 297, MODE = 298, THUMBL = 299, THUMBR = 300
 *   Note that most controllers do not have C or Z buttons.  Also, some
 *   controllers only use axes for TL2 and TR2.  My other remappers support
 *   hex and symbolic names (using the C preprocessor with input-event-codes.h),
 *   but not this time.  As a compromise, the above-listed names are supported,
 *   case-insensitive (without the BTN_ prefix, as shown).
 *
 * pass_buttons
 *   Normally, if there are any buttons keywords at all, any inputs not
 *   explicilty mapped are ignored.  This passes through any inputs not
 *   conflicting with outputs through.
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
 *
 */

/* FIXME: check all integer axis and key nubers below ABS_MAX/KEY_MAX */
/* FIXME: support % instead of values for axis->key */

/* for RTLD_NEXT */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <regex.h>
/* why would you be scanning for devices in parallel?  Oh well, some
 * jackass will try and screw this up, so may as well support it */
#include <pthread.h>

static int joy_fd = -1;
static int bt_low, nbt = 0, nax = 0;
/* cant just use nbt/nax because ax map may have buttons & vice-versa */
static int filter_ax = 0, filter_bt = 0;
static struct axmap {
    struct input_absinfo ai; /* for rescaling */
    int flags;
    int target;
    int onthresh, offthresh;
    /* for 2nd key event */
    int ntarget, nonthresh, noffthresh;
} *ax_map;
#define AXFL_MAP      (1<<0)  /* does this need processing? */
#define AXFL_BUTTON   (1<<1)  /* is this a button map? else ax map */
#define AXFL_INVERT   (1<<2)  /* invert before sending on? */
                              /* onthresh is min+max if AXFL_INVERT */
#define AXFL_RESCALE  (1<<3)  /* rescale using ai? */
#define AXFL_NINVERT  (1<<4)  /* invert ntarget before sending out */
#define AXFL_PRESSED  (1<<5)  /* is the button currently pressed? */
                              /* defaults to no, even if axis says otherwise */
#define AXFL_NPRESSED (1<<6)  /* is the neg button pressed? */

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

static unsigned long absout[MINBITS(ABS_MAX)]; /* sent GBITS(EV_ABS) */
static int axval[ABS_MAX]; /* value for key-generated axes */

/* FIXME:  do I need to support EVIOC[GS]KEYCODE*? */
static struct butmap {
    int flags;
    int target;
#define onax target
    int offax, onval, offval; /* val is -1 0 1 on init */
} *bt_map;
#define BTFL_MAP      (1<<0)  /* does this need processing? */
#define BTFL_AXIS     (1<<1)  /* is this an axis map?  else bt map */
#define BTFL_INVERT   (1<<2)  /* invert before sending on? */

static unsigned long keysout[MINBITS(KEY_MAX)],  /* sent GBITS(EV_KEY) */
                     keystates[MINBITS(KEY_MAX)], /* sent GKEY */
                     keystates_in[MINBITS(KEY_MAX)];  /* device GKEY */

static regex_t allow, reject;
static int has_allow = 0, has_reject = 0, filter_dev = 0;

static char *repl_name = NULL, *repl_id = NULL, *repl_uniq = NULL;
static struct input_id repl_id_val;

static char buf[1024];

/* array must be alphabetized */
static const struct bname {
    const char *nm;
    int code;
} bname[] = {
    { "a",	BTN_A },
    { "b",	BTN_B },
    { "c",	BTN_C },
    { "east",	BTN_EAST },
    { "mode",	BTN_MODE },
    { "north",	BTN_NORTH },
    { "select",	BTN_SELECT },
    { "south",	BTN_SOUTH },
    { "start",	BTN_START },
    { "thumbl",	BTN_THUMBL },
    { "thumbr",	BTN_THUMBR },
    { "tl",	BTN_TL },
    { "tl2",	BTN_TL2 },
    { "tr",	BTN_TR },
    { "tr2",	BTN_TR2 },
    { "west",	BTN_WEST },
    { "x",	BTN_X },
    { "y",	BTN_Y },
    { "z",	BTN_Z },
};

static int bncmp(const void *_a, const void *_b)
{
    const struct bname *a = (const struct bname *)_a;
    const struct bname *b = _b;
    int ret = strncasecmp(a->nm, b->nm, a->code);
    if(ret)
	return ret;
    if(b->nm[a->code])
	return -1;
    return 0;
}

/* technically s could be const char **, but then compiler omplains too often */
static int bnum(char **s)
{
    if(!**s)
	return -1;
    if(isdigit(**s))
	return strtol(*s, (char **)s, 10);
    const char *e;
    for(e = *s; isalnum(*e); e++);
    struct bname str = { *s, e - *s};
    struct bname *m = bsearch(&str, bname, sizeof(bname)/sizeof(bname[0]),
			      sizeof(bname[0]), bncmp);
    if(m) {
	*s += strlen(m->nm);
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
    "match",
    "name",
    "pass_axes",
    "pass_buttons",
    "reject",
    "rescale",
    "uniq"
};

enum kw {
    KW_AXES, KW_BUTTONS, KW_FILTER, KW_ID, KW_MATCH, KW_NAME, KW_PASS_AX,
    KW_PASS_BT, KW_REJECT, KW_RESCALE, KW_UNIQ
};

static int kwcmp(const void *_a, const void *_b)
{
    const char *a = _a, * const *b = _b;
    return strcasecmp(a, *b);
}

static void init(void)
{
    /* not thread safe, but shouldn't need to be in early ibit */
    static int did_init = 0;
    if(did_init)
	return;
    did_init = 1;
    const char *fname = getenv("EV_JOY_REMAP_CONFIG");
    FILE *f;
    long fsize;
    char *cfg;

    if(!fname || !(f = fopen(fname, "r"))) {
	perror("EV_JOY_REMAP_CONFIG");
	return;
    }
    /* config should be short enough to fit in memory */
    /* this eliminates the need for line read gymnastics */
    if(fseek(f, 0, SEEK_END) || (fsize = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) ||
       !(cfg = malloc(fsize + 1))) {
	perror(fname);
	fclose(f);
	return;
    }
    if(fread(cfg, fsize, 1, f) != 1) {
	perror(fname);
	fclose(f);
	free(cfg);
	return;
    }
    fclose(f);
    cfg[fsize] = 0;
    char *ln = cfg, *e, c;
    int max_ax = 18, max_bt = 15;
    int auto_ax = -1, auto_bt = BTN_A - 1;
    ax_map = calloc(max_ax, sizeof(*ax_map));
    bt_map = calloc(max_bt, sizeof(*bt_map));
    if(!ax_map || !bt_map) {
	perror("mapping");
	free(cfg);
	if(ax_map)
	    free(ax_map);
	if(bt_map)
	    free(bt_map);
	return;
    }
#define map_resize(what, sz) do { \
    if(max_##what < sz) { \
	int old_max = max_##what; \
	void *old_map = what##_map; \
	while(max_##what < sz) \
	    max_##what *= 2; \
	what##_map = realloc(what##_map, max_##what * sizeof(*what##_map)); \
	if(!what##_map) { \
	    what##_map = old_map; \
	    abort_parse(#what " map too large"); \
	} \
	memset(what##_map + old_max, 0, (max_##what - old_max) * sizeof(*what##_map)); \
    } \
} while(0)
#define abort_parse(msg) do { \
    perror("mapping parse error: " msg); \
    goto err; \
} while(0)
    while(1) {
	while(isspace(*ln))
	    ln++;
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
	for(ln = e; isspace(*ln) && *ln != '\n'; ln++);
	for(e = ln; *e && *e != '\n'; e++);
	while(e > ln && isspace(e[-1]))
	    e--;
	c = *e;
	*e = 0;
	int ret;
	switch(kw - kws) {
	  case KW_MATCH:
	    if(has_allow)
		abort_parse("duplicate match");
	    if((ret = regcomp(&allow, ln, REG_EXTENDED | REG_NOSUB))) {
		regerror(ret, &allow, buf, sizeof(buf));
		fprintf(stderr, "match pattern error: %.*s\n", (int)sizeof(buf), buf);
		regfree(&allow);
		goto err;
	    } else
		has_allow = 1;
	    break;
	  case KW_REJECT:
	    if(has_reject)
		abort_parse("duplicate reject");
	    if((ret = regcomp(&allow, ln, REG_EXTENDED | REG_NOSUB))) {
		regerror(ret, &allow, buf, sizeof(buf));
		fprintf(stderr, "reject pattern error: %.*s\n", (int)sizeof(buf), buf);
		regfree(&allow);
		goto err;
	    } else
		has_reject = 1;
	    break;
	  case KW_FILTER:
	    if(*ln)
		abort_parse("filter takes no parameter");
	    filter_dev = 1;
	    break;
	  case KW_NAME:
	    if(repl_name)
		abort_parse("duplicate name");
	    repl_name = strdup(ln);
	    if(!repl_name)
		abort_parse("name");
	    break;
	  case KW_ID:
	    if(repl_id)
		abort_parse("duplicate id");
	    repl_id = strdup(ln);
	    if(!repl_id)
		abort_parse("id");
	    break;
	  case KW_UNIQ:
	    if(repl_uniq)
		abort_parse("duplicate uniq");
	    repl_uniq = strdup(ln);
	    if(!repl_uniq)
		abort_parse("uniq");
	    break;
	  case KW_AXES:
	    /* I guess duplicates are OK here */
	    /* blank list just skips an auto */
	    if(!*ln) {
		int t;
#define next_auto_axis do { \
    auto_ax++; \
    for(t = 0; t < nax; t++) \
	if((ax_map[t].flags & (AXFL_MAP | AXFL_BUTTON)) == AXFL_MAP && \
	   ax_map[t].target == auto_ax) { \
	    auto_ax++; \
	    t = -1; \
	} \
    for(t = 0; t < nbt; t++) \
	if((bt_map[t].flags & (BTFL_MAP | BTFL_AXIS)) == (BTFL_MAP | BTFL_AXIS) && \
	   (bt_map[t].onax == auto_ax || bt_map[t].offax == auto_ax)) { \
	    auto_ax++; \
	    t = -1; \
	} \
    t = -1; \
} while(0)
		next_auto_axis;
		break;
	    }
	    filter_ax |= 1;
	    while(*ln) {
		int a = -1, t = -1;
		if(*ln == '!' && isdigit(ln[1])) {
		    a = strtol(ln + 1, &ln, 10);
		    if(*ln && *ln != ',')
			abort_parse("invalid !");
		    if(nax <= a)
			nax = a + 1;
		    map_resize(ax, nax);
		    ax_map[a].flags = AXFL_MAP;
		    ax_map[a].target = -1;
		    if(*ln == ',')
			ln++;
		    continue;
		}
		if(isdigit(*ln))
		    a = t = strtol(ln, (char **)&ln, 10); /* yeah, this could overflow.  Who cares? */
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
		    a = strtol(ln, (char **)&ln, 10);
		if(a >= 0) {
		    if(nax <= a)
			nax = a + 1;
		    map_resize(ax, nax);
		    memset(ax_map + a, 0, sizeof(*ax_map));
		    ax_map[a].flags = AXFL_MAP | invert;
		    ax_map[a].target = t < 0 ? auto_ax : t;
		    if(*ln == '-' && isdigit(ln[1])) {
			int b = strtol(ln + 1, (char **)&ln, 10);
			if(b < a)
			    abort_parse("invalid range");
			if(nax <= b)
			    nax = b + 1;
			map_resize(ax, nax);
			for(;++a <= b;) {
			    if(t < 0)
				next_auto_axis;
			    else
				t++;
			    memset(&ax_map[a], 0, sizeof(*ax_map));
			    ax_map[a].flags = AXFL_MAP | invert;
			    ax_map[a].target = t < 0 ? auto_ax : t;
			}
		    }
		} else if(*ln == 'b') {
		    ln++;
		    if(t < 0) /* we don't need this flag any more as there are no ranges */
			t = auto_ax;
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
    if(!nbt) { \
	if(n < BTN_A || n >= BTN_A + max_bt) \
	    bt_low = n; \
	else \
	    bt_low = BTN_A; \
    } \
    if(n < bt_low) { \
	map_resize(bt, nbt + bt_low - n); \
	memmove(bt_map + bt_low - n, bt_map, nbt * sizeof(*bt_map)); \
	memset(bt_map, 0, (bt_low - n) * sizeof(*bt_map)); \
	nbt += bt_low - n; \
	bt_low = n; \
    } \
    if(nbt <= n - bt_low) { \
	nbt = n - bt_low + 1; \
	map_resize(bt, nbt); \
    } \
} while(0)
		    if(l >= 0) {
			expand_bt(l);
			bt_map[l - bt_low].offax = bt_map[l - bt_low].onax = -1;
		    }
		    if(m >= 0) {
			expand_bt(m);
			bt_map[m - bt_low].offax = bt_map[m - bt_low].onax = -1;
		    }
		    if(h >= 0) {
			expand_bt(h);
			bt_map[h - bt_low].offax = bt_map[h - bt_low].onax = -1;
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
	int wa = w - bt_low; \
	bt_map[wa].flags = BTFL_MAP | BTFL_AXIS; \
	if(w##i) { \
	    bt_map[wa].offax = t; \
	    bt_map[wa].offval = v; \
	} else { \
	    bt_map[wa].onax = t; \
	    bt_map[wa].onval = v; \
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
		int t = strtol(ln, &ln, 10), a;
		if(*ln++ != '=')
		    abort_parse("rescale w/o =");
		for(a = 0; a < nax; a++)
		    if(ax_map[a].target == t)
			break;
		if(a == nax) {
		    a = t;
		    if(a >= nax) {
			nax = a + 1;
			map_resize(ax, nax);
		    }
		    if(ax_map[a].flags & AXFL_MAP)
			abort_parse("rescale target unavailable");
		    ax_map[a].flags = AXFL_MAP;
		    ax_map[a].target = a;
		}
		ax_map[a].flags |= AXFL_RESCALE;
		memset(&ax_map[a].ai, 0, sizeof(ax_map[a].ai));
		ax_map[a].ai.minimum = strtol(ln, &ln, 10);
		if(*ln == ':')
		    ax_map[a].ai.maximum = strtol(ln + 1, &ln, 10);
		if(*ln == ':')
		    ax_map[a].ai.fuzz = strtol(ln + 1, &ln, 10);
		if(*ln == ':')
		    ax_map[a].ai.flat = strtol(ln + 1, &ln, 10);
		if(*ln == ':')
		    ax_map[a].ai.resolution = strtol(ln + 1, &ln, 10);
		if(*ln && *ln != ',')
		    abort_parse("invalid rescale entry");
		if(ax_map[a].ai.maximum <= ax_map[a].ai.minimum)
		    abort_parse("invalid rescale range");
		/* since I don't entirely understand fuzz & flat, I won't check */
	    }
	    break;
	  case KW_PASS_AX:
	    filter_ax = -1;
	    break;
	  case KW_BUTTONS:
	    /* I guess duplicates are OK here */
	    filter_bt |= 1;
	    /* blank list just skips an auto */
	    if(!*ln) {
		int t;
#define next_auto_bt do { \
    auto_bt++; \
    for(t = 0; t < nbt; t++) \
	if((bt_map[t].flags & (BTFL_MAP | BTFL_AXIS)) == BTFL_MAP && \
	   bt_map[t].target == auto_bt) { \
	    auto_bt++; \
	    t = -1; \
	} \
    for(t = 0; t < nbt; t++) \
	if((ax_map[t].flags & (AXFL_MAP | AXFL_BUTTON)) == (AXFL_MAP | AXFL_BUTTON) && \
	   (ax_map[t].target == auto_bt || ax_map[t].ntarget == auto_bt)) { \
	    auto_bt++; \
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
		    a -= bt_low;
		    bt_map[a].flags = BTFL_MAP;
		    bt_map[a].target = -1;
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
			   (*ln != 'a' || ln[1] != 'x' || !isdigit(ln[2]))))
			abort_parse("unexpected -");
		    invert = BTFL_INVERT;
		} else if(isalnum(*ln))
			a = bnum(&ln);
		if(a >= 0) {
		    expand_bt(a);
		    a -= bt_low;
		    memset(bt_map + a, 0, sizeof(*bt_map));
		    bt_map[a].flags = BTFL_MAP | invert;
		    bt_map[a].target = t < 0 ? auto_bt : t;
		    if(*ln == '-' && isalnum(ln[1])) {
			++ln;
			a += bt_low;
			int b = bnum(&ln);
			if(b < a)
			    abort_parse("invalid range");
			expand_bt(b);
			for(;++a <= b;) {
			    if(t < 0)
				next_auto_bt;
			    else
				t++;
			    memset(&bt_map[a - bt_low], 0, sizeof(*bt_map));
			    bt_map[a - bt_low].flags = BTFL_MAP | invert;
			    bt_map[a - bt_low].target = t < 0 ? auto_bt : t;
			}
		    }
		} else if(*ln == 'a' && ln[1] == 'x' && isdigit(ln[2])) {
		    if(t < 0) /* we don't need this flag any more as there are no ranges */
			t = auto_bt;
		    a = strtol(ln + 2, (char **)&ln, 10);
		    int l, h;
		    if(*ln != '>' || !isdigit(ln[1]))
			abort_parse("invalid axis-to-button");
		    l = strtol(ln + 1, (char **)&ln, 10);
		    if(*ln != '<' || !isdigit(ln[1]))
			abort_parse("invalid axis-to-button");
		    h = strtol(ln + 1, (char **)&ln, 10);
		    if(l == h)
			abort_parse("invalid axis-to-button thresholds");
		    if(a >= nax)
			nax = a + 1;
		    map_resize(ax, nax);
		    if(!(ax_map[a].flags & AXFL_BUTTON)) {
			memset(ax_map + a, 0, sizeof(*ax_map));
			ax_map[a].target = ax_map[a].ntarget = -1;
			ax_map[a].flags = AXFL_MAP | AXFL_BUTTON;
		    }
		    if(l < h) {
			ax_map[a].target = t;
			ax_map[a].onthresh = l;
			ax_map[a].offthresh = h;
			ax_map[a].flags |= invert;
		    } else {
			ax_map[a].ntarget = t;
			ax_map[a].nonthresh = l;
			ax_map[a].noffthresh = h;
			ax_map[a].flags |= invert ? AXFL_NINVERT : 0;
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
	    filter_bt = -1;
	    break;
	}
	*e = c;
	ln = e;
    }
    if(filter_ax < 0)
	filter_ax = 0;
    if(filter_bt < 0)
	filter_bt = 0;
    /* disable individual passthrough for mapped outputs */
#define dis_ax(axno) do { \
    if(axno >= 0) { \
	if(nax <= axno) \
	    nax = axno + 1; \
	map_resize(ax, nax); \
	if(!(ax_map[axno].flags & AXFL_MAP)) { \
	    ax_map[axno].flags = AXFL_MAP; \
	    ax_map[axno].target = -1; \
	} \
    } \
} while(0)
#define dis_bt(btno) do { \
    int bno = btno; \
    if(bno >= 0) { \
	expand_bt(bno); \
	bno -= bt_low; \
	if(!(bt_map[bno].flags & BTFL_MAP)) { \
	    bt_map[bno].flags = BTFL_MAP; \
	    bt_map[bno].target = -1; \
	} \
    } \
} while(0)
    int i;
    for(i = 0; i < nbt; i++) {
	if((bt_map[i].flags & (BTFL_MAP | BTFL_AXIS)) == BTFL_MAP) {
	    int ol = bt_low;
	    dis_bt(bt_map[i].target);
	    if(ol != bt_low)
		i += ol - bt_low;
	} else if(bt_map[i].flags & BTFL_MAP) {
	    dis_ax(bt_map[i].onax);
	    dis_ax(bt_map[i].offax);
	}
    }
    for(i = 0; i < nax; i++) {
	if((ax_map[i].flags & (AXFL_MAP | AXFL_BUTTON)) == AXFL_MAP)
	    dis_ax(ax_map[i].target);
	else if(ax_map[i].flags & AXFL_MAP) {
	    dis_bt(ax_map[i].target);
	    dis_bt(ax_map[i].ntarget);
	}
    }
    /* even though technically the first device may be a gamepad due to
     * permissions, I'm forcing you to have a pattern */
    if(!has_allow)
	abort_parse("match pattern required");
    free(cfg);
    fputs("Installed event device remapper\n", stderr);
    return;
err:
    free(ax_map);
    free(bt_map);
    if(has_allow)
	regfree(&allow);
    has_allow = 0;
    if(has_reject)
	regfree(&reject);
    if(repl_uniq)
	free(repl_uniq);
    if(repl_id)
	free(repl_id);
    if(repl_name)
	free(repl_name);
    nbt = nax = 0;
    free(cfg);
}

static int real_ioctl(int fd, unsigned long request, ...);

static void init_joy(int fd)
{
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&lock);
    if(joy_fd >= 0) {
	pthread_mutex_unlock(&lock);
	return;
    }
    joy_fd = fd;
    /* set up ID from string */
    if(repl_id) {
	real_ioctl(fd, EVIOCGID, &repl_id_val);
	char *s = repl_id;
	if(*s && *s != ':')
	    repl_id_val.bustype = strtol(s, &s, 16);
	/* I guess it's OK if fields are missing */
	if(*s == ':') {
	    if(*++s && *s != ':')
		repl_id_val.vendor = strtol(s, &s, 16);
	    if(*s == ':') {
		if(*++s && *s != ':')
		    repl_id_val.product = strtol(s, &s, 16);
		if(*s == ':' && *++s && *s != ':')
		    repl_id_val.version = strtol(s, &s, 16);
	    }
	}
	if(*s) {
	    fprintf(stderr, "joy-remap:  invalid id @ %s\n", s);
	    free(repl_id);
	    repl_id = NULL;
	}
    }
    /* adjust button/axis mappings */
    static unsigned long absin[MINBITS(ABS_MAX)];
    memset(absin, 0, sizeof(absin));
    memset(keysout, 0, sizeof(keysout));
    /* use keystates as temp buffer for keysin */
    memset(keystates, 0, sizeof(keystates));
    if(real_ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keystates)), keystates) < 0 ||
       real_ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absin)), absin) < 0) {
	perror("init_joy");
	joy_fd = -1;
	pthread_mutex_unlock(&lock);
	return;
    }
    int i;
    /* add keys that are mapped if src is present */
    for(i = 0; i < nbt; i++) {
	if((bt_map[i].flags & (BTFL_MAP | BTFL_AXIS)) != BTFL_MAP)
	    continue;
	if(bt_map[i].target < 0) { /* passthrough disable */
	    ULCLR(keystates, bt_low + i);
	    continue;
	}
	if(!ULISSET(keystates, bt_low + i)) {
	    fprintf(stderr, "warning: disabling button %d due to missing %d\n",
		    bt_map[i].target, bt_low + i);
	    continue;
	}
	ULSET(keysout, bt_map[i].target);
	ULCLR(keystates, bt_low + i);
    }
    for(i = 0; i < nax; i++) {
	if((ax_map[i].flags & (AXFL_MAP | AXFL_BUTTON)) != (AXFL_MAP | AXFL_BUTTON))
	    continue;
	if(!ULISSET(absin, i)) {
	    fprintf(stderr, "warning: disabling button %d/%d due to missing axis %d\n",
		    ax_map[i].target, ax_map[i].ntarget, i);
	    continue;
	}
	if(ax_map[i].target >= 0)
	    ULSET(keysout, ax_map[i].target);
	if(ax_map[i].ntarget >= 0)
	    ULSET(keysout, ax_map[i].ntarget);
	ULCLR(absin, i);
    }
    /* add axes that are mapped if src is present */
    for(i = 0; i < nax; i++) {
	if((ax_map[i].flags & (AXFL_MAP | AXFL_BUTTON)) != BTFL_MAP)
	    continue;
	if(ax_map[i].target < 0) { /* passthrough disable */
	    ULCLR(absin, i);
	    continue;
	}
	if(!ULISSET(absin, i)) {
	    fprintf(stderr, "warning: disabling axis %d due to missing %d\n",
		    ax_map[i].target, i);
	    continue;
	}
	ULSET(absout, ax_map[i].target);
	ULCLR(absin, i);
	/* we only need absinfo for INVERT and RESCALE */
	/* and axis->key if I ever support % */
	if(ax_map[i].flags & (AXFL_INVERT | AXFL_RESCALE)) {
	    struct input_absinfo ai;
	    if(real_ioctl(fd, EVIOCGABS(i), &ai) < 0) {
		perror("init_joy");
		joy_fd = -1;
		pthread_mutex_unlock(&lock);
		return;
	    }
	    ax_map[i].offthresh = ai.minimum;
	    ax_map[i].onthresh = ai.minimum + ai.maximum;
	}
    }
    for(i = 0; i < nbt; i++) {
	if((bt_map[i].flags & (AXFL_MAP | AXFL_BUTTON)) != (AXFL_MAP | AXFL_BUTTON))
	    continue;
	if(!ULISSET(keystates, bt_low + i)) {
	    fprintf(stderr, "warning: disabling axis %d/%d due to missing button %d\n",
		    bt_map[i].onax, bt_map[i].offax, i);
	    continue;
	}
	if(bt_map[i].onax >= 0) {
	    ULSET(absout, bt_map[i].onax);
	    axval[bt_map[i].onax] = 0;
	}
	if(bt_map[i].offax >= 0) {
	    ULSET(absout, bt_map[i].offax);
	    axval[bt_map[i].offax] = 0;
	}
	ULCLR(keystates, bt_low + i);
    }
    /* add all remaining keys & axes if no mapping given */
    /* note that if already set above, the raw input will still be dropped
     * if it's not used by a mapping */
    if(!filter_bt)
	for(i = 0; i < MINBITS(KEY_MAX); i++)
	    keysout[i] |= keystates[i];
    if(!filter_ax)
	for(i = 0; i < MINBITS(ABS_MAX); i++)
	    absout[i] |= absin[i];
    pthread_mutex_unlock(&lock);
    return;
}

static int real_ioctl(int fd, unsigned long request, ...)
{
    static void *next = NULL;
    if(!next)
	next = dlsym(RTLD_NEXT, "ioctl");
    void *argp = NULL;
    if(_IOC_SIZE(request)) {
	va_list va;
	va_start(va, request);
	argp = va_arg(va, void *);
	va_end(va);
    }
    return ((int (*)(int, unsigned long, ...))next)(fd, request, argp);
}

static int is_allowed(const char *pathname, int fd)
{
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    /* this is small enough to be local, but we're locking for buf anyway */
    static struct input_id id;

    if(!has_allow)
	return 1;
    /* the lock is for buf & id */
    pthread_mutex_lock(&lock);
    if(real_ioctl(fd, EVIOCGNAME(sizeof(buf)), buf) < 0)
	strcpy(buf, "ERROR: Device name unavailable");
    int rej = has_reject && !regexec(&reject, buf, 0, NULL, 0),
	nmok = !rej && !regexec(&allow, buf, 0, NULL, 0);
    if(!rej) {
	if(real_ioctl(fd, EVIOCGID, &id) < 0)
	    memset(&id, 0, sizeof(id));
	sprintf(buf, "%04X-%04X-%04X-%04X-%.4s", (int)id.bustype,
		(int)id.vendor, (int)id.product, (int)id.version,
		pathname + 16);
	rej = has_reject && !regexec(&reject, buf, 0, NULL, 0);
	if(rej)
	    nmok = 0;
	else if(!nmok)
	    nmok = !regexec(&allow, buf, 0, NULL, 0);
    }
    pthread_mutex_unlock(&lock);
    return nmok;
}

int open(const char *pathname, int flags, ...)
{
    static void *next = NULL;
    if(!next) {
	next = dlsym(RTLD_NEXT, "open");
	init();
    }
    mode_t mode = 0;
    if(flags & O_CREAT) {
	va_list va;
	va_start(va, flags);
	mode = va_arg(va, mode_t);
	va_end(va);
    }
    int ret = ((int (*)(const char *, int, ...))next)(pathname, flags, mode);
    if(ret < 0 || strncmp(pathname, "/dev/input/event", 16))
	return ret;
    if(!is_allowed(pathname, ret)) {
/*    fprintf(stderr, "Rejecting open of %s\n", pathname); */
	close(ret);
	errno = EPERM;
	return -1;
    }
    if(has_allow)
	fprintf(stderr, "Attempting to capture %s (%d)\n", pathname, ret);
    if(has_allow && joy_fd < 0)
	init_joy(ret);
    return ret;
}

/* identical to above, for programs that call this alias */
int open64(const char *pathname, int flags, ...)
{
    static void *next = NULL;
    if(!next) {
	next = dlsym(RTLD_NEXT, "open64");
	init();
    }
    mode_t mode = 0;
    if(flags & O_CREAT) {
	va_list va;
	va_start(va, flags);
	mode = va_arg(va, mode_t);
	va_end(va);
    }
    int ret = ((int (*)(const char *, int, ...))next)(pathname, flags, mode);
    if(ret < 0 || strncmp(pathname, "/dev/input/event", 16))
	return ret;
    if(!is_allowed(pathname, ret)) {
/*    fprintf(stderr, "Rejecting open of %s\n", pathname); */
	close(ret);
	errno = EPERM;
	return -1;
    }
    if(has_allow)
	fprintf(stderr, "Attempting to capture %s (%d)\n", pathname, ret);
    if(has_allow && joy_fd < 0)
	init_joy(ret);
    return ret;
}

int close(int fd)
{
    static void *next = NULL;
    if(!next)
	next = dlsym(RTLD_NEXT, "close");
    int ret = ((int (*)(int))next)(fd);
    if(fd == joy_fd) {
	fprintf(stderr, "closing %d\n", fd);
	joy_fd = -1;
    }
    return ret;
}

ssize_t read(int fd, void *buf, size_t count)
{
    static void *next = NULL;
    if(!next)
	next = dlsym(RTLD_NEXT, "read");
    /* this is complicated if the caller read less than even multiple of
     * sizeof(ev).  Need to force a read of even multiple from device and
     * keep the excess read for future returns */
    static int excess_read = 0;
    struct input_event ev;
    static char ebuf[sizeof(ev)];
    int ret_adj = 0;
    if(fd == joy_fd && excess_read) {
	ret_adj = count < excess_read ? count : excess_read;
	memcpy(buf, ebuf, ret_adj);
	if(ret_adj < excess_read)
	    memmove(ebuf, ebuf + ret_adj, excess_read - ret_adj);
	excess_read -= ret_adj;
	count -= ret_adj;
	if(!count)
	    return ret_adj;
	buf += ret_adj;
    }
    ssize_t ret = ((ssize_t (*)(int, void *, size_t))next)(fd, buf, count);
    if(ret < 0 || fd != joy_fd)
	return ret;
    int nread = ret;
    while(nread > 0) {
	if(nread < sizeof(ev)) {
	    int todo = excess_read = sizeof(ev) - nread;
	    while(todo > 0) {
		int r = ((ssize_t (*)(int, void *, size_t))next)(fd, ebuf + excess_read - todo, todo);
		if(r < 0 && errno != EINTR && errno != EAGAIN) {
		    excess_read = 0;
		    return r;
		}
		if(r > 0)
		    todo -= r;
	    }
	    memcpy(&ev, buf, nread);
	    memcpy(((char *)&ev) + nread, ebuf, excess_read);
	} else
	    memcpy(&ev, buf, sizeof(ev));
	int drop = 0, mod = 0;
	if(ev.type == EV_KEY) {
	    drop = filter_bt;
	    const struct butmap *m = &bt_map[ev.code - bt_low];
	    if(ev.code >= bt_low && ev.code < bt_low + nbt &&
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
			axval[ax] = ev.value = pressed ? m->onval : m->offval;
			mod = 1;
		    }
		}
	    }
	} else if(ev.type == EV_ABS) {
	    drop = filter_ax;
	    struct axmap *m = &ax_map[ev.code];
	    if(ev.code >= 0 && ev.code < nax && (m->flags & AXFL_MAP)) {
		if((drop = m->target == -1))
		    ;
		else if(!(m->flags & AXFL_BUTTON)) {
		    mod = ev.code != m->target || (m->flags & (AXFL_INVERT | AXFL_RESCALE));
		    ev.code = m->target;
		    if(m->flags & AXFL_RESCALE) {
			ev.value = (ev.value - m->offthresh) * (m->ai.maximum - m->ai.minimum) / (m->onthresh - 2 * m->offthresh) + m->ai.minimum;
			if(m->flags & AXFL_INVERT)
			    ev.value = m->ai.minimum + m->ai.maximum - ev.value;
		    } else if(m->flags & AXFL_INVERT)
			ev.value = m->onthresh - ev.value;
		} else {
		    mod = 1;
		    ev.type = EV_KEY;

		    int pressed = !!(m->flags & AXFL_PRESSED);
		    int npressed = !(m->flags & AXFL_NPRESSED);
		    int tog, ntog;
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
	if(drop) {
	    ev.code = SYN_DROPPED; /* safe?  probably not if SYN filtered out */
	    ev.type = EV_SYN;
	    ev.value = 0;
	    mod = 1;
	}
	if(mod) {
	    memcpy(buf, &ev, excess_read ? nread : sizeof(ev));
	    if(excess_read)
		memcpy(ebuf, (char *)&ev + nread, excess_read);
	}
	nread -= sizeof(ev);
	buf += sizeof(ev);
    }
    return ret + ret_adj;
}

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
    if(fd == joy_fd)
#define cpstr(s) do { \
    if(!s) \
	return real_ioctl(fd, request, argp); \
    len = strlen(s); \
    if(++len < _IOC_SIZE(request)) \
	memcpy(argp, s, len); \
    else \
	memcpy(argp, repl_name, (len = _IOC_SIZE(request))); \
} while(0)
#define cpmem(m) do { \
    len = _IOC_SIZE(request); \
    if(len > sizeof(m)) { \
	memset((char *)argp + sizeof(m), 0, len - sizeof(m)); \
	len = sizeof(m); \
    } \
    memcpy(argp, &m, len); \
} while(0)
	switch(_IOC_NR(request)) {
	  case _IOC_NR(EVIOCGNAME(0)):
	    cpstr(repl_name);
	    return len;
	  case _IOC_NR(EVIOCGID):
	    if(!repl_id)
		break;
	    memcpy(argp, &repl_id_val, sizeof(repl_id_val));
	    return 0;
	  case _IOC_NR(EVIOCGUNIQ(0)):
	    cpstr(repl_uniq);
	    return len;
	  case _IOC_NR(EVIOCGKEY(0)):
	    memset(&keystates, 0, sizeof(keystates));
	    ret = real_ioctl(fd, EVIOCGKEY(sizeof(keystates_in)), keystates_in);
	    if(ret < 0)
		return ret;
	    for(i = 0; i < nbt; i++) {
		if((bt_map[i].flags & (BTFL_MAP | BTFL_AXIS)) != BTFL_MAP)
		    continue;
		if(!ULISSET(keystates_in, bt_low + i))
		    continue;
		if(bt_map[i].target >= 0)
		    ULSET(keystates, bt_map[i].target);
		ULCLR(keystates_in, bt_low + i);
	    }
	    for(i = 0; i < nax; i++)
		if((ax_map[i].flags & (AXFL_MAP | AXFL_BUTTON)) == (AXFL_MAP | AXFL_BUTTON)) {
		    if(ax_map[i].target >= 0)
			ULCLR(keystates_in, ax_map[i].target);
		    if(ax_map[i].ntarget >= 0)
			ULCLR(keystates_in, ax_map[i].ntarget);
		    if(ax_map[i].flags & AXFL_PRESSED)
			ULSET(keystates, ax_map[i].target);
		    if(ax_map[i].flags & AXFL_NPRESSED)
			ULSET(keystates, ax_map[i].ntarget);
		}
	    if(!filter_bt)
		for(i = 0; i < MINBITS(KEY_MAX); i++)
		    keystates[i] |= keystates_in[i];
	    cpmem(keystates);
	    return len;
	  case _IOC_NR(EVIOCGBIT(EV_ABS, 0)):
	    cpmem(absout);
	    return len;
	  case _IOC_NR(EVIOCGBIT(EV_KEY, 0)):
	    cpmem(keysout);
	    return len;
	  default:
	    if(_IOC_NR(request) >= _IOC_NR(EVIOCGABS(0)) &&
	       _IOC_NR(request) < _IOC_NR(EVIOCGABS(ABS_MAX))) {
		int ax = _IOC_NR(request) - _IOC_NR(EVIOCGABS(0));
		for(i = 0; i < nax; i++)
		    if((ax_map[i].flags & (AXFL_MAP | AXFL_BUTTON)) == AXFL_MAP &&
		       ax_map[i].target == ax) {
			int ret = real_ioctl(fd, EVIOCGABS(i), argp);
			if(ret >= 0 && (ax_map[i].flags & AXFL_RESCALE)) {
			    const struct axmap *m = &ax_map[i];
			    int value = ((struct input_absinfo *)argp)->value;
			    memcpy(argp, &m->ai, sizeof(m->ai));
			    value = (value - m->offthresh) * (m->ai.maximum - m->ai.minimum) / (m->onthresh - 2 * m->offthresh) + m->ai.minimum;
			    if(m->flags & AXFL_INVERT)
				value = m->ai.minimum + m->ai.maximum - value;
			    ((struct input_absinfo *)argp)->value = value;
			} else if(ret >= 0 && (ax_map[i].flags & AXFL_INVERT)) {
			    struct input_absinfo *ai = argp;
			    ai->value = ax_map[i].onthresh - ai->value;
			}
			return ret;
		    }
		for(i = 0; i < nbt; i++)
		    if((bt_map[i].flags & (BTFL_MAP | BTFL_AXIS)) == (BTFL_MAP | BTFL_AXIS) &&
		       (bt_map[i].onax == ax || bt_map[i].offax == ax)) {
			struct input_absinfo ai = {
			    .minimum = -1, .maximum = 1, .value = axval[i]
			    /* resolution? */
			};
			cpmem(ai);
			return 0;
		    }
		if(!filter_ax)
		    return real_ioctl(fd, request, argp);
		errno = EINVAL;
		return -1;
	    }
	}
    return real_ioctl(fd, request, argp);
}
