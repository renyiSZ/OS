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

#include "types.h"
#include "globals.h"
#include "errno.h"

#include "util/debug.h"
#include "util/string.h"

#include "proc/proc.h"
#include "proc/kthread.h"

#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/page.h"
#include "mm/pframe.h"
#include "mm/mmobj.h"
#include "mm/pagetable.h"
#include "mm/tlb.h"

#include "fs/file.h"
#include "fs/vnode.h"

#include "vm/shadow.h"
#include "vm/vmmap.h"

#include "api/exec.h"

#include "main/interrupt.h"

/* Pushes the appropriate things onto the kernel stack of a newly forked thread
 * so that it can begin execution in userland_entry.
 * regs: registers the new thread should have on execution
 * kstack: location of the new thread's kernel stack
 * Returns the new stack pointer on success. */
static uint32_t
fork_setup_stack(const regs_t *regs, void *kstack)
{
        /* Pointer argument and dummy return address, and userland dummy return
         * address */
        uint32_t esp = ((uint32_t) kstack) + DEFAULT_STACK_SIZE - (sizeof(regs_t) + 12);
        *(void **)(esp + 4) = (void *)(esp + 8); /* Set the argument to point to location of struct on stack */
        memcpy((void *)(esp + 8), regs, sizeof(regs_t)); /* Copy over struct */
        return esp;
}


/*
 * The implementation of fork(2). Once this works,
 * you're practically home free. This is what the
 * entirety of Weenix has been leading up to.
 * Go forth and conquer.
 */
int
do_fork(struct regs *regs)
{

        KASSERT(regs != NULL); 
        KASSERT(curproc != NULL);
        KASSERT(curproc->p_state == PROC_RUNNING);
        /*create new process*/
        proc_t *newproc = proc_create("childproc");
        if(newproc == NULL){
            return -1;
        }

        /*copy vmmap and set up shadow obj*/
	    vmmap_destroy(newproc->p_vmmap);
        newproc->p_vmmap = vmmap_clone(curproc->p_vmmap);
        vmmap_t *oldmap = curproc->p_vmmap;
        vmmap_t *newmap = newproc->p_vmmap;

        if(newmap == NULL){
            return -1;
        }

        /*iterate old and new vmas, set obj and off*/
        vmarea_t *oldvma = NULL, *newvma = NULL;
        list_link_t *oldvma_plink = (list_link_t *) (oldmap->vmm_list).l_next;
        list_link_t *newvma_plink = (list_link_t *) (newmap->vmm_list).l_next;
        while(oldvma_plink != (list_link_t *)&(oldmap->vmm_list) && newvma_plink != (list_link_t *)&(newmap->vmm_list)){
            oldvma = list_item(oldvma_plink, vmarea_t, vma_plink);
            newvma = list_item(newvma_plink, vmarea_t, vma_plink);

            //newvma->vma_off = oldvma->vma_off;
            if((newvma->vma_flags & MAP_TYPE) == MAP_SHARED){
                newvma->vma_obj = oldvma->vma_obj;
  		        newvma->vma_obj->mmo_ops->ref(newvma->vma_obj);
            }
            else{
                mmobj_t *cshadow = shadow_create();
                if(cshadow == NULL){
                    vmmap_destroy(newmap);
                    return -1;
                }

                mmobj_t *pshadow = shadow_create();
                if(pshadow == NULL){
                    cshadow->mmo_ops->put(cshadow);
                    vmmap_destroy(newmap);
                    return -1;
                }
                
                /*set and ref shadowed obj*/
                cshadow->mmo_shadowed = oldvma->vma_obj;
                pshadow->mmo_shadowed = oldvma->vma_obj;

                oldvma->vma_obj->mmo_ops->ref(oldvma->vma_obj);
                //cshadow->mmo_shadowed->mmo_ops->ref(cshadow->mmo_shadowed);
                //pshadow->mmo_shadowed->mmo_ops->ref(pshadow->mmo_shadowed);

                /*set and ref bottom obj*/
                cshadow->mmo_un.mmo_bottom_obj = mmobj_bottom_obj(cshadow->mmo_shadowed);
                
                //cshadow->mmo_un.mmo_bottom_obj->mmo_ops->ref(cshadow->mmo_un.mmo_bottom_obj);
                pshadow->mmo_un.mmo_bottom_obj = mmobj_bottom_obj(pshadow->mmo_shadowed);

                list_insert_tail(&pshadow->mmo_un.mmo_bottom_obj->mmo_un.mmo_vmas, &newvma->vma_olink);
                //list_insert_tail(&cshadow->mmo_un.mmo_bottom_obj->mmo_un.mmo_vmas, &oldvma->vma_olink);
                
                //pshadow->mmo_un.mmo_bottom_obj->mmo_ops->ref(pshadow->mmo_un.mmo_bottom_obj);

                newvma->vma_obj = cshadow;
                //mmobj_t *temp = oldvma->vma_obj;
                oldvma->vma_obj = pshadow;
                
                //oldvma->vma_obj->mmo_ops->ref(oldvma->vma_obj);
                
                //temp->mmo_ops->put(temp);
            }
            /*ref newvma->vma_obj*/
            
            //newvma->vma_obj->mmo_ops->ref(newvma->vma_obj);

            /*add vma_olink to bottom vmas list*/
            //list_insert_tail(mmobj_bottom_vmas(newvma->vma_obj), &newvma->vma_olink);

            oldvma_plink = oldvma_plink->l_next;
            newvma_plink = newvma_plink->l_next;
        }




        /*create new thread*/
        kthread_t *newthr = kthread_clone(curthr);

        KASSERT(newthr->kt_kstack != NULL);
        dbg(DBG_PRINT, "(GRADING3A 7.a)\n");
        dbg(DBG_PRINT, "(SELF)\n");

        /*tie newthr with newproc*/
        newthr->kt_proc = newproc;
        list_insert_head(&newproc->p_threads, &newthr->kt_plink);

        /*setup stack*/
        regs->r_eax = (uint32_t) 0;
        uint32_t esp = fork_setup_stack(regs, newthr->kt_kstack);

        /*copy file discription table*/
        int i = 0;
        for(;i<NFILES;i++){
            newproc->p_files[i] = curproc->p_files[i];
            if(newproc->p_files[i] != NULL){
                fref(newproc->p_files[i]);
            }
        }

        /*setup brk*/
        newproc->p_brk = curproc->p_brk;
        newproc->p_start_brk = curproc->p_start_brk;
        newproc->p_cwd = curproc->p_cwd; 
        vref(curproc->p_cwd);

        /*set up thread context*/
        newthr->kt_ctx.c_eip = (uint32_t) userland_entry;
        newthr->kt_ctx.c_esp = esp;
        newthr->kt_ctx.c_ebp = curthr->kt_ctx.c_ebp;
        newthr->kt_ctx.c_pdptr = newproc->p_pagedir;
        newthr->kt_ctx.c_kstack = (uintptr_t) newthr->kt_kstack;
        newthr->kt_ctx.c_kstacksz = DEFAULT_STACK_SIZE;

        

        /*reset page table and flush tlb*/
        pt_unmap_range(curproc->p_pagedir, USER_MEM_LOW, USER_MEM_HIGH);
        tlb_flush_all();

        /*make child runnable*/
        sched_make_runnable(newthr);
        regs->r_eax = newproc->p_pid;
        
        return newproc->p_pid;
}
