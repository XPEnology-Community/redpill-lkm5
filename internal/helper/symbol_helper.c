#include "symbol_helper.h" //kln_func
#include <linux/module.h> //__symbol_get(), __symbol_put()
#include <linux/kallsyms.h> //kallsyms_lookup_name
#include "../../common.h" //pr_loc_*

unsigned long (*kln_func)(const char* name) = NULL;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,7,0)
int get_kln_p(void)
{
    kln_func = kallsyms_lookup_name;
    return 0;
}
#else
/*
 * In kernel version 5.7, kallsyms_lookup_name() was unexported, so we can't use it anymore.
 * The alternative method below is slower (but not really noticably), and works by brute-forcing
 * possible addresses for the function name by starting at the kernel base address and using
 * sprint_symbol() (which is still exported) to check if the symbol name at each address
 * matches the one we want.
 * 
 * https://github.com/xcellerator/linux_kernel_hacking/blob/446789fd152d2663cd2c7d7f8a5aaae873a92a30/3_RootkitTechniques/3.3_set_root/ftrace_helper.h
 */
static unsigned long kaddr_lookup_name(const char *fname_raw)
{
    int i;
    unsigned long kaddr;
    char *fname_lookup, *fname;

    fname_lookup = kzalloc(255, GFP_KERNEL);
    if (!fname_lookup)
        return 0;

    fname = kzalloc(strlen(fname_raw)+4, GFP_KERNEL);
    if (!fname)
        return 0;

    /*
     * We have to add "+0x0" to the end of our function name
     * because that's the format that sprint_symbol() returns
     * to us. If we don't do this, then our search can stop
     * prematurely and give us the wrong function address!
     */
    strcpy(fname, fname_raw);
    strcat(fname, "+0x0");

    /*
     * Get the kernel base address:
     * sprint_symbol() is less than 0x100000 from the start of the kernel, so
     * we can just AND-out the last 3 bytes from it's address to the the base
     * address.
     * There might be a better symbol-name to use?
     */
    kaddr = (unsigned long) &sprint_symbol;
    kaddr &= 0xffffffffff000000;

    /*
     * All the syscalls (and all interesting kernel functions I've seen so far)
     * are within the first 0x100000 bytes of the base address. However, the kernel
     * functions are all aligned so that the final nibble is 0x0, so we only
     * have to check every 16th address.
     */
    for ( i = 0x0 ; i < 0x100000 ; i++ )
    {
        /*
         * Lookup the name ascribed to the current kernel address
         */
        sprint_symbol(fname_lookup, kaddr);

        /*
         * Compare the looked-up name to the one we want
         */
        if ( strncmp(fname_lookup, fname, strlen(fname)) == 0 )
        {
            /*
             * Clean up and return the found address
             */
            kfree(fname_lookup);
            return kaddr;
        }
        /*
         * Jump 16 addresses to next possible address
         */
        kaddr += 0x10;
    }
    /*
     * We didn't find the name, so clean up and return 0
     */
    kfree(fname_lookup);
    return 0;
}

int get_kln_p(void)
{
    kln_func = (long unsigned int (*)(const char *))kaddr_lookup_name("kallsyms_lookup_name");
    if (kln_func == 0) {
        pr_loc_err("Error searching kallsyms_lookup_name address!");
        return -1;
    }
    pr_loc_dbg("kallsyms_lookup_name address = 0x%lx\n", (long unsigned int)kln_func);
    return 0;
}
#endif

bool kernel_has_symbol(const char *name) {
    if (__symbol_get(name)) { //search for public symbols
        __symbol_put(name);

        return true;
    }
    return kln_func(name) != 0;
}