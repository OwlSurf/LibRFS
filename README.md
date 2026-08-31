# RingFileSystem

Ring file system implementation.

## Overview

The RingFileSystem is a C-based implementation designed to manage external memory using a ring buffer mechanism. It allows for efficient use of memory by overwriting old data when new data is written.

**Illustrated guide (RU):** [docs/ILLUSTRATED_GUIDE.ru.md](docs/ILLUSTRATED_GUIDE.ru.md) — architecture diagrams, overflow behavior, recover/discard lifecycle.

## Features

- Configurable buffer size, sector size, and slot size.
- Customizable functions for sector erasing, data writing, and data reading.
- Efficient memory management with automatic sector erasing on buffer overflow.

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
    void* em_driver_init(void* pp_sector_erase,
                         void* pp_read,
                         void* pp_write,
                         uint8_t* sector_read_buffer,
                         uint8_t* sector_write_buffer,
                         uint32_t em_size,
                         uint16_t em_sector_size,
                         uint16_t em_slot_size,
                         uint32_t em_start_address);
  ```
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
void* em_driver_init(void* pp_sector_erase,
                     void* pp_read,
                     void* pp_write,
                     uint8_t* sector_read_buffer,
                     uint8_t* sector_write_buffer,
                     uint32_t em_size,
                     uint16_t em_sector_size,
                     uint16_t em_slot_size,
                     uint32_t em_start_address);
```

Initializes the external memory driver.

```c
void em_reset(void *ext_m);
```

Resets the external memory by erasing all sectors.

```c
void em_init(void *ext_m);
```

Initializes the external memory by reading timestamps from sectors.

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