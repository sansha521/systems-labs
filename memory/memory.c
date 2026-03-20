#include "oslabs.h"
 
struct MEMORY_BLOCK build_mblock(int start_address, int end_address, int segment_size, int process_id)
{
    struct MEMORY_BLOCK block;
    block.start_address = start_address;
    block.end_address = end_address;
    block.segment_size = segment_size;
    block.process_id = process_id;
    return block;
}

struct MEMORY_BLOCK best_fit_allocate(int request_size, struct MEMORY_BLOCK memory_map[MAPMAX], int *map_cnt, int process_id)
{
    int best_index = -1;
    int best_size = 999999;

    // Find smallest free block that fits
    for (int i = 0; i < *map_cnt; i++) {
        if (memory_map[i].process_id == 0 && memory_map[i].segment_size >= request_size) {
            if (memory_map[i].segment_size < best_size) {
                best_size = memory_map[i].segment_size;
                best_index = i;
            }
        }
    }

    // No fit found
    if (best_index == -1)
        return build_mblock(0, 0, 0, 0);

    struct MEMORY_BLOCK allocated;
    allocated.start_address = memory_map[best_index].start_address;
    allocated.end_address = allocated.start_address + request_size - 1;
    allocated.segment_size = request_size;
    allocated.process_id = process_id;

    // Adjust free block
    if (memory_map[best_index].segment_size == request_size) {
        memory_map[best_index] = allocated;
    } else {
        memory_map[best_index].start_address += request_size;
        memory_map[best_index].segment_size -= request_size;

        // Insert allocated block before it
        for (int j = *map_cnt; j > best_index; j--)
            memory_map[j] = memory_map[j - 1];

        memory_map[best_index] = allocated;
        (*map_cnt)++;
    }

    return allocated;
}

struct MEMORY_BLOCK first_fit_allocate(int request_size, struct MEMORY_BLOCK memory_map[MAPMAX], int *map_cnt, int process_id)
{
    for (int i = 0; i < *map_cnt; i++) {
        if (memory_map[i].process_id == 0 && memory_map[i].segment_size >= request_size) {
            struct MEMORY_BLOCK allocated = build_mblock(
                memory_map[i].start_address,
                memory_map[i].start_address + request_size - 1,
                request_size,
                process_id
            );

            if (memory_map[i].segment_size == request_size) {
                memory_map[i] = allocated;
            } else {
                memory_map[i].start_address += request_size;
                memory_map[i].segment_size -= request_size;

                for (int j = *map_cnt; j > i; j--)
                    memory_map[j] = memory_map[j - 1];

                memory_map[i] = allocated;
                (*map_cnt)++;
            }

            return allocated;
        }
    }
    return build_mblock(0, 0, 0, 0); // no fit
}

struct MEMORY_BLOCK worst_fit_allocate(int request_size, struct MEMORY_BLOCK memory_map[MAPMAX], int *map_cnt, int process_id)
{
    int worst_index = -1;
    int worst_size = -1;

    for (int i = 0; i < *map_cnt; i++) {
        if (memory_map[i].process_id == 0 && memory_map[i].segment_size >= request_size) {
            if (memory_map[i].segment_size > worst_size) {
                worst_size = memory_map[i].segment_size;
                worst_index = i;
            }
        }
    }

    if (worst_index == -1)
        return build_mblock(0, 0, 0, 0);

    struct MEMORY_BLOCK allocated = build_mblock(
        memory_map[worst_index].start_address,
        memory_map[worst_index].start_address + request_size - 1,
        request_size,
        process_id
    );

    if (memory_map[worst_index].segment_size == request_size) {
        memory_map[worst_index] = allocated;
    } else {
        memory_map[worst_index].start_address += request_size;
        memory_map[worst_index].segment_size -= request_size;

        for (int j = *map_cnt; j > worst_index; j--)
            memory_map[j] = memory_map[j - 1];

        memory_map[worst_index] = allocated;
        (*map_cnt)++;
    }

    return allocated;
}

struct MEMORY_BLOCK next_fit_allocate(int request_size, struct MEMORY_BLOCK memory_map[MAPMAX], int *map_cnt, int process_id, int last_address)
{
    int start_index = 0;
    for (int i = 0; i < *map_cnt; i++) {
        if (memory_map[i].start_address <= last_address && memory_map[i].end_address >= last_address) {
            start_index = i;
            break;
        }
    }

    int i = start_index;
    do {
        if (memory_map[i].process_id == 0 && memory_map[i].segment_size >= request_size) {
            struct MEMORY_BLOCK allocated = build_mblock(
                memory_map[i].start_address,
                memory_map[i].start_address + request_size - 1,
                request_size,
                process_id
            );

            if (memory_map[i].segment_size == request_size) {
                memory_map[i] = allocated;
            } else {
                memory_map[i].start_address += request_size;
                memory_map[i].segment_size -= request_size;

                for (int j = *map_cnt; j > i; j--)
                    memory_map[j] = memory_map[j - 1];

                memory_map[i] = allocated;
                (*map_cnt)++;
            }

            return allocated;
        }

        i = (i + 1) % (*map_cnt);
    } while (i != start_index);

    return build_mblock(0, 0, 0, 0);
}

void release_memory(struct MEMORY_BLOCK freed_block, struct MEMORY_BLOCK memory_map[MAPMAX], int *map_cnt)
{
    int index = -1;
    for (int i = 0; i < *map_cnt; i++) {
        if (memory_map[i].start_address == freed_block.start_address) {
            memory_map[i].process_id = 0;
            index = i;
            break;
        }
    }

    // Merge with previous
    if (index > 0 && memory_map[index - 1].process_id == 0) {
        memory_map[index - 1].end_address = memory_map[index].end_address;
        memory_map[index - 1].segment_size += memory_map[index].segment_size;
        for (int j = index; j < *map_cnt - 1; j++)
            memory_map[j] = memory_map[j + 1];
        (*map_cnt)--;
        index--;
    }

    // Merge with next
    if (index < *map_cnt - 1 && memory_map[index + 1].process_id == 0) {
        memory_map[index].end_address = memory_map[index + 1].end_address;
        memory_map[index].segment_size += memory_map[index + 1].segment_size;
        for (int j = index + 1; j < *map_cnt - 1; j++)
            memory_map[j] = memory_map[j + 1];
        (*map_cnt)--;
    }
}

