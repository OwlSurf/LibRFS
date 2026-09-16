/**
 * \file ring_file_system.c
 * \brief Ring buffer over external flash: read, write, erase-ahead, recover.
 * \author Roman Garanin
 */

#include "ring_file_system.h"

#define SLOT_ERASED_BYTE   0xFFu
#define SLOT_STATUS_UNREAD 0xFFu
#define SLOT_STATUS_READ   0x00u
#define EMPTY_CHUNK_SIZE   32u

struct ext_memory_s {
    uint16_t sector_size;
    uint16_t sector_size_in_slots;
    uint16_t slot_size;
    uint32_t sectors;
    uint32_t max_slots;
    uint32_t max_data_slots;
    uint32_t slot_count;
    uint32_t slot_rec_count;
    uint32_t slot_windex;
    uint32_t slot_rindex;
    uint32_t slot_rec_index;
    uint32_t start_address;
    em_sector_erase_fn p_sector_erase;
    em_read_fn p_read;
    em_write_fn p_write;
};

static uint32_t slot_address(const struct ext_memory_s *em, uint32_t slot_index)
{
    return em->start_address + slot_index * (uint32_t)em->slot_size;
}

static uint32_t next_slot_index(const struct ext_memory_s *em, uint32_t slot_index)
{
    slot_index++;
    if (slot_index >= em->max_slots) {
        slot_index = 0;
    }
    return slot_index;
}

static uint32_t prev_slot_index(const struct ext_memory_s *em, uint32_t slot_index)
{
    if (slot_index == 0) {
        return em->max_slots - 1u;
    }
    return slot_index - 1u;
}

static uint32_t ring_distance(uint32_t from, uint32_t to, uint32_t max_slots)
{
    if (to >= from) {
        return to - from;
    }
    return max_slots - from + to;
}

static uint8_t slot_status_byte(const struct ext_memory_s *em, uint32_t slot_index)
{
    uint8_t status = 0;
    em->p_read(slot_address(em, slot_index) + (uint32_t)em->slot_size - 1u, &status, 1);
    return status;
}

static bool slot_is_empty(const struct ext_memory_s *em, uint32_t slot_index)
{
    uint8_t chunk[EMPTY_CHUNK_SIZE];
    uint32_t address = slot_address(em, slot_index);
    uint16_t remaining = em->slot_size;

    while (remaining > 0) {
        uint16_t n = remaining;
        if (n > EMPTY_CHUNK_SIZE) {
            n = EMPTY_CHUNK_SIZE;
        }
        em->p_read(address, chunk, n);
        for (uint16_t i = 0; i < n; i++) {
            if (chunk[i] != SLOT_ERASED_BYTE) {
                return false;
            }
        }
        address += n;
        remaining = (uint16_t)(remaining - n);
    }
    return true;
}

static bool slot_has_data(const struct ext_memory_s *em, uint32_t slot_index)
{
    return !slot_is_empty(em, slot_index);
}

static bool slot_is_unread(const struct ext_memory_s *em, uint32_t slot_index)
{
    if (!slot_has_data(em, slot_index)) {
        return false;
    }
    return slot_status_byte(em, slot_index) == SLOT_STATUS_UNREAD;
}

static void mark_slot_read(struct ext_memory_s *em, uint32_t slot_index)
{
    uint8_t mark = SLOT_STATUS_READ;
    em->p_write(slot_address(em, slot_index) + (uint32_t)em->slot_size - 1u, &mark, 1);
}

static bool slot_in_window(uint32_t slot, uint32_t from, uint32_t to, uint32_t max_slots)
{
    (void)max_slots;
    if (from < to) {
        return slot >= from && slot < to;
    }
    if (from > to) {
        return slot >= from || slot < to;
    }
    return false;
}

static bool find_write_index(const struct ext_memory_s *em, uint32_t *out_windex)
{
    bool saw_data = false;
    bool found_transition = false;
    uint32_t windex = 0;

    for (uint32_t slot = 0; slot < em->max_slots; slot++) {
        uint32_t nxt = next_slot_index(em, slot);
        bool data_here = slot_has_data(em, slot);
        if (data_here) {
            saw_data = true;
            if (slot_is_empty(em, nxt)) {
                windex = nxt;
                found_transition = true;
            }
        }
    }

    if (!saw_data) {
        *out_windex = 0;
        return true;
    }

    if (found_transition) {
        *out_windex = windex;
        return true;
    }

    return false;
}

static uint32_t find_recovery_index(const struct ext_memory_s *em, uint32_t windex)
{
    uint32_t slot = windex;

    for (uint32_t checked = 0; checked < em->max_slots; checked++) {
        if (slot_has_data(em, slot)) {
            return slot;
        }
        slot = next_slot_index(em, slot);
    }

    return windex;
}

