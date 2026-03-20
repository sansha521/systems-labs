#include "oslabs.h"
/* Helper: mark a PTE as invalid */
static void invalidate_pte(struct PTE *pte) {
    pte->frame_number = -1;
    pte->is_valid = 0;
    pte->arrival_timestamp = -1;
    pte->last_access_timestamp = -1;
    pte->reference_count = 0;
}

/* FIFO page access */
int process_page_access_fifo(struct PTE page_table[TABLEMAX],
                             int *table_cnt,
                             int page_number,
                             int frame_pool[POOLMAX],
                             int *frame_cnt,
                             int current_timestamp)
{
    /* If already valid, update metadata and return frame */
    if (page_table[page_number].is_valid == 1) {
        page_table[page_number].last_access_timestamp = current_timestamp;
        page_table[page_number].reference_count += 1;
        return page_table[page_number].frame_number;
    }

    /* Page fault path: have a free frame? */
    if (*frame_cnt > 0) {
        /* pop a free frame from pool */
        int fn = frame_pool[--(*frame_cnt)];

        page_table[page_number].frame_number = fn;
        page_table[page_number].is_valid = 1;
        page_table[page_number].arrival_timestamp = current_timestamp;
        page_table[page_number].last_access_timestamp = current_timestamp;
        page_table[page_number].reference_count = 1;

        return fn;
    }

    /* No free frames: FIFO replacement -> evict smallest arrival_timestamp */
    int min_arrival_timestamp = 2147483647; /* big */
    int min_arrival_index = -1;
    for (int i = 0; i < *table_cnt; i++) {
        if (page_table[i].is_valid == 1) {
            if (page_table[i].arrival_timestamp < min_arrival_timestamp) {
                min_arrival_timestamp = page_table[i].arrival_timestamp;
                min_arrival_index = i;
            }
        }
    }

    /* min_arrival_index should be valid if table_cnt and frames consistent */
    int evicted_frame = page_table[min_arrival_index].frame_number;

    /* Evict the victim */
    invalidate_pte(&page_table[min_arrival_index]);

    /* Install new page into that frame */
    page_table[page_number].frame_number = evicted_frame;
    page_table[page_number].is_valid = 1;
    page_table[page_number].arrival_timestamp = current_timestamp;
    page_table[page_number].last_access_timestamp = current_timestamp;
    page_table[page_number].reference_count = 1;

    return evicted_frame;
}

/* Count page faults using FIFO */
int count_page_faults_fifo(struct PTE page_table[TABLEMAX],
                           int table_cnt,
                           int refrence_string[REFERENCEMAX],
                           int reference_cnt,
                           int frame_pool[POOLMAX],
                           int frame_cnt)
{
    int page_faults = 0;

    /* Initialize page table entries (all invalid) */
    for (int i = 0; i < table_cnt; i++) {
        invalidate_pte(&page_table[i]);
    }

    /* init frame pool as frames 0..frame_cnt-1 (stack style) */
    for (int i = 0; i < frame_cnt; i++) {
        frame_pool[i] = i;
    }
    int local_frame_cnt = frame_cnt;

    /* Process references */
    for (int i = 0; i < reference_cnt; i++) {
        int page = refrence_string[i];
        if (page < 0 || page >= table_cnt) {
            /* ignore out-of-range page numbers (defensive) */
            continue;
        }
        if (page_table[page].is_valid == 0) {
            page_faults++;
        }
        process_page_access_fifo(page_table, &table_cnt, page, frame_pool, &local_frame_cnt, i + 1);
    }

    return page_faults;
}

/* LRU page access */
int process_page_access_lru(struct PTE page_table[TABLEMAX],
                            int *table_cnt,
                            int page_number,
                            int frame_pool[POOLMAX],
                            int *frame_cnt,
                            int current_timestamp)
{
    /* If already valid, update metadata */
    if (page_table[page_number].is_valid == 1) {
        page_table[page_number].last_access_timestamp = current_timestamp;
        page_table[page_number].reference_count += 1;
        return page_table[page_number].frame_number;
    }

    /* If free frames exist, allocate one */
    if (*frame_cnt > 0) {
        int fn = frame_pool[--(*frame_cnt)];
        page_table[page_number].frame_number = fn;
        page_table[page_number].is_valid = 1;
        page_table[page_number].arrival_timestamp = current_timestamp;
        page_table[page_number].last_access_timestamp = current_timestamp;
        page_table[page_number].reference_count = 1;
        return fn;
    }

    /* No free frames: pick page with oldest last_access_timestamp */
    int oldest_access = 2147483647;
    int victim = -1;
    for (int i = 0; i < *table_cnt; i++) {
        if (page_table[i].is_valid == 1) {
            if (page_table[i].last_access_timestamp < oldest_access) {
                oldest_access = page_table[i].last_access_timestamp;
                victim = i;
            }
        }
    }

    int evicted_frame = page_table[victim].frame_number;
    invalidate_pte(&page_table[victim]);

    page_table[page_number].frame_number = evicted_frame;
    page_table[page_number].is_valid = 1;
    page_table[page_number].arrival_timestamp = current_timestamp;
    page_table[page_number].last_access_timestamp = current_timestamp;
    page_table[page_number].reference_count = 1;

    return evicted_frame;
}

