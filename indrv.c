/*
 * TODO:  make this literate code, or at least cleaner overall
 *
 * uinput driver to replace gamepad
 *
 * Usage:  indrv [-d] "joystick to replace" ["new joystick name"] < mapping
 *
 * Run this as root, preferably before a joystick is plugged in.
 *
 * This will wait until "joystick to replace" is present, and remove its
 * device files.  With -d, it will also remove any device files with same
 * initial string in name (e.g. the ds4 motion sensor & touchpad).
 * TODO:  support multiple input pads with same name
 * TODO:  support multiple output pads
 * TODO:  support "any pad" and/or patterns
 * Mapping is of form:
 *   key <from-id> key <to-id>
 *   axis <from-id> axis <to-id>
 * This is run through C preprocessor with <linux/input-event-codes.h> for
 * convenience:  you can have comments and use symbolic names for ids
 * Any missing from-id is ignored, and any missing to-id is never generated
 * TODO:  support key -> axis and axis -> key
 *        [both require more complex config:  on/off values & threashold/speed]
 * TODO:  support default mapping XBOX 360 -> XBOX 360
 * TODO:  test/fix vibration support
 * TODO:  compute raw value remapping instead of doing a recalibrate
 * TODO longterm:  support input from keyboard as well
 * TODO longterm:  companion LD_PRELOAD for keyboard output overrides
 * TODO longterm:  support chording & custom state flags
 * TODO longterm:  support repeats
 * TODO longterm:  support macros (via pipe?)
 * TODO longterm:  support alt. configs depending on program (via pipe?)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/file.h>
#include <errno.h>
#include <poll.h>

#include <linux/input.h>
#include <linux/joystick.h>
#include <linux/uinput.h>

struct map {
    /* FIXME: ax should support +, -, and from "0" */
    short from_key, from_ax, from_jax;
    short to_key, to_ax, to_jax;
} *map;
int num_map = 0;

int sort_by_to_ax(const void *av, const void *bv)
{
    const struct map *a = av, *b = bv;
    return a->to_ax - b->to_ax;
}

int sort_by_from_ax(const void *av, const void *bv)
{
    const struct map *a = av, *b = bv;
    return a->from_ax - b->from_ax;
}

int deldev = 0;

#define wioctl(a, b, c) if(ioctl(a, b, c) < 0) { perror(#b); exit(1); }

const char *joy_name;

int my_what(int idev, const char *what)
{
    char in_name[80], ipath[80];
    int i;
    wioctl(idev, UI_GET_SYSNAME(sizeof(in_name)), &in_name);
    for(i = 0; i < 64; i++) {
	sprintf(ipath, "/sys/class/input/%.20s/%s%d", in_name, what, i);
	if(!access(ipath, F_OK))
	    return i;
    }
    perror(ipath);
    exit(1);
}

int my_event(int idev)
{
    static int ev_no = -1;
    if(ev_no < 0)
	ev_no = my_what(idev, "event");
    return ev_no;
}

int my_joy(int idev)
{
    static int j_no = -1;
    if(j_no < 0)
	j_no = my_what(idev, "js");
    return j_no;
}

