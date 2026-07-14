#include <fcntl.h>
#include <linux/fb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main(void) {
    int fd = open("/dev/fb0", O_RDONLY);
    struct fb_var_screeninfo var;
    struct fb_fix_screeninfo fix;
    unsigned char *row;
    unsigned int y;

    if (fd < 0 || ioctl(fd, FBIOGET_VSCREENINFO, &var) < 0 ||
        ioctl(fd, FBIOGET_FSCREENINFO, &fix) < 0) {
        perror("framebuffer");
        return 1;
    }
    row = malloc(fix.line_length);
    if (row == NULL) {
        return 1;
    }
    for (y = 0; y < var.yres; ++y) {
        off_t offset = (off_t)(var.yoffset + y) * fix.line_length;
        if (pread(fd, row, fix.line_length, offset) != (ssize_t)fix.line_length ||
            fwrite(row, 1, fix.line_length, stdout) != fix.line_length) {
            perror("capture");
            free(row);
            close(fd);
            return 1;
        }
    }
    free(row);
    close(fd);
    return 0;
}
