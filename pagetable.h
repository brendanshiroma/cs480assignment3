#ifndef PAGETABLE_H
#define PAGETABLE_H

/*
 * Multi-level page table (tree) for gpummu.
 *
 * Implementation requirements honored:
 *   - Level is implemented as a tree node.
 *   - Interior nodes store their children in a Level** array (no STL).
 */

struct PageTable;

/* Map: one page-table entry at the leaf level (VPN -> PPN). */
struct Map {
    bool valid;          /* true once a physical page has been assigned */
    unsigned int ppn;    /* physical page number */
};

/* Level (PageNode): one node of the page tree. */
struct Level {
    unsigned int depth;      /* 0 = root */
    PageTable *pageTable;    /* back-pointer to the table descriptor */
    Level **nextLevel;       /* interior nodes: array of child pointers */
    Map *map;                /* leaf nodes: array of mappings */
};

/* PageTable: top-level descriptor of the N-level table. */
struct PageTable {
    unsigned int levelCount;
    unsigned int *bitsPerLevel;   /* bits used by each level        */
    unsigned int *bitmask;        /* mask to extract each level idx */
    unsigned int *shift;          /* right shift for each level idx */
    unsigned int *entryCount;     /* 2^bits for each level          */

    unsigned int vpnBits;         /* sum of all level bits */
    unsigned int offsetBits;      /* 32 - vpnBits          */
    unsigned int vpnMask;         /* mask of the full VPN  */
    unsigned int offsetMask;      /* mask of the offset    */

    Level *root;                  /* level 0 node */
};

/* ---- mandatory interfaces ---- */
unsigned int extractPageNumberFromAddress(unsigned int virtualAddress,
                                          unsigned int mask,
                                          unsigned int shift);
Map *lookup_vpn2ppn(PageTable *pageTable, unsigned int virtualAddress);
bool insert_vpn2ppn(PageTable *pageTable, unsigned int virtualAddress,
                    unsigned int ppn);

/* ---- helpers ---- */
PageTable *createPageTable(unsigned int levelCount, const unsigned int *bits);
void destroyPageTable(PageTable *pageTable);

unsigned int getVPN(PageTable *pageTable, unsigned int virtualAddress);
unsigned int getOffset(PageTable *pageTable, unsigned int virtualAddress);
/* Fills indices[0..levelCount-1] with the per-level page numbers. */
void getLevelIndices(PageTable *pageTable, unsigned int virtualAddress,
                     unsigned int *indices);
/* Total page table entries across all allocated nodes. */
unsigned long countPageTableEntries(PageTable *pageTable);

#endif
