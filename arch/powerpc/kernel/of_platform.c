/*
 *    Copyright (C) 2006 Benjamin Herrenschmidt, IBM Corp.
 *			 <benh@kernel.crashing.org>
 *    and		 Arnd Bergmann, IBM Corp.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 */

#undef DEBUG

#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/pci.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>

#include <asm/errno.h>
#include <asm/topology.h>
#include <asm/pci-bridge.h>
#include <asm/ppc-pci.h>
#include <asm/atomic.h>

/*
 * The list of OF IDs below is used for matching bus types in the
 * system whose devices are to be exposed as of_platform_devices.
 *
 * This is the default list valid for most platforms. This file provides
 * functions who can take an explicit list if necessary though
 *
 * The search is always performed recursively looking for children of
 * the provided device_node and recursively if such a children matches
 * a bus type in the list
 */

const struct of_device_id of_default_bus_ids[] = {
	{ .type = "soc", },
	{ .compatible = "soc", },
	{ .type = "spider", },
	{ .type = "axon", },
	{ .type = "plb5", },
	{ .type = "plb4", },
	{ .type = "opb", },
	{ .type = "ebc", },
	{},
};

static int of_dev_node_match(struct device *dev, void *data)
{
	return to_platform_device(dev)->dev.of_node == data;
}

struct platform_device *of_find_device_by_node(struct device_node *np)
{
	struct device *dev;

	dev = bus_find_device(&platform_bus_type, NULL, np, of_dev_node_match);
	if (dev)
		return to_platform_device(dev);
	return NULL;
}
EXPORT_SYMBOL(of_find_device_by_node);

#ifdef CONFIG_PPC_OF_PLATFORM_PCI

/* The probing of PCI controllers from of_platform is currently
 * 64 bits only, mostly due to gratuitous differences between
 * the 32 and 64 bits PCI code on PowerPC and the 32 bits one
 * lacking some bits needed here.
 */

static int __devinit of_pci_phb_probe(struct platform_device *dev,
				      const struct of_device_id *match)
{
	struct pci_controller *phb;

	/* Check if we can do that ... */
	if (ppc_md.pci_setup_phb == NULL)
		return -ENODEV;

	pr_info("Setting up PCI bus %s\n", dev->dev.of_node->full_name);

	/* Alloc and setup PHB data structure */
	phb = pcibios_alloc_controller(dev->dev.of_node);
	if (!phb)
		return -ENODEV;

	/* Setup parent in sysfs */
	phb->parent = &dev->dev;

	/* Setup the PHB using arch provided callback */
	if (ppc_md.pci_setup_phb(phb)) {
		pcibios_free_controller(phb);
		return -ENODEV;
	}

	/* Process "ranges" property */
	pci_process_bridge_OF_ranges(phb, dev->dev.of_node, 0);

	/* Init pci_dn data structures */
	pci_devs_phb_init_dynamic(phb);

	/* Register devices with EEH */
#ifdef CONFIG_EEH
	if (dev->dev.of_node->child)
		eeh_add_device_tree_early(dev->dev.of_node);
#endif /* CONFIG_EEH */

	/* Scan the bus */
	pcibios_scan_phb(phb, dev->dev.of_node);
	if (phb->bus == NULL)
		return -ENXIO;

	/* Claim resources. This might need some rework as well depending
	 * wether we are doing probe-only or not, like assigning unassigned
	 * resources etc...
	 */
	pcibios_claim_one_bus(phb->bus);

	/* Finish EEH setup */
#ifdef CONFIG_EEH
	eeh_add_device_tree_late(phb->bus);
#endif

	/* Add probed PCI devices to the device model */
	pci_bus_add_devices(phb->bus);

	return 0;
}

static struct of_device_id of_pci_phb_ids[] = {
	{ .type = "pci", },
	{ .type = "pcix", },
	{ .type = "pcie", },
	{ .type = "pciex", },
	{ .type = "ht", },
	{}
};

static struct of_platform_driver of_pci_phb_driver = {
	.probe = of_pci_phb_probe,
	.driver = {
		.name = "of-pci",
		.owner = THIS_MODULE,
		.of_match_table = of_pci_phb_ids,
	},
};

static __init int of_pci_phb_init(void)
{
	return of_register_platform_driver(&of_pci_phb_driver);
}

device_initcall(of_pci_phb_init);

#endif /* CONFIG_PPC_OF_PLATFORM_PCI */
