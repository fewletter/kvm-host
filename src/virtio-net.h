#pragma once
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "diskimg.h"
#include "pci.h"
#include "virtio-pci.h"
#include "virtq.h"

#define VIRTIO_NET_VIRTQ_NUM 2
#define VIRTIO_NET_PCI_CLASS 0x019000

struct virtio_net_config {
    /* ethernet mac address*/
    uint8_t mac[6];
    uint16_t status;
    uint16_t max_virtqueue_pairs;
    uint16_t mtu;
} __attribute__((packed));

struct virtio_net_dev {
    struct virtio_pci_dev virtio_pci_dev;
    struct virtio_net_config config;
    struct virtq vq[VIRTIO_NET_VIRTQ_NUM];
    int tap_fd;
    int irqfd;
    int ioeventfd;
    bool enable;
    int irq_num;
    pthread_t vq_avail_thread;
    pthread_t worker_thread;
};

void virtio_net_read(struct virtio_net_dev *vnet);
void virtio_net_write(struct virtio_net_dev *vnet);
bool virtio_net_init(struct virtio_net_dev *virtio_net_dev);
void virtio_net_exit(struct virtio_net_dev *dev);
void virtio_net_init_pci(struct virtio_net_dev *dev,
                         struct pci *pci,
                         struct bus *io_bus,
                         struct bus *mmio_bus);