#pragma once
#include <windows.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct KinokoArchiveReader KinokoArchiveReader;
/* Mount count chooses the type only when opening a new reader. */
extern int32_t kinoko_archive_count;
void kinoko_archive_initialize(void);
int32_t kinoko_archive_mount(const char *path);
int32_t kinoko_archive_insert(const char *path, uint32_t archive, uint32_t offset, uint32_t size);
HANDLE kinoko_archive_open_entry(const char *path, uint32_t *offset, uint32_t *size);
int32_t kinoko_reader_open(KinokoArchiveReader **slot, const char *path);
int32_t kinoko_writer_open(KinokoArchiveReader **slot, const char *path);
void kinoko_reader_close(KinokoArchiveReader *reader);
uint32_t kinoko_reader_size(KinokoArchiveReader *reader);
int32_t kinoko_reader_read(KinokoArchiveReader *reader, void *data, uint32_t size);
int32_t kinoko_writer_write(KinokoArchiveReader *writer, const void *data, uint32_t size);
uint32_t kinoko_reader_seek(KinokoArchiveReader *reader, int32_t distance, uint32_t origin);
/* Existing validated-loader contract: reject short reads / out-of-entry skips.
   Distinct from the original virtual stream methods, which allow partial reads. */
int32_t kinoko_reader_read_exact(KinokoArchiveReader *reader, void *data, uint32_t size);
int32_t kinoko_reader_seek_relative(KinokoArchiveReader *reader, uint32_t distance);
#ifdef __cplusplus
}
#endif
