/*
 * Submodule used to hook the execve() syscall, used by the userland to execute binaries.
 *
 * This submodule can currently block calls to specific binaries and fake a successful return of the execution. In the
 * future, if needed, an option to fake certain response and/or execute a different binary instead can be easily added
 * here.
 *
 * execve() is a rather special syscall. This submodule utilized override_symbool.c:override_syscall() to do the actual
 * ground work of replacing the call. However some syscalls (execve, fork, etc.) use ASM stubs with a non-GCC call
 * convention. Up until Linux v3.18 it wasn't a problem as long as the stub was called back. However, since v3.18 the
 * stub was changed in such a way that calling it using a normal convention from (i.e. from the shim here) will cause
 * IRET imbalance and a crash. This is worked around by skipping the whole stub and calling do_execve() with a filename
 * struct directly. This requires re-exported versions of these functions, so it may be marginally slower.
 * Because of that this trick is only utilized on Linux >v3.18 and older ones call the stub as normal.
 *
 * References:
 *  - https://github.com/torvalds/linux/commit/b645af2d5905c4e32399005b867987919cbfc3ae
 *  - https://my.oschina.net/macwe/blog/603583
 *  - https://stackoverflow.com/questions/8372912/hooking-sys-execve-on-linux-3-x
 */
#include "intercept_execve.h"
#include "../common.h"
#include <linux/limits.h>
#include <linux/fs.h> //struct filename
#include "override/override_syscall.h" //SYSCALL_SHIM_DEFINE3, override_symbol
#include "call_protected.h" //do_execve(), getname(), putname()
#include "helper/ftrace_helper.h"

#ifdef RPDBG_EXECVE
#include "../debug/debug_execve.h"
#endif

#define MAX_INTERCEPTED_FILES 10

static char * intercepted_filenames[MAX_INTERCEPTED_FILES] = { NULL };

int add_blocked_execve_filename(const char *filename)
{
    if (unlikely(strlen(filename) > PATH_MAX))
        return -ENAMETOOLONG;

    unsigned int idx = 0;
    while (likely(intercepted_filenames[idx])) { //Find free spot
        if (unlikely(strcmp(filename, intercepted_filenames[idx]) == 0)) { //Does it exist already?
            pr_loc_bug("File %s was already added at %d", filename, idx);
            return -EEXIST;
        }

        if(unlikely(++idx >= MAX_INTERCEPTED_FILES)) { //Are we out of indexes?
            pr_loc_bug("Tried to add %d intercepted filename (max=%d)", idx, MAX_INTERCEPTED_FILES);
            return -ENOMEM;
        }
    }

    kmalloc_or_exit_int(intercepted_filenames[idx], strsize(filename));
    strcpy(intercepted_filenames[idx], filename); //Size checked above

    pr_loc_inf("Filename %s will be blocked from execution", filename);
    return 0;
}

static asmlinkage long (*orig_execve)(const struct pt_regs *);

/*
 * The hook for sys_execve()
 */
asmlinkage int hook_execve(const struct pt_regs *regs)
{
    char *filename = (char *)regs->di;

    char *kbuf;
    long error;
    int i;

    /*
     * We need a buffer to copy filename into
     */
    kbuf = kzalloc(NAME_MAX, GFP_KERNEL);
    if(kbuf == NULL)
        return orig_execve(regs);

    /*
     * Copy filename from userspace into our kernel buffer
     */
    error = copy_from_user(kbuf, filename, NAME_MAX);
    if(error){
        kfree(kbuf);
        return orig_execve(regs);
    }

    for (i = 0; i < MAX_INTERCEPTED_FILES; i++) {
        if (!intercepted_filenames[i])
            break;

        if (unlikely(strcmp(kbuf, intercepted_filenames[i]) == 0)) {
            pr_loc_inf("Blocked %s from running", kbuf);
            kfree(kbuf);
            //We cannot just return 0 here - execve() *does NOT* return on success, but replaces the current process ctx
            do_exit(0);
        }
    }

    /*
     * Clean up and return
     */
    kfree(kbuf);
    return orig_execve(regs);
}

/* Declare the struct that ftrace needs to hook the syscall */
static struct ftrace_hook hooks[] = {
    HOOK("__x64_sys_execve", hook_execve, &orig_execve),
};

static override_symbol_inst *sys_execve_ovs = NULL;
int register_execve_interceptor()
{
    pr_loc_dbg("Registering execve() interceptor");

    int err;
    err = fh_install_hooks(hooks, ARRAY_SIZE(hooks));
    if(err)
        return err;

    pr_loc_inf("execve() interceptor registered");
    return 0;
}

int unregister_execve_interceptor()
{
    pr_loc_dbg("Unregistering execve() interceptor");
    /* Unhook and restore the syscall and print to the kernel buffer */
    fh_remove_hooks(hooks, ARRAY_SIZE(hooks));
    pr_loc_inf("execve() interceptor unregistered");
    return 0;
}

