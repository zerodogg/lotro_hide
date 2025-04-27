#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
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

void usage (char **argv) {
    printf("Usage: %s\n",argv[0]);
    printf(" or  : %s --wrap command\n",argv[0]);
    printf("\nWith %s installed in your $PATH, you can use the following under \"launch options\"\nin Steam:\n%s --wrap %%command%%\n","lotro_hide","lotro_hide");
}

int main(int argc, char **argv){
    int xi_opcode, event, error;
    XEvent ev;

    if(argc > 0) {
        if (strcmp(argv[1],"--wrap") == 0) {
            if(argc < 3) {
                printf("Error: --wrap needs at least one parameter\n\n");
                usage(argv);
                return 101;
            }
            pid_t systemChild = fork();
            // 0 = this is the fork
            if ( systemChild == 0)
            {
                // Exec our parameters
                int ret = execvp(argv[2],&argv[2]);
                printf("failed to exec() argument: %d\n",ret);
                return 100;
            }
            pid_t lotrohide = fork();
            if(lotrohide != 0) {
                int status;
                waitpid(systemChild,&status,0);
                kill(lotrohide,SIGTERM);
                return 0;
            }
        // Anything other than --wrap is treated as "show me the usage screen"
        } else {
            usage(argv);
            return 0;
        }
    }

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
