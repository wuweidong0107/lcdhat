#include <unistd.h>
#include <linux/input.h>
#include <stdio.h>
#include <errno.h>

static int input_read_event(int fd)
{
    struct input_event ev;
    int n = read(fd, &ev, sizeof(ev));
    if (n != sizeof(ev)) {
        fprintf(stderr, "input: fail to read fd%d\n", fd);
        return -1;
    }

    if (ev.type == EV_KEY && ev.value == 1)
        return ev.code;
    else
        return -1;
}

int input_poll(int fd, int timeout)
{
    fd_set readfds;
    struct timeval tv;
    int ret;

    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    ret = select(fd + 1,
            &readfds,
            NULL,
            NULL,
            &tv);
    if(ret>0) {
        if (FD_ISSET(fd, &readfds)) {
            return input_read_event(fd);
        }
    }
    return ret;
}