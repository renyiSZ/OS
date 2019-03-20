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

#include "kernel.h"
#include "errno.h"
#include "globals.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "proc/proc.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/fcntl.h"
#include "fs/vfs_syscall.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/mmobj.h"

static slab_allocator_t *vmmap_allocator;
static slab_allocator_t *vmarea_allocator;

void
vmmap_init(void)
{
        vmmap_allocator = slab_allocator_create("vmmap", sizeof(vmmap_t));
        KASSERT(NULL != vmmap_allocator && "failed to create vmmap allocator!");
        vmarea_allocator = slab_allocator_create("vmarea", sizeof(vmarea_t));
        KASSERT(NULL != vmarea_allocator && "failed to create vmarea allocator!");
}

vmarea_t *
vmarea_alloc(void)
{
        vmarea_t *newvma = (vmarea_t *) slab_obj_alloc(vmarea_allocator);
        if (newvma) {
                newvma->vma_vmmap = NULL;
        }
        return newvma;
}

void
vmarea_free(vmarea_t *vma)
{
        KASSERT(NULL != vma);
        slab_obj_free(vmarea_allocator, vma);
}

/* a debugging routine: dumps the mappings of the given address space. */
size_t
vmmap_mapping_info(const void *vmmap, char *buf, size_t osize)
{
        KASSERT(0 < osize);
        KASSERT(NULL != buf);
        KASSERT(NULL != vmmap);

        vmmap_t *map = (vmmap_t *)vmmap;
        vmarea_t *vma;
        ssize_t size = (ssize_t)osize;

        int len = snprintf(buf, size, "%21s %5s %7s %8s %10s %12s\n",
                           "VADDR RANGE", "PROT", "FLAGS", "MMOBJ", "OFFSET",
                           "VFN RANGE");

        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {
                size -= len;
                buf += len;
                if (0 >= size) {
                        goto end;
                }

                len = snprintf(buf, size,
                               "%#.8x-%#.8x  %c%c%c  %7s 0x%p %#.5x %#.5x-%#.5x\n",
                               vma->vma_start << PAGE_SHIFT,
                               vma->vma_end << PAGE_SHIFT,
                               (vma->vma_prot & PROT_READ ? 'r' : '-'),
                               (vma->vma_prot & PROT_WRITE ? 'w' : '-'),
                               (vma->vma_prot & PROT_EXEC ? 'x' : '-'),
                               (vma->vma_flags & MAP_SHARED ? " SHARED" : "PRIVATE"),
                               vma->vma_obj, vma->vma_off, vma->vma_start, vma->vma_end);
        } list_iterate_end();

end:
        if (size <= 0) {
                size = osize;
                buf[osize - 1] = '\0';
        }
        /*
        KASSERT(0 <= size);
        if (0 == size) {
                size++;
                buf--;
                buf[0] = '\0';
        }
        */
        return osize - size;
}

/* Create a new vmmap, which has no vmareas and does
 * not refer to a process. */
vmmap_t *
vmmap_create(void)
{
        vmmap_t *new_vmm = (vmmap_t *) slab_obj_alloc(vmmap_allocator);
        memset(new_vmm, 0, sizeof(vmmap_t));
        if (new_vmm != NULL){
            list_init(&new_vmm->vmm_list);
            new_vmm->vmm_proc = NULL;
            dbg(DBG_PRINT, "(GRADING3B 1)\n");
        }
        dbg(DBG_PRINT, "(GRADING3B 1)\n");
        return new_vmm;
}

/* Removes all vmareas from the address space and frees the
 * vmmap struct. */
void
vmmap_destroy(vmmap_t *map)
{
        KASSERT(NULL != map);
        dbg(DBG_PRINT, "(GRADING3A 3.a)\n");

        if(!list_empty(&map->vmm_list)) {
            dbg(DBG_PRINT, "(GRADING3B 1)\n");
            vmarea_t *vma;
            list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink){
                list_remove(&vma->vma_plink); 

                if (list_link_is_linked(&vma->vma_olink)){
                    list_remove(&vma->vma_olink);
                    dbg(DBG_PRINT, "(GRADING3B 1)\n");
                }
                if (vma->vma_obj != NULL){
                    vma->vma_obj->mmo_ops->put(vma->vma_obj);
                    dbg(DBG_PRINT, "(GRADING3B 1)\n");
                }

                vmarea_free(vma);
                dbg(DBG_PRINT, "(GRADING3B 1)\n");
            } list_iterate_end();
        }
        slab_obj_free(vmmap_allocator, (void*) map);
        dbg(DBG_PRINT, "(GRADING3B 1)\n");
}

