#include <stdlib.h>
#include <stdio.h>
#include <X11/extensions/XInput2.h>

static Display *dpy;
static int hiding = 0;
XIEventMask evmasks;

/* Return 1 if XI2 is available, 0 otherwise */
static int has_xi2(Display *dpy){
    int major, minor;
    int rc;

    /* We support XI 2.2 */
    major = 2;
    minor = 2;

    rc = XIQueryVersion(dpy, &major, &minor);
    if (rc == BadRequest) {
    printf("No XI2 support. Server supports version %d.%d only.\n", major, minor);
    return 0;
    } else if (rc != Success) {
    fprintf(stderr, "Internal Error! This is a bug in Xlib.\n");
    }

    printf("XI2 supported. Server provides version %d.%d.\n", major, minor);

    return 1;
}

static void select_events(Display *dpy, Window win){

    evmasks.deviceid = XIAllDevices;
    evmasks.mask_len = XIMaskLen(XI_LASTEVENT);
    evmasks.mask = (unsigned char*)calloc(evmasks.mask_len, sizeof(char));

    XISetMask(evmasks.mask, XI_RawButtonPress);
    XISetMask(evmasks.mask, XI_RawButtonRelease);

    XISelectEvents(dpy, win, &evmasks, 1);

    XFlush(dpy);
}

int main (int argc, char **argv){
    int xi_opcode, event, error;
    XEvent ev;

    dpy = XOpenDisplay(NULL);

    if (!dpy) {
    fprintf(stderr, "Failed to open display.\n");
    return -1;
    }

    if (!XQueryExtension(dpy, "XInputExtension", &xi_opcode, &event, &error)) {
       printf("X Input extension not available.\n");
          return -1;
    }

    if (!has_xi2(dpy))
    return -1;

    /* select for XI2 events */
    select_events(dpy, DefaultRootWindow(dpy));

    while(1) {
    	XGenericEventCookie *cookie = &ev.xcookie;
    	XNextEvent(dpy, &ev);
	

    	if (cookie->type != GenericEvent || cookie->extension != xi_opcode ||
    	    !XGetEventData(dpy, cookie))
    	    continue;
		XIDeviceEvent *xie = (XIDeviceEvent *)cookie->data;

    	switch (cookie->evtype) {
			case XI_RawButtonPress:
				if(xie->detail >= 4 && xie->detail <= 7){break;}//IGNORE SCROLL
				//SHIDE CURSOR
					if (hiding)
						{break;}
				XFixesHideCursor(dpy, DefaultRootWindow(dpy));
				hiding = 1;
			break;
			case XI_RawButtonRelease:
				if(xie->detail >= 4 && xie->detail <= 7){break;}//IGNORE SCROLL
				//SHOW CURSOR
				if (!hiding)
					{break;}
				XFixesShowCursor(dpy, DefaultRootWindow(dpy));
				hiding = 0;
			break;
    	}

    	XFreeEventData(dpy, cookie);
    }

    return 0;
}


