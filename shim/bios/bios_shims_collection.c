#include "bios_shims_collection.h"
#include "../../config/platform_types.h"
#include "rtc_proxy.h"
#include "bios_hwmon_shim.h"
#include "../../common.h"
#include "../../internal/helper/symbol_helper.h" //kernel_has_symbol()
#include "../../internal/override/override_symbol.h" //shimming leds stuff


#define DECLARE_NULL_ZERO_INT(for_what)                         \
    static __used int bios_##for_what##_null_zero_int(void) {   \
        pr_loc_dbg("mfgBIOS: nullify zero-int for " #for_what); \
        return 0;                                               \
    }
#define SHIM_TO_NULL_ZERO_INT(for_what) _shim_bios_module_entry(for_what, bios_##for_what##_null_zero_int);

/********************************************* mfgBIOS LKM static shims ***********************************************/
static unsigned long org_shimmed_entries[VTK_SIZE] = { '\0' }; //original entries which were shimmed by custom entries
static unsigned long cust_shimmed_entries[VTK_SIZE] = { '\0' }; //custom entries which were set as shims

static int shim_get_gpio_pin_usable(int *pin)
{
    pin[1] = 0;
    return 0;
}

static int shim_set_gpio_pin_usable(int *pin)
{
    pr_loc_dbg("set_gpio pin info 0  %d", pin[0]);
    pr_loc_dbg("set_gpio pin info 1  %d", pin[1]);
    pr_loc_dbg("set_gpio pin info 2  %d", pin[2]);
    pr_loc_dbg("set_gpio pin info 3  %d", pin[3]);
    return 0;
}

static int bios_get_buz_clr(unsigned char *state)
{
    *state = 0;
    return 0;
}

static int bios_get_power_status(POWER_INFO *power)
{
    power->power_1 = POWER_STATUS_GOOD;
    power->power_2 = POWER_STATUS_GOOD;
    return 0;
}

/***************************************** Debug shims for unknown bios functions **************************************/
DECLARE_NULL_ZERO_INT(VTK_SET_FAN_STATE);
DECLARE_NULL_ZERO_INT(VTK_SET_DISK_LED);
DECLARE_NULL_ZERO_INT(VTK_SET_PWR_LED);
// DECLARE_NULL_ZERO_INT(VTK_SET_GPIO_PIN);
DECLARE_NULL_ZERO_INT(VTK_SET_GPIO_PIN_BLINK);
DECLARE_NULL_ZERO_INT(VTK_SET_ALR_LED);
DECLARE_NULL_ZERO_INT(VTK_SET_BUZ_CLR);
DECLARE_NULL_ZERO_INT(VTK_SET_CPU_FAN_STATUS);
DECLARE_NULL_ZERO_INT(VTK_SET_PHY_LED);
DECLARE_NULL_ZERO_INT(VTK_SET_HDD_ACT_LED);
DECLARE_NULL_ZERO_INT(VTK_GET_MICROP_ID);
DECLARE_NULL_ZERO_INT(VTK_SET_MICROP_ID);

/********************************************** mfgBIOS shimming routines *********************************************/
static unsigned long *vtable_start = NULL; //set when shim_bios_module is called()
void _shim_bios_module_entry(const unsigned int idx, const void *new_sym_ptr)
{
    if (unlikely(!vtable_start)) {
        pr_loc_bug("%s called without vtable start populated - are you calling it outside of shim_bios_module scope?!",
                  __FUNCTION__);
        return;
    }

    if (unlikely(idx > VTK_SIZE-1)) {
        pr_loc_bug("Attempted shim on index %d - out of range", idx);
        return;
    }

    //The vtable entry is either not shimmed OR already shimmed with what we set before OR already *was* shimmed but
    // external (i.e. mfgBIOS) code overrode the shimmed entry.
    //We only save the original entry if it was set by the mfgBIOS (so not shimmed yet or ext. override situation)

    //it was already shimmed and the shim is still there => noop
    if (cust_shimmed_entries[idx] && cust_shimmed_entries[idx] == vtable_start[idx])
        return;

    pr_loc_dbg("mfgBIOS vtable [%d] originally %ps<%p> will now be %ps<%p>", idx, (void *) vtable_start[idx],
               (void *) vtable_start[idx], new_sym_ptr, new_sym_ptr);
    org_shimmed_entries[idx] = vtable_start[idx];
    cust_shimmed_entries[idx] = (unsigned long)new_sym_ptr;
    vtable_start[idx] = cust_shimmed_entries[idx];
}

/**
 * Prints a table of memory between vtable_start and vtable_end, trying to resolve symbols as it goes
 */
static void print_debug_symbols(const unsigned long *vtable_end)
{
    if (unlikely(!vtable_start)) {
        pr_loc_dbg("Cannot print - no vtable address");
        return;
    }

    int im = vtable_end - vtable_start; //Should be multiplies of 8 in general (64 bit alignment)
    pr_loc_dbg("Will print %d bytes of memory from %p", im, vtable_start);

    unsigned long *call_ptr = vtable_start;
    unsigned char *byte_ptr = (char *)vtable_start;
    for (int i = 0; i < im; i++, byte_ptr++) {
        pr_loc_dbg_raw("%02x ", *byte_ptr);
        if ((i+1) % 8 == 0) {
            pr_loc_dbg_raw(" [%02d] 0x%03x \t%p\t%pS\n", i / 8, i-7, (void *) (*call_ptr), (void *) (*call_ptr));
            call_ptr++;
        }
    }
    pr_loc_dbg_raw("\n");

    pr_loc_dbg("Finished printing memory at %p", byte_ptr);
}

/**
 * Applies shims to the vtable used by the bios
 *
 * These calls may execute multiple times as the mfgBIOS is loading.
 *
 * @return true when shimming succeeded, false otherwise
 */
bool shim_bios_module(const struct hw_config *hw, struct module *mod, unsigned long *vt_start, unsigned long *vt_end)
{
    if (unlikely(!vt_start || !vt_end)) {
        pr_loc_bug("%s called without vtable start or vt_end populated?!", __FUNCTION__);
        return false;
    }

    vtable_start = vt_start;

    print_debug_symbols(vt_end);
    SHIM_TO_NULL_ZERO_INT(VTK_SET_FAN_STATE);
    SHIM_TO_NULL_ZERO_INT(VTK_SET_DISK_LED);
    SHIM_TO_NULL_ZERO_INT(VTK_SET_PWR_LED);
    // SHIM_TO_NULL_ZERO_INT(VTK_SET_GPIO_PIN);
    _shim_bios_module_entry(VTK_GET_GPIO_PIN, shim_get_gpio_pin_usable);
    _shim_bios_module_entry(VTK_SET_GPIO_PIN, shim_set_gpio_pin_usable);
    SHIM_TO_NULL_ZERO_INT(VTK_SET_GPIO_PIN_BLINK);
    SHIM_TO_NULL_ZERO_INT(VTK_SET_ALR_LED);
    _shim_bios_module_entry(VTK_GET_BUZ_CLR, bios_get_buz_clr);
    SHIM_TO_NULL_ZERO_INT(VTK_SET_BUZ_CLR);
    _shim_bios_module_entry(VTK_GET_PWR_STATUS, bios_get_power_status);
    SHIM_TO_NULL_ZERO_INT(VTK_SET_CPU_FAN_STATUS);
    SHIM_TO_NULL_ZERO_INT(VTK_SET_PHY_LED);
    SHIM_TO_NULL_ZERO_INT(VTK_SET_HDD_ACT_LED);
    SHIM_TO_NULL_ZERO_INT(VTK_GET_MICROP_ID);
    SHIM_TO_NULL_ZERO_INT(VTK_SET_MICROP_ID);

    if (hw->emulate_rtc) {
        pr_loc_dbg("Platform requires RTC proxy - enabling");
        register_rtc_proxy_shim();
        _shim_bios_module_entry(VTK_RTC_GET_TIME, rtc_proxy_get_time);
        _shim_bios_module_entry(VTK_RTC_SET_TIME, rtc_proxy_set_time);
        _shim_bios_module_entry(VTK_RTC_INT_APWR, rtc_proxy_init_auto_power_on);
        _shim_bios_module_entry(VTK_RTC_GET_APWR, rtc_proxy_get_auto_power_on);
        _shim_bios_module_entry(VTK_RTC_SET_APWR, rtc_proxy_set_auto_power_on);
        _shim_bios_module_entry(VTK_RTC_UINT_APWR, rtc_proxy_uinit_auto_power_on);
    } else {
        pr_loc_dbg("Native RTC supported - not enabling proxy (emulate_rtc=%d)", hw->emulate_rtc ? 1:0);
    }

    shim_bios_module_hwmon_entries(hw); //Shim all hardware environment stuff (temps, fans, etc.)

    print_debug_symbols(vt_end);

    return true;
}

bool unshim_bios_module(unsigned long *vt_start, unsigned long *vt_end)
{
    for (int i = 0; i < VTK_SIZE; i++) {
        //make sure to check the cust_ one as org_ may contain NULL ptrs and we should restore them as NULL if they were
        // so originally
        if (!cust_shimmed_entries[i])
            continue;

        pr_loc_dbg("Restoring vtable [%d] from %ps<%p> to %ps<%p>", i, (void *) vt_start[i],
                   (void *) vt_start[i], (void *) org_shimmed_entries[i], (void *) org_shimmed_entries[i]);
        vtable_start[i] = org_shimmed_entries[i];
    }

    reset_bios_shims();

    return true;
}

void reset_bios_shims(void)
{
    memset(org_shimmed_entries, 0, sizeof(org_shimmed_entries));
    memset(cust_shimmed_entries, 0, sizeof(cust_shimmed_entries));
    unregister_rtc_proxy_shim();
    reset_bios_module_hwmon_shim();
}

/******************************** Kernel-level shims related to mfgBIOS functionality *********************************/
extern void *funcSYNOSATADiskLedCtrl; //if this explodes one day we need to do kernel_has_symbol() on it dynamically

