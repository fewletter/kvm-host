#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <stddef.h>

#include "virtio-net.h"
#include "vm.h"
#include "utils.h"

#define DEVICE_NAME "tun%d"

bool virtio_net_init(struct virtio_net_dev *virtio_net_dev)
{
    virtio_net_dev->tap_fd = open("/dev/net/tun", O_RDWR);
    if (virtio_net_dev->tap_fd < 0) {
        fprintf(stderr, "failed to open TAP device: %s\n", strerror(errno));
        return false;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    strcpy(ifr.ifr_name, DEVICE_NAME);
    if (ioctl(virtio_net_dev->tap_fd, TUNSETIFF, &ifr) < 0) {
        fprintf(stderr, "failed to allocate TAP device: %s\n", strerror(errno));
        return false;
    }

    fprintf(stderr, "allocated TAP interface: %s\n", ifr.ifr_name);
    return true;
}