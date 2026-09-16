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
    uint8_t *mem;
};

uint32_t flashsim_erase_count = 0;
int flashsim_last_erase_addr = -1;

void flashsim_reset_erase_stats(void)
{
    flashsim_erase_count = 0;
    flashsim_last_erase_addr = -1;
}

struct flashsim *flashsim_open(const char *name, int size, int sector_size)
{
    (void)name;

    struct flashsim *sim = malloc(sizeof(struct flashsim));
    assert(sim != NULL);

    sim->size = size;
    sim->sector_size = sector_size;
    sim->mem = malloc((size_t)size);
    assert(sim->mem != NULL);
    memset(sim->mem, 0xff, (size_t)size);
    flashsim_reset_erase_stats();

    return sim;
}

void flashsim_close(struct flashsim *sim)
{
    free(sim->mem);
    free(sim);
}

void flashsim_sector_erase(struct flashsim *sim, int addr)
{
    int sector_start = addr - (addr % sim->sector_size);
    logprintf("flashsim_erase  (0x%08x) * erasing sector at 0x%08x\n", addr, sector_start);

    assert(sector_start >= 0 && sector_start + sim->sector_size <= sim->size);
    memset(sim->mem + sector_start, 0xff, (size_t)sim->sector_size);
    flashsim_erase_count++;
    flashsim_last_erase_addr = sector_start;
}

void flashsim_read(struct flashsim *sim, int addr, uint8_t *buf, int len)
{
    assert(addr >= 0 && len >= 0 && addr + len <= sim->size);
    memcpy(buf, sim->mem + addr, (size_t)len);

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
    assert(addr >= 0 && len >= 0 && addr + len <= sim->size);

    logprintf("flashsim_program(0x%08x) + %d bytes [ ", addr, len);
    for (int i=0; i<len; i++) {
        logprintf("%02x ", buf[i]);
        if (i == 15) {
            logprintf("... ");
            break;
        }
    }
    logprintf("]\n");

    for (int i = 0; i < len; i++) {
        sim->mem[addr + i] &= buf[i];
    }
}

struct flashsim *sim;
void op_sector_erase(uint32_t address)
{
    flashsim_sector_erase(sim, (int)address);
}

void op_program(uint32_t address, const uint8_t *data, uint16_t size)
{
    flashsim_program(sim, (int)address, data, (int)size);
}

void op_read(uint32_t address, uint8_t *data, uint16_t size)
{
    flashsim_read(sim, (int)address, data, (int)size);
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
	data.dummy = 0xFF00;

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

/* vim: set ts=4 sw=4 et: */
