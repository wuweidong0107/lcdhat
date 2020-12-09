#include <unistd.h>
#include <linux/input.h>
#include <stdio.h>

static int input_read_event(int fd)
{
    struct input_event ev;
    int n = read(fd, &ev, sizeof(ev));
    if (n != sizeof(ev)) {
        fprintf(stderr, "fail to read fd%d\n", fd);
        return -1;
    }

    if (ev.type == EV_KEY && ev.value == 1)
        return ev.code;
    else
        return -1;
}

int input_poll(int fd)
{
    fd_set readfds;
    
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    select(fd + 1,
            &readfds,
            NULL,
            NULL,
            NULL);

    if (FD_ISSET(fd, &readfds)) {
        return input_read_event(fd);
    }
    return -1;
}