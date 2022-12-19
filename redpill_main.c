#include "internal/stealth.h"
#include "redpill_main.h"
#include "config/runtime_config.h"
#include "common.h" //commonly used headers in this module
#include "internal/intercept_execve.h" //Handling of execve() replacement
#include "internal/scsi/scsi_notifier.h" //the missing pub/sub handler for SCSI driver
#include "config/cmdline_delegate.h" //Parsing of kernel cmdline
#include "shim/boot_device_shim.h" //Registering & deciding between boot device shims
#include "shim/bios_shim.h" //Shimming various mfgBIOS functions to make them happy
#include "shim/block_fw_update_shim.h" //Prevent firmware update from running
#include "shim/disable_exectutables.h" //Disable common problematic executables
#include "shim/pci_shim.h" //Handles PCI devices emulation
#include "shim/storage/smart_shim.h" //Handles emulation of SMART data for devices without it
#include "shim/storage/sata_port_shim.h" //Handles VirtIO & SAS storage devices/disks peculiarities
#include "shim/uart_fixer.h" //Various fixes for UART weirdness
#include "shim/pmu_shim.h" //Emulates the platform management unit
#include "internal/helper/symbol_helper.h" //kln_func

//Handle versioning stuff
#ifndef RP_VERSION_POSTFIX
#define RP_VERSION_POSTFIX "(NULL)"
#endif
#define RP_VERSION_MAJOR 0
#define RP_VERSION_MINOR 6
#define STRINGIFY(x) #x
#define VERSIONIFY(major,minor,postfix) "v" STRINGIFY(major) "." STRINGIFY(minor) "-" postfix
#define RP_VERSION_STR VERSIONIFY(RP_VERSION_MAJOR, RP_VERSION_MINOR, RP_VERSION_POSTFIX)

/**
 * Force panic to land on a stack trace
 *
 * This ensures we always get this on the stack trace so that we know it was an intentional crash due to a detected
 * error rather than an accidental bug.
 */
void noinline __noreturn rp_crash(void) {
    //Deliberately not reveling any context in case we're running in stealth mode
    //This message is a generic one from arch/x86/kernel/dumpstack.c
    panic("Fatal exception");
}

static int __init init_(void)
{
    int out = 0;

    pr_loc_dbg("================================================================================================");
    pr_loc_inf("RedPill %s loading...", RP_VERSION_STR);

    if (
         get_kln_p() < 0 //Find pointer of kallsyms_lookup_name function, This MUST be the first entry
         || (out = extract_config_from_cmdline(&current_config)) != 0 //This MUST be the second entry
         || (out = populate_runtime_config(&current_config)) != 0 //This MUST be third
         || (out = register_uart_fixer(current_config.hw_config)) != 0 //Fix consoles ASAP
         || (out = register_scsi_notifier()) != 0 //Load SCSI notifier so that boot shim (& others) can use it
         || (out = register_sata_port_shim()) //This should be bfr boot shim as it can fix some things need by boot
         || (out = register_boot_shim(&current_config.boot_media)) //Make sure we're quick with this one
         || (out = register_execve_interceptor()) != 0 //Register this reasonably high as other modules can use it blindly
         || (out = register_bios_shim(current_config.hw_config)) != 0
         || (out = register_disable_executables_shim()) != 0
         || (out = register_fw_update_shim()) != 0
#ifndef DBG_DISABLE_UNLOADABLE
         || (out = register_pci_shim(current_config.hw_config)) != 0 //it's a core hw but it's not checked early
#endif
         || (out = register_disk_smart_shim()) != 0 //provide fake SMART to userspace
         || (out = register_pmu_shim(current_config.hw_config)) != 0 //this is used as early as mfgBIOS loads (=late)
         || (out = initialize_stealth(&current_config)) != 0 //Should be after any shims to let shims have real stuff
       )
        goto error_out;

    pr_loc_inf("RedPill %s loaded successfully (stealth=%d)", RP_VERSION_STR, STEALTH_MODE);
    return 0;

    error_out:
        pr_loc_crt("RedPill %s cannot be loaded, initializer error=%d", RP_VERSION_STR, out);
#ifdef KP_ON_LOAD_ERROR
        rp_crash();
#else
        return out;
#endif
}
module_init(init_);

#if STEALTH_MODE < STEALTH_MODE_FULL //module cannot be unloaded in full-stealth anyway
static void __exit cleanup_(void)
{
    pr_loc_inf("RedPill %s unloading...", RP_VERSION_STR);

    int (*cleanup_handlers[])(void ) = {
        uninitialize_stealth,
        unregister_pmu_shim,
        unregister_disk_smart_shim,
#ifndef DBG_DISABLE_UNLOADABLE
        unregister_pci_shim,
#endif
        unregister_fw_update_shim,
        unregister_disable_executables_shim,
        unregister_bios_shim,
        unregister_execve_interceptor,
        unregister_boot_shim,
        unregister_sata_port_shim,
        unregister_scsi_notifier,
        unregister_uart_fixer
    };

    int out;
    for (int i = 0; i < ARRAY_SIZE(cleanup_handlers); i++) {
        pr_loc_dbg("Calling cleanup handler %pF<%p>", cleanup_handlers[i], cleanup_handlers[i]);
        out = cleanup_handlers[i]();
        if (out != 0)
            pr_loc_wrn("Cleanup handler %pF failed with code=%d", cleanup_handlers[i], out);
    }

    free_runtime_config(&current_config); //A special snowflake ;)

    pr_loc_inf("RedPill %s is dead", RP_VERSION_STR);
    pr_loc_dbg("================================================================================================");
}
module_exit(cleanup_);

MODULE_AUTHOR("TTG");
MODULE_VERSION(RP_VERSION_STR);
#endif


MODULE_LICENSE("GPL");
