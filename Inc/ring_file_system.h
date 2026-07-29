/**
 * \file ring_file_system.h
 * \brief
 * \author: Roman Garanin
 */
#ifndef INC_RING_FILE_SYSTEM_H_
#define INC_RING_FILE_SYSTEM_H_

#define _CRT_SECURE_NO_WARNINGS

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "stdlib.h"

/**
 * \brief Initialize the external memory driver.
 * \param pp_sector_erase Pointer to the sector erase function.
 * \param pp_read Pointer to the read function.
 * \param pp_write Pointer to the write function.
 * \param pp_load_index Pointer to the function loads read and write slot indexes.
 * \param pp_save_index Pointer to the function saves read and write slot indexes.
 * \param em_size Size of the external memory.
 * \param em_sector_size Size of the sector in external memory.
 * \param em_slot_size Size of the slot in external memory.
 * \param em_start_address Start address in external memory.
 * \return Pointer to the initialized external memory structure.
 */
void *em_driver_init_(void* pp_sector_erase,
                     void* pp_read,
                     void* pp_write,
					 void* pp_load_index,
					 void* pp_save_index,
                     uint32_t em_size,
                     uint16_t em_sector_size,
                     uint16_t em_slot_size,
                     uint32_t em_start_address);

/**
 * \brief Reset the external memory.
 * \param ext_m Pointer to the external memory structure.
 */
void em_reset_(void* ext_m);

/**
 * \brief Initialize the external memory.
 * \param ext_m Pointer to the external memory structure.
 */
void em_init_(void* ext_m);

/**
 * \brief Save the indexes.
 * \param ext_m Pointer to the external memory structure.
 */
void save_em_indexes_ (void *ext_m);
/**
 * \brief Add a slot to the external memory.
 * \param ext_m Pointer to the external memory structure.
 * \param slot_ptr Pointer to the slot data.
 */
void add_slot_(void* ext_m, uint8_t * slot_ptr);

/**
 * \brief Read a slot from the external memory.
 * \param ext_m Pointer to the external memory structure.
 * \param out_buffer Pointer to the buffer to store read data.
 * \return The number of remaining slots, or -1 if an error occurs.
 */
int32_t read_slot_(void* ext_m, uint8_t* out_buffer);

/**
 * \brief Disсards a slot had read from the external memory.
 * \param ext_m Pointer to the external memory structure.
 * \return The number of slots, or -1 if an error occurs.
 */
int32_t discard_slot_(void* ext_m);

/**
 * \brief Recovers a slot had read from the external memory.
 * \param ext_m Pointer to the external memory structure.
 * \return The number of slots, or -1 if an error occurs.
 */
int32_t recover_slot_(void* ext_m);

int32_t discard_all_slots_(void *ext_m);

int32_t recover_all_slots_(void *ext_m);

/**
 * \brief Get the count of slots in the external memory.
 * \param ext_m Pointer to the external memory structure.
 * \return The number of slots, or -1 if an error occurs.
 */
int32_t get_slot_count_(void* ext_m);

/**
 * \brief Free the external memory driver allocated by em_driver_init_.
 * \param ext_m Pointer to the external memory structure.
 */
void em_driver_deinit_(void* ext_m);
#ifdef __cplusplus
}
#endif
#endif /* INC_RING_FILE_SYSTEM_H_ */
