# LibRFS (Ring File System)

A small C library for a **circular event log on NOR flash**.

It is not a POSIX filesystem. It is a ring of fixed-size slots with:

- sector erase-ahead before overwrite
- unread / read marks in the last byte of each slot
- **index recovery from flash after reboot** — empty-slot geometry and status bytes, no EEPROM copy of pointers, no parsing of payload

If you log telemetry, GPS points, or fault records on SPI NOR and the MCU can reset at any time, that is the problem this code is written for.

**Illustrated guide (RU):** [docs/ILLUSTRATED_GUIDE.ru.md](docs/ILLUSTRATED_GUIDE.ru.md)

## Why it is interesting

Most “ring buffer in flash” sketches keep `head`/`tail` in RAM (lost on reset) or in a separate metadata sector (wear + extra failure mode).

This implementation treats flash itself as the source of truth:

1. **Write pointer** — the first empty slot after data (data → empty transition).
2. **Read / recover pointers** — last-byte mark (`0xFF` unread, `0x00` read) in the live window.
3. **Overflow** — one sector is kept as a spare so the ring can erase ahead of the writer.

```
  oldest data          unread / read marks           spare (erased)     write
 |====================|=======|.....................|#################|  ->
  rec_index            rindex                        buffer sector      windex
```

That recovery path (including overflow + reboot) is covered by the GoogleTest suite under `gtest/`.

## Constraints (read before integrating)

| Rule | Why |
|------|-----|
| `slot_size >= 2`, `sector_size % slot_size == 0`, region multiple of sector, ≥ 2 sectors | Packed slots; `em_driver_init_` returns NULL otherwise |
| Last byte of every slot is reserved | Status: `0xFF` unread, programmed to `0x00` after `read_slot_` |
| `add_slot_` programs `slot_size - 1` bytes | Status stays `0xFF` from erase |
| Payload is not interpreted | Recovery does not need a monotonic header |
| Indexes are `uint32_t` | Supports more than 65536 slots |
| One sector is never used for live data | `max_data_slots = total_slots - slots_per_sector` |
| Backend must behave like NOR | Erase → `0xFF`; program only clears bits |
| `em_driver_init_` uses `malloc` | Pair with `em_driver_deinit_(em)` |
| `recover_slot_` is RAM-only on the status byte | Cannot raise `0x00` back to `0xFF` without a sector erase |
| Thread safety / flash errors | None; callbacks are `void` |

Flash callbacks you supply:

```c
typedef void (*em_sector_erase_fn)(uint32_t address);
typedef void (*em_read_fn)(uint32_t address, uint8_t *data, uint16_t length);
typedef void (*em_write_fn)(uint32_t address, const uint8_t *data, uint16_t length);
```

## Quick start on a PC

You do not need a board to try it:

```sh
git clone https://github.com/OwlSurf/LibRFS.git
cd LibRFS
cmake -S . -B build
cmake --build build
./build/host_demo
```

The demo writes records, calls `em_init_()` as if the MCU rebooted, then fills the ring until overflow.

Full test suite (GoogleTest):

```sh
cmake -S gtest -B gtest/build -DCMAKE_BUILD_TYPE=Release
cmake --build gtest/build --config Release
ctest --test-dir gtest/build -C Release --output-on-failure --rerun-failed
```

Or `./build_test.sh`.

## Usage on a device

```c
void *em = em_driver_init_(erase_cb, read_cb, write_cb,
                           flash_bytes, sector_bytes, slot_bytes, start_addr);
if (em == NULL) {
    /* invalid geometry or OOM */
}

em_reset_(em);   /* factory / first boot: erase the region */
em_init_(em);    /* every later boot: scan flash, restore indexes */

uint8_t slot[SLOT_SIZE];
/* fill payload in bytes [0, SLOT_SIZE - 2]; last byte stays 0xFF from erase */
add_slot_(em, slot);

uint8_t out[SLOT_SIZE];
if (read_slot_(em, out) >= 0) {
    /* process out */
    discard_slot_(em);   /* or recover_slot_() if processing failed */
}

em_driver_deinit_(em);
```

Typical lifecycle:

- `add_slot_` — append (erases the **next** sector when the current one is filled)
- `read_slot_` — copy next unread slot and mark it read
- `discard_slot_` — drop a slot that was already read (advances REC; may erase **that** sector)
- `recover_slot_` — rewind the last read in RAM (status byte stays `0x00`)
- `get_slot_count_` — unread slots (R → W)
- `get_recover_count_` — recover window (REC → R)

## Layout

```
Inc/ring_file_system.h   public API
Src/ring_file_system.c   ring + flash scan
examples/host_demo.c     NOR-like RAM backend, reboot + overflow
gtest/                   GoogleTest + flash simulator
docs/                    illustrated guide (RU)
```

Drop `ring_file_system.c` / `.h` into firmware, or link the `rfs` static library from the root CMake project.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Bug reports that include geometry (`size`, `sector`, `slot`) and whether overflow had already happened are the most useful.

## License

MIT. See [LICENSE](LICENSE).

Author: Roman Garanin. Flash simulator in `gtest/flashsim.*` is from Kosma Moczek (WTFPL).
