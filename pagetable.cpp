/*
 * pagetable.cpp - multi-level page tree for gpummu.
 *
 * Interior nodes keep children in a Level** array; leaf nodes keep a
 * Map array. No STL collections are used in this file.
 */
#include <cstdlib>
#include "pagetable.h"

/* Apply the mask, then shift right to get a page number / index. */
unsigned int extractPageNumberFromAddress(unsigned int virtualAddress,
                                          unsigned int mask,
                                          unsigned int shift) {
    return (virtualAddress & mask) >> shift;
}

/* Allocate one node at the given depth with all entries empty. */
static Level *createLevel(PageTable *pageTable, unsigned int depth) {
    Level *level = new Level;
    level->depth = depth;
    level->pageTable = pageTable;
    level->nextLevel = NULL;
    level->map = NULL;

    unsigned int count = pageTable->entryCount[depth];
    if (depth == pageTable->levelCount - 1) {
        /* leaf: array of mappings, all invalid */
        level->map = new Map[count];
        for (unsigned int i = 0; i < count; i++) {
            level->map[i].valid = false;
            level->map[i].ppn = 0;
        }
    } else {
        /* interior: array of child pointers, all NULL */
        level->nextLevel = new Level *[count];
        for (unsigned int i = 0; i < count; i++)
            level->nextLevel[i] = NULL;
    }
    return level;
}

PageTable *createPageTable(unsigned int levelCount, const unsigned int *bits) {
    PageTable *pt = new PageTable;
    pt->levelCount = levelCount;
    pt->bitsPerLevel = new unsigned int[levelCount];
    pt->bitmask = new unsigned int[levelCount];
    pt->shift = new unsigned int[levelCount];
    pt->entryCount = new unsigned int[levelCount];

    /* Levels are laid out from the most significant bit downward. */
    unsigned int used = 0;
    for (unsigned int i = 0; i < levelCount; i++) {
        pt->bitsPerLevel[i] = bits[i];
        used += bits[i];
        pt->shift[i] = 32 - used;
        pt->entryCount[i] = 1u << bits[i];
        pt->bitmask[i] = ((1u << bits[i]) - 1) << pt->shift[i];
    }

    pt->vpnBits = used;
    pt->offsetBits = 32 - used;                   /* used <= 28, so >= 4 */
    pt->offsetMask = (1u << pt->offsetBits) - 1;
    pt->vpnMask = ~pt->offsetMask;

    pt->root = createLevel(pt, 0);
    return pt;
}

unsigned int getVPN(PageTable *pt, unsigned int virtualAddress) {
    return extractPageNumberFromAddress(virtualAddress, pt->vpnMask,
                                        pt->offsetBits);
}

unsigned int getOffset(PageTable *pt, unsigned int virtualAddress) {
    return extractPageNumberFromAddress(virtualAddress, pt->offsetMask, 0);
}

void getLevelIndices(PageTable *pt, unsigned int virtualAddress,
                     unsigned int *indices) {
    for (unsigned int i = 0; i < pt->levelCount; i++)
        indices[i] = extractPageNumberFromAddress(virtualAddress,
                                                  pt->bitmask[i],
                                                  pt->shift[i]);
}

/* Walk the tree; NULL if any node on the path is missing or the
 * leaf entry has not been mapped yet. */
Map *lookup_vpn2ppn(PageTable *pt, unsigned int virtualAddress) {
    Level *level = pt->root;
    for (unsigned int d = 0; d < pt->levelCount; d++) {
        unsigned int idx = extractPageNumberFromAddress(
            virtualAddress, pt->bitmask[d], pt->shift[d]);
        if (d == pt->levelCount - 1) {
            Map *m = &level->map[idx];
            return m->valid ? m : NULL;
        }
        level = level->nextLevel[idx];
        if (level == NULL)
            return NULL;
    }
    return NULL; /* not reached */
}

/* Insert VPN -> ppn, creating only the nodes that don't exist yet. */
bool insert_vpn2ppn(PageTable *pt, unsigned int virtualAddress,
                    unsigned int ppn) {
    Level *level = pt->root;
    for (unsigned int d = 0; d < pt->levelCount; d++) {
        unsigned int idx = extractPageNumberFromAddress(
            virtualAddress, pt->bitmask[d], pt->shift[d]);
        if (d == pt->levelCount - 1) {
            level->map[idx].valid = true;
            level->map[idx].ppn = ppn;
            return true;
        }
        if (level->nextLevel[idx] == NULL)
            level->nextLevel[idx] = createLevel(pt, d + 1);
        level = level->nextLevel[idx];
    }
    return false; /* not reached */
}

/* Count this node's entries plus those of every non-null child. */
static unsigned long countEntries(Level *level) {
    PageTable *pt = level->pageTable;
    unsigned long total = pt->entryCount[level->depth];
    if (level->nextLevel != NULL) {
        for (unsigned int i = 0; i < pt->entryCount[level->depth]; i++)
            if (level->nextLevel[i] != NULL)
                total += countEntries(level->nextLevel[i]);
    }
    return total;
}

unsigned long countPageTableEntries(PageTable *pt) {
    return countEntries(pt->root);
}

static void destroyLevel(Level *level) {
    PageTable *pt = level->pageTable;
    if (level->nextLevel != NULL) {
        for (unsigned int i = 0; i < pt->entryCount[level->depth]; i++)
            if (level->nextLevel[i] != NULL)
                destroyLevel(level->nextLevel[i]);
        delete[] level->nextLevel;
    }
    delete[] level->map;
    delete level;
}

void destroyPageTable(PageTable *pt) {
    if (pt == NULL)
        return;
    destroyLevel(pt->root);
    delete[] pt->bitsPerLevel;
    delete[] pt->bitmask;
    delete[] pt->shift;
    delete[] pt->entryCount;
    delete pt;
}
