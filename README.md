# File System Consistency Check

## Overview

**File System Consistency Check** is a command-line utility designed for Unix-like operating systems. Its primary purpose is to **check and optionally repair inconsistencies in a file system**, ensuring the integrity of the metadata. By operating on unmounted partitions, it guarantees the safety and consistency of the file system without interfering with active processes or risking data corruption.

This tool is fundamental for maintaining the health of disks and is often used by system administrators and advanced users to prevent and fix filesystem errors that can lead to data loss or system malfunctions.

## Features

- **Integrity Checking**: Scans the file system for inconsistencies and errors in metadata.
- **Repair Capabilities**: Optionally repairs detected issues to restore file system health.
- **Safety**: Operates strictly on unmounted partitions to avoid interfering with active files and processes.
- **Detailed Reporting**: Provides comprehensive information about detected inconsistencies and performed repairs.

## How It Works

The utility analyzes the file system structures, such as superblocks, inode tables, and directory entries, to identify mismatches or errors. If inconsistencies are found, the utility can fix them, depending on the user's choice. Common issues addressed include orphaned files, incorrect block allocations, and metadata mismatches.

## Usage

**Note:** You must have appropriate permissions (usually root) to run this utility on system partitions.

```bash
./fsck <options> <device>
```

- `<options>`: Various flags to control checking and repair behavior (see below).
- `<device>`: The path to the unmounted partition (e.g., `/dev/sda1`).

### Example

To check a file system without repairing:
```bash
./fsck -n /dev/sda1
```

To check and automatically repair:
```bash
./fsck -y /dev/sda1
```

### Options

- `-n` : Check only, do not repair
- `-y` : Automatically repair any detected inconsistencies
- `-v` : Verbose output

## Installation

1. **Clone the repository:**
   ```bash
   git clone https://github.com/SrejonJoy/File-System-Consistency-Check.git
   cd File-System-Consistency-Check
   ```

2. **Build the utility:**
   ```bash
   make
   ```

   *(Requires a C compiler and Make utility)*

## Project Structure

- **src/**: Contains the C source code for the utility.
- **include/**: Header files.
- **README.md**: Project documentation.
- **Makefile**: Build instructions.

## Contribution

Contributions are welcome! Please open issues for bug reports or feature requests, or submit pull requests for improvements.

## License

This project is licensed under the MIT License.

---

**Disclaimer:** Always ensure that the partition is unmounted before running consistency checks or repairs to avoid data loss or corruption.
