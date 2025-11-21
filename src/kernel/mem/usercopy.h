#ifndef _KERNEL_USERCOPY_H
#define _KERNEL_USERCOPY_H

#include <stdint.h>
#include <stddef.h>

int copy_to_user(void *user_dest, const void *kern_src, size_t n);
int copy_from_user(void *kern_dest, const void *user_src, size_t n);

#endif
