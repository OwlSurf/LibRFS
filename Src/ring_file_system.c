/**
 * \file ring_file_sysyem.c
 * \brief This file manages the reading, writing, and erasing of data slots in external memory using a ring buffer mechanism.
 *        The function pointers allow for flexible implementation of hardware-specific memory operations.
 * \author: Roman Garanin
 */

#include "ring_file_system.h"

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
    /** Pointer to load index function. */
    void (*p_load_index)(
    		             uint16_t *slot_windex,      /**< Pointer to write slot index. */
    		             uint16_t *slot_rindex,      /**< Pointer to read slot index. */
			             uint16_t *slot_count,       /**< Pointer to slot counter. */
						 uint16_t *slot_rec_index,   /**< Pointer to slot recovery index. */
						 uint16_t *slot_rec_count    /**< Pointer to slot recovery counter. */
						 );
    /** Pointer to save index function. */
    void (*p_save_index)(
    		             uint16_t *slot_windex,      /**< Pointer to write slot index. */
    		             uint16_t *slot_rindex,      /**< Pointer to read slot index. */
						 uint16_t *slot_count,       /**< Pointer to slot counter. */
						 uint16_t *slot_rec_index,   /**< Pointer to slot recovery index. */
						 uint16_t *slot_rec_count    /**< Pointer to slot recovery counter. */
						 );
};

void *em_driver_init_(void* pp_sector_erase,
                     void* pp_read,
                     void* pp_write,
					 void* pp_load_index,
					 void* pp_save_index,
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

    em->p_load_index = (void (*)(uint16_t *, uint16_t *, uint16_t *, uint16_t *, uint16_t *))pp_load_index;
    em->p_save_index = (void (*)(uint16_t *, uint16_t *, uint16_t *, uint16_t *, uint16_t *))pp_save_index;
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
    em->p_save_index (&em->slot_windex, &em->slot_rindex, &em->slot_count, &em->slot_rec_index, &em->slot_rec_count);
}

void em_init_(void *ext_m) {
    struct ext_memory_s *em = (struct ext_memory_s*)ext_m;
    em->p_load_index(&em->slot_windex, &em->slot_rindex, &em->slot_count, &em->slot_rec_index, &em->slot_rec_count);
}

void save_em_indexes_ (void *ext_m)
{
    if (NULL == ext_m) {
        return;
    }
    struct ext_memory_s *em = (struct ext_memory_s*)ext_m;
    em->p_save_index (&em->slot_windex, &em->slot_rindex, &em->slot_count, &em->slot_rec_index, &em->slot_rec_count);
}

void add_slot_(void* ext_m, uint8_t *slot_ptr) {
    struct ext_memory_s *em = (struct ext_memory_s*)ext_m;
    if (NULL == slot_ptr) {
        return;
    }

    em->p_write(em->start_address + em->slot_windex * em->slot_size, slot_ptr, em->slot_size);

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
	em->slot_rindex--;
	if ( em->slot_rindex == 0 ) {
		em->slot_rindex = em->max_slots-1;
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

    em->slot_count = 0;
    return 0;
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
