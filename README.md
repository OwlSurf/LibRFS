# RingFileSystem

Ring file system implementation.

## Overview

The RingFileSystem is a C-based implementation designed to manage external memory using a ring buffer mechanism. It allows for efficient use of memory by overwriting old data when new data is written.

## Features

- Configurable buffer size, sector size, and slot size.
- Customizable functions for sector erasing, data writing, and data reading.
- Efficient memory management with automatic sector erasing on buffer overflow.
- **Index recovery from flash** — no external storage for read/write pointers; `em_init_()` scans the buffer sector (erased tail with `0xFF`) and read marks in the last byte of each slot.

## Getting Started

### Prerequisites

- A C compiler (e.g., GCC)
- Necessary permissions to access and modify external memory.



### Installation

1. Clone the repository:
  ```sh
    git clone https://github.com/Ethalon-emb/RingFileSystem.git
    cd RingFileSystem
  ```
2. Include the `ring_file_system.h` and `ring_file_system.c` files in your project.



### Usage

1. Initialize the ring file system:
  ```c
    void* em = em_driver_init_(pp_sector_erase,
                               pp_read,
                               pp_write,
                               em_size,
                               em_sector_size,
                               em_slot_size,
                               em_start_address);
    em_reset_(em);   /* first boot */
    em_init_(em);    /* after reboot — scan flash and restore indexes */
  ```
  The last byte of each slot (`slot_size - 1`) is reserved for read status (`0xFF` = unread). Keep it at `0xFF` when writing data.
2. Add a slot:
  ```c
    uint8_t data[slot_size] = { /* your data */ };
    add_slot(em, data);
  ```
3. Read a slot:
  ```c
    uint8_t buffer[slot_size];
    read_slot(em, buffer);
  ```
4. Discard a slot:
  ```c
    discard_slot(em);
  ```
5. Recover a slot:
  ```c
    recover_slot(em);
  ```
6. Reset the memory:
  ```c
    em_reset(em);
  ```



## API Reference



### Functions

```c
void* em_driver_init_(void* pp_sector_erase,
                     void* pp_read,
                     void* pp_write,
                     uint32_t em_size,
                     uint16_t em_sector_size,
                     uint16_t em_slot_size,
                     uint32_t em_start_address);
```

Initializes the external memory driver.

```c
void em_reset_(void *ext_m);
```

Resets the external memory by erasing all sectors.

```c
void em_init_(void *ext_m);
```

Scans flash and restores indexes from the buffer sector and slot read marks.

```c
void add_slot(void* ext_m, uint8_t *slot_ptr);
```

Adds a new slot to the external memory.

```c
int32_t read_slot(void* ext_m, uint8_t* out_buffer);
```

Reads a slot from the external memory into the provided buffer.

```c
int32_t discard_slot(void* ext_m);
```

Discards a slot that has been read from the external memory.

```c
int32_t recover_slot(void* ext_m);
```

Recovers a slot that has been discarded from the external memory.

```c
int32_t get_slot_count(void* ext_m);
```

Returns the total number of slots in the external memory.

## Contributing

Contributions are welcome! Please submit a pull request with your improvements or bug fixes.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Acknowledgements

- Author: Roman Garanin

Feel free to customize this README further to better fit your project's needs.

For more details on recent commits, visit [recent commits](https://github.com/Ethalon-emb/RingFileSystem/commits?per_page=5&sort=updated&order=desc).