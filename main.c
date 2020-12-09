
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "inih/ini.h"

#define KEY_MAX_NUM 32
#define KEY_MAX_CMD 512
#define DEFAULT_FONTSIZE 28
#define DEFAULT_FONTCOLOR 0xFFFFFF

extern int input_poll(int fd);
extern int fb_ft_init(int fd, const char *font, unsigned int size, unsigned int color);
extern void fb_ft_print(const char *str, int line, unsigned int size, unsigned int color);
extern void fb_exit();
extern void fb_clear();

struct keycmd {
    int code;
    char cmd[KEY_MAX_CMD];
    int fontsize;
};

struct lcdhat {
    int input_fd;
    int fb_fd;
    int key;            // key occur
    struct keycmd kc[KEY_MAX_NUM];
};

void *show_thread(void *arg)
{
    int res;
    struct lcdhat *lh = (struct lcdhat *)arg;
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
    FILE *read_fp = NULL;
    int chars_read = 0;
    int i = 0;
    for (i=0; i<KEY_MAX_NUM; i++) {
        if (lh->kc[i].code == lh->key) {
            break;
        }
    }

    while(1) {
        read_fp = popen(lh->kc[i].cmd, "r");
        if (read_fp != NULL) {
            chars_read = fread(str, sizeof(char), 1024, read_fp);
            if (chars_read > 0) {
                fb_clear();
                fb_ft_print(str, 1, lh->kc[i].fontsize, 0xffffff);
            }
        }
        sleep(1);
    }
    pthread_exit(0);
}

void load_conf(struct lcdhat *lh)
{
	// TODO: load config from lcdhat.conf
    int i = 0;
    lh->kc[0].code = 258;
    lh->kc[1].code = 259;
    lh->kc[2].code = 260;
    lh->kc[3].code = 261;

    for (i=0; i<4; i++) {
        lh->kc[i].fontsize = DEFAULT_FONTSIZE;
        sprintf(lh->kc[i].cmd, "./lcdhat-helper.sh %d", i+1);
    }
}

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

    if (fb_ft_init(lh.fb_fd, argv[3], DEFAULT_FONTSIZE, DEFAULT_FONTCOLOR) < 0) {
        fprintf(stderr, "fail to fb_ft_init");
        exit(EXIT_FAILURE);
    }

    load_conf(&lh);

    int res, keycode;
    int thread_num = 0;
    pthread_t last_thread;

    lh.key = 258;
    res = pthread_create(&last_thread, NULL, show_thread, &lh);
    if (res != 0) {
        fprintf(stderr, "fail to pthread_create");
    }
    thread_num++;

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
                res = pthread_create(&last_thread, NULL, show_thread, &lh);
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