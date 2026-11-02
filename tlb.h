#ifndef TLB_H
#define TLB_H

#include <unordered_map>

/*
 * TLB caching VPN -> PPN mappings, with approximate LRU replacement.
 *
 * Recency is tracked for at most RECENT_WINDOW (8) distinct pages, or
 * fewer when the TLB capacity is smaller. Time is virtual: the index of
 * the address being processed.
 */
class TLB {
public:
    static const unsigned int RECENT_WINDOW = 8;

    explicit TLB(unsigned int capacity);

    bool enabled() const { return capacity > 0; }

    /* Returns true and sets ppn on a TLB hit. Does not touch recency. */
    bool lookup(unsigned int vpn, unsigned int &ppn) const;

    /* Insert a mapping after a TLB miss. If the TLB is full, the oldest
     * tracked page is first evicted from both the TLB and the recent
     * pages. Records the access in the recent pages. */
    void insert(unsigned int vpn, unsigned int ppn, unsigned long time);

    /* Record an access (used on a TLB hit). */
    void touch(unsigned int vpn, unsigned long time);

private:
    unsigned int capacity;
    unsigned int windowSize;   /* min(8, capacity) */
    std::unordered_map<unsigned int, unsigned int> entries;     /* vpn->ppn  */
    std::unordered_map<unsigned int, unsigned long> recent;     /* vpn->time */

    /* VPN with the smallest last access time among recent pages. */
    unsigned int oldestRecent() const;
};

#endif