/* Add a vmarea to an address space. Assumes (i.e. asserts to some extent)
 * the vmarea is valid.  This involves finding where to put it in the list
 * of VM areas, and adding it. Don't forget to set the vma_vmmap for the
 * area. */
void
vmmap_insert(vmmap_t *map, vmarea_t *newvma)
{
    KASSERT(NULL != map && NULL != newvma); /* both function arguments must not be NULL */
    KASSERT(NULL == newvma->vma_vmmap); /* newvma must be newly create and must not be part of any existing vmmap */
    KASSERT(newvma->vma_start < newvma->vma_end); /* newvma must not be empty */
    KASSERT(ADDR_TO_PN(USER_MEM_LOW) <= newvma->vma_start && ADDR_TO_PN(USER_MEM_HIGH) >= newvma->vma_end);/* addresses in this memory segment must lie completely within the user space */
    dbg(DBG_PRINT, "(GRADING3A 3.b)\n");
    dbg(DBG_PRINT, "(GRADING3B 1)\n");
    
    if(!list_empty(&map->vmm_list)) {
        dbg(DBG_PRINT, "(GRADING3B 1)\n");

        vmarea_t *vmarea = NULL;
        list_link_t *list = &map->vmm_list;
        list_iterate_begin(list, vmarea, vmarea_t, vma_plink){
            if (newvma->vma_end <= vmarea->vma_start) {
                dbg(DBG_PRINT, "(GRADING3B 1)\n");
                if(list == vmarea->vma_plink.l_prev){
                    list_insert_head(list, &newvma->vma_plink);
                    newvma->vma_vmmap = map;
                    dbg(DBG_PRINT, "(GRADING3B 1)\n");
                    return;
                }
                else{
                    vmarea_t *vma_prev = list_item(vmarea->vma_plink.l_prev, vmarea_t, vma_plink);
                    dbg(DBG_PRINT, "(GRADING3D 1)\n");
                    if(newvma->vma_start >= vma_prev->vma_end) {
                            list_insert_before(&vmarea->vma_plink, &newvma->vma_plink);
                            newvma->vma_vmmap = map;
                            dbg(DBG_PRINT, "(GRADING3D 1)\n");
                            return;
                    }
                }   
            } 
        }list_iterate_end();
        list_insert_tail(&map->vmm_list, &newvma->vma_plink); 
        dbg(DBG_PRINT, "(GRADING3B 1)\n");
    } 
    else {  
        list_insert_head(&map->vmm_list, &newvma->vma_plink); 
        dbg(DBG_PRINT, "(GRADING3B 1)\n");        
    }
    newvma->vma_vmmap = map;
    dbg(DBG_PRINT, "(GRADING3B 1)\n");
}

/* Find a contiguous range of free virtual pages of length npages in
 * the given address space. Returns starting vfn for the range,
 * without altering the map. Returns -1 if no such range exists.
 *
 * Your algorithm should be first fit. If dir is VMMAP_DIR_HILO, you
 * should find a gap as high in the address space as possible; if dir
 * is VMMAP_DIR_LOHI, the gap should be as low as possible. */
int
vmmap_find_range(vmmap_t *map, uint32_t npages, int dir)
{
    /*NOT_YET_IMPLEMENTED("VM: vmmap_find_range");*/
        uint32_t page_high = ADDR_TO_PN(USER_MEM_HIGH);
        uint32_t page_low = ADDR_TO_PN(USER_MEM_LOW);

        if(list_empty(&map->vmm_list)){
            if(dir == VMMAP_DIR_HILO){
                return page_high - npages;
            }else{
                return page_low;
            }    
        }

        if(dir == VMMAP_DIR_HILO){
            vmarea_t *iter;
                list_iterate_reverse(&map->vmm_list, iter, vmarea_t, vma_plink) {
                        if(iter->vma_plink.l_next == &map->vmm_list){
                                if(page_high - iter->vma_end >= npages) {
                                        return (page_high - npages);
                                } 
                        } else {
                                vmarea_t *next = list_item(iter->vma_plink.l_next, vmarea_t, vma_plink);
                                if(next->vma_start - iter->vma_end >= npages) {
                                        return (next->vma_start - npages);
                                }
                        }
                } list_iterate_end();
                if(iter->vma_start - page_low >= npages) {
                        return iter->vma_start - npages;
                }
        }
        return -1;
}

/* Find the vm_area that vfn lies in. Simply scan the address space
 * looking for a vma whose range covers vfn. If the page is unmapped,
 * return NULL. */