/*
 * Syno kernel has ifdefs for "MY_ABC_HERE" for syno_ahci_disk_led_enable() and syno_ahci_disk_led_enable_by_port() so
 * we need to check if they really exist and we cannot determine it statically
 */
static override_symbol_inst *ov_funcSYNOSATADiskLedCtrl = NULL;
static override_symbol_inst *ov_syno_ahci_disk_led_enable = NULL;
static override_symbol_inst *ov_syno_ahci_disk_led_enable_by_port = NULL;

static int funcSYNOSATADiskLedCtrl_shim(int host_num, SYNO_DISK_LED led)
{
    pr_loc_dbg("Received %s with host=%d led=%d", __FUNCTION__, host_num, led);
    //exit code is not used anywhere in the public code, so this value is an educated guess based on libata-scsi.c
    return 0;
}

int syno_ahci_disk_led_enable_shim(const unsigned short host_num, const int value)
{
    pr_loc_dbg("Received %s with host=%d val=%d", __FUNCTION__, host_num, value);
    return 0;
}

int syno_ahci_disk_led_enable_by_port_shim(const unsigned short port, const int value)
{
    pr_loc_dbg("Received %s with port=%d val=%d", __FUNCTION__, port, value);
    return 0;
}

int shim_disk_leds_ctrl(const struct hw_config *hw)
{
    //we're checking this here to remove knowledge of "struct hw_config" from bios_shim letting others know it's NOT
    //the place to do BIOS shimming decisions
    if (!hw->fix_disk_led_ctrl)
        return 0;

    pr_loc_dbg("Shimming disk led control API");

    int out;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,0,0)
    //funcSYNOSATADiskLedCtrl exists on (almost?) all platforms, but it's null on some... go figure ;)
    if (funcSYNOSATADiskLedCtrl) {
        ov_funcSYNOSATADiskLedCtrl = override_symbol("funcSYNOSATADiskLedCtrl", funcSYNOSATADiskLedCtrl_shim);
        if (unlikely(IS_ERR(ov_funcSYNOSATADiskLedCtrl))) {
            out = PTR_ERR(ov_funcSYNOSATADiskLedCtrl);
            ov_funcSYNOSATADiskLedCtrl = NULL;
            pr_loc_err("Failed to shim funcSYNOSATADiskLedCtrl, error=%d", out);
            return out;
        }
    }
#endif

    if (kernel_has_symbol("syno_ahci_disk_led_enable")) {
        ov_syno_ahci_disk_led_enable = override_symbol("syno_ahci_disk_led_enable", syno_ahci_disk_led_enable_shim);
        if (unlikely(IS_ERR(ov_syno_ahci_disk_led_enable))) {
            out = PTR_ERR(ov_syno_ahci_disk_led_enable);
            ov_syno_ahci_disk_led_enable = NULL;
            pr_loc_err("Failed to shim syno_ahci_disk_led_enable, error=%d", out);
            return out;
        }
    }

    if (kernel_has_symbol("syno_ahci_disk_led_enable_by_port")) {
        ov_syno_ahci_disk_led_enable_by_port = override_symbol("syno_ahci_disk_led_enable_by_port", syno_ahci_disk_led_enable_by_port_shim);
        if (unlikely(IS_ERR(ov_syno_ahci_disk_led_enable_by_port))) {
            out = PTR_ERR(ov_syno_ahci_disk_led_enable_by_port);
            ov_syno_ahci_disk_led_enable_by_port = NULL;
            pr_loc_err("Failed to shim syno_ahci_disk_led_enable_by_port, error=%d", out);
            return out;
        }
    }

    pr_loc_dbg("Finished %s", __FUNCTION__);
    return 0;
}

int unshim_disk_leds_ctrl(void)
{
    pr_loc_dbg("Unshimming disk led control API");

    int out;
    bool failed = false;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,0,0)
    if (ov_funcSYNOSATADiskLedCtrl) {
        out = restore_symbol(ov_funcSYNOSATADiskLedCtrl);
        ov_funcSYNOSATADiskLedCtrl = NULL;
        if (unlikely(out != 0)) { //falling through to try to unshim others too
            pr_loc_err("Failed to unshim funcSYNOSATADiskLedCtrl, error=%d", out);
            failed = true;
        }
    }
#endif

    if (ov_syno_ahci_disk_led_enable) {
        out = restore_symbol(ov_syno_ahci_disk_led_enable);
        ov_syno_ahci_disk_led_enable = NULL;
        if (unlikely(out != 0)) { //falling through to try to unshim others too
            pr_loc_err("Failed to unshim syno_ahci_disk_led_enable, error=%d", out);
            failed = true;
        }
    }

    if (ov_syno_ahci_disk_led_enable_by_port) {
        out = restore_symbol(ov_syno_ahci_disk_led_enable_by_port);
        ov_syno_ahci_disk_led_enable_by_port = NULL;
        if (unlikely(out != 0)) {
            pr_loc_err("Failed to unshim syno_ahci_disk_led_enable_by_port, error=%d", out);
            failed = true;
        }
    }

    out = failed ? -EINVAL : 0;
    pr_loc_dbg("Finished %s (exit=%d)", __FUNCTION__, out);

    return out;
}
