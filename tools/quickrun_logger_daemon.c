/* Minimal diagnostic launcher: detach /bin/logcat from the ADB shell. */
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PREFIX "/mnt/extsd/cj96_quickrun_logs/quickrun_"

int main(int argc, char **argv) {
    if (argc != 2 || strncmp(argv[1], PREFIX, sizeof(PREFIX) - 1) != 0 ||
        strstr(argv[1], "/logcat.txt") == NULL || strlen(argv[1]) > 240) {
        fprintf(stderr, "invalid board-side log path\n");
        return 2;
    }
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 3;
    }
    if (pid > 0) {
        printf("%ld\n", (long)pid);
        fflush(stdout);
        return 0;
    }
    if (setsid() < 0) _exit(4);
    signal(SIGHUP, SIG_IGN);
    int fd = open("/dev/null", O_RDWR);
    if (fd < 0) _exit(5);
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    if (fd > STDERR_FILENO) close(fd);
    execl("/bin/logcat", "logcat", "-b", "all", "-v", "threadtime",
          "-f", argv[1], "-r", "1024", "-n", "15", "*:V", (char *)NULL);
    _exit(127);
}