/* Count page faults using LRU */
int count_page_faults_lru(struct PTE page_table[TABLEMAX],
                          int table_cnt,
                          int refrence_string[REFERENCEMAX],
                          int reference_cnt,
                          int frame_pool[POOLMAX],
                          int frame_cnt)
{
    int page_faults = 0;

    for (int i = 0; i < table_cnt; i++) {
        invalidate_pte(&page_table[i]);
    }

    for (int i = 0; i < frame_cnt; i++) {
        frame_pool[i] = i;
    }
    int local_frame_cnt = frame_cnt;

    for (int i = 0; i < reference_cnt; i++) {
        int page = refrence_string[i];
        if (page < 0 || page >= table_cnt) continue;

        if (page_table[page].is_valid == 0) page_faults++;

        process_page_access_lru(page_table, &table_cnt, page, frame_pool, &local_frame_cnt, i + 1);
    }

    return page_faults;
}

/* LFU page access */
int process_page_access_lfu(struct PTE page_table[TABLEMAX],
                            int *table_cnt,
                            int page_number,
                            int frame_pool[POOLMAX],
                            int *frame_cnt,
                            int current_timestamp)
{
    /* If already valid, update metadata */
    if (page_table[page_number].is_valid == 1) {
        page_table[page_number].last_access_timestamp = current_timestamp;
        page_table[page_number].reference_count += 1;
        return page_table[page_number].frame_number;
    }

    /* If free frames exist, allocate one */
    if (*frame_cnt > 0) {
        int fn = frame_pool[--(*frame_cnt)];
        page_table[page_number].frame_number = fn;
        page_table[page_number].is_valid = 1;
        page_table[page_number].arrival_timestamp = current_timestamp;
        page_table[page_number].last_access_timestamp = current_timestamp;
        page_table[page_number].reference_count = 1;
        return fn;
    }

    /* No free frames: pick page with smallest reference_count (LFU).
       Tie-breaker: oldest arrival_timestamp (or oldest last_access). */
    int min_refs = 2147483647;
    int victim = -1;
    for (int i = 0; i < *table_cnt; i++) {
        if (page_table[i].is_valid == 1) {
            if (page_table[i].reference_count < min_refs) {
                min_refs = page_table[i].reference_count;
                victim = i;
            } else if (page_table[i].reference_count == min_refs) {
                /* tie-breaker: evict oldest arrival */
                if (page_table[i].arrival_timestamp < page_table[victim].arrival_timestamp) {
                    victim = i;
                }
            }
        }
    }

    int evicted_frame = page_table[victim].frame_number;
    invalidate_pte(&page_table[victim]);

    page_table[page_number].frame_number = evicted_frame;
    page_table[page_number].is_valid = 1;
    page_table[page_number].arrival_timestamp = current_timestamp;
    page_table[page_number].last_access_timestamp = current_timestamp;
    page_table[page_number].reference_count = 1;

    return evicted_frame;
}

/* Count page faults using LFU */
int count_page_faults_lfu(struct PTE page_table[TABLEMAX],
                          int table_cnt,
                          int refrence_string[REFERENCEMAX],
                          int reference_cnt,
                          int frame_pool[POOLMAX],
                          int frame_cnt)
{
    int page_faults = 0;

    for (int i = 0; i < table_cnt; i++) {
        invalidate_pte(&page_table[i]);
    }

    for (int i = 0; i < frame_cnt; i++) {
        frame_pool[i] = i;
    }
    int local_frame_cnt = frame_cnt;

    for (int i = 0; i < reference_cnt; i++) {
        int page = refrence_string[i];
        if (page < 0 || page >= table_cnt) continue;

        if (page_table[page].is_valid == 0) page_faults++;

        process_page_access_lfu(page_table, &table_cnt, page, frame_pool, &local_frame_cnt, i + 1);
    }

    return page_faults;
}
