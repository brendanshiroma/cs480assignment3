/*
 * tlb.cpp - TLB simulation with approximate LRU replacement.
 */
#include "tlb.h"

TLB::TLB(unsigned int capacity)
    : capacity(capacity),
      windowSize(capacity < RECENT_WINDOW ? capacity : RECENT_WINDOW) {}

bool TLB::lookup(unsigned int vpn, unsigned int &ppn) const {
    if (capacity == 0)
        return false;
    std::unordered_map<unsigned int, unsigned int>::const_iterator it =
        entries.find(vpn);
    if (it == entries.end())
        return false;
    ppn = it->second;
    return true;
}

unsigned int TLB::oldestRecent() const {
    std::unordered_map<unsigned int, unsigned long>::const_iterator it =
        recent.begin();
    unsigned int oldestVpn = it->first;
    unsigned long oldestTime = it->second;
    for (++it; it != recent.end(); ++it) {
        if (it->second < oldestTime) {
            oldestTime = it->second;
            oldestVpn = it->first;
        }
    }
    return oldestVpn;
}

void TLB::touch(unsigned int vpn, unsigned long time) {
    if (capacity == 0)
        return;
    std::unordered_map<unsigned int, unsigned long>::iterator it =
        recent.find(vpn);
    if (it != recent.end()) {
        it->second = time;          /* already tracked: update time */
        return;
    }
    /* New page in the window: drop the oldest if the window is full. */
    if (recent.size() >= windowSize)
        recent.erase(oldestRecent());
    recent[vpn] = time;
}

void TLB::insert(unsigned int vpn, unsigned int ppn, unsigned long time) {
    if (capacity == 0)
        return;
    if (entries.size() >= capacity) {
        /* Evict the oldest tracked page from both the TLB and the window. */
        unsigned int victim = oldestRecent();
        recent.erase(victim);
        entries.erase(victim);
    }
    entries[vpn] = ppn;
    touch(vpn, time);
}
