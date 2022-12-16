#ifndef REDPILL_SYMBOL_HELPER_H
#define REDPILL_SYMBOL_HELPER_H

#include <linux/types.h> //bool

/**
 * Workaround for kallsyms_lookup_name in kernels > 5.7
 * https://github.com/xcellerator/linux_kernel_hacking/issues/3
 */
extern unsigned long (*kln_func)(const char*);
int get_kln_p(void);

/**
 * Check if a given symbol exists
 *
 * This function will return true for both public and private kernel symbols
 *
 * @param name name of the symbol
 */
bool kernel_has_symbol(const char *name);

#endif //REDPILL_SYMBOL_HELPER_H
