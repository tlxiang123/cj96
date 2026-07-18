#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>

static int emit(int fd, unsigned short type, unsigned short code, int value) {
    struct input_event event;
    memset(&event, 0, sizeof(event));
    gettimeofday(&event.time, NULL);
    event.type = type;
    event.code = code;
    event.value = value;
    return write(fd, &event, sizeof(event)) == sizeof(event) ? 0 : -1;
}

static int scale(int value, int screen_max, const struct input_absinfo *info) {
    return info->minimum + value * (info->maximum - info->minimum) / screen_max;
}

int main(int argc, char **argv) {
    int fd;
    int x;
    int y;
    struct input_absinfo x_info;
    struct input_absinfo y_info;

    if (argc != 3) {
        fprintf(stderr, "usage: tap_device X Y\n");
        return 2;
    }
    x = atoi(argv[1]);
    y = atoi(argv[2]);
    fd = open("/dev/input/event0", O_RDWR);
    if (fd < 0 || ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &x_info) < 0 ||
        ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &y_info) < 0) {
        perror("touchscreen");
        return 1;
    }
    x = scale(x, 1023, &x_info);
    y = scale(y, 599, &y_info);

    emit(fd, EV_ABS, ABS_MT_SLOT, 0);
    emit(fd, EV_ABS, ABS_MT_TRACKING_ID, 123);
    emit(fd, EV_ABS, ABS_MT_TOUCH_MAJOR, 20);
    emit(fd, EV_ABS, ABS_MT_POSITION_X, x);
    emit(fd, EV_ABS, ABS_MT_POSITION_Y, y);
    emit(fd, EV_KEY, BTN_TOUCH, 1);
    emit(fd, EV_SYN, SYN_REPORT, 0);
    usleep(80000);
    emit(fd, EV_KEY, BTN_TOUCH, 0);
    emit(fd, EV_ABS, ABS_MT_TRACKING_ID, -1);
    emit(fd, EV_SYN, SYN_REPORT, 0);
    close(fd);
    return 0;
}
