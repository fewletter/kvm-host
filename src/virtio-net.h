#pragma once
#include <linux/virtio_net.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "diskimg.h"
#include "pci.h"
#include "virtio-pci.h"
#include "virtq.h"

#define VIRTIO_NET_VIRTQ_NUM 8

struct virtio_net_dev {
    struct virtio_pci_dev virtio_pci_dev;
    struct virtq vq[VIRTIO_NET_VIRTQ_NUM * 2 + 1];
    int tap_fd;
    int ioeventfd;
    bool enable;
    int irq_num;
};

bool virtio_net_init(struct virtio_net_dev *virtio_net_dev);
void virtio_net_exit(struct virtio_net_dev *dev);
void virtio_net_init_pci(struct virtio_net_dev *dev,
                         struct pci *pci,
                         struct bus *io_bus,
                         struct bus *mmio_bus);