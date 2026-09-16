/**
 * Host demo: circular NOR-flash log that survives a simulated reboot.
 *
 * Build from repo root:
 *   cmake -S . -B build && cmake --build build && ./build/host_demo
 */

#include "ring_file_system.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define DEMO_SECTOR_SIZE  4096u
#define DEMO_SLOT_SIZE    16u
#define DEMO_SECTORS      4u
#define DEMO_SIZE         (DEMO_SECTOR_SIZE * DEMO_SECTORS)

struct slot_record {
    uint32_t seq;
    uint32_t event_id;
    uint8_t  pad[7];
    uint8_t  status;   /* reserved; add_slot_ does not program this byte */
};

static uint8_t g_flash[DEMO_SIZE];

static void nor_erase(uint32_t address)
{
    uint32_t start = address - (address % DEMO_SECTOR_SIZE);
    memset(g_flash + start, 0xFF, DEMO_SECTOR_SIZE);
}

static void nor_read(uint32_t address, uint8_t *data, uint16_t length)
{
    memcpy(data, g_flash + address, length);
}

static void nor_write(uint32_t address, const uint8_t *data, uint16_t length)
{
    /* NOR program: bits may only go 1 -> 0 */
    for (uint16_t i = 0; i < length; i++) {
        g_flash[address + i] &= data[i];
    }
}

static void write_events(void *em, uint32_t from, uint32_t count)
{
    struct slot_record rec;
    memset(&rec, 0xFF, sizeof(rec));
    rec.status = 0xFF;

    for (uint32_t i = 0; i < count; i++) {
        rec.seq = from + i;
        rec.event_id = 0xA0000000u | (from + i);
        add_slot_(em, (uint8_t *)&rec);
    }
}

static void drain_unread(void *em, const char *label)
{
    struct slot_record rec;
    int32_t remaining;
    uint32_t n = 0;

    printf("%s\n", label);
    while ((remaining = read_slot_(em, (uint8_t *)&rec)) >= 0) {
        n++;
        printf("  seq=%u event=0x%08x remaining_unread=%d\n",
               rec.seq, rec.event_id, remaining);
    }
    if (n == 0) {
        printf("  (empty)\n");
    }
}

int main(void)
{
    memset(g_flash, 0xFF, sizeof(g_flash));

    void *em = em_driver_init_(nor_erase,
                               nor_read,
                               nor_write,
                               DEMO_SIZE,
                               DEMO_SECTOR_SIZE,
                               DEMO_SLOT_SIZE,
                               0);
    if (em == NULL) {
        fprintf(stderr, "em_driver_init_ failed\n");
        return 1;
    }

    printf("=== first boot ===\n");
    em_reset_(em);
    write_events(em, 1, 5);
    printf("unread after 5 writes: %d\n", get_slot_count_(em));

    struct slot_record rec;
    read_slot_(em, (uint8_t *)&rec);
    printf("read one slot (seq=%u), unread now: %d\n", rec.seq, get_slot_count_(em));
    discard_slot_(em);
    printf("discarded that slot (cannot recover it anymore)\n");

    printf("\n=== simulated reboot (RAM indexes gone) ===\n");
    em_init_(em);
    printf("recovered unread count: %d\n", get_slot_count_(em));
    drain_unread(em, "slots after reboot:");

    printf("\n=== overflow + reboot ===\n");
    em_reset_(em);
    write_events(em, 1, 1200); /* more than max data slots for this geometry */
    em_init_(em);
    printf("unread after overflow and reboot: %d\n", get_slot_count_(em));
    if (read_slot_(em, (uint8_t *)&rec) >= 0) {
        printf("oldest surviving seq=%u (early events were overwritten)\n", rec.seq);
    }

    em_driver_deinit_(em);
    return 0;
}
