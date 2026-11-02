#include <stdio.h>
#include "log.h"

/* Handle C++ namespaces, ignore if compiled in C 
 * C++ usually uses this #define to declare the C++ standard.
 * It will not be defined if a C compiler is used.
 */
#ifdef __cplusplus
using namespace std;
#endif

/**
 * @brief Print out a number in hex, one per line
 * @param number 
 */
void hexnum(uint32_t number) {
  printf("%08X\n", number);
  fflush(stdout);
}

/**
 * @brief Print out bitmasks for all page table levels.
 * 
 * @param levels - Number of levels
 * @param masks - Pointer to array of bitmasks
 */
void log_bitmasks(int levels, uint32_t *masks) {
  printf("Bitmasks\n");
  for (int idx = 0; idx < levels; idx++) 
    /* show mask entry and move to next */
    printf("level %d mask %08X\n", idx, masks[idx]);

  fflush(stdout);
}

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
void log_virtualAddr2physicalAddr(uint32_t src, uint32_t dest) {
  fprintf(stdout, "%08X -> %08X\n", src, dest);
  fflush(stdout);
}

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
void log_va2pa_TLB_PTwalk(uint32_t src, uint32_t dest, bool tlbhit, bool pthit) {
  
  fprintf(stdout, "%08X -> %08X, ", src, dest);

  if (tlbhit)
    fprintf(stdout, "tlb hit\n");
  else {
    fprintf(stdout, "tlb miss, pagetable %s\n", pthit ? "hit" : "miss");
  } 

  fflush(stdout);
}

/**
 * @brief Write out page numbers and physical page number
 * 
 * @param levels - specified number of levels in page table
 * @param pages - pages[idx] is the page number associated with 
 *	              level idx (0 < idx < levels)
 * @param ppn - page is mapped to specified physical page number
 */
void log_pagemapping(int levels, uint32_t *pages, uint32_t ppn) {
  /* output pages */
  for (int idx=0; idx < levels; idx++)
    printf("%X ", pages[idx]);
  /* output physical page */
  printf("-> %X\n", ppn);

  fflush(stdout);
}

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
		    unsigned long int pgtableEntries) {
  unsigned int misses;
  double hit_percent;

  printf("Page size: %d bytes\n", page_size);
  /* Compute misses (page faults) and hit percentage */
  int totalhits = tlbHits + pageTableHits;
  misses = addresses - totalhits;
  hit_percent = (double) (totalhits) / (double) addresses * 100.0;
  printf("KV-cache accesses processed: %d\n", addresses);
  printf("TLB hits: %d, Page table hits: %d, Total hits: %d, Misses: %d\n", 
         tlbHits, pageTableHits, totalhits, misses);
  printf("Total hit percentage: %.2f%%, miss percentage: %.2f%%\n", 
         hit_percent, 100 - hit_percent);
  printf("Physical pages mapped: %d\n", physPages_mapped);
  printf("Number of page table entries: %ld\n", pgtableEntries);

  fflush(stdout);
}