int find_joy(int idev)
{
    char in_name[80];
    int dev;
    while(1) {
	for(int i = 0; i < 64; i++) {
	    sprintf(in_name, "/dev/input/event%d", i);
	    if((dev = open(in_name, O_RDWR)) < 0)
		continue;
	    if(ioctl(dev, EVIOCGNAME(sizeof(in_name)), in_name) < 0) {
		close(dev);
		continue;
	    }
	    if(!strcmp(joy_name, in_name)) {
		sprintf(in_name, "/dev/input/event%d", i);
		remove(in_name);
		if(deldev) {
		    int ddev;
		    int nlen = strlen(joy_name);
		    for(i = 0; i < 64; i++) {
			sprintf(in_name, "/dev/input/event%d", i);
			if((ddev = open(in_name, O_RDWR)) < 0)
			    continue;
			if(ioctl(ddev, EVIOCGNAME(sizeof(in_name)), in_name) < 0) {
			    close(ddev);
			    continue;
			}
			if(!strncmp(in_name, joy_name, nlen)) {
			    sprintf(in_name, "/dev/input/event%d", i);
			    remove(in_name);
			}
			close(ddev);
		    }
		}
		int jdev = -1, jno = -1;
		for(i = 0; i < 10; i++) {
		    sprintf(in_name, "/dev/input/js%d", i);
		    jdev = open(in_name, O_RDONLY);
		    if(jdev < 0)
			continue;
		    if(ioctl(jdev, JSIOCGNAME(sizeof(in_name)), in_name) < 0 ||
		       strcmp(joy_name, in_name)) {
			close(jdev);
			continue;
		    }
		    jno = i;
		    break;
		}
		/* need to "recalibrate" axes */
		/* need to do this via event/js, not uinput.  ugh */
		/* FIXME:  do this by adjusting values at remap time instead */
		sprintf(in_name, "/dev/input/event%d", my_event(idev));
		int myev = open(in_name, O_RDWR);
		if(myev < 0) {
		    perror(in_name);
		    exit(1);
		}
		sprintf(in_name, "/dev/input/js%d", my_joy(idev));
		int myjs = open(in_name, O_RDWR);
		if(myjs < 0) {
		    perror(in_name);
		    exit(1);
		}
		__u8 ninax, noutax;
		wioctl(jdev, JSIOCGAXES, &ninax);
		wioctl(myjs, JSIOCGAXES, &noutax);
		struct js_corr inc[ninax], outc[noutax];
		wioctl(jdev, JSIOCGCORR, inc);
		wioctl(myjs, JSIOCGCORR, outc);
		for(i = 0; i < num_map; i++)
		    if(map[i].to_ax >= 0 && map[i].from_ax >= 0) {
			struct input_absinfo iabs;
			wioctl(dev, EVIOCGABS(map[i].from_ax), &iabs);
			wioctl(myev, EVIOCSABS(map[i].to_ax), &iabs);
			if(map[i].to_jax < noutax && map[i].from_jax < ninax)
			    outc[map[i].to_jax] = inc[map[i].from_jax];
		    }
		wioctl(myjs, JSIOCSCORR, outc);
		close(myev);
		close(myjs);
		if(jno >= 0) {
		    sprintf(in_name, "/dev/input/js%d", jno);
		    remove(in_name);
		    close(jdev);
		}
		fprintf(stderr, "joy connect\n");
		return dev;
	    }
	    close(dev);
	}
	struct pollfd pfd;
	struct input_event ev;
	pfd.fd = idev;
	pfd.events = POLLIN;
	poll(&pfd, 1, 2000);
	if(pfd.revents & POLLIN)
	    if(read(idev, &ev, sizeof(ev)) != sizeof(ev))
		continue;
    }
}

