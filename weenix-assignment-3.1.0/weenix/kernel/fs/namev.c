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
#include "globals.h"
#include "types.h"
#include "errno.h"

#include "util/string.h"
#include "util/printf.h"
#include "util/debug.h"

#include "fs/dirent.h"
#include "fs/fcntl.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "fs/vnode.h"

/* This takes a base 'dir', a 'name', its 'len', and a result vnode.
 * Most of the work should be done by the vnode's implementation
 * specific lookup() function.
 *
 * If dir has no lookup(), return -ENOTDIR.
 *
 * Note: returns with the vnode refcount on *result incremented.
 */
int
lookup(vnode_t *dir, const char *name, size_t len, vnode_t **result)
{
       //NOT_YET_IMPLEMENTED("VFS: lookup");
        KASSERT(NULL != dir);
        KASSERT(NULL != name);
        KASSERT(NULL != result);
        dbg(DBG_PRINT, "(GRADING2A 2.a)\n");
        dbg(DBG_PRINT,"(GRADING2B)\n");
        /*if(dir->vn_ops->lookup == NULL){
	    dbg(DBG_TEMP, "\n Self-check Code Sequence at FILE -> %s -> LINE %d\n", __FILE__, __LINE__ );
            // vnode dir do not have lookup function 
            if(dir->vn_mode & S_IFDIR){
		dbg(DBG_TEMP, "\n Self-check Code Sequence at FILE -> %s -> LINE %d\n", __FILE__, __LINE__ );
                return  -ENOENT;
            }
            return -ENOTDIR;
        }*/
        
        /*if(len > NAME_LEN)
        {
		dbg(DBG_TEMP, "\n Self-check Code Sequence at FILE -> %s -> LINE %d\n", __FILE__, __LINE__ );
		return -ENAMETOOLONG;
        }*/

        if(len == 0){
            dbg(DBG_PRINT,"(GRADING2B)\n");
            //*result = vget(dir->vn_fs,dir->vn_vno);
            //return 0;
            return -EINVAL;
        }

        int retval = dir->vn_ops->lookup(dir,name,len,result);
	    dbg(DBG_PRINT,"(GRADING2B)\n");
        return retval;
}


/* When successful this function returns data in the following "out"-arguments:
 *  o res_vnode: the vnode of the parent directory of "name"
 *  o name: the `basename' (the element of the pathname)
 *  o namelen: the length of the basename
 *
 * For example: dir_namev("/s5fs/bin/ls", &namelen, &name, NULL,
 * &res_vnode) would put 2 in namelen, "ls" in name, and a pointer to the
 * vnode corresponding to "/s5fs/bin" in res_vnode.
 *
 * The "base" argument defines where we start resolving the path from:
 * A base value of NULL means to use the process's current working directory,
 * curproc->p_cwd.  If pathname[0] == '/', ignore base and start with
 * vfs_root_vn.  dir_namev() should call lookup() to take care of resolving each
 * piece of the pathname.
 *
 * Note: A successful call to this causes vnode refcount on *res_vnode to
 * be incremented.
 */
int
dir_namev(const char *pathname, size_t *namelen, const char **name,
          vnode_t *base, vnode_t **res_vnode)
{
        /*NOT_YET_IMPLEMENTED("VFS: dir_namev");
        return 0;*/
        KASSERT(NULL != pathname);
	dbg(DBG_PRINT,"(GRADING2A 2.b)\n");
        KASSERT(NULL != namelen);
	dbg(DBG_PRINT,"(GRADING2A 2.b)\n");
        KASSERT(NULL != name);
	dbg(DBG_PRINT,"(GRADING2A 2.b)\n");
        KASSERT(NULL != res_vnode);
	dbg(DBG_PRINT,"(GRADING2A 2.b)\n");

	if(strcmp(pathname, "/") == 0){
		dbg(DBG_PRINT,"(GRADING2B)\n");
		*res_vnode = vfs_root_vn;
		vref(vfs_root_vn);
		*name = ".";
		*namelen = 1;
		return 0;
	}

	int baseIsNull = 0;
        if(base == NULL)
        {
		dbg(DBG_PRINT,"(GRADING2A)\n");
		baseIsNull = 1;
                base = curproc->p_cwd;
		vref(base);
        }

        if(pathname[0] == '/')
        {
		dbg(DBG_PRINT,"(GRADING2A)\n");
		vput(base);
                base = vfs_root_vn;
		vref(base);
        }

        int i = 0, j = 0;
        int err = 0;

        while(1)
        {
                while(pathname[i] == '/')
                {
			dbg(DBG_PRINT,"(GRADING2A)\n");
                        i++;
                }
                
                while(pathname[i + j] != '/' && pathname[i + j] != '\0')
                {
			dbg(DBG_PRINT,"(GRADING2A)\n");
                        j++;
		}

                if(j > NAME_LEN)
                {
			if(baseIsNull == 1) 
			{
				dbg(DBG_PRINT,"(GRADING2B)\n");
				vput(base);
			}
			dbg(DBG_PRINT,"(GRADING2B)\n");
               		return -ENAMETOOLONG;
                }

				int k = 0;
				if(pathname[i + j] == '/')
				{
					dbg(DBG_PRINT,"(GRADING2A)\n");
					k = i + j;
					while(pathname[k] == '/') 
					{
						dbg(DBG_PRINT,"(GRADING2A)\n");
						k++;
					}
					if(pathname[k] == '\0')
					{
						dbg(DBG_PRINT,"(GRADING2B)\n");
						vnode_t *last = NULL;
						/*if(0 != (err = lookup(base, &pathname[i], j, &last))) 
						{
								if(baseIsNull == 1) 
								{
									dbg(DBG_PRINT,"(self10)\n");
									vput(base);
								}
								dbg(DBG_PRINT,"(self11)\n");
								return err;
						}*/
						err = lookup(base, &pathname[i], j, &last);
						if(!S_ISDIR(last->vn_mode))
						{
							vput(last);
							if(baseIsNull == 1) 
							{
								dbg(DBG_PRINT,"(GRADING2B)\n");
								vput(base);
							}																														 								dbg(DBG_PRINT,"(GRADING2B)\n");
							return -ENOTDIR;
						}
						dbg(DBG_PRINT,"(GRADING2B)\n");
						vput(last);
					}
				}

                if(pathname[i + j] == '\0' || pathname[k] == '\0')
                {
			dbg(DBG_PRINT,"(GRADING2A)\n");
                	*res_vnode = base;/*vget(base->vn_fs, base->vn_vno);*/
			vref(*res_vnode);
                    	KASSERT(NULL != *res_vnode);
                    	dbg(DBG_PRINT,"(GRADING2A 2.b)\n");
                    	if(!S_ISDIR((*res_vnode)->vn_mode))
                    	{
				dbg(DBG_PRINT,"(GRADING2B)\n");
                    		vput(*res_vnode);
				if(baseIsNull == 1) 
				{
					dbg(DBG_PRINT,"(GRADING2B)\n");
					vput(base);
				}
				dbg(DBG_PRINT,"(GRADING2B)\n");
                        	return -ENOTDIR;
                  	}
                    	*namelen = j;
                    	*name = &pathname[i];
			if(baseIsNull == 1) 
			{
				dbg(DBG_PRINT,"(GRADING2A)\n");
				vput(base);
			}
			dbg(DBG_PRINT,"(GRADING2A)\n");
                    	return 0;
                }

                err = lookup(base, &pathname[i], j, res_vnode);
                if(err != 0)
                {
			dbg(DBG_PRINT,"(GRADING2B)\n");
			if(baseIsNull == 1) 
			{
				dbg(DBG_PRINT,"(GRADING2B)\n");
				vput(base);
			}
			dbg(DBG_PRINT,"(GRADING2B)\n");
                        return err;
                }
		dbg(DBG_PRINT,"(GRADING2A)\n");
		vput(base);
                base = *res_vnode;
		vref(base);
                vput(*res_vnode);
                i = i + j + 1;
                j = 0;
        }
		
		/*KASSERT(NULL != pathname);
        KASSERT(NULL != namelen);
        KASSERT(NULL != name);
        KASSERT(NULL != res_vnode);
        dbg(DBG_PRINT, "(GRADING2A 2.b)\n");
        dbg(DBG_PRINT, "(GRADING2B)\n");

        vnode_t *root = NULL;
        int startIdx = 0, endIdx = 0, len = 0;
        int retVal = 0;
         
        if(pathname[0] == '/'){
            root = vfs_root_vn;
            dbg(DBG_PRINT, "(GRADING2B)\n");
        }else{
            dbg(DBG_PRINT, "(GRADING2B)\n");
            if(base == NULL){
                root = curproc->p_cwd; 
                dbg(DBG_PRINT, "(GRADING2B)\n");
            }
        }
        while(pathname[endIdx] == '/') {
                endIdx++;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }
        
        while(1){
            startIdx = endIdx;
            while(pathname[endIdx] != '\0' && pathname[endIdx] != '/') {
                endIdx ++;
                dbg(DBG_PRINT, "(GRADING2B)\n");
            }
            len = endIdx - startIdx;
            if(len > NAME_LEN) {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -ENAMETOOLONG;
            }
            if(pathname[endIdx] == '\0'){
                root = vget(root->vn_fs, root->vn_vno);
                dbg(DBG_PRINT, "(GRADING2B)\n");
                if(!S_ISDIR(root->vn_mode)){
                    vput(root);
                    dbg(DBG_PRINT, "(GRADING2B)\n");
                    return -ENOTDIR;
                }
                dbg(DBG_PRINT, "(GRADING2B)\n");
                break;
            }
            retVal = lookup(root, (pathname + startIdx), len, &root);  
            dbg(DBG_PRINT, "(GRADING2B)\n");    
            if(retVal < 0){
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return retVal;
            }
            vput(root);
            dbg(DBG_PRINT, "(GRADING2B)\n");
            KASSERT(NULL != root);
            dbg(DBG_PRINT, "(GRADING2A 2.b)\n");
            dbg(DBG_PRINT, "(GRADING2B)\n");
            while(pathname[endIdx] == '/') {
                endIdx ++;  
                dbg(DBG_PRINT, "(GRADING2B)\n");
            }
        }
        *namelen = len;
        *name = pathname + startIdx;
        *res_vnode = root;
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return retVal;*/
}

