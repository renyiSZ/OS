/******************************************************************************/
/* Important Fall 2018 CSCI 402 usage information:                            */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/*         53616c7465645f5fd1e93dbf35cbffa3aef28f8c01d8cf2ffc51ef62b26a       */
/*         f9bda5a68e5ed8c972b17bab0f42e24b19daa7bd408305b1f7bd6c7208c1       */
/*         0e36230e913039b3046dd5fd0ba706a624d33dbaa4d6aab02c82fe09f561       */
/*         01b0fd977b0051f0b0ce0c69f7db857b1b5e007be2db6d42894bf93de848       */
/*         806d9152bd5715e9                                                   */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#include "globals.h"
#include "errno.h"
#include "types.h"

#include "mm/mm.h"
#include "mm/tlb.h"
#include "mm/mman.h"
#include "mm/page.h"

#include "proc/proc.h"

#include "util/string.h"
#include "util/debug.h"

#include "fs/vnode.h"
#include "fs/vfs.h"
#include "fs/file.h"

#include "vm/vmmap.h"
#include "vm/mmap.h"

/*
 * This function implements the mmap(2) syscall, but only
 * supports the MAP_SHARED, MAP_PRIVATE, MAP_FIXED, and
 * MAP_ANON flags.
 *
 * Add a mapping to the current process's address space.
 * You need to do some error checking; see the ERRORS section
 * of the manpage for the problems you should anticipate.
 * After error checking most of the work of this function is
 * done by vmmap_map(), but remember to clear the TLB.
 */
int
do_mmap(void *addr, size_t len, int prot, int flags,
        int fd, off_t off, void **ret)
{
        /* NOT_YET_IMPLEMENTED("VM: do_mmap"); */
        
        //EINVAL errors 
        if ( (addr == NULL &&(flags & MAP_FIXED)) || len <= 0 ||!PAGE_ALIGNED(addr) || !PAGE_ALIGNED(off)) {
                return -EINVAL;
        }

        // anno,fix,private,shared
        int access_mode = flags % 4;
        if (!(access_mode == MAP_SHARED || access_mode == MAP_PRIVATE)) {
                return -EINVAL;
        }


        file_t *file = NULL;

        // not anonymous file
        if ((flags & MAP_ANON) == 0) {

                file = fget(fd);
                if (file == NULL) {
                        return -EBADF;
                }
                if ((file->f_mode & FMODE_READ) == 0) {
                        fput(file);
                        return -EACCES;
                }
                if (access_mode == MAP_SHARED && (prot & PROT_WRITE)) {
                        if ((file->f_mode & FMODE_WRITE) == 0 || (file->f_mode & FMODE_APPEND)) {
                                fput(file);
                                return -EACCES;
                        }
                }
        }

        vmarea_t *vma;
        vnode_t *vnode = NULL;

        uint32_t start = ADDR_TO_PN(addr);
        size_t npages = (len-1) / PAGE_SIZE + 1;
        uint32_t end = start + npages;


        if (file != NULL) {
                vnode = file->f_vnode;
        }
        int success = vmmap_map(curproc->p_vmmap, vnode, start, npages, prot, flags, off, VMMAP_DIR_HILO, &vma);

        if (success >= 0) {
                uint32_t new_addr = (addr != NULL) ? (uint32_t)addr : (uint32_t)PN_TO_ADDR(vma->vma_start);
                
                *ret = (void*)new_addr;

                tlb_flush_range(new_addr, npages);
                pt_unmap_range(pt_get(), (uint32_t)PN_TO_ADDR(vma->vma_start), (uint32_t)PN_TO_ADDR(vma->vma_start) + (unsigned int)PAGE_ALIGN_UP(len));

        }

        if (file) {
                fput(file);
        }

        KASSERT(NULL != curproc->p_pagedir);
        dbg(DBG_PRINT, "(GRADING3A 2.a)\n");

        return success;
}

/*
 * This function implements the munmap(2) syscall.
 *
 * As with do_mmap() it should perform the required error checking,
 * before calling upon vmmap_remove() to do most of the work.
 * Remember to clear the TLB.
 */
int
do_munmap(void *addr, size_t len)
{
        /*NOT_YET_IMPLEMENTED("VM: do_munmap");*/
        uint32_t address = (uint32_t) addr;
        /*error checking */
        if (address == NULL) {
                return -EINVAL;
        }
        if (!PAGE_ALIGNED(address) || len <= 0 || len > (USER_MEM_HIGH - USER_MEM_LOW)) {
              return -EINVAL;
        }

        uint32_t end_addr = address + len;
        if (!(address >= USER_MEM_LOW  && end_addr <= USER_MEM_HIGH)) {
                return -EINVAL;
        }
        if (address == NULL) {
                return -EINVAL;
        }

        uint32_t start = ADDR_TO_PN(addr);
        size_t npages = (len-1) / PAGE_SIZE + 1;
        uint32_t end = start + npages;

        if (vmmap_is_range_empty(curproc->p_vmmap, start, npages)) {
                return 0;
        }

        int ret = vmmap_remove(curproc->p_vmmap, start, npages);
        tlb_flush_range(address, npages);
        pagedir_t *page_dir = pt_get();
        pt_unmap_range(page_dir, address, (uint32_t)PN_TO_ADDR(start + npages));
        /*tlb_flush_range(vaddr, npages);*/
        return ret;
}