int main(int argc, const char **argv)
{
    if(argc > 1 && (deldev = !strcmp(argv[1], "-d"))) {
	argc--;
	argv++;
    }
    if(argc < 2)
	exit(1);
    argc--;
    joy_name = *++argv;
    char inkey, outkey;
    short incode, outcode;
    /* allow use of symbolic key/axis names */
    /* not sure when this was split out of input.h (which can't be used) */
    FILE *inf = popen("cpp -P -include linux/input-event-codes.h", "r");
    if(!inf)
	exit(1);
    /* dead simple parser; doesn't even check if "axis" is the other word */
    /* FIXME: allow keyboard & mouse events as well (but how?) */
    while(fscanf(inf, " %c%*s %hi %c%*s %hi", &inkey, &incode, &outkey, &outcode) == 4) {
	if(!num_map)
	    map = malloc(16 * sizeof(*map));
	else if(!(num_map / 16))
	    map = realloc(map, (num_map + 16) * sizeof(*map));
	if(!map)
	    exit(1);
	map[num_map].from_key = map[num_map].to_key = map[num_map].from_ax = map[num_map].to_ax = -1;
	if(inkey == 'k')
	    map[num_map].from_key = incode;
	else
	    map[num_map].from_ax = incode;
	if(outkey == 'k')
	    map[num_map].to_key = outcode;
	else
	    map[num_map].to_ax = outcode;
	num_map++;
    }
    fclose(inf);
    if(!num_map)
	exit(1);
    /* to support gaps in axes, the js axis must be found */
    qsort(map, num_map, sizeof(*map), sort_by_to_ax);
    for(int i = 0, j = 0; i < num_map; i++)
	if(map[i].to_ax >= 0)
	    map[i].to_jax = j++;
    qsort(map, num_map, sizeof(*map), sort_by_from_ax);
    for(int i = 0, j = 0; i < num_map; i++)
	if(map[i].from_ax >= 0)
	    map[i].from_jax = j++;
    /* FIXME: scan for joystick device(s) */
    /* FIXME: delete old joystick somehow */
    /* Can't use locks (advisory is useless and mandatory is painful at best) */
    /* perhaps the best thing would be to scan for joysticks, so this can run first */
    /* at leat then js0 is this instead of the real joystick */
    /* Another alternative is to delete js0 and rename js1 to js0 */
    /* Similarly for the event device */
    int idev = open("/dev/input/uinput", O_RDWR);
    if(idev < 0 && errno == ENOENT)
	idev = open("/dev/uinput", O_RDWR);
    if(idev < 0) {
	perror("uinput");
	exit(1);
    }
    struct uinput_setup is = {};
    if(argc > 1)
	strncpy(is.name, argv[1], sizeof(is.name));
    else
	strcpy(is.name, "Microsoft X-Box 360 pad");
    is.name[sizeof(is.name) - 1] = 0;
    is.id.bustype = BUS_USB;
    is.id.vendor = 0x045e;
    is.id.product = 0x028e;
    is.id.version = 1;
    is.ff_effects_max = 16;
    wioctl(idev, UI_DEV_SETUP, &is);
    wioctl(idev, UI_SET_EVBIT, EV_SYN);
    wioctl(idev, UI_SET_EVBIT, EV_KEY);
    wioctl(idev, UI_SET_EVBIT, EV_ABS);

    for(int i = 0; i < num_map; i++) {
	if(map[i].to_key >= 0) {
	    wioctl(idev, UI_SET_KEYBIT, map[i].to_key);
	} else if(map[i].to_ax >= 0) {
	    /* wioctl(idev, UI_SET_ABSBIT, map[i].to_ax); */ /* automatic after ABS_SETUP */
	    struct uinput_abs_setup as = {};
	    as.code = map[i].to_ax;
	    /* this stuff really doesn't matter since it is overridden on connect */
	    switch (as.code) {
	      case ABS_X:
	      case ABS_Y:
	      case ABS_RX:
	      case ABS_RY:	/* the two sticks */
		as.absinfo.minimum = -32768;
		as.absinfo.maximum = 32767;
		as.absinfo.fuzz = 16;
		as.absinfo.flat = 128;
		break;
	      case ABS_Z:
	      case ABS_RZ:	/* the triggers (if mapped to axes) */
		as.absinfo.minimum = 0;
		as.absinfo.maximum = 255; /* 1023 for xb1 */
		break;
	      case ABS_HAT0X:
	      case ABS_HAT0Y:	/* the d-pad (only if dpad is mapped to axes */
		as.absinfo.minimum = -1;
		as.absinfo.maximum = 1;
		break;
	    }
	    /* resolution?? */
	    wioctl(idev, UI_ABS_SETUP, &as);
	}
    }
    wioctl(idev, UI_SET_EVBIT, EV_FF);
    wioctl(idev, UI_SET_FFBIT, FF_RUMBLE);
    wioctl(idev, UI_SET_FFBIT, FF_PERIODIC);
    wioctl(idev, UI_SET_FFBIT, FF_SQUARE);
    wioctl(idev, UI_SET_FFBIT, FF_TRIANGLE);
    wioctl(idev, UI_SET_FFBIT, FF_SINE);
    wioctl(idev, UI_SET_FFBIT, FF_GAIN);
    wioctl(idev, UI_DEV_CREATE, 0);

    int joy = find_joy(idev);
    if(joy < 0) {
	perror("joy");
	exit(1);
    }
#if 0
    int nff;
    wioctl(joy, EVIOCGEFFECTS, &nff);
    is.ff_effects_max = nff;
    wioctl(idev, UI_DEV_SETUP, &is);
#endif

    struct pollfd pfd[2];
    pfd[0].fd = joy;
    pfd[1].fd = idev;
    pfd[0].events = pfd[1].events = POLLIN;
    struct input_event ev;
    while(1) {
	pfd[0].revents = pfd[1].revents = 0;
	poll(pfd, 2, -1);
	if(pfd[0].revents & POLLERR) {
	    close(joy);
	    fprintf(stderr, "joy disconnect\n");
	    joy = find_joy(idev);
	}
	if(pfd[0].revents & POLLIN) {
	    do {
		if(read(joy, &ev, sizeof(ev)) != sizeof(ev))
		    continue;
		for(int i = 0; i < num_map; i++) {
		    if((ev.type == EV_KEY && map[i].from_key == ev.code) ||
		       (ev.type == EV_ABS && map[i].from_ax == ev.code)) {
			ev.code = map[i].to_ax < 0 ? map[i].to_key : map[i].to_ax;
			if(ev.code < 0)
			    continue;
			/* FIXME: ax-to-ax may need scaling */
			if((map[i].to_ax < 0) != (map[i].from_ax < 0)) {
			    /* FIXME: convert key->axis or vice-versa */
			}
			break;
		    }
		}
		if(write(idev, &ev, sizeof(ev)) != sizeof(ev)) {
		    perror("write");
		    exit(1);
		}
	    } while(poll(pfd, 1, 0) > 0 && (pfd[0].revents & POLLIN));
	}
	if(pfd[1].revents & POLLIN) {
	    do {
		if(read(idev, &ev, sizeof(ev)) != sizeof(ev))
		    continue;
		if(ev.type != EV_UINPUT) {
		    if(write(joy, &ev, sizeof(ev)) != sizeof(ev))
			perror("write ff");
		    continue;
		}
		if(ev.code == UI_FF_UPLOAD) {
		    struct uinput_ff_upload ffu = {};
		    ffu.request_id = ev.value;
		    wioctl(idev, UI_BEGIN_FF_UPLOAD, &ffu);
		    /* input apparently sets this for new ones */
		    /* so we either need to track whether or not new */
		    /* or just always assume new */
		    /* probably need to maintain mapping table */
		    /* and reset to -1 on disconnect */
		    ffu.effect.id = -1;
		    ffu.retval = ioctl(joy, EVIOCSFF, &ffu.effect);
		    if(ffu.retval < 0)
			ffu.retval = -errno;
		    wioctl(idev, UI_END_FF_UPLOAD, &ffu);
		} else if(ev.code == UI_FF_ERASE) {
		    struct uinput_ff_erase ffe = {};
		    ffe.request_id = ev.value;
		    wioctl(idev, UI_BEGIN_FF_ERASE, &ffe);
		    ffe.retval = ioctl(joy, EVIOCRMFF, ffe.effect_id);
		    if(ffe.retval < 0)
			ffe.retval = -errno;
		    wioctl(idev, UI_END_FF_ERASE, &ffe);
		}
	    } while(poll(pfd + 1, 1, 0) > 0 && (pfd[1].revents & POLLIN));
	}
    }
    return 0;
}