/* This returns in res_vnode the vnode requested by the other parameters.
 * It makes use of dir_namev and lookup to find the specified vnode (if it
 * exists).  flag is right out of the parameters to open(2); see
 * <weenix/fcntl.h>.  If the O_CREAT flag is specified and the file does
 * not exist, call create() in the parent directory vnode. However, if the
 * parent directory itself does not exist, this function should fail - in all
 * cases, no files or directories other than the one at the very end of the path
 * should be created.
 *
 * Note: Increments vnode refcount on *res_vnode.
 */
int
open_namev(const char *pathname, int flag, vnode_t **res_vnode, vnode_t *base)
{
        //NOT_YET_IMPLEMENTED("VFS: open_namev");
        /*if(pathname[0]=='\0')
        {
		dbg(DBG_TEMP, "\n Self-check Code Sequence at FILE -> %s -> LINE %d\n", __FILE__, __LINE__ );
            	return -EINVAL;
        }*/
        size_t namelen;
        const char* name = NULL;
        vnode_t *ret_vnode;

        int retval = dir_namev(pathname,&namelen,&name,base,&ret_vnode);
	    dbg(DBG_PRINT,"(GRADING2B)\n");
        if(retval < 0){
	        dbg(DBG_PRINT,"(GRADING2B)\n");
            return retval;
	    }
        retval = lookup(ret_vnode, name , namelen, res_vnode);
    	dbg(DBG_PRINT,"(GRADING2B)\n");
        

        if(retval == -ENOENT && (flag & O_CREAT)){
            KASSERT(NULL != ret_vnode->vn_ops->create);
            dbg(DBG_PRINT, "(GRADING2A 2.c)\n");
            dbg(DBG_PRINT,"(GRADING2B)\n");
            retval = ret_vnode->vn_ops->create(ret_vnode, name, namelen, res_vnode);
	        dbg(DBG_PRINT,"(GRADING2B)\n");
        }

        vput(ret_vnode);
	    dbg(DBG_PRINT,"(GRADING2B)\n");
        
        return retval;
}
#ifdef __GETCWD__
/* Finds the name of 'entry' in the directory 'dir'. The name is writen
 * to the given buffer. On success 0 is returned. If 'dir' does not
 * contain 'entry' then -ENOENT is returned. If the given buffer cannot
 * hold the result then it is filled with as many characters as possible
 * and a null terminator, -ERANGE is returned.
 *
 * Files can be uniquely identified within a file system by their
 * inode numbers. */
int
lookup_name(vnode_t *dir, vnode_t *entry, char *buf, size_t size)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_name");
        return -ENOENT;
}


/* Used to find the absolute path of the directory 'dir'. Since
 * directories cannot have more than one link there is always
 * a unique solution. The path is writen to the given buffer.
 * On success 0 is returned. On error this function returns a
 * negative error code. See the man page for getcwd(3) for
 * possible errors. Even if an error code is returned the buffer
 * will be filled with a valid string which has some partial
 * information about the wanted path. */
ssize_t
lookup_dirpath(vnode_t *dir, char *buf, size_t osize)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_dirpath");

        return -ENOENT;
}
#endif /* __GETCWD__ */
