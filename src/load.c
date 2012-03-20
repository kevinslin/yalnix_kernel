#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <comp421/hardware.h>
#include <comp421/loadinfo.h>
#include "yalnix_mem.h"


/*
 *  Load a program into the current process's address space.  The
 *  program comes from the Unix file identified by "name", and its
 *  arguments come from the array at "args", which is in standard
 *  argv format.
 *
 *  Returns:
 *      0 on success
 *     -1 on any error for which the current process is still runnable
 *     -2 on any error for which the current process is no longer runnable
 *
 *  This function, after a series of initial checks, deletes the
 *  contents of Region 0, thus making the current process no longer
 *  runnable.  Before this point, it is possible to return ERROR
 *  to an Exec() call that has called LoadProgram, and this function
 *  returns -1 for errors up to this point.  After this point, the
 *  contents of Region 0 no longer exist, so the calling user process
 *  is no longer runnable, and this function returns -2 for errors
 *  in this case.
 */
  int
LoadProgram(char *name, char **args, ExceptionStackFrame *frame, struct pcb **pcb_p)
{
  int fd;
  int status;
  /* Struct containing heap space sizes */
  struct loadinfo li;
  char *cp;
  char *cp2;
  char **cpp;
  char *argbuf;
  int i;
  unsigned long argcount;
  int size;
  int text_npg;
  int data_bss_npg;
  int stack_npg;
  int j,k;
  struct pte *page_table;
  dprintf("in load program", 1);

  TracePrintf(0, "LoadProgram '%s', args %p\n", name, args);

  if ((fd = open(name, O_RDONLY)) < 0) {
    TracePrintf(0, "LoadProgram: can't open file '%s'\n", name);
    return (-1);
  }

  status = LoadInfo(fd, &li);
  TracePrintf(0, "LoadProgram: LoadInfo status %d\n", status);
  switch (status) {
    case LI_SUCCESS:
      break;
    case LI_FORMAT_ERROR:
      TracePrintf(0,
          "LoadProgram: '%s' not in Yalnix format\n", name);
      close(fd);
      return (-1);
    case LI_OTHER_ERROR:
      TracePrintf(0, "LoadProgram: '%s' other error\n", name);
      close(fd);
      return (-1);
    default:
      TracePrintf(0, "LoadProgram: '%s' unknown error\n", name);
      close(fd);
      return (-1);
  }
  TracePrintf(0, "text_size 0x%lx, data_size 0x%lx, bss_size 0x%lx\n",
      li.text_size, li.data_size, li.bss_size);
  TracePrintf(0, "entry 0x%lx\n", li.entry);

  /*
   *  Figure out how many bytes are needed to hold the arguments on
   *  the new stack that we are building.  Also count the number of
   *  arguments, to become the argc that the new "main" gets called with.
   */
  size = 0;
  for (i = 0; args[i] != NULL; i++) {
    size += strlen(args[i]) + 1;
  }
  argcount = i;
  TracePrintf(0, "LoadProgram: size %d, argcount %d\n", size, argcount);

  /*
   *  Now save the arguments in a separate buffer in Region 1, since
   *  we are about to delete all of Region 0.
   */
  dprintf("saving args to separate buffer in region0...", 0);
  cp = argbuf = (char *)malloc(size);
  for (i = 0; args[i] != NULL; i++) {
    strcpy(cp, args[i]);
    cp += strlen(cp) + 1;
  }

  /*
   *  The arguments will get copied starting at "cp" as set below,
   *  and the argv pointers to the arguments (and the argc value)
   *  will get built starting at "cpp" as set below.  The value for
   *  "cpp" is computed by subtracting off space for the number of
   *  arguments plus 4 (for the argc value, a 0 (AT_NULL) to
   *  terminate the auxiliary vector, a NULL pointer terminating
   *  the argv pointers, and a NULL pointer terminating the envp
   *  pointers) times the size of each (sizeof(void *)).  The
   *  value must also be aligned down to a multiple of 8 boundary.
   */
  cp = ((char *)USER_STACK_LIMIT) - size;
  cpp = (char **)((unsigned long)cp & (-1 << 4));	/* align cpp */
  cpp = (char **)((unsigned long)cpp - ((argcount + 4) * sizeof(void *)));

  text_npg = li.text_size >> PAGESHIFT;
  data_bss_npg = UP_TO_PAGE(li.data_size + li.bss_size) >> PAGESHIFT;
  stack_npg = (USER_STACK_LIMIT - DOWN_TO_PAGE(cpp)) >> PAGESHIFT;

  TracePrintf(0, "LoadProgram: text_npg %d, data_bss_npg %d, stack_npg %d\n",
      text_npg, data_bss_npg, stack_npg);

  /*
   *  Make sure we will leave at least one page between heap and stack
   */
  dprintf("check if program can fit inside virtual memory...", 0);
  if (MEM_INVALID_PAGES + text_npg + data_bss_npg + stack_npg + 1 + KERNEL_STACK_PAGES >= PAGE_TABLE_LEN) {
    TracePrintf(0, "LoadProgram: program '%s' size too large for VM\n",
        name);
    free(argbuf);
    close(fd);
    return (-1);
  }

  /*
     //TODO: (second part of this..)
     >>>> In checking that there is enough free physical
     >>>> memory for this, be sure to allow for the physical memory
     >>>> pages already allocated to this process that will be
     >>>> freed below before we allocate the needed pages for
     >>>> the new program being loaded.
     */
  /*
   * Check if we have enough physical space for the new process
   * Need to have space for the text, data/bss and stack pages
   */
  dprintf("checking if we have enough free memory...", 0);
  if (len_free_frames() < (text_npg + data_bss_npg + stack_npg)) {
    TracePrintf(0,
        "LoadProgram: program '%s' size too large for physical memory\n",
        name);
    free(argbuf);
    close(fd);
    return (-1);
  }

  // Initialize the stack pointer for current process
  frame->sp = cpp;

  /*
   *  Free all the old physical memory belonging to this process,
   *  but be sure to leave the kernel stack for this process (which
   *  is also in Region 0) alone. For any PTE that is valid,
   *  free the physical memory indicated by the pfn. Set all
   *  of these PTE to invalid.
   */
  dprintf("freeing old memory in page table...", 0);
  page_table = (*pcb_p)->page_table_p;
  page_table = reset_page_table_limited(page_table);

  /*
   *  Fill in the page table with the right number of text,
   *  data+bss, and stack pages.  We set all the text pages
   *  here to be read/write, just like the data+bss and
   *  stack pages, so that we can read the text into them
   *  from the file.  We then change them read/execute.
   */

  dprintf("initializing text, data/bss and stack...", 0);
  /* First, the text pages */
  // Leave the first MEM_INVALID_PAGES num of PTEs invalid
  // kprot to READ | WRITE
  // uprot to READ | EXEC
  k = MEM_INVALID_PAGES;
  for (j = 0; j < text_npg; j++) {
    (page_table + k)->valid = PTE_VALID;
    (page_table + k)->pfn = get_free_frame();
    (page_table + k)->kprot = (PROT_READ | PROT_WRITE);
    (page_table + k)->uprot = (PROT_READ | PROT_EXEC);
    k++;
  }

  /* Then the data and bss pages */
  // krpot to READ | WRITE
  // uprot to READ | WRITE
  for (j = 0; j < data_bss_npg; j++) {
    (page_table + k)->valid = PTE_VALID;
    (page_table + k)->pfn = get_free_frame(); //DETAIL: make sure we can get a free frame, should never happen but maybe put in assert staetment?
    (page_table + k)->kprot = (PROT_READ | PROT_WRITE);
    (page_table + k)->uprot = (PROT_READ | PROT_WRITE);
    k++;
  }

  // Update heap limit
  dprintf("updating region0 heap limit...", 0);
  (*pcb_p)->brk_index = k;


  /* And finally the user stack pages */
  // k should be equivalent to USER_STACK_BASE
  // go from base to limit
  // kprot to READ | WRITE
  // uprot to READ | WRITE
  dprintf("updaing user stack pages...", 0);
  k = get_page_index(USER_STACK_LIMIT) - stack_npg;
  for (j = 0; j < stack_npg ; j++) {
    (page_table + k)->valid = PTE_VALID;
    (page_table + k)->pfn = get_free_frame();
    (page_table + k)->kprot = (PROT_READ | PROT_WRITE);
    (page_table + k)->uprot = (PROT_READ | PROT_WRITE);
    k++;
  }
  (*pcb_p)->stack_limit_index = k;

  /*
   *  All pages for the new address space are now in place.  Flush
   *  the TLB to get rid of all the old PTEs from this process, so
   *  we'll be able to do the read() into the new pages below.
   */
  WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

  /*
   *  Read the text and data from the file into memory.
   */
  dprintf("reading text and data into memory...", 0);
  if (read(fd, (void *)MEM_INVALID_SIZE, li.text_size+li.data_size)
      != li.text_size+li.data_size) {
    TracePrintf(0, "LoadProgram: couldn't read for '%s'\n", name);
    free(argbuf);
    close(fd);
    return (-2);
  }

  close(fd);			/* we've read it all now */

  /*
   *  Now set the page table entries for the program text to be readable
   *  and executable, but not writable.
   */

  // set text to READ | EXEC
  k = MEM_INVALID_PAGES;
  for (j = 0; j < text_npg; j++) {
    (page_table + k)->kprot = (PROT_READ | PROT_EXEC);
    k++;
  }

  WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

  /*
   *  Zero out the bss
   */
  memset((void *)(MEM_INVALID_SIZE + li.text_size + li.data_size),
      '\0', li.bss_size);

  /*
   *  Set the entry point in the exception frame.
   */
  // initialize pc for current process
  frame->pc = (void *)li.entry;

  /*
   *  Now, finally, build the argument list on the new stack.
   */
  *cpp++ = (char *)argcount;		/* the first value at cpp is argc */
  cp2 = argbuf;
  for (i = 0; i < argcount; i++) {      /* copy each argument and set argv */
    *cpp++ = cp;
    strcpy(cp, cp2);
    cp += strlen(cp) + 1;
    cp2 += strlen(cp2) + 1;
  }
  free(argbuf);
  *cpp++ = NULL;	/* the last argv is a NULL pointer */
  *cpp++ = NULL;	/* a NULL pointer for an empty envp */
  *cpp++ = 0;		/* and terminate the auxiliary vector */

  /*
   *  Initialize all regs[] registers for the current process to 0,
   *  initialize the PSR for the current process also to 0.  This
   *  value for the PSR will make the process run in user mode,
   *  since this PSR value of 0 does not have the PSR_MODE bit set.
   */
  // initialize regs to zero
  for (j=0; j<NUM_REGS; j++) {
    frame->regs[j] = 0;
  }
  // initialize psr to 0
  frame->psr = 0;
  (*pcb_p)->frame = frame;
  return (0);
}
