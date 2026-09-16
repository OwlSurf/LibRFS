/**
 * \file ring_file_system.h
 * \brief Ring buffer file system for external NOR flash.
 * \author Roman Garanin
 */
#ifndef INC_RING_FILE_SYSTEM_H_
#define INC_RING_FILE_SYSTEM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

typedef void (*em_sector_erase_fn)(uint32_t address);
typedef void (*em_read_fn)(uint32_t address, uint8_t *data, uint16_t length);
typedef void (*em_write_fn)(uint32_t address, const uint8_t *data, uint16_t length);

/**
 * \brief Initialize the external memory driver.
 *
 * Requirements: \p em_slot_size >= 2, \p em_sector_size is a multiple of
 * \p em_slot_size, \p em_size is a multiple of \p em_sector_size, at least
 * two sectors. The last byte of each slot is reserved for read status
 * (0xFF unread, 0x00 read). \p add_slot_ programs only the first
 * (slot_size - 1) bytes.
 *
 * \return Pointer to the driver, or NULL on invalid arguments / OOM.
 */
void *em_driver_init_(em_sector_erase_fn pp_sector_erase,
                     em_read_fn pp_read,
                     em_write_fn pp_write,
                     uint32_t em_size,
                     uint16_t em_sector_size,
                     uint16_t em_slot_size,
                     uint32_t em_start_address);

/** \brief Erase every sector and zero indexes. */
void em_reset_(void *ext_m);

/**
 * \brief Scan flash and restore write/read/recover indexes.
 *
 * Recovery uses empty-slot geometry and the status byte. It does not
 * interpret payload. \c recover_slot_ after reboot still rewinds over
 * read-but-not-discarded slots; \c recover_all_ before reboot cannot
 * restore 0x00 status bytes (NOR limitation).
 */
void em_init_(void *ext_m);

/** \brief Free a driver allocated by \c em_driver_init_. */
void em_driver_deinit_(void *ext_m);

/**
 * \brief Append a slot. Only \c slot_size - 1 payload bytes are programmed;
 * the status byte stays 0xFF from erase (unread).
 */
void add_slot_(void *ext_m, const uint8_t *slot_ptr);

/**
 * \brief Read the oldest unread slot, mark it read (status 0x00).
 * \return Remaining unread slots, or -1.
 */
int32_t read_slot_(void *ext_m, uint8_t *out_buffer);

/**
 * \brief Discard the oldest recovered/read slot. Erases a sector after its
 * last slot is discarded.
 * \return Unread slot count, or -1.
 */
int32_t discard_slot_(void *ext_m);

/**
 * \brief Rewind the last read (RAM pointer only; status byte stays 0x00).
 * \return Unread slot count, or -1.
 */
int32_t recover_slot_(void *ext_m);

int32_t discard_all_slots_(void *ext_m);

int32_t recover_all_slots_(void *ext_m);

/** \brief Number of unread slots, or -1. */
int32_t get_slot_count_(void *ext_m);

/** \brief Number of slots in the recover window [REC, R), or -1. */
int32_t get_recover_count_(void *ext_m);

#ifdef __cplusplus
}
#endif
#endif /* INC_RING_FILE_SYSTEM_H_ */
