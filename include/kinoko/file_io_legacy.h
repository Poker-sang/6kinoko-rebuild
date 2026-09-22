#pragma once
#include "kinoko/file_io.h"
/* Integer-address adapters only for not-yet-migrated serialized parser ports. */
static inline int32_t retdec_reader_read_exact(int32_t reader, void *data, uint32_t size) {
    return kinoko_reader_read_exact((KinokoArchiveReader *)(intptr_t)reader, data, size);
}
static inline int32_t retdec_reader_seek_relative(int32_t reader, uint32_t distance) {
    return kinoko_reader_seek_relative((KinokoArchiveReader *)(intptr_t)reader, distance);
}
