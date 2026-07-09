/*
 * Copyright © 2014 Kosma Moczek <kosma@cloudyourcar.com>
 * This program is free software. It comes without any warranty, to the extent
 * permitted by applicable law. You can redistribute it and/or modify it under
 * the terms of the Do What The Fuck You Want To Public License, Version 2, as
 * published by Sam Hocevar. See the COPYING file for more details.
 */

#ifndef FLASHSIM_H
#define FLASHSIM_H
#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>

#define DIST_MEM_SIZE 524288 // (512kB)
#define DIST_START_ADDR 0
#define DIST_SLOT_SIZE 8
#define EXT_MEM_SECTOR_SIZE 4096
#define TRACK_MEM_SIZE 0x10000   //(1024kB)
#define TRACK_SLOT_SIZE 16     //(128B Bytes)
#define TRACK_START_ADDR 0x80000 //()


extern void* em_distance;
extern void* em_track;
extern uint8_t distance_sector_read_buffer[];
extern uint8_t distance_sector_write_buffer[];
extern uint8_t track_sector_read_buffer[];
extern uint8_t track_sector_write_buffer[];
struct flashsim;

struct flashsim *flashsim_open(const char *name, int size, int sector_size);
void flashsim_close(struct flashsim *sim);

void flashsim_sector_erase(struct flashsim *sim, int addr);
void flashsim_read(struct flashsim *sim, int addr, uint8_t *buf, int len);
void flashsim_program(struct flashsim *sim, int addr, const uint8_t *buf, int len);

extern struct flashsim *sim;
void op_sector_erase(int address);
void op_program(int address, uint8_t *data, size_t size);
void op_read(int address, uint8_t *data, size_t size);

void load_index(
                       uint16_t *windex,      /**< Pointer to write slot index.      */
                       uint16_t *rindex,      /**< Pointer to read slot index.       */
                       uint16_t *count,       /**< Pointer to slot counter.          */
                       uint16_t *rec_index,   /**< Pointer to slot recovery index.   */
                       uint16_t *rec_count    /**< Pointer to slot recovery counter. */
                       );

void save_index(
                       uint16_t *windex,      /**< Pointer to write slot index.      */
                       uint16_t *rindex,      /**< Pointer to read slot index.       */
                       uint16_t *count,       /**< Pointer to slot counter.          */
                       uint16_t *rec_index,   /**< Pointer to slot recovery index.   */
                       uint16_t *rec_count    /**< Pointer to slot recovery counter. */
                       );

void add_distance_data(uint16_t dist, uint32_t ts);
int32_t read_distance_data(uint16_t* distance_data, uint32_t* ts);

#ifdef __cplusplus
}
#endif


#endif

/* vim: set ts=4 sw=4 et: */
