/**
 * \file ring_file_sysyem.c
 * \brief This file manages the reading, writing, and erasing of data slots in external memory using a ring buffer mechanism.
 *        The function pointers allow for flexible implementation of hardware-specific memory operations.
 * \author: Roman Garanin
 */

#include "ring_file_system.h"

#define SLOT_ERASED_BYTE 0xFFu
#define SLOT_STATUS_UNREAD 0xFFu
#define SLOT_STATUS_READ   0x00u

/**
 * \struct ext_memory_s
 * \brief Structure representing external memory and its management.
 */
struct ext_memory_s {
    uint16_t sector_size;          /**< Sector size (bytes). */
    uint16_t sector_size_in_slots; /**< Sector size (slots). */
    uint16_t sectors;              /**< Size in sectors. */
    uint16_t slot_size;            /**< Data slot size (bytes). */
    uint16_t in_sector_slot_index; /**< Slot index in sector. To control slots in sector for erase. */
    uint16_t slot_count;           /**< Slots counter. */
    uint16_t slot_rec_count;       /**< Slots count can be recovered. */
    uint32_t max_slots;            /**< Full size of memory in slots. */
    uint32_t max_data_slots;       /**< Maximum data slots */
    uint16_t slot_windex;          /**< Write slot index. */
    uint16_t slot_rindex;          /**< Read slot index. */
    uint16_t slot_rec_index;       /**< Recovery slot index */
    uint32_t start_address;        /**< Start address in external memory. */

    void (*p_sector_erase)(uint32_t address); /**< Pointer to erase function sector in external memory. */
    void (*p_read)(uint32_t address, uint8_t *data, uint16_t length); /**< Pointer to read data function in external memory. */
    void (*p_write)(uint32_t address, const uint8_t *data, uint16_t length); /**< Pointer to write data function in external memory. */
};

static uint32_t slot_address(const struct ext_memory_s *em, uint32_t slot_index)
{
    return em->start_address + slot_index * em->slot_size;
}

static uint16_t next_slot_index(const struct ext_memory_s *em, uint16_t slot_index)
{
    slot_index++;
    if (slot_index >= em->max_slots) {
        slot_index = 0;
    }
    return slot_index;
}

static uint8_t slot_status_byte(const struct ext_memory_s *em, uint32_t slot_index)
{
    uint8_t status = 0;
    em->p_read(slot_address(em, slot_index) + em->slot_size - 1u, &status, 1);
    return status;
}

static bool slot_is_empty(const struct ext_memory_s *em, uint32_t slot_index)
{
    uint8_t byte = 0;
    uint32_t address = slot_address(em, slot_index);

    for (uint16_t i = 0; i < em->slot_size; i++) {
        em->p_read(address + i, &byte, 1);
        if (byte != SLOT_ERASED_BYTE) {
            return false;
        }
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

static void mark_slot_unread(struct ext_memory_s *em, uint32_t slot_index)
{
    uint8_t mark = SLOT_STATUS_UNREAD;
    em->p_write(slot_address(em, slot_index) + em->slot_size - 1u, &mark, 1);
}

static void mark_slot_read(struct ext_memory_s *em, uint32_t slot_index)
{
    uint8_t mark = SLOT_STATUS_READ;
    em->p_write(slot_address(em, slot_index) + em->slot_size - 1u, &mark, 1);
}

static bool sector_is_fully_empty(const struct ext_memory_s *em, uint16_t sector_index)
{
    uint32_t first_slot = (uint32_t)sector_index * em->sector_size_in_slots;

    for (uint16_t i = 0; i < em->sector_size_in_slots; i++) {
        if (!slot_is_empty(em, first_slot + i)) {
            return false;
        }
    }
    return true;
}

static bool sector_is_partially_written(const struct ext_memory_s *em, uint16_t sector_index)
{
    uint32_t first_slot = (uint32_t)sector_index * em->sector_size_in_slots;
    bool has_data = false;
    bool has_empty = false;

    for (uint16_t i = 0; i < em->sector_size_in_slots; i++) {
        if (slot_is_empty(em, first_slot + i)) {
            has_empty = true;
        } else {
            has_data = true;
        }
        if (has_data && has_empty) {
            return true;
        }
    }
    return false;
}

static void update_in_sector_slot_index(struct ext_memory_s *em, uint16_t slot_index)
{
    uint16_t sector_index = slot_index / em->sector_size_in_slots;

    if (sector_index == 0) {
        em->in_sector_slot_index = slot_index;
    } else {
        em->in_sector_slot_index = slot_index % em->sector_size_in_slots;
    }
}

static bool ring_is_physically_full(const struct ext_memory_s *em)
{
    for (uint32_t slot = 0; slot < em->max_slots; slot++) {
        if (slot_is_empty(em, slot)) {
            return false;
        }
    }
    return true;
}

static bool find_write_index_from_buffer_sector(const struct ext_memory_s *em, uint16_t *out_windex)
{
    bool all_empty = true;

    for (uint16_t sector = 0; sector < em->sectors; sector++) {
        if (!sector_is_fully_empty(em, sector)) {
            all_empty = false;
        }
        if (sector_is_partially_written(em, sector)) {
            uint32_t first_slot = (uint32_t)sector * em->sector_size_in_slots;

            for (uint16_t i = 0; i < em->sector_size_in_slots; i++) {
                if (slot_is_empty(em, first_slot + i)) {
                    *out_windex = (uint16_t)(first_slot + i);
                    return true;
                }
            }
            break;
        }
    }

    if (all_empty) {
        *out_windex = 0;
        return true;
    }

    for (uint16_t sector = 0; sector < em->sectors; sector++) {
        if (!sector_is_fully_empty(em, sector)) {
            continue;
        }

        uint16_t prev_sector = (sector == 0) ? (uint16_t)(em->sectors - 1) : (uint16_t)(sector - 1);
        if (!sector_is_fully_empty(em, prev_sector)) {
            *out_windex = (uint16_t)sector * em->sector_size_in_slots;
            return true;
        }
    }

    return false;
}

static bool find_write_index_from_full_ring(const struct ext_memory_s *em, uint16_t *out_windex)
{
    if (!ring_is_physically_full(em)) {
        return false;
    }

    for (uint32_t slot = 0; slot < em->max_slots; slot++) {
        uint32_t cur = 0;
        uint32_t next = 0;
        uint32_t next_slot = (slot + 1u) % em->max_slots;

        em->p_read(slot_address(em, slot), (uint8_t *)&cur, sizeof(cur));
        em->p_read(slot_address(em, next_slot), (uint8_t *)&next, sizeof(next));

        if (next < cur || (next - cur) != 1u) {
            *out_windex = (uint16_t)next_slot;
            return true;
        }
    }

    return false;
}

static bool find_write_index_from_backward_jump(const struct ext_memory_s *em, uint16_t *out_windex)
{
    for (uint32_t slot = 0; slot < em->max_slots; slot++) {
        uint32_t cur = 0;
        uint32_t next = 0;
        uint32_t next_slot = (slot + 1u) % em->max_slots;

        em->p_read(slot_address(em, slot), (uint8_t *)&cur, sizeof(cur));
        em->p_read(slot_address(em, next_slot), (uint8_t *)&next, sizeof(next));

        if (next < cur) {
            *out_windex = (uint16_t)next_slot;
            return true;
        }
    }

    return false;
}

static bool find_write_index(const struct ext_memory_s *em, uint16_t *out_windex)
{
    if (find_write_index_from_buffer_sector(em, out_windex)) {
        return true;
    }
    if (find_write_index_from_backward_jump(em, out_windex)) {
        return true;
    }
    return find_write_index_from_full_ring(em, out_windex);
}

static uint16_t find_recovery_index(const struct ext_memory_s *em, uint16_t windex)
{
    if (ring_is_physically_full(em)) {
        return (uint16_t)((windex + em->max_slots - em->max_data_slots) % em->max_slots);
    }

    uint16_t slot = windex;
    for (uint32_t checked = 0; checked < em->max_slots; ) {
        uint16_t sector = (uint16_t)(slot / em->sector_size_in_slots);

        if (sector_is_fully_empty(em, sector)) {
            checked += em->sector_size_in_slots;
            slot = (uint16_t)((slot + em->sector_size_in_slots) % em->max_slots);
            if (slot_has_data(em, slot)) {
                return slot;
            }
            continue;
        }

        if (slot_has_data(em, slot)) {
            return slot;
        }

        slot = next_slot_index(em, slot);
        checked++;
    }

    slot = 0;
    while (slot < em->max_slots) {
        uint16_t sector = (uint16_t)(slot / em->sector_size_in_slots);

        if (sector_is_fully_empty(em, sector)) {
            slot = (uint16_t)(slot + em->sector_size_in_slots);
            continue;
        }

        if (slot_has_data(em, slot)) {
            return slot;
        }

        slot++;
    }

    return windex;
}

static uint16_t find_read_index(const struct ext_memory_s *em, uint16_t rec_index, uint16_t windex)
{
    uint16_t slot = rec_index;

    while (slot != windex) {
        if (slot_is_unread(em, slot)) {
            return slot;
        }
        slot = next_slot_index(em, slot);
    }

    return windex;
}

static uint16_t count_unread_slots(const struct ext_memory_s *em, uint16_t from, uint16_t to)
{
    uint16_t count = 0;
    uint16_t slot = from;

    while (slot != to) {
        if (slot_is_unread(em, slot)) {
            count++;
        }
        slot = next_slot_index(em, slot);
    }

    return count;
}

/* Live window size [from, to), matching add_slot_/discard_* slot_rec_count. */
static uint16_t ring_slot_distance(uint16_t from, uint16_t to, uint32_t max_slots)
{
    if (to >= from) {
        return (uint16_t)(to - from);
    }
    return (uint16_t)(max_slots - (uint32_t)from + (uint32_t)to);
}

static uint16_t count_all_unread_slots(const struct ext_memory_s *em)
{
    uint16_t count = 0;

    for (uint32_t slot = 0; slot < em->max_slots; slot++) {
        if (slot_is_unread(em, slot)) {
            count++;
        }
    }

    return count;
}

static void push_w_candidate(uint16_t *candidates, uint16_t *count, uint16_t candidate, uint16_t max_count)
{
    uint16_t i = 0;

    for (i = 0; i < *count; i++) {
        if (candidates[i] == candidate) {
            return;
        }
    }

    if (*count < max_count) {
        candidates[*count] = candidate;
        (*count)++;
    }
}

static bool slot_in_window(uint16_t slot, uint16_t from, uint16_t to, uint32_t max_slots)
{
    if (from < to) {
        return slot >= from && slot < to;
    }
    if (from > to) {
        return slot >= from || slot < to;
    }
    return false;
}

static bool slot_in_buffer_gap(uint16_t slot, uint16_t windex, uint16_t rec_index, uint32_t max_slots)
{
    if (windex < rec_index) {
        return slot >= windex && slot < rec_index;
    }
    if (windex > rec_index) {
        return slot >= windex || slot < rec_index;
    }
    return false;
}

static bool validate_overflow_window(const struct ext_memory_s *em,
                                     uint16_t rec_candidate,
                                     uint16_t w_candidate,
                                     uint16_t *out_unread)
{
    uint16_t slot = rec_candidate;
    uint16_t unread_count = 0;
    uint16_t read_count = 0;

    while (slot != w_candidate) {
        uint8_t status = 0;

        if (!slot_has_data(em, slot)) {
            return false;
        }

        em->p_read(slot_address(em, slot) + em->slot_size - 1u, &status, 1);
        if (status == SLOT_STATUS_UNREAD) {
            unread_count++;
        } else if (status == SLOT_STATUS_READ) {
            read_count++;
        }
        slot = next_slot_index(em, slot);
    }

    if ((uint32_t)unread_count + (uint32_t)read_count != em->max_data_slots) {
        return false;
    }

    for (uint32_t check = 0; check < em->max_slots; check++) {
        if (slot_is_unread(em, (uint16_t)check)
            && !slot_in_window((uint16_t)check, rec_candidate, w_candidate, em->max_slots)
            && !slot_in_buffer_gap((uint16_t)check, w_candidate, rec_candidate, em->max_slots)) {
            return false;
        }
    }

    *out_unread = unread_count;
    return true;
}

static uint16_t find_first_read_marked_slot(const struct ext_memory_s *em)
{
    for (uint32_t slot = 0; slot < em->max_slots; slot++) {
        if (slot_has_data(em, slot) && slot_status_byte(em, slot) == SLOT_STATUS_READ) {
            return (uint16_t)slot;
        }
    }

    return (uint16_t)em->max_slots;
}

static bool find_overflow_indexes(const struct ext_memory_s *em,
                                  uint16_t *out_windex,
                                  uint16_t *out_rindex,
                                  uint16_t *out_rec_index,
                                  uint16_t *out_unread)
{
    uint16_t w_candidates[8];
    uint16_t w_count = 0;
    uint16_t windex = 0;
    uint16_t matched_w[8];
    uint16_t matched_rec[8];
    uint16_t matched_unread[8];
    uint16_t matched_count = 0;
    uint16_t first_read_mark = find_first_read_marked_slot(em);

    if (find_write_index_from_buffer_sector(em, &windex)) {
        push_w_candidate(w_candidates, &w_count, windex, 8);
    }
    if (find_write_index_from_backward_jump(em, &windex)) {
        push_w_candidate(w_candidates, &w_count, windex, 8);
    }
    for (uint32_t slot = 0; slot < em->max_slots; slot++) {
        uint32_t cur = 0;
        uint32_t next = 0;
        uint32_t next_slot = (slot + 1u) % em->max_slots;

        em->p_read(slot_address(em, slot), (uint8_t *)&cur, sizeof(cur));
        em->p_read(slot_address(em, next_slot), (uint8_t *)&next, sizeof(next));

        if (next < cur || (next - cur) != 1u) {
            push_w_candidate(w_candidates, &w_count, (uint16_t)next_slot, 8);
        }
    }

    for (uint16_t i = 0; i < w_count; i++) {
        uint16_t w_candidate = w_candidates[i];
        uint16_t rec_index = (uint16_t)((w_candidate + em->max_slots - em->max_data_slots) % em->max_slots);
        uint16_t unread = 0;

        if (!validate_overflow_window(em, rec_index, w_candidate, &unread)) {
            continue;
        }

        if (matched_count < 8) {
            matched_w[matched_count] = w_candidate;
            matched_rec[matched_count] = rec_index;
            matched_unread[matched_count] = unread;
            matched_count++;
        }
    }

    if (0 == matched_count) {
        return false;
    }

  uint16_t selected = 0;
    if (first_read_mark < em->max_slots) {
        for (uint16_t i = 0; i < matched_count; i++) {
            if (matched_rec[i] == first_read_mark) {
                selected = i;
                break;
            }
        }
    }

    *out_windex = matched_w[selected];
    *out_rec_index = matched_rec[selected];
    *out_unread = matched_unread[selected];
    *out_rindex = find_read_index(em, matched_rec[selected], matched_w[selected]);
    return true;
}

static void scan_indexes_from_flash(struct ext_memory_s *em)
{
    uint16_t windex = 0;
    uint16_t rindex = 0;
    uint16_t rec_index = 0;
    uint16_t unread = 0;

    if (find_overflow_indexes(em, &windex, &rindex, &rec_index, &unread)) {
        em->slot_windex = windex;
        em->slot_rindex = rindex;
        em->slot_rec_index = rec_index;
        em->slot_count = unread;
        /* Runtime slot_rec_count is distance(rec, w), not unread marks in [rec, r). */
        em->slot_rec_count = ring_slot_distance(rec_index, windex, em->max_slots);
        update_in_sector_slot_index(em, windex);
        return;
    }

    if (!find_write_index(em, &windex)) {
        windex = 0;
    }

    em->slot_windex = windex;
    em->slot_rec_index = find_recovery_index(em, windex);
    em->slot_rindex = find_read_index(em, em->slot_rec_index, windex);
    em->slot_count = count_unread_slots(em, em->slot_rindex, windex);
    em->slot_rec_count = ring_slot_distance(em->slot_rec_index, windex, em->max_slots);
    update_in_sector_slot_index(em, windex);
}

void *em_driver_init_(void* pp_sector_erase,
                     void* pp_read,
                     void* pp_write,
                     uint32_t em_size,
                     uint16_t em_sector_size,
                     uint16_t em_slot_size,
                     uint32_t em_start_address) {
    struct ext_memory_s *em = (struct ext_memory_s*)malloc(sizeof(struct ext_memory_s));

    if (NULL == em) {
        return NULL;
    }

    memset((void*)em, 0, sizeof(struct ext_memory_s));

    em->start_address = em_start_address;
    em->p_sector_erase = (void (*)(uint32_t))pp_sector_erase;
    em->p_read = (void (*)(uint32_t, uint8_t*, uint16_t))pp_read;
    em->p_write = (void (*)(uint32_t, const uint8_t*, uint16_t))pp_write;
    em->sector_size = em_sector_size;
    em->sector_size_in_slots = em_sector_size/em_slot_size;
    em->sectors = em_size / em_sector_size;
    em->slot_size = em_slot_size;
    em->max_slots = em_size / em_slot_size;
    em->max_data_slots = em->max_slots - em->sector_size_in_slots;
    em->slot_windex = 0;
    em->slot_rindex = 0;
    em->slot_rec_index = 0;
    em->in_sector_slot_index = 0;
    return (void*)em;
}

void em_reset_(void *ext_m) {
    struct ext_memory_s *em = (struct ext_memory_s*)ext_m;
    for (uint16_t i = 0; i < em->sectors; i++) {
        em->p_sector_erase(em->start_address + i * em->sector_size);
    }
    em->slot_count = 0;
    em->slot_rec_count = 0;
    em->slot_windex = 0;
    em->slot_rindex = 0;
    em->slot_rec_index = 0;
    em->in_sector_slot_index = 0;
}

void em_init_(void *ext_m) {
    struct ext_memory_s *em = (struct ext_memory_s*)ext_m;
    if (NULL == em) {
        return;
    }
    scan_indexes_from_flash(em);
}

void add_slot_(void* ext_m, uint8_t *slot_ptr) {
    struct ext_memory_s *em = (struct ext_memory_s*)ext_m;
    if (NULL == slot_ptr) {
        return;
    }

    em->p_write(em->start_address + em->slot_windex * em->slot_size, slot_ptr, em->slot_size);
    mark_slot_unread(em, em->slot_windex);

    em->slot_count++;

    if (em->slot_count > em->max_data_slots) { // Buffer overflow.
        em->slot_count = em->max_data_slots;
        em->slot_rindex++;
        if ( em->slot_rindex == em->max_slots) {
        	 em->slot_rindex = 0;
        }
    }

    em->slot_rec_count++;
    if ( em->slot_rec_count > em->max_data_slots ) {
    	em->slot_rec_count = em->max_data_slots;
        em->slot_rec_index++;
        if ( em->slot_rec_index == em->max_slots) {
        	 em->slot_rec_index = 0;
        }
    }

    uint16_t sector_index = em->slot_windex / em->sector_size_in_slots;
    em->slot_windex++;
    if (sector_index == 0) {
    	em->in_sector_slot_index = em->slot_windex;
    } else {
    	em->in_sector_slot_index = em->slot_windex % sector_index;
    }

    if (em->slot_windex == em->max_slots) {
        em->slot_windex = 0;
    }

    sector_index = em->slot_windex / em->sector_size_in_slots;
    if (em->in_sector_slot_index == em->sector_size_in_slots) {
    	uint32_t address = em->start_address + sector_index * em->sector_size;
        em->p_sector_erase(address);
    }
}

int32_t read_slot_(void* ext_m, uint8_t* out_buffer) {
    if ((NULL == out_buffer) || (NULL == ext_m)) {
        return -1;
    }

    struct ext_memory_s *em = (struct ext_memory_s*)ext_m;

    if (0 == em->slot_count) {
        return -1;
    }

    uint32_t address = em->start_address + em->slot_rindex * em->slot_size;
    em->p_read(address, out_buffer, em->slot_size);
    mark_slot_read(em, em->slot_rindex);

    em->slot_rindex++;
    if (em->slot_rindex == em->max_slots) {
        em->slot_rindex = 0;
    }

    em->slot_count--;
    return em->slot_count;
}

int32_t discard_slot_(void* ext_m)
{
    if (NULL == ext_m) {
        return -1;
    }
	struct ext_memory_s *em = (struct ext_memory_s*)ext_m;

	if ( em->slot_rec_index == em->slot_rindex) {
		return -1;
	}

	em->slot_rec_count--;
	uint16_t sector_index = em->slot_rec_index / em->sector_size_in_slots;
	em->slot_rec_index++;
	if ( sector_index == 0 ) {
		em->in_sector_slot_index = em->slot_rec_index;
	} else {
		em->in_sector_slot_index = em->slot_rec_index % sector_index;
	}

    if (em->slot_rec_index == em->max_slots) {
        em->slot_rec_index = 0;
    }

    if (em->in_sector_slot_index == em->sector_size_in_slots) {
        uint32_t address = em->start_address + sector_index * em->sector_size;
        em->p_sector_erase(address);
    }
    return em->slot_count;
}

int32_t recover_slot_(void* ext_m)
{
    if (NULL == ext_m) {
        return -1;
    }
	struct ext_memory_s *em = (struct ext_memory_s*)ext_m;

	if ( em->slot_rindex == em->slot_rec_index ) {
		return -1;
	}
	if (em->slot_rindex == 0) {
		em->slot_rindex = em->max_slots - 1;
	} else {
		em->slot_rindex--;
	}
	em->slot_count++;
	return em->slot_count;
}

int32_t discard_all_slots_(void *ext_m)
{
    if (ext_m == NULL) {
        return -1;
    }

    struct ext_memory_s *em = (struct ext_memory_s *)ext_m;

    while (em->slot_rec_index != em->slot_rindex) {

        uint16_t sector_index =
            em->slot_rec_index / em->sector_size_in_slots;

        em->slot_rec_count--;
        em->slot_rec_index++;

        if (em->slot_rec_index == em->max_slots) {
            em->slot_rec_index = 0;
        }

        if (sector_index == 0) {
            em->in_sector_slot_index = em->slot_rec_index;
        } else {
            em->in_sector_slot_index =
                em->slot_rec_index % em->sector_size_in_slots;
        }

        if (em->in_sector_slot_index == em->sector_size_in_slots) {
            uint32_t address =
                em->start_address + sector_index * em->sector_size;

            em->p_sector_erase(address);
        }
    }

    /* Only the recover window (read, not yet discarded) was dropped.
     * Unread slots between rindex and windex must remain readable. */
    return em->slot_count;
}

int32_t recover_all_slots_(void *ext_m)
{
    if (ext_m == NULL) {
        return -1;
    }

    struct ext_memory_s *em = (struct ext_memory_s *)ext_m;

    while (em->slot_rindex != em->slot_rec_index) {

        if (em->slot_rindex == 0) {
            em->slot_rindex = em->max_slots - 1;
        } else {
            em->slot_rindex--;
        }

        em->slot_count++;
    }

    return em->slot_count;
}

int32_t get_slot_count_(void* ext_m) {
    if (NULL == ext_m) {
        return -1;
    }
    struct ext_memory_s *em = (struct ext_memory_s*)ext_m;

    return em->slot_count;
}
