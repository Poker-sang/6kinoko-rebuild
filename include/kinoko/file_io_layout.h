#pragma once
#include "kinoko/file_io.h"
/* Recovered x86 layouts shared only by legacy virtual users and fixtures. */
#ifdef __cplusplus
typedef struct KinokoReaderMethods {
    KinokoArchiveReader *(__thiscall *destroy)(KinokoArchiveReader *, uint8_t);
    int32_t (__thiscall *open_string)(KinokoArchiveReader *, const void *);
    int32_t (__thiscall *open_path)(KinokoArchiveReader *, const char *);
    int32_t (__thiscall *transfer)(KinokoArchiveReader *, void *, uint32_t);
    uint32_t (__thiscall *transferred)(KinokoArchiveReader *);
    uint32_t (__thiscall *seek)(KinokoArchiveReader *, int32_t, uint32_t);
    uint32_t (__thiscall *size)(KinokoArchiveReader *);
} KinokoReaderMethods;
#else
/* MSVC C cannot express thiscall. C consumers borrow table identities only;
   all typed virtual dispatch lives in file_io.cpp. */
typedef struct KinokoReaderMethods KinokoReaderMethods;
#endif
struct KinokoArchiveReader {
    const KinokoReaderMethods *methods;
    HANDLE handle;
    DWORD transferred;
};
typedef struct KinokoPackageReader {
    KinokoArchiveReader base;
    uint32_t entry_size, entry_offset;
    /* Original starts absolute, but its virtual Seek stores a relative value.
       The guarded loader API keeps this absolute (existing compatibility). */
    uint32_t read_position;
    uint8_t xor_key, padding[3];
} KinokoPackageReader;
#ifdef __cplusplus
extern "C" {
#endif
extern const KinokoReaderMethods kinoko_file_reader_methods;
extern const KinokoReaderMethods kinoko_package_reader_methods;
extern const KinokoReaderMethods kinoko_file_writer_methods;
#ifdef __cplusplus
}
#endif