static uint32_t find_read_index(const struct ext_memory_s *em, uint32_t rec_index, uint32_t windex)
{
    uint32_t slot = rec_index;

    while (slot != windex) {
        if (slot_is_unread(em, slot)) {
            return slot;
        }
        slot = next_slot_index(em, slot);
    }

    return windex;
}

static uint32_t count_unread_slots(const struct ext_memory_s *em, uint32_t from, uint32_t to)
{
    uint32_t count = 0;
    uint32_t slot = from;

    while (slot != to) {
        if (slot_is_unread(em, slot)) {
            count++;
        }
        slot = next_slot_index(em, slot);
    }

    return count;
}

static bool validate_overflow_window(const struct ext_memory_s *em,
                                     uint32_t rec_candidate,
                                     uint32_t w_candidate,
                                     uint32_t *out_unread)
{
    uint32_t slot = rec_candidate;
    uint32_t unread_count = 0;
    uint32_t occupied = 0;

    if (ring_distance(rec_candidate, w_candidate, em->max_slots) != em->max_data_slots) {
        return false;
    }

    while (slot != w_candidate) {
        if (!slot_has_data(em, slot)) {
            return false;
        }
        occupied++;
        if (slot_status_byte(em, slot) == SLOT_STATUS_UNREAD) {
            unread_count++;
        }
        slot = next_slot_index(em, slot);
    }

    if (occupied != em->max_data_slots) {
        return false;
    }

    for (uint32_t check = 0; check < em->max_slots; check++) {
        if (slot_is_unread(em, check) && !slot_in_window(check, rec_candidate, w_candidate, em->max_slots)) {
            return false;
        }
    }

    *out_unread = unread_count;
    return true;
}

static void apply_indexes(struct ext_memory_s *em,
                          uint32_t windex,
                          uint32_t rindex,
                          uint32_t rec_index,
                          uint32_t unread)
{
    em->slot_windex = windex;
    em->slot_rindex = rindex;
    em->slot_rec_index = rec_index;
    em->slot_count = unread;
    em->slot_rec_count = ring_distance(rec_index, rindex, em->max_slots);
}

static void scan_indexes_from_flash(struct ext_memory_s *em)
{
    uint32_t windex = 0;
    uint32_t unread = 0;

    if (!find_write_index(em, &windex)) {
        /* Physically full ring: treat W as 0 and take the overflow window. */
        windex = 0;
    }

    {
        uint32_t rec_index = (windex + em->max_slots - em->max_data_slots) % em->max_slots;

        if (validate_overflow_window(em, rec_index, windex, &unread)) {
            uint32_t rindex = find_read_index(em, rec_index, windex);
            apply_indexes(em, windex, rindex, rec_index, unread);
            return;
        }
    }

    {
        uint32_t rec_index = find_recovery_index(em, windex);
        uint32_t rindex = find_read_index(em, rec_index, windex);
        unread = count_unread_slots(em, rindex, windex);
        apply_indexes(em, windex, rindex, rec_index, unread);
    }
}

static void erase_sector_of_slot(struct ext_memory_s *em, uint32_t slot_index)
{
    uint32_t sector_index = slot_index / em->sector_size_in_slots;
    em->p_sector_erase(em->start_address + sector_index * (uint32_t)em->sector_size);
}

void *em_driver_init_(em_sector_erase_fn pp_sector_erase,
                     em_read_fn pp_read,
                     em_write_fn pp_write,
                     uint32_t em_size,
                     uint16_t em_sector_size,
                     uint16_t em_slot_size,
                     uint32_t em_start_address)
{
    struct ext_memory_s *em = NULL;

    if ((NULL == pp_sector_erase) || (NULL == pp_read) || (NULL == pp_write)) {
        return NULL;
    }
    if ((0u == em_slot_size) || (em_slot_size < 2u) || (0u == em_sector_size) || (0u == em_size)) {
        return NULL;
    }
    if ((em_sector_size % em_slot_size) != 0u) {
        return NULL;
    }
    if ((em_size % em_sector_size) != 0u) {
        return NULL;
    }
    if ((em_size / em_sector_size) < 2u) {
        return NULL;
    }

    em = (struct ext_memory_s *)malloc(sizeof(struct ext_memory_s));
    if (NULL == em) {
        return NULL;
    }

    memset(em, 0, sizeof(struct ext_memory_s));
    em->start_address = em_start_address;
    em->p_sector_erase = pp_sector_erase;
    em->p_read = pp_read;
    em->p_write = pp_write;
    em->sector_size = em_sector_size;
    em->slot_size = em_slot_size;
    em->sector_size_in_slots = (uint16_t)(em_sector_size / em_slot_size);
    em->sectors = em_size / em_sector_size;
    em->max_slots = em_size / em_slot_size;
    em->max_data_slots = em->max_slots - em->sector_size_in_slots;
    return em;
}

void em_driver_deinit_(void *ext_m)
{
    free(ext_m);
}

