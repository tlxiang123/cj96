#include <fcntl.h>
#include <linux/fb.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main(void) {
    int fd = open("/dev/fb0", O_RDONLY);
    struct fb_var_screeninfo var;
    if (fd < 0 || ioctl(fd, FBIOGET_VSCREENINFO, &var) < 0) {
        return 1;
    }
    printf("bpp=%u yoffset=%u red=%u/%u green=%u/%u blue=%u/%u alpha=%u/%u\n",
           var.bits_per_pixel, var.yoffset,
           var.red.offset, var.red.length,
           var.green.offset, var.green.length,
           var.blue.offset, var.blue.length,
           var.transp.offset, var.transp.length);
    close(fd);
    return 0;
}
