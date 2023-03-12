#ifndef REDPILL_BIOS_PSU_STATUS_SHIM_H
#define REDPILL_BIOS_PSU_STATUS_SHIM_H

#include <linux/types.h> //bool

struct hw_config;
int register_bios_psu_status_shim(const struct hw_config *hw);

/**
 * This function should be called when we're unloading cleanly (=mfgBIOS is alive, we're going away). If the bios went
 * away on its own call reset_bios_psu_status_shim()
 */
int unregister_bios_psu_status_shim(void);

/**
 * This function should be called when we're unloading because mfgBIOS went away. If the unload should be clean and
 * restore all mfgBIOS elements to its original state (i.e. the mfgBIOS is still loaded and not currently unloading)
 * call unregister_bios_psu_status_shim() instead.
 */
int reset_bios_psu_status_shim(void);


#endif //REDPILL_BIOS_PSU_STATUS_SHIM_H
