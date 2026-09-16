# RingFileSystem

A small C library for a **circular event log on NOR flash**.

It is not a POSIX filesystem. It is a ring of fixed-size slots with:

- sector erase before overwrite
- unread / read marks in the last byte of each slot
- **index recovery from flash after reboot** — no EEPROM copy of read/write pointers

If you log telemetry, GPS points, or fault records on SPI NOR and the MCU can reset at any time, that is the problem this code is written for.

## Why it is interesting

Most “ring buffer in flash” sketches keep `head`/`tail` in RAM (lost on reset) or in a separate metadata sector (wear + extra failure mode).

This implementation treats flash itself as the source of truth:

1. **Write pointer** — find the *buffer sector* (erased `0xFF` tail / first empty slot after data).
2. **Read pointer** — walk slots and look at the last-byte mark (`0xFF` unread, `0x00` read).
3. **Overflow** — one sector is kept as a spare so the ring can erase ahead of the writer. After wrap, recovery also uses a **monotonic `uint32` in the first four bytes** of the slot to find the sequence break.

```
  oldest data          unread / read marks           spare (erased)     write
 |====================|=======|.....................|#################|  ->
  rec_index            rindex                        buffer sector      windex
```

That recovery path (including overflow + reboot) is covered by the GoogleTest suite under `gtest/`.

## Constraints (read before integrating)

| Rule | Why |
|------|-----|
| `sector_size % slot_size == 0` | Slots are packed into erase sectors |
| Last byte of every slot is reserved | Status: `0xFF` unread, programmed to `0x00` after `read_slot_` |
| Caller leaves that byte `0xFF` on write | `add_slot_` programs the unread mark |
| Payload should start with a monotonic `uint32` | Needed to reconstruct indexes when the ring is full |
| One sector is never used for live data | `max_data_slots = total_slots - slots_per_sector` |
| Backend must behave like NOR | Erase → `0xFF`; program only clears bits |
| `em_driver_init_` uses `malloc` | Pair with `free(em)` when the handle is dropped |

Flash callbacks you supply:

```c
void erase_sector(uint32_t address);
void read(uint32_t address, uint8_t *data, uint16_t length);
void write(uint32_t address, const uint8_t *data, uint16_t length);
```

## Quick start on a PC

You do not need a board to try it:

```sh
git clone https://github.com/Ethalon-emb/RingFileSystem.git
cd RingFileSystem
cmake -S . -B build
cmake --build build
./build/host_demo
```

The demo writes records, calls `em_init_()` as if the MCU rebooted, then fills the ring until overflow.

Full test suite (GoogleTest):

```sh
cmake -S gtest -B gtest/build -DCMAKE_BUILD_TYPE=Release
cmake --build gtest/build --config Release
ctest --test-dir gtest/build -C Release --output-on-failure
```

## Usage on a device

```c
void *em = em_driver_init_(erase_cb, read_cb, write_cb,
                           flash_bytes, sector_bytes, slot_bytes, start_addr);

em_reset_(em);   /* factory / first boot: erase the region */
em_init_(em);    /* every later boot: scan flash, restore indexes */

uint8_t slot[SLOT_SIZE];
memset(slot, 0xFF, sizeof slot);
/* fill payload; keep slot[SLOT_SIZE - 1] == 0xFF */
add_slot_(em, slot);

uint8_t out[SLOT_SIZE];
if (read_slot_(em, out) >= 0) {
    /* process out */
    discard_slot_(em);   /* or recover_slot_() if processing failed */
}
```

Typical lifecycle:

- `add_slot_` — append (erases the next sector when the current one is full)
- `read_slot_` — copy next unread slot and mark it read
- `discard_slot_` — drop a slot that was already read (advances recovery index; may erase)
- `recover_slot_` — unread the last read slot (processing failed, try again after reboot)
- `get_slot_count_` — unread slots still waiting

## Layout

```
Inc/ring_file_system.h   public API
Src/ring_file_system.c   ring + flash scan
examples/host_demo.c     NOR-like RAM backend, reboot + overflow
gtest/                   GoogleTest + flash simulator
```

Drop `ring_file_system.c` / `.h` into firmware, or link the `rfs` static library from the CMake test project.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Bug reports that include geometry (`size`, `sector`, `slot`) and whether overflow had already happened are the most useful.

## License

MIT. See [LICENSE](LICENSE).

Flash simulator in `gtest/flashsim.*` is from Kosma Moczek (WTFPL).
