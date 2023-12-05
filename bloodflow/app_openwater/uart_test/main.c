#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h> /*termio.h for serial IO api*/

#include <pthread.h> /* POSIX Threads */

#include "../uart.h"

#define MAX_STR_LEN 1024

int main(int argc, char *argv[])
{
    int fd, c, res;
    struct termios oldtio, newtio;
    char buf[MAX_STR_LEN];
    int k;
    char deviceName[MAX_STR_LEN];

    memset(&deviceName[0], 0, MAX_STR_LEN);
    snprintf(&deviceName[0], MAX_STR_LEN, "/dev/ttyUSB0");

    k = 1;
    while (argc > k)
    {
        if (0 == strncmp(argv[k], "-d", MAX_STR_LEN))
        {
            if (k + 1 < argc)
            {
                snprintf(&deviceName[0], MAX_STR_LEN, "%s", argv[k + 1]);
            }
            else
            {
                printf("error : -d should be follow a device!\n");
                return 0;
            } /*if */
        }
        k++;
    }

    // non-blocking mode
    // fd = open(&deviceName[0], O_RDWR | O_NOCTTY |O_NONBLOCK| O_NDELAY);

    fd = open(&deviceName[0], O_RDWR | O_NOCTTY);
    if (0 > fd)
    {
        perror(&deviceName[0]);
        exit(-1);
    }

//  res = setInterfaceAttribs(fd, B115200, 0, 20); /* set speed to 115200 bps, 8n1 (no parity), timeout 2 secs*/
    res = setInterfaceAttribs(fd, B115200, 0, -1); /* set speed to 115200 bps, 8n1 (no parity), blocking*/

    if (res == -1)
    {
        return res;
    }

    pthread_t readThread_t; /* thread variable */

    pthread_create(&readThread_t, NULL, (void *)readThread, (void *)&fd);
    pthread_join(readThread_t, NULL);

    close(fd);

    return 0;
}
