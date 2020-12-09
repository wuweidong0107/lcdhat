
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

extern void fb_print(char *str);
extern int fb_init(int fd, char *font, int fontsize);
extern int input_poll(int fd);
extern void fb_exit();

struct lcdhat {
    int input_fd;
    int fb_fd;
    int key;
};

void *fbshow_thread(void *arg)
{
    int res;
    struct lcdhat lh = *((struct lcdhat *)arg);
    char str[1024];

    res = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    if (res != 0) {
        fprintf(stderr, "fail to pthread_setcancelstate");
        exit(EXIT_FAILURE);
    }

    res = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    if (res != 0) {
        fprintf(stderr, "fail to pthread_setcanceltype");
        exit(EXIT_FAILURE);
    }

    memset(str, 0, sizeof(str));
    sprintf(str, "%d", lh.key);
    while(1) {
        printf("key%d fbshow_thread running\n", lh.key);
        fb_print(str);
        sleep(1);
    }
    pthread_exit(0);
}

/*
 * usage:
 * ./lcdhat /dev/fb0 /dev/input/event1
 */
int main(int argc, char **argv)
{
    struct lcdhat lh;

    if (argc != 4) {
        fprintf(stderr, "usage: %s inputdev fbdev font\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    lh.input_fd = open(argv[1], O_RDWR);
    if (lh.input_fd < 0) {
        close(lh.fb_fd);
        fprintf(stderr, "fail to open %s\n", argv[1]);
    }

    lh.fb_fd = open(argv[2], O_RDWR);
    if (lh.fb_fd < 0) {
        fprintf(stderr, "fail to open %s\n", argv[2]);
        exit(EXIT_FAILURE);
    }

    if (fb_init(lh.fb_fd, argv[3], 24) < 0) {
        fprintf(stderr, "fail to fb_init");
        exit(EXIT_FAILURE);
    }

    int res, keycode;
    int thread_num = 0;
    pthread_t last_thread;
    while(1) {
        if ((keycode = input_poll(lh.input_fd)) >=0) {
            if (thread_num > 0) {
                res = pthread_cancel(last_thread);
                if (res != 0) {
                    fprintf(stderr, "fail to pthread_cancel");
                    exit(EXIT_FAILURE);
                }

                res = pthread_join(last_thread, NULL);
                if (res != 0) {
                    fprintf(stderr, "fail to pthread_join");
                    exit(EXIT_FAILURE);
                }
                thread_num = 0;
            }
            if (thread_num == 0) {
                lh.key = keycode;
                res = pthread_create(&last_thread, NULL, fbshow_thread, &lh);
                if (res != 0) {
                    fprintf(stderr, "fail to pthread_create");
                }
                thread_num++;
            }
        }
    }
    fb_exit();
    close(lh.input_fd);
    close(lh.fb_fd);
}