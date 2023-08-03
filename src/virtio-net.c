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
#include <sys/eventfd.h>
#include <signal.h>
#include <stddef.h>

#include "virtio-net.h"
#include "vm.h"
#include "utils.h"

#define DEVICE_NAME "tap%d"

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

static int virtio_net_virtq_available(struct virtio_net_dev *dev, int timeout)
{
    struct pollfd pollfd = (struct pollfd){
        .fd = dev->ioeventfd,
        .events = POLLIN,
    };
    return (poll(&pollfd, 1, timeout) > 0) && (pollfd.revents & POLLIN);
}

static volatile bool thread_stop = false;

static void *virtio_net_thread(struct virtio_net_dev *dev)
{
    while (!__atomic_load_n(&thread_stop, __ATOMIC_RELAXED)) {
        if (virtio_net_virtq_available(dev, -1))
            pthread_kill((pthread_t) dev->vq_avail_thread, SIGUSR1);
    }

    return NULL;
}

static void virtio_net_setup(struct virtio_net_dev *dev)
{
    vm_t *v = container_of(dev, vm_t, virtio_net_dev);

    dev->enable = true;
    dev->irq_num = VIRTIO_NET_IRQ;
    dev->ioeventfd = eventfd(0, EFD_CLOEXEC);
    dev->irqfd = eventfd(0, EFD_CLOEXEC);
    vm_irqfd_register(v, dev->irqfd, dev->irq_num, 0);
}

void virtio_net_init_pci(struct virtio_net_dev *virtio_net_dev,
                         struct pci *pci,
                         struct bus *io_bus,
                         struct bus *mmio_bus)
{
    struct virtio_pci_dev *dev = &virtio_net_dev->virtio_pci_dev;
    virtio_net_setup(virtio_net_dev);
    virtio_pci_init(dev, pci, io_bus, mmio_bus);
    virtio_pci_set_pci_hdr(dev, VIRTIO_PCI_DEVICE_ID_NET, VIRTIO_NET_PCI_CLASS,
                           virtio_net_dev->irq_num);
    virtio_pci_set_virtq(dev, virtio_net_dev->vq, VIRTIO_NET_VIRTQ_NUM);
    virtio_pci_add_feature(dev, 0);
    virtio_pci_enable(dev);
    pthread_create(&virtio_net_dev->worker_thread, NULL,
                   (void *) virtio_net_thread, (void *) virtio_net_dev);
    fprintf(stderr, "Initialize net device through pci \n");
}