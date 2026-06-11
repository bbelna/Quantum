# Quantum Executable Format (QXF) Specification

**Version:** 1 (Draft)

## Overview

QXF is a container format wrapping ELF32 payloads with type-specific metadata
and optional embedded resources. Two executable types share a common header:

| Extension | Type | Description |
|-----------|------|-------------|
| `.qx`     | Generic (1) | General-purpose executable, behaviour determined by flags |
| `.qapp`   | Application (2) | GUI application, may bundle icons and resources |

## File Layout

```
┌──────────────────────────────┐  offset 0
│  QXF Header (32 bytes)       │
├──────────────────────────────┤  MetadataOffset
│  Metadata Section (variable) │
├──────────────────────────────┤  ResourceTableOffset (qapp only)
│  Resource Table              │
├──────────────────────────────┤
│  Resource Data (qapp only)   │
├──────────────────────────────┤  ELFOffset
│  ELF32 Payload               │
└──────────────────────────────┘
```

## QXF Header (32 bytes, packed)

| Offset | Size | Field                | Description |
|--------|------|----------------------|-------------|
| 0      | 4    | `Magic`              | `"QXF\0"` (`0x51 0x58 0x46 0x00`) |
| 4      | 2    | `Version`            | Format version (initially `1`) |
| 6      | 1    | `Type`               | `1` = Generic, `2` = Application |
| 7      | 1    | `Flags`              | Bitfield (see below) |
| 8      | 4    | `ELFOffset`          | Byte offset from file start to ELF32 payload |
| 12     | 4    | `ELFSize`            | Size of the ELF32 payload in bytes |
| 16     | 4    | `MetadataOffset`     | Byte offset to metadata section (`0` if none) |
| 20     | 4    | `MetadataSize`       | Size of metadata section in bytes |
| 24     | 4    | `ResourceTableOffset`| Byte offset to resource table (`0` if none) |
| 28     | 2    | `ResourceCount`      | Number of resource entries |
| 30     | 2    | `Reserved`           | Must be `0` |

### Flags

| Bit | Name           | Meaning |
|-----|----------------|---------|
| 0   | `NeedsDisplay` | Executable will create windows (compositor hint) |
| 1   | `Privileged`   | Executable requests elevated privilege |
| 2–7 | Reserved       | Must be `0` |

## Inherited Streams

### Motivation

Previous design passed stream descriptors as `argv[1]` strings (`"stream:N"`).
Every command had to parse this manually. The new design mirrors Linux/POSIX:
streams are inherited from the parent at spawn time and pre-initialized by the
runtime before `Main()` executes.

### SpawnParameters Extension

```
SpawnParameters {
  SourceBase        : UIntPtr       (existing)
  SizeInBytes       : Size          (existing)
  TargetBase        : UIntPtr       (existing)
  SegmentCount      : UInt32        (existing)
  Segments          : SpawnSegment[](existing)
  StreamCount       : UInt8         (NEW - 0–3)
  StreamBufferIDs   : SharedBufferID[3] (NEW - 0=stdin, 1=stdout, 2=stderr)
}
```

### Process Stream Table

When `StreamCount > 0`, the kernel writes buffer IDs into a fixed-address page
in the child's address space (`0x0FFFF000`), readable by the child:

```
ProcessStreamTable {
  Count    : UInt8
  Reserved : UInt8[3]
  Entries  : SharedBufferID[3]   // [0]=stdin, [1]=stdout, [2]=stderr
}
```

### Runtime Auto-Initialization

CRT0 checks the process stream table before calling `Main()`. If populated, it
initializes global stream handles:

```cpp
namespace Quantum::Streaming {
  extern Stream StandardIn;
  extern Stream StandardOut;
  extern Stream StandardError;
}
```

These are valid immediately when `Main()` executes. If no streams were inherited
(qapp, bare qx), `IsValid()` returns `false`.

### Parent-Side Flow

```
1. Read QXF header
2. If Type == Generic (.qx):
     a. StreamClient::CreateStream() -> bufferID
     b. StreamHost::AttachToBuffer(bufferID)
     c. params.StreamCount = 3
     d. params.StreamBufferIDs = {0, bufferID, bufferID}
3. Spawn via RunClient with stream params
4. If Type == Generic: drain StreamHost until closed
5. If Type == Application: fire-and-forget
```

## Metadata Section

Flat key-value table. Each entry is packed:

| Offset | Size | Field   | Description |
|--------|------|---------|-------------|
| 0      | 2    | `Key`   | Metadata key ID |
| 2      | 2    | `Length` | Value length in bytes |
| 4      | N    | `Value` | Raw bytes (null-terminated for strings) |

Entries are consecutive, padded to 4-byte alignment.

### Metadata Keys

| Key | Name           | Type   | Applies To | Description |
|-----|----------------|--------|------------|-------------|
| 1   | `DisplayName`  | string | qapp, qx   | Human-readable name |
| 2   | `Version`      | string | all        | Semantic version string |
| 3   | `Author`       | string | all        | Author or vendor |
| 4   | `Category`     | UInt16 | qapp       | App category (Utility=1, System=2, Game=3, …) |
| 5   | `Description`  | string | all        | One-line description |
| 6   | `UsageSummary` | string | qx         | Short usage string for `help` |
| 7   | `MinOSVersion` | UInt32 | all        | Minimum required OS version |

## Resource Table (qapp only)

Each entry is 44 bytes:

| Offset | Size | Field       | Description |
|--------|------|-------------|-------------|
| 0      | 2    | `Type`      | Resource type |
| 2      | 2    | `SubType`   | Type-specific qualifier |
| 4      | 4    | `DataOffset`| Byte offset from file start |
| 8      | 4    | `DataSize`  | Resource size in bytes |
| 12     | 32   | `Name`      | Null-terminated resource name |

### Resource Types

| Type | Name     | SubType meaning |
|------|----------|-----------------|
| 1    | `Icon`   | `1`=16×16, `2`=32×32, `3`=48×48, `4`=64×64 |
| 2    | `Bitmap` | `0` = raw BGRA pixel data |
| 3    | `String` | Locale ID (`0` = default) |
| 4    | `Font`   | `0` = default |
| 5    | `Raw`    | Application-defined |

### Runtime Resource Access

```cpp
Size App::ReadResource(UInt16 type, UInt16 subType, void* outBuffer, Size bufferSize);
```

The app re-opens its own executable (path from `argv[0]`), reads the QXF header
to locate the resource table, and extracts the requested resource.

## Backward Compatibility

The Run server checks the first 4 bytes of any executable:

- `QXF\0` -> parse QXF header, extract ELF at `ELFOffset`
- `\x7FELF` -> legacy path, load as raw ELF (implicit qx, no streams)

Existing ELF32 binaries continue to work without modification.

## Validation

1. Magic matches `"QXF\0"`
2. Version is supported
3. Type is valid (1–2)
4. `ELFOffset + ELFSize` ≤ file size
5. ELF payload passes `Run::Validate()`
6. If `ResourceTableOffset ≠ 0`, table fits within file bounds
7. All resource `DataOffset + DataSize` fit within file bounds
8. No section overlaps

## Toolchain: `qxf-pack`

Python tool (`Toolchain/QXF/`) taking an ELF32 binary, optional manifest JSON,
and optional resource files, producing the packed `.qx` / `.qapp`.

### Example qapp manifest

```json
{
  "type": "app",
  "displayName": "About",
  "version": "1.0.0",
  "author": "Quantum Project",
  "category": "system",
  "icons": {
    "16": "icons/about-16.raw",
    "32": "icons/about-32.raw"
  }
}
```

### Example qx command manifest

```json
{
  "type": "qx",
  "displayName": "ls",
  "version": "1.0.0",
  "usageSummary": "ls [path] - list directory contents"
}
```
