/*
 * gpummu.cpp - main flow: argument parsing, trace processing, output.
 *
 * Usage: gpummu [-n N] [-c N] [-o mode] tracefile bits0 [bits1 ...]
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <unistd.h>

#include "log.h"
#include "tracereader.h"
#include "pagetable.h"
#include "tlb.h"

#define MAX_TOTAL_BITS 28
#define BADFLAG 1

enum OutputMode {
    MODE_BITMASKS,
    MODE_VA2PA,
    MODE_VA2PA_TLB_PTWALK,
    MODE_VPN2PPN,
    MODE_OFFSET,
    MODE_SUMMARY
};

static void usage(const char *prog) {
    fprintf(stderr,
            "Usage: %s [-n N] [-c N] [-o mode] tracefile bits0 [bits1 ...]\n"
            "  modes: bitmasks, va2pa, va2pa_tlb_ptwalk, vpn2ppn, offset, "
            "summary\n",
            prog);
}

/* Parse a whole string as a base-10 integer; false if it isn't one. */
static bool parseInt(const char *s, long &value) {
    if (s == NULL || *s == '\0')
        return false;
    char *end;
    value = strtol(s, &end, 10);
    return *end == '\0';
}

int main(int argc, char **argv) {
    long numAccesses = -1;          /* -1 = process the whole trace */
    long tlbCapacity = 0;           /* 0 = no TLB */
    OutputMode mode = MODE_SUMMARY;

    int option;
    while ((option = getopt(argc, argv, "n:c:o:")) != -1) {
        switch (option) {
        case 'n':
            if (!parseInt(optarg, numAccesses) || numAccesses <= 0) {
                printf("Number of KV-cache accesses must be a number, "
                       "greater than 0\n");
                exit(BADFLAG);
            }
            break;
        case 'c':
            if (!parseInt(optarg, tlbCapacity) || tlbCapacity < 0) {
                printf("TLB capacity must be a number, greater than or "
                       "equal to 0\n");
                exit(BADFLAG);
            }
            break;
        case 'o':
            if (strcmp(optarg, "bitmasks") == 0)
                mode = MODE_BITMASKS;
            else if (strcmp(optarg, "va2pa") == 0)
                mode = MODE_VA2PA;
            else if (strcmp(optarg, "va2pa_tlb_ptwalk") == 0)
                mode = MODE_VA2PA_TLB_PTWALK;
            else if (strcmp(optarg, "vpn2ppn") == 0)
                mode = MODE_VPN2PPN;
            else if (strcmp(optarg, "offset") == 0)
                mode = MODE_OFFSET;
            else if (strcmp(optarg, "summary") == 0)
                mode = MODE_SUMMARY;
            else {
                usage(argv[0]);
                exit(BADFLAG);
            }
            break;
        default:
            usage(argv[0]);
            exit(BADFLAG);
        }
    }

    /* Mandatory: trace file, then one bit count per level. */
    int idx = optind;
    if (argc - idx < 2) {
        usage(argv[0]);
        exit(BADFLAG);
    }
    const char *tracePath = argv[idx];

    unsigned int levelCount = argc - idx - 1;
    unsigned int *bits = new unsigned int[levelCount];
    long totalBits = 0;
    for (unsigned int lvl = 0; lvl < levelCount; lvl++) {
        long b;
        if (!parseInt(argv[idx + 1 + lvl], b) || b < 1) {
            printf("Level %u page table must be at least 1 bit\n", lvl);
            exit(BADFLAG);
        }
        if (b > MAX_TOTAL_BITS) {           /* avoid overflow in the sum */
            printf("Too many bits used in page tables\n");
            exit(BADFLAG);
        }
        bits[lvl] = (unsigned int)b;
        totalBits += b;
    }
    if (totalBits > MAX_TOTAL_BITS) {
        printf("Too many bits used in page tables\n");
        exit(BADFLAG);
    }

    FILE *traceFile = fopen(tracePath, "rb");
    if (traceFile == NULL) {
        printf("Unable to open <<%s>>\n", tracePath);
        exit(BADFLAG);
    }

    PageTable *pageTable = createPageTable(levelCount, bits);
    delete[] bits;

    if (mode == MODE_BITMASKS) {
        log_bitmasks(levelCount, pageTable->bitmask);
        fclose(traceFile);
        destroyPageTable(pageTable);
        return 0;
    }

    TLB tlb((unsigned int)tlbCapacity);

    unsigned int tlbHits = 0;
    unsigned int pageTableHits = 0;
    unsigned int nextPPN = 0;           /* sequential physical pages */
    unsigned long addressCount = 0;     /* also serves as virtual time */
    unsigned int *levelIndices = new unsigned int[levelCount];

    p2AddrTr trace;
    while ((numAccesses < 0 || (long)addressCount < numAccesses) &&
           NextAddress(traceFile, &trace)) {
        unsigned int va = trace.addr;
        unsigned int vpn = getVPN(pageTable, va);
        unsigned int offset = getOffset(pageTable, va);

        unsigned int ppn;
        bool tlbHit = false;
        bool ptHit = false;

        if (tlb.lookup(vpn, ppn)) {
            /* TLB hit: just refresh the page's access time. */
            tlbHit = true;
            tlbHits++;
            tlb.touch(vpn, addressCount);
        } else {
            /* TLB miss: walk the page table, demand-page if needed. */
            Map *m = lookup_vpn2ppn(pageTable, va);
            if (m != NULL) {
                ptHit = true;
                pageTableHits++;
                ppn = m->ppn;
            } else {
                ppn = nextPPN++;
                insert_vpn2ppn(pageTable, va, ppn);
            }
            tlb.insert(vpn, ppn, addressCount);
        }

        unsigned int pa = (ppn << pageTable->offsetBits) | offset;

        switch (mode) {
        case MODE_VA2PA:
            log_virtualAddr2physicalAddr(va, pa);
            break;
        case MODE_VA2PA_TLB_PTWALK:
            log_va2pa_TLB_PTwalk(va, pa, tlbHit, ptHit);
            break;
        case MODE_VPN2PPN:
            getLevelIndices(pageTable, va, levelIndices);
            log_pagemapping(levelCount, levelIndices, ppn);
            break;
        case MODE_OFFSET:
            hexnum(offset);
            break;
        default:
            break;
        }

        addressCount++;
    }

    if (mode == MODE_SUMMARY) {
        log_summary(1u << pageTable->offsetBits, tlbHits, pageTableHits,
                    (unsigned int)addressCount, nextPPN,
                    countPageTableEntries(pageTable));
    }

    delete[] levelIndices;
    fclose(traceFile);
    destroyPageTable(pageTable);
    return 0;
}