vmarea_t *
vmmap_lookup(vmmap_t *map, uint32_t vfn)
{
        /*NOT_YET_IMPLEMENTED("VM: vmmap_lookup");*/
        vmarea_t *iter;
        list_iterate_begin(&map->vmm_list, iter, vmarea_t, vma_plink){
            if(vfn >= iter->vma_start && vfn < iter->vma_end){
                return iter;
            }
        }list_iterate_end();
        return NULL;
}

/* Allocates a new vmmap containing a new vmarea for each area in the
 * given map. The areas should have no mmobjs set yet. Returns pointer
 * to the new vmmap on success, NULL on failure. This function is
 * called when implementing fork(2). */
vmmap_t *
vmmap_clone(vmmap_t *map)
{
       vmmap_t *clonevmm = vmmap_create();
       dbg(DBG_PRINT, "(GRADING3B 1)\n");
        if(clonevmm != NULL){
            dbg(DBG_PRINT, "(GRADING3B 1)\n");
            vmarea_t *vmarea;
            list_iterate_begin(&map->vmm_list, vmarea, vmarea_t, vma_plink) {
                vmarea_t *new_vmarea = vmarea_alloc();
                if(new_vmarea !=NULL) {
                    new_vmarea->vma_start = vmarea->vma_start;
                    new_vmarea->vma_end = vmarea->vma_end;
                    new_vmarea->vma_off = vmarea->vma_off;
                    new_vmarea->vma_prot = vmarea->vma_prot;
                    new_vmarea->vma_flags = vmarea->vma_flags;
                    list_link_init(&new_vmarea->vma_plink);
                    vmmap_insert(clonevmm, new_vmarea);
                    list_link_init(&new_vmarea->vma_olink);
                    new_vmarea->vma_obj = NULL;
                    dbg(DBG_PRINT, "(GRADING3B 1)\n");
                    //new_vmarea->vma_obj->mmo_ops->ref(new_vmarea->vma_obj);
                } 
                dbg(DBG_PRINT, "(GRADING3B 1)\n");
                /*else{
                    //no dbg delete!!!
                    vmmap_destroy(clonevmm);
                    return NULL;
                }*/
            } list_iterate_end();
        }
        dbg(DBG_PRINT, "(GRADING3B 1)\n");
        return clonevmm;
}

/* Insert a mapping into the map starting at lopage for npages pages.
 * If lopage is zero, we will find a range of virtual addresses in the
 * process that is big enough, by using vmmap_find_range with the same
 * dir argument.  If lopage is non-zero and the specified region
 * contains another mapping that mapping should be unmapped.
 *
 * If file is NULL an anon mmobj will be used to create a mapping
 * of 0's.  If file is non-null that vnode's file will be mapped in
 * for the given range.  Use the vnode's mmap operation to get the
 * mmobj for the file; do not assume it is file->vn_obj. Make sure all
 * of the area's fields except for vma_obj have been set before
 * calling mmap.
 *
 * If MAP_PRIVATE is specified set up a shadow object for the mmobj.
 *
 * All of the input to this function should be valid (KASSERT!).
 * See mmap(2) for for description of legal input.
 * Note that off should be page aligned.
 *
 * Be very careful about the order operations are performed in here. Some
 * operation are impossible to undo and should be saved until there
 * is no chance of failure.
 *
 * If 'new' is non-NULL a pointer to the new vmarea_t should be stored in it.
 */
int
vmmap_map(vmmap_t *map, vnode_t *file, uint32_t lopage, uint32_t npages,
          int prot, int flags, off_t off, int dir, vmarea_t **new)
{
        /* NOT_YET_IMPLEMENTED("VM: vmmap_map"); */
        vmarea_t *new_vma;
        int retVal = 0, startvfn = 0;
        
        if(lopage == 0){
            startvfn = vmmap_find_range(map, npages, dir);
            if(startvfn < 0){
                return -ENOMEM;
            }
        }else{
            if(!vmmap_is_range_empty(map, lopage, npages)){
                retVal = vmmap_remove(map, lopage, npages);
                if(retVal < 0){
                    return retVal;
                }  
            }
            startvfn = lopage;
        }   
        new_vma = vmarea_alloc();
        memset(new_vma, 0, sizeof(vmarea_t));
        new_vma->vma_start = startvfn;
        new_vma->vma_end = startvfn + npages;
        new_vma->vma_prot = prot;
        new_vma->vma_flags = flags;
        new_vma->vma_off = off;
        vmmap_insert(map, new_vma); 
        
        if (file == NULL) {
            new_vma->vma_obj = anon_create();
        }
        else {
            retVal = file->vn_ops->mmap(file, new_vma, &(new_vma->vma_obj));
            if (retVal < 0) {
                    vmarea_free(new_vma);
                    return retVal;
            }
        }

        
        if (flags == MAP_PRIVATE) {
                mmobj_t *shadow = shadow_create();
                shadow->mmo_shadowed = new_vma->vma_obj;
                shadow->mmo_un.mmo_bottom_obj = new_vma->vma_obj;
                list_insert_tail(&new_vma->vma_obj->mmo_un.mmo_vmas, &new_vma->vma_olink);
                new_vma->vma_obj = shadow;
        }

        if (new != NULL) {
                *new = new_vma;       
        }
        return 0;
}

/*
 * We have no guarantee that the region of the address space being
 * unmapped will play nicely with our list of vmareas.
 *
 * You must iterate over each vmarea that is partially or wholly covered
 * by the address range [addr ... addr+len). The vm-area will fall into one
 * of four cases, as illustrated below:
 *
 * key:
 *          [             ]   Existing VM Area
 *        *******             Region to be unmapped
 *
 * Case 1:  [   ******    ]
 * The region to be unmapped lies completely inside the vmarea. We need to
 * split the old vmarea into two vmareas. be sure to increment the
 * reference count to the file associated with the vmarea.
 *
 * Case 2:  [      *******]**
 * The region overlaps the end of the vmarea. Just shorten the length of
 * the mapping.
 *
 * Case 3: *[*****        ]
 * The region overlaps the beginning of the vmarea. Move the beginning of
 * the mapping (remember to update vma_off), and shorten its length.
 *
 * Case 4: *[*************]**
 * The region completely contains the vmarea. Remove the vmarea from the
 * list.
 */
int
vmmap_remove(vmmap_t *map, uint32_t lopage, uint32_t npages)
{
          if(!list_empty(&map->vmm_list)){
            dbg(DBG_PRINT, "(GRADING3B 1)\n");
            vmarea_t *vmarea;
            list_iterate_begin(&map->vmm_list, vmarea, vmarea_t, vma_plink) {
                //case1:[   ******    ]
                if(lopage > vmarea->vma_start && (lopage + npages) < vmarea->vma_end) {
                    vmarea_t * new_vmarea=vmarea_alloc();
                    /* 
                    //no dbg delete
                    if (new_vmarea == NULL) {
                        return -ENOSPC;
                    }*/
                    if(new_vmarea != NULL){
                        new_vmarea->vma_vmmap = vmarea->vma_vmmap;
                        new_vmarea->vma_prot=vmarea->vma_prot;
                        new_vmarea->vma_flags=vmarea->vma_flags;
                        new_vmarea->vma_obj=vmarea->vma_obj;
                        new_vmarea->vma_obj->mmo_ops->ref(new_vmarea->vma_obj);

                        list_link_init(&new_vmarea->vma_plink);
                        list_link_init(&new_vmarea->vma_olink);
                        list_insert_before(vmarea->vma_plink.l_next, &new_vmarea->vma_plink);
                        list_insert_tail(mmobj_bottom_vmas(new_vmarea->vma_obj), &new_vmarea->vma_olink);

                        new_vmarea->vma_start=lopage + npages;
                        new_vmarea->vma_end= vmarea->vma_end;
                        new_vmarea->vma_off=vmarea->vma_off + lopage + npages - vmarea->vma_start;
                        vmarea->vma_end = lopage;
                        dbg(DBG_PRINT, "(GRADING3D 2)\n");
                        break;  
                    } 
                    dbg(DBG_PRINT, "(GRADING3D 2)\n");
                }
                //case2:[      *******]**
                else if(lopage > vmarea->vma_start && lopage < vmarea->vma_end && (lopage + npages) >= vmarea->vma_end) {
                    vmarea->vma_end = lopage;
                    dbg(DBG_PRINT, "(GRADING3D 2)\n");
                }
                //case3: *[*****        ]
                else if(lopage <= vmarea->vma_start && (lopage + npages) > vmarea->vma_start && (lopage + npages) < vmarea->vma_end) {
                    vmarea->vma_off = vmarea->vma_off + (lopage + npages - vmarea->vma_start);
                    vmarea->vma_start = lopage + npages;  
                    dbg(DBG_PRINT, "(GRADING3D 2)\n");
                }
                //case4: *[*************]**
                else if(lopage <= vmarea->vma_start && (lopage + npages) >= vmarea->vma_end) {
                    list_remove(&vmarea->vma_plink);
                    if (list_link_is_linked(&vmarea->vma_olink)){
                        list_remove(&vmarea->vma_olink);
                        dbg(DBG_PRINT, "(GRADING3D 2)\n");
                    }
                    if (vmarea->vma_obj != NULL) {
                        vmarea->vma_obj->mmo_ops->put(vmarea->vma_obj);
                        dbg(DBG_PRINT, "(GRADING3B 1)\n");
                    } 
                    vmarea_free(vmarea);
                    dbg(DBG_PRINT, "(GRADING3B 1)\n");
                }
            }list_iterate_end();
        }
        //pt_unmap_range(curproc->p_pagedir, (uintptr_t)PN_TO_ADDR(lopage), (uintptr_t)PN_TO_ADDR(lopage + npages));
        dbg(DBG_PRINT, "(GRADING3B 1)\n");
        return 0;
}

/*
 * Returns 1 if the given address space has no mappings for the
 * given range, 0 otherwise.
 */
int
vmmap_is_range_empty(vmmap_t *map, uint32_t startvfn, uint32_t npages)
{
        /*NOT_YET_IMPLEMENTED("VM: vmmap_is_range_empty");*/
        uint32_t endvfn = startvfn + npages;
        vmarea_t *iter = NULL;
        list_iterate_begin(&map->vmm_list, iter, vmarea_t, vma_plink){
            if((startvfn >= iter->vma_start && startvfn < iter->vma_end) 
                || (endvfn > iter->vma_start && endvfn <= iter->vma_end) 
                || (startvfn < iter->vma_start && endvfn >= iter->vma_end)){
                return 0;
            }
        }list_iterate_end();

        return 1;
}

/* Read into 'buf' from the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do so, you will want to find the vmareas
 * to read from, then find the pframes within those vmareas corresponding
 * to the virtual addresses you want to read, and then read from the
 * physical memory that pframe points to. You should not check permissions
 * of the areas. Assume (KASSERT) that all the areas you are accessing exist.
 * Returns 0 on success, -errno on error.
 */
int
vmmap_read(vmmap_t *map, const void *vaddr, void *buf, size_t count)
{
        /*NOT_YET_IMPLEMENTED("VM: vmmap_read");*/
    char* num_buf = (char *)buf;
    char* num_vaddr=(char *)vaddr;          
    vmarea_t *vmarea;
    pframe_t *temppgFrame;
        while(count > 0) {
                vmarea = vmmap_lookup(map, ADDR_TO_PN(num_vaddr));
                int result = pframe_lookup(vmarea->vma_obj, ADDR_TO_PN(num_vaddr) - vmarea->vma_start + vmarea->vma_off, 0, &temppgFrame);

                
                if(result < 0) {
                        
                        return result;
                }

                size_t length = (PAGE_SIZE-PAGE_OFFSET(num_vaddr)) < count ? (PAGE_SIZE-PAGE_OFFSET(num_vaddr)) : count;
                memcpy(num_buf, (char *)temppgFrame->pf_addr + PAGE_OFFSET(num_vaddr), length);

                num_buf = num_buf + length;
                num_vaddr = num_vaddr + length;
                count = count - length;
              
        }
        
        return 0;
}

/* Write from 'buf' into the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do this, you will need to find the correct
 * vmareas to write into, then find the correct pframes within those vmareas,
 * and finally write into the physical addresses that those pframes correspond
 * to. You should not check permissions of the areas you use. Assume (KASSERT)
 * that all the areas you are accessing exist. Remember to dirty pages!
 * Returns 0 on success, -errno on error.
 */
int
vmmap_write(vmmap_t *map, void *vaddr, const void *buf, size_t count)
{
    char* num_buf = (char *)buf;
    char* num_vaddr=(char *)vaddr;
        vmarea_t *vmarea;
    pframe_t *temppgFrame;
        while(count > 0) {
                vmarea = vmmap_lookup(map, ADDR_TO_PN(num_vaddr));
                int result = pframe_lookup(vmarea->vma_obj, ADDR_TO_PN(num_vaddr) - vmarea->vma_start + vmarea->vma_off, 1, &temppgFrame);

                
                if(result < 0) {
                
                        return result;
                }

                size_t length = (PAGE_SIZE-PAGE_OFFSET(num_vaddr)) < count ? (PAGE_SIZE-PAGE_OFFSET(num_vaddr)) : count;
                memcpy((char *)temppgFrame->pf_addr + PAGE_OFFSET(num_vaddr),buf,length);

                num_buf = num_buf + length;
                num_vaddr = num_vaddr + length;
                count = count - length;
              
        }
        
        return 0;
}