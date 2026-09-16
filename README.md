# LibRFS (Ring File System)

C library that stores a FIFO of fixed-size records on NOR flash. Old data is overwritten when the ring is full. Indexes are recovered from flash after reboot — no external NVRAM.

**Illustrated guide (RU):** [docs/ILLUSTRATED_GUIDE.ru.md](docs/ILLUSTRATED_GUIDE.ru.md)

## Features

- Configurable region size, sector size, and slot size
- Caller-supplied erase / read / write callbacks
- Erase-ahead of the next sector after a sector is filled
- Discard erases a sector after its last slot is confirmed
- `em_init_()` restores W / R / REC from empty-slot geometry and the status byte
- Last byte of each slot is reserved (`0xFF` unread, `0x00` read)

## Clone

```sh
git clone https://github.com/OwlSurf/LibRFS.git
cd LibRFS
```

Use `Inc/ring_file_system.h` and `Src/ring_file_system.c` in your firmware project.

## Usage

```c
void *em = em_driver_init_(pp_sector_erase,
                           pp_read,
                           pp_write,
                           em_size,
                           em_sector_size,
                           em_slot_size,
                           em_start_address);
if (em == NULL) {
    /* invalid geometry or OOM */
}

em_reset_(em);   /* first boot: erase all sectors */
em_init_(em);    /* after reboot: scan flash and restore indexes */

uint8_t data[slot_size] = { /* payload in bytes [0, slot_size - 2] */ };
add_slot_(em, data);         /* programs slot_size - 1 bytes; status stays 0xFF */

uint8_t buffer[slot_size];
read_slot_(em, buffer);      /* marks status 0x00 */

discard_slot_(em);
recover_slot_(em);
discard_all_slots_(em);
recover_all_slots_(em);
get_slot_count_(em);
get_recover_count_(em);

em_driver_deinit_(em);
```

## Contracts

| Topic | Rule |
|-------|------|
| Slot layout | Bytes `[0 .. slot_size-2]` are payload. Byte `slot_size-1` is read status. |
| Geometry | `slot_size >= 2`, sector size multiple of slot size, region multiple of sector, at least two sectors. |
| Erase-ahead | After the last slot of a sector is written, the **next** sector is erased. |
| Discard erase | After the last slot of a sector is discarded, **that** sector is erased. |
| Index types | Write/read/recover indexes are `uint32_t` (supports more than 65536 slots). |
| Recovery source | Empty ↔ data transitions and status bytes. Payload is not interpreted. |
| Recover vs reboot | `recover_slot_` rewinds R in RAM over read-but-not-discarded slots. It cannot program status back to `0xFF` (NOR). After reboot, those slots stay marked read unless discarded/erased. `recover_all_` *before* reboot then `em_init_` restores the flash marks, not the RAM rewind. |
| Thread safety | None — serialize access externally. |
| Flash errors | Callbacks are `void`; the library does not retry failed erase/program. |

## API

```c
typedef void (*em_sector_erase_fn)(uint32_t address);
typedef void (*em_read_fn)(uint32_t address, uint8_t *data, uint16_t length);
typedef void (*em_write_fn)(uint32_t address, const uint8_t *data, uint16_t length);

void *em_driver_init_(em_sector_erase_fn erase,
                      em_read_fn read,
                      em_write_fn write,
                      uint32_t em_size,
                      uint16_t em_sector_size,
                      uint16_t em_slot_size,
                      uint32_t em_start_address);

void em_reset_(void *ext_m);
void em_init_(void *ext_m);
void em_driver_deinit_(void *ext_m);
void add_slot_(void *ext_m, const uint8_t *slot_ptr);
int32_t read_slot_(void *ext_m, uint8_t *out_buffer);
int32_t discard_slot_(void *ext_m);
int32_t recover_slot_(void *ext_m);
int32_t discard_all_slots_(void *ext_m);
int32_t recover_all_slots_(void *ext_m);
int32_t get_slot_count_(void *ext_m);
int32_t get_recover_count_(void *ext_m);
```

`get_slot_count_` is the unread queue (R → W). `get_recover_count_` is the recover window (REC → R).

## Tests

```sh
cmake -DCMAKE_BUILD_TYPE=Release -S gtest -B gtest/build
cmake --build gtest/build
ctest --test-dir gtest/build -C Release --output-on-failure --rerun-failed
```

Or `./build_test.sh`.

## License

MIT. See [LICENSE](LICENSE).

## Author

Roman Garanin
