/* SPDX-License-Identifier: GPL-2.0 */
#ifndef REDPILL_PCI_H
#define REDPILL_PCI_H

#warning "Using compatibility file for drivers/pci/pci.h - if possible do NOT compile using toolkit"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,18,0) && LINUX_VERSION_CODE < KERNEL_VERSION(4,20,0) //v4.18 - v4.20
/* pci_dev priv_flags */
#define PCI_DEV_ADDED 1

static inline bool pci_dev_is_added(const struct pci_dev *dev)
{
	return test_bit(PCI_DEV_ADDED, &dev->priv_flags);
}

#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4,20,0) && LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0) //v4.20 - v6.0
#define PCI_DEV_ADDED 0

static inline bool pci_dev_is_added(const struct pci_dev *dev)
{
	return test_bit(PCI_DEV_ADDED, &dev->priv_flags);
}
#else
static inline bool pci_dev_is_added(const struct pci_dev *dev)
{
	return 1 == dev->is_added;
}
#endif //LINUX_VERSION_CODE check

#endif /* REDPILL_PCI_H */
