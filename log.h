#ifndef LOG_H
#define LOG_H

/*
 * Compilation notes:
 * C compilers:
 *    uses bool, must compile with -std=c99 or later (bool was introduced
 *    in the 1999 C standard.
 *
 * C++ compilers
 *    uses uint32_t, unsigned 32 bit integer type, introduced in C++11,
 *    The defaults in the g++ compiler on edoras should be fine with this
 */

/* C and C++ define some of their types in different places.
 * Check and see if we are using C or C++ and include appropriately
 * so that this will compile under C and C++
 */
#ifdef __cplusplus
/* C++ includes */
#include <stdint.h>
#else
/* C includes */
#include <inttypes.h>
#include <stdbool.h>
#endif 

/*
 * structure that can be used to maintain which output types are enabled.
 * Note that this uses the bool keyword. 
 *
 * If compiled with a C compiler, make sure that the C99 dialect or later is used.
 * (-std=c99 with a GNU C compiler)
 */
typedef struct {
  bool bitmasks;  /* display bitmaks */
  bool va2pa;  /* show virtual to physical address translation */
  bool va2pa_tlb_ptwalk; /* show virtual to physical address translation 
                            using TLB then Pagetable walk (if TLB miss)*/
  bool vpn2ppn;  /* show mapping between page numbers and physical page */
  bool offset; /* show the offset of each address */
  bool summary; /* summary statistics */
} OutputOptionsType;

/**
 * @brief Print out a number in hex, one per line
 * @param number 
 */
void hexnum(uint32_t number);

/**
 * @brief Print out bitmasks for all page table levels.
 * 
 * @param levels - Number of levels
 * @param masks - Pointer to array of bitmasks
 */
void log_bitmasks(int levels, uint32_t *masks);

/**
 * @brief Given a pair of numbers, output a line: 
 *        src -> dest  
 * Example usages:
 * Map between virtual and physical addresses:  log_virtualAddr2physicalAddr(va, pa)
 * (the vpn2ppn mode does NOT use this function: it uses log_pagemapping)
 * 
 * @param src 
 * @param dest 
 */
void log_virtualAddr2physicalAddr(uint32_t src, uint32_t dest);

/**
 * @brief Given a pair of addresses, hit or miss along the tlb 
 *        then pagetable walk, output a line: 
 *          src -> dest, tlb hit 
 *          src -> dest, tlb miss, pagetable hit
 *          src -> dest, tlb miss, pagetable miss  
 * Example usages:
 * Report translation/mapping from logical to physical address: 
 *        e.g., log_va2pa_TLB_PTwalk(va, pa, false, true)
 *              tlb miss, pagetable hit
 * Report translation/mapping from virtual page number to physical page: 
 *        e.g., log_va2pa_TLB_PTwalk(vpn, ppn, false, true)
 *              tlb miss, pagetable hit
 * 
 * @param src 
 * @param dest 
 * @param tlbhit 
 * @param pthit 
 */
void log_va2pa_TLB_PTwalk(uint32_t src, uint32_t dest, bool tlbhit, bool pthit);

/**
 * @brief Write out page numbers and physical page number
 * 
 * @param levels - specified number of levels in page table
 * @param pages - pages[idx] is the page number associated with 
 *	              level idx (0 < idx < levels)
 * @param ppn - page is mapped to specified physical page number
 */
void log_pagemapping(int levels, uint32_t *pages, uint32_t ppn);

/**
 * @brief Write out a message that indicates summary information for the page table.
 * 
 * @param page_size - Number of bytes per page
 * @param tlbHits - Number of vpn->ppn mappings found in the TLB
 * @param pageTableHits - Number of times a page was mapped
 * @param addresses - Number of addresses processed
 * @param physPages_mapped - Number of physical pages mapped
 * @param pgtableEntries - Total number of page table entries across all levels.
 */
void log_summary(unsigned int page_size, 
                    unsigned int tlbHits,
                    unsigned int pageTableHits, 
                    unsigned int addresses, unsigned int physPages_mapped,
                    unsigned long int pgtableEntries);


#endif
