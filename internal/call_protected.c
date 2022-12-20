#include "call_protected.h"
#include "../common.h"
#include <linux/errno.h> //common exit codes
//#include <linux/kallsyms.h> //kallsyms_lookup_name()
#include "helper/symbol_helper.h" //kln_func
#include <linux/module.h> //symbol_get()/put

//This will eventually stop working (since Linux >=5.7.0 has the kallsyms_lookup_name() removed)
//Workaround will be needed: https://github.com/xcellerator/linux_kernel_hacking/issues/3

#define __VOID_RETURN__
//This macro should be used to export symbols which aren't normally EXPORT_SYMBOL/EXPORT_SYMBOL_GPL in the kernel but
// they exist within the kernel (and not a loadable module!). Keep in mind that most of the time "static" cannot be
// reexported using this trick.
//All re-exported function will have _ prefix (e.g. foo() becomes _foo())
#define DEFINE_UNEXPORTED_SHIM(return_type, org_function_name, call_args, call_vars, fail_return) \
  extern asmlinkage return_type org_function_name(call_args);                                     \
  typedef typeof(org_function_name) *org_function_name##__ret;                                    \
  static unsigned long org_function_name##__addr = 0;                                             \
  return_type _##org_function_name(call_args)                                                     \
  {                                                                                               \
      if (unlikely(org_function_name##__addr == 0)) {                                             \
          org_function_name##__addr = kln_func(#org_function_name);                               \
          if (org_function_name##__addr == 0) {                                                   \
              pr_loc_bug("Failed to fetch %s() syscall address", #org_function_name);             \
              return fail_return;                                                                 \
          }                                                                                       \
          pr_loc_dbg("Got addr %lx for %s", org_function_name##__addr, #org_function_name);       \
      }                                                                                           \
                                                                                                  \
      return ((org_function_name##__ret)org_function_name##__addr)(call_vars);                    \
  }

//This macro should be used to export symbols which aren't normally EXPORT_SYMBOL/EXPORT_SYMBOL_GPL in the kernel but
// they exist within the kernel and are defined as __init. These symbol can only be called when the system is still
// booting (i.e. before init user-space binary was called). After that calling such functions is a lottery - the memory
// of them is freed by free_initmem() [called in main.c:kernel_init()]. That's why we skip any caching kere as these are
// called mostly as a one-off during boot process when this module was loaded as a I/O scheduler.
//All re-exported function will have _ prefix (e.g. foo() becomes _foo())
#define DEFINE_UNEXPORTED_INIT_SHIM(return_type, org_function_name, call_args, call_vars, fail_return) \
  extern asmlinkage return_type org_function_name(call_args);                                          \
  typedef typeof(org_function_name) *org_function_name##__ret;                                         \
  return_type _##org_function_name(call_args)                                                          \
  {                                                                                                    \
      unsigned long org_function_name##__addr = 0;                                                     \
      if (unlikely(!is_system_booting())) {                                                            \
          pr_loc_bug("Attempted to call %s() when the system is already booted (state=%d)",            \
                     #org_function_name, system_state);                                                \
          return fail_return;                                                                          \
      }                                                                                                \
      org_function_name##__addr = kln_func(#org_function_name);                                        \
      if (org_function_name##__addr == 0) {                                                            \
          pr_loc_bug("Failed to fetch %s() syscall address", #org_function_name);                      \
          return fail_return;                                                                          \
      }                                                                                                \
      pr_loc_dbg("Got addr %lx for %s", org_function_name##__addr, #org_function_name);                \
                                                                                                       \
      return ((org_function_name##__ret)org_function_name##__addr)(call_vars);                         \
  }

//This macro should be used to export symbols which are normally exported by modules in situations where this module
// must be loaded before such module exporting the symbol.
//Normally if symbol for module "X" is used in "Y" the kernel will complain that "X" muse be loaded before "Y".
//All re-exported function will have _ prefix (e.g. foo() becomes _foo())
#define DEFINE_DYNAMIC_SHIM(return_type, org_function_name, call_args, call_vars, fail_return)                        \
  extern asmlinkage return_type org_function_name(call_args);                                                         \
  typedef typeof(org_function_name) *org_function_name##__ret;                                                        \
  return_type _##org_function_name(call_args)                                                                         \
  {                                                                                                                   \
      org_function_name##__ret org_function_name##__ptr = (org_function_name##__ret)__symbol_get(#org_function_name); \
      if (!org_function_name##__ptr) {                                                                                \
          pr_loc_bug("Failed to fetch %s() symbol (is that module loaded?)", #org_function_name);                     \
          return fail_return;                                                                                         \
      }                                                                                                               \
      pr_loc_dbg("Got ptr %p for %s", org_function_name##__ptr, #org_function_name);                                  \
      /*Doing this BEFORE the call makes a TINY window where the symbol can "escape" but it's protects from deadlock*/\
      __symbol_put(#org_function_name);                                                                               \
                                                                                                                      \
      return ((org_function_name##__ret)org_function_name##__ptr)(call_vars);                                         \
  }
//********************************************************************************************************************//

DEFINE_UNEXPORTED_SHIM(int, cmdline_proc_show, CP_LIST(struct seq_file *m, void *v), CP_LIST(m, v), -EFAULT);
DEFINE_UNEXPORTED_SHIM(void, flush_tlb_all, CP_LIST(void), CP_LIST(), __VOID_RETURN__);

//See header file for detailed explanation what's going on here as it's more complex than a single commit
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,14,0)
DEFINE_UNEXPORTED_SHIM(int, do_execve, CP_LIST(const char *filename,
        const char __user *const __user *__argv,
        const char __user *const __user *__envp), CP_LIST(filename, __argv, __envp), -EINTR);

#ifndef CONFIG_AUDITSYSCALL
DEFINE_UNEXPORTED_SHIM(void, final_putname, CP_LIST(struct filename *name), CP_LIST(name), __VOID_RETURN__);
#else
DEFINE_UNEXPORTED_SHIM(void, putname, CP_LIST(struct filename *name), CP_LIST(name), __VOID_RETURN__);
#endif
#else
DEFINE_UNEXPORTED_SHIM(int, do_execve, CP_LIST(struct filename *filename,
        const char __user *const __user *__argv,
        const char __user *const __user *__envp), CP_LIST(filename, __argv, __envp), -EINTR);
DEFINE_UNEXPORTED_SHIM(struct filename *, getname, CP_LIST(const char __user *name), CP_LIST(name), ERR_PTR(-EFAULT));
#endif

DEFINE_UNEXPORTED_SHIM(int, scsi_scan_host_selected, CP_LIST(struct Scsi_Host *shost, unsigned int channel, unsigned int id, u64 lun, int rescan), CP_LIST(shost, channel, id, lun, rescan), -EIO);
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,19,0)
DEFINE_UNEXPORTED_SHIM(int, ida_pre_get, CP_LIST(struct ida *ida, gfp_t gfp_mask), CP_LIST(ida, gfp_mask), -EINVAL);
#else
DEFINE_UNEXPORTED_SHIM(int, ida_alloc_range, CP_LIST(struct ida *ida, unsigned int min, unsigned int max, gfp_t gfp_mask), CP_LIST(ida, min, max, gfp_mask), -EINVAL);
#endif

DEFINE_UNEXPORTED_SHIM(int, early_serial_setup, CP_LIST(struct uart_port *port), port, -EIO);
DEFINE_UNEXPORTED_SHIM(int, serial8250_find_port, CP_LIST(struct uart_port *p), CP_LIST(p), -EIO);

DEFINE_UNEXPORTED_INIT_SHIM(int, elevator_setup, CP_LIST(char *str), CP_LIST(str), -EINVAL);

DEFINE_DYNAMIC_SHIM(void, usb_register_notify, CP_LIST(struct notifier_block *nb), CP_LIST(nb), __VOID_RETURN__);
DEFINE_DYNAMIC_SHIM(void, usb_unregister_notify, CP_LIST(struct notifier_block *nb), CP_LIST(nb), __VOID_RETURN__);