void em_reset_(void *ext_m)
{
    struct ext_memory_s *em = (struct ext_memory_s *)ext_m;
    uint32_t i = 0;

    if (NULL == em) {
        return;
    }

    for (i = 0; i < em->sectors; i++) {
        em->p_sector_erase(em->start_address + i * (uint32_t)em->sector_size);
    }
    em->slot_count = 0;
    em->slot_rec_count = 0;
    em->slot_windex = 0;
    em->slot_rindex = 0;
    em->slot_rec_index = 0;
}

void em_init_(void *ext_m)
{
    struct ext_memory_s *em = (struct ext_memory_s *)ext_m;
    if (NULL == em) {
        return;
    }
    scan_indexes_from_flash(em);
}

void add_slot_(void *ext_m, const uint8_t *slot_ptr)
{
    struct ext_memory_s *em = (struct ext_memory_s *)ext_m;
    uint16_t payload_len = 0;

    if ((NULL == em) || (NULL == slot_ptr)) {
        return;
    }

    payload_len = (uint16_t)(em->slot_size - 1u);
    em->p_write(slot_address(em, em->slot_windex), slot_ptr, payload_len);

    em->slot_count++;
    if (em->slot_count > em->max_data_slots) {
        em->slot_count = em->max_data_slots;
        em->slot_rindex = next_slot_index(em, em->slot_rindex);
    }

    em->slot_rec_count++;
    if (em->slot_rec_count > em->max_data_slots) {
        em->slot_rec_count = em->max_data_slots;
        em->slot_rec_index = next_slot_index(em, em->slot_rec_index);
    }

    em->slot_windex = next_slot_index(em, em->slot_windex);
    if ((em->slot_windex % em->sector_size_in_slots) == 0u) {
        erase_sector_of_slot(em, em->slot_windex);
    }
}

int32_t read_slot_(void *ext_m, uint8_t *out_buffer)
{
    struct ext_memory_s *em = (struct ext_memory_s *)ext_m;

    if ((NULL == out_buffer) || (NULL == em)) {
        return -1;
    }
    if (0u == em->slot_count) {
        return -1;
    }

    em->p_read(slot_address(em, em->slot_rindex), out_buffer, em->slot_size);
    mark_slot_read(em, em->slot_rindex);
    em->slot_rindex = next_slot_index(em, em->slot_rindex);
    em->slot_count--;
    return (int32_t)em->slot_count;
}

int32_t discard_slot_(void *ext_m)
{
    struct ext_memory_s *em = (struct ext_memory_s *)ext_m;
    uint32_t old_rec = 0;

    if (NULL == em) {
        return -1;
    }
    if (em->slot_rec_index == em->slot_rindex) {
        return -1;
    }

    old_rec = em->slot_rec_index;
    em->slot_rec_count--;
    em->slot_rec_index = next_slot_index(em, em->slot_rec_index);
    if ((em->slot_rec_index % em->sector_size_in_slots) == 0u) {
        erase_sector_of_slot(em, old_rec);
    }
    return (int32_t)em->slot_count;
}

int32_t recover_slot_(void *ext_m)
{
    struct ext_memory_s *em = (struct ext_memory_s *)ext_m;

    if (NULL == em) {
        return -1;
    }
    if (em->slot_rindex == em->slot_rec_index) {
        return -1;
    }

    em->slot_rindex = prev_slot_index(em, em->slot_rindex);
    em->slot_count++;
    return (int32_t)em->slot_count;
}

int32_t discard_all_slots_(void *ext_m)
{
    struct ext_memory_s *em = (struct ext_memory_s *)ext_m;

    if (NULL == em) {
        return -1;
    }

    while (em->slot_rec_index != em->slot_rindex) {
        uint32_t old_rec = em->slot_rec_index;

        em->slot_rec_count--;
        em->slot_rec_index = next_slot_index(em, em->slot_rec_index);
        if ((em->slot_rec_index % em->sector_size_in_slots) == 0u) {
            erase_sector_of_slot(em, old_rec);
        }
    }

    em->slot_count = 0;
    return 0;
}

int32_t recover_all_slots_(void *ext_m)
{
    struct ext_memory_s *em = (struct ext_memory_s *)ext_m;

    if (NULL == em) {
        return -1;
    }

    while (em->slot_rindex != em->slot_rec_index) {
        em->slot_rindex = prev_slot_index(em, em->slot_rindex);
        em->slot_count++;
    }

    return (int32_t)em->slot_count;
}

int32_t get_slot_count_(void *ext_m)
{
    struct ext_memory_s *em = (struct ext_memory_s *)ext_m;
    if (NULL == em) {
        return -1;
    }
    return (int32_t)em->slot_count;
}

int32_t get_recover_count_(void *ext_m)
{
    struct ext_memory_s *em = (struct ext_memory_s *)ext_m;
    if (NULL == em) {
        return -1;
    }
    return (int32_t)ring_distance(em->slot_rec_index, em->slot_rindex, em->max_slots);
}
