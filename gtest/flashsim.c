/*
 * Copyright © 2014 Kosma Moczek <kosma@cloudyourcar.com>
 * This program is free software. It comes without any warranty, to the extent
 * permitted by applicable law. You can redistribute it and/or modify it under
 * the terms of the Do What The Fuck You Want To Public License, Version 2, as
 * published by Sam Hocevar. See the COPYING file for more details.
 */

#include "flashsim.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include "ring_file_system.h"

#ifdef FLASHSIM_LOG
#define logprintf(args...) printf(args)
#else
#define logprintf(args...) do {} while (0)
#endif

uint8_t distance_sector_read_buffer[EXT_MEM_SECTOR_SIZE] = {0};
uint8_t distance_sector_write_buffer[EXT_MEM_SECTOR_SIZE] = {0};
uint8_t track_sector_read_buffer[EXT_MEM_SECTOR_SIZE] = {0};
uint8_t track_sector_write_buffer[EXT_MEM_SECTOR_SIZE] = {0};
void* em_distance = NULL;
void* em_track = NULL;

struct flashsim {
    int size;
    int sector_size;
    FILE *fh;
};

struct flashsim *flashsim_open(const char *name, int size, int sector_size)
{
    struct flashsim *sim = malloc(sizeof(struct flashsim));

    sim->size = size;
    sim->sector_size = sector_size;
    sim->fh = fopen(name, "w+");
    assert(sim->fh != NULL);
    assert(ftruncate(fileno(sim->fh), size) == 0);

    return sim;
}

void flashsim_close(struct flashsim *sim)
{
    fclose(sim->fh);
    free(sim);
}

void flashsim_sector_erase(struct flashsim *sim, int addr)
{
    int sector_start = addr - (addr % sim->sector_size);
    logprintf("flashsim_erase  (0x%08x) * erasing sector at 0x%08x\n", addr, sector_start);

    void *empty = malloc(sim->sector_size);
    memset(empty, 0xff, sim->sector_size);

    assert(fseek(sim->fh, sector_start, SEEK_SET) == 0);
    assert(fwrite(empty, 1, sim->sector_size, sim->fh) == (size_t) sim->sector_size);

    free(empty);
}

void flashsim_read(struct flashsim *sim, int addr, uint8_t *buf, int len)
{
    assert(fseek(sim->fh, addr, SEEK_SET) == 0);
    assert(fread(buf, 1, len, sim->fh) == (size_t) len);

    logprintf("flashsim_read   (0x%08x) = %d bytes [ ", addr, len);
    for (int i=0; i<len; i++) {
        logprintf("%02x ", buf[i]);
        if (i == 15) {
            logprintf("... ");
            break;
        }
    }
    logprintf("]\n");
}

void flashsim_program(struct flashsim *sim, int addr, const uint8_t *buf, int len)
{
    logprintf("flashsim_program(0x%08x) + %d bytes [ ", addr, len);
    for (int i=0; i<len; i++) {
        logprintf("%02x ", buf[i]);
        if (i == 15) {
            logprintf("... ");
            break;
        }
    }
    logprintf("]\n");

    uint8_t *data = malloc(len);

    assert(fseek(sim->fh, addr, SEEK_SET) == 0);
    assert(fread(data, 1, len, sim->fh) == (size_t) len);

    for (int i=0; i<(int) len; i++)
        data[i] &= buf[i];

    assert(fseek(sim->fh, addr, SEEK_SET) == 0);
    assert(fwrite(data, 1, len, sim->fh) == (size_t) len);

    free(data);
}

struct flashsim *sim;
void op_sector_erase(int address)
{
    flashsim_sector_erase(sim, address);
    return;
}

void op_program(int address, uint8_t *data, size_t size)
{
    flashsim_program(sim, address, data, size);
}

void op_read(int address, uint8_t *data, size_t size)
{
    flashsim_read(sim, address, data, size);
}

void add_distance_data(uint16_t dist, uint32_t ts)
{
	struct distance_data_s {
		uint32_t ts;
		uint16_t distance;
		uint16_t dummy;
	} data;
	static  uint16_t prev_dist = 0;

	if ( get_slot_count_(em_distance) != 0 ) {
        if ( dist > prev_dist ) {
                if ( ( dist - prev_dist ) < 2 ) {
                        return;
                }
        } else if ( prev_dist > dist ) {
                if ( ( prev_dist - dist ) < 2 ) {
                        return;
                }
        } else {
                return;
        }
	}

	prev_dist = dist;

	data.distance = dist;
	data.ts = ts;
	data.dummy = 0;

	add_slot_(em_distance,  (uint8_t*)&data);


}

int32_t read_distance_data(uint16_t* distance_data, uint32_t* ts)
{
	struct distance_data_s {
		uint32_t ts;
		uint16_t distance;
		uint16_t dummy;
	} data = {0};

	int32_t count  = read_slot_(em_distance, (uint8_t*)&data);
	if ( -1 != count ) {
		*distance_data = data.distance;
		*ts = data.ts;
	}
	return count;
}

uint16_t reg_windex;
uint16_t reg_rindex;
uint16_t reg_count;
uint16_t reg_rec_index;
uint16_t reg_rec_count;

void load_index(
                       uint16_t *windex,      /**< Pointer to write slot index.      */
                       uint16_t *rindex,      /**< Pointer to read slot index.       */
                       uint16_t *count,       /**< Pointer to slot counter.          */
                       uint16_t *rec_index,   /**< Pointer to slot recovery index.   */
                       uint16_t *rec_count    /**< Pointer to slot recovery counter. */
                       )
{
	*windex    = reg_windex;
	*rindex    = reg_rindex;
	*count     = reg_count;
	*rec_index = reg_rec_index;
	*rec_count = reg_rec_count;

	return;
}

void save_index(
                       uint16_t *windex,      /**< Pointer to write slot index.      */
                       uint16_t *rindex,      /**< Pointer to read slot index.       */
                       uint16_t *count,       /**< Pointer to slot counter.          */
                       uint16_t *rec_index,   /**< Pointer to slot recovery index.   */
                       uint16_t *rec_count    /**< Pointer to slot recovery counter. */
                       )
{
	reg_windex = *windex;
	reg_rindex = *rindex;
	reg_count = *count;
	reg_rec_index = *rec_index;
	reg_rec_count = *rec_count;

	return;
}

/* vim: set ts=4 sw=4 et: */
