#include <stdlib.h>
#include <stdio.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/Xfixes.h>

static Display *dpy;
static int buttons_held = 0; // Tracks number of buttons currently held
XIEventMask evmasks;

/* Return 1 if XI2 is available, 0 otherwise */
static int has_xi2(Display *dpy){
    int major = 2, minor = 2;
    int rc = XIQueryVersion(dpy, &major, &minor);
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

int main(int argc, char **argv){
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

    if (!has_xi2(dpy)) return -1;

    /* Select for XI2 events */
    select_events(dpy, DefaultRootWindow(dpy));

    while (1) {
        XGenericEventCookie *cookie = &ev.xcookie;
        XNextEvent(dpy, &ev);

        if (cookie->type != GenericEvent || cookie->extension != xi_opcode ||
            !XGetEventData(dpy, cookie))
            continue;
        
        XIDeviceEvent *xie = (XIDeviceEvent *)cookie->data;
        
        switch (cookie->evtype) {
            case XI_RawButtonPress:
                if (xie->detail >= 4 && xie->detail <= 7) break; // Ignore scroll
                
                if (buttons_held == 0) {
                    XFixesHideCursor(dpy, DefaultRootWindow(dpy));
                }
                buttons_held++;
                break;
            
            case XI_RawButtonRelease:
                if (xie->detail >= 4 && xie->detail <= 7) break; // Ignore scroll
                
                if (buttons_held > 0) {
                    buttons_held--;
                }
                if (buttons_held == 0) {
                    XFixesShowCursor(dpy, DefaultRootWindow(dpy));
                }
                break;
        }

        XFreeEventData(dpy, cookie);
    }

    return 0;
}
