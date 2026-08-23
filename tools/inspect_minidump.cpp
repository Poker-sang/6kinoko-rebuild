#include <windows.h>
#include <dbghelp.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

struct MappedDump {
    HANDLE file;
    HANDLE mapping;
    unsigned char *base;
    std::uint64_t size;
};

static bool dump_range(const MappedDump *dump, std::uint64_t offset,
                       std::uint64_t size, const void **result)
{
    if (offset > dump->size || size > dump->size - offset) {
        return false;
    }
    *result = dump->base + offset;
    return true;
}

static bool stream_data(const MappedDump *dump, ULONG stream_number,
                        void **stream, ULONG *stream_size)
{
    MINIDUMP_DIRECTORY *directory = nullptr;
    PVOID data = nullptr;
    ULONG size = 0;
    if (!MiniDumpReadDumpStream(dump->base, stream_number, &directory,
                                &data, &size)) {
        return false;
    }
    *stream = data;
    *stream_size = size;
    return true;
}

static const char *module_for_address(const MINIDUMP_MODULE_LIST *modules,
                                      std::uint64_t address,
                                      const MappedDump *dump)
{
    static char name[512];
    for (ULONG index = 0; index < modules->NumberOfModules; ++index) {
        const MINIDUMP_MODULE &module = modules->Modules[index];
        const std::uint64_t start = module.BaseOfImage;
        const std::uint64_t end = start + module.SizeOfImage;
        if (address < start || address >= end) {
            continue;
        }
        const void *raw_name = nullptr;
        if (!dump_range(dump, module.ModuleNameRva, sizeof(ULONG), &raw_name)) {
            return "<module>";
        }
        const ULONG byte_length = *static_cast<const ULONG *>(raw_name);
        const void *raw_buffer = nullptr;
        if (!dump_range(dump, module.ModuleNameRva + sizeof(ULONG),
                        byte_length, &raw_buffer)) {
            return "<module>";
        }
        const wchar_t *wide_name = static_cast<const wchar_t *>(raw_buffer);
        int converted = WideCharToMultiByte(CP_ACP, 0, wide_name,
                                            static_cast<int>(byte_length / sizeof(wchar_t)),
                                            name, sizeof(name) - 1, nullptr, nullptr);
        if (converted <= 0) {
            return "<module>";
        }
        name[converted] = '\0';
        return name;
    }
    return nullptr;
}

static bool read_dump_memory64(const MINIDUMP_MEMORY64_LIST *memory,
                               const MappedDump *dump, std::uint64_t address,
                               void *buffer, std::size_t size)
{
    std::uint64_t data_offset = memory->BaseRva;
    for (ULONG64 index = 0; index < memory->NumberOfMemoryRanges; ++index) {
        const MINIDUMP_MEMORY_DESCRIPTOR64 &range = memory->MemoryRanges[index];
        const std::uint64_t start = range.StartOfMemoryRange;
        const std::uint64_t end = start + range.DataSize;
        if (address >= start && address <= end && size <= end - address) {
            const std::uint64_t offset = data_offset + (address - start);
            const void *source = nullptr;
            if (!dump_range(dump, offset, size, &source)) {
                return false;
            }
            std::memcpy(buffer, source, size);
            return true;
        }
        data_offset += range.DataSize;
    }
    return false;
}

static bool read_dump_memory32(const MINIDUMP_MEMORY_LIST *memory,
                               const MappedDump *dump, std::uint64_t address,
                               void *buffer, std::size_t size)
{
    for (ULONG index = 0; index < memory->NumberOfMemoryRanges; ++index) {
        const MINIDUMP_MEMORY_DESCRIPTOR &range = memory->MemoryRanges[index];
        const std::uint64_t start = range.StartOfMemoryRange;
        const std::uint64_t end = start + range.Memory.DataSize;
        if (address >= start && address <= end && size <= end - address) {
            const void *source = nullptr;
            if (!dump_range(dump, range.Memory.Rva + (address - start),
                            size, &source)) {
                return false;
            }
            std::memcpy(buffer, source, size);
            return true;
        }
    }
    return false;
}

struct MemoryReader {
    const MINIDUMP_MEMORY64_LIST *memory64;
    const MINIDUMP_MEMORY_LIST *memory32;
    const MappedDump *dump;
};

static bool read_memory(const MemoryReader *reader, std::uint64_t address,
                        void *buffer, std::size_t size)
{
    if (reader->memory64 != nullptr) {
        return read_dump_memory64(reader->memory64, reader->dump, address,
                                  buffer, size);
    }
    if (reader->memory32 != nullptr) {
        return read_dump_memory32(reader->memory32, reader->dump, address,
                                  buffer, size);
    }
    return false;
}

static const char *symbol_for_address(HANDLE process, DWORD64 address,
                                      DWORD64 *displacement)
{
    static unsigned char storage[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
    SYMBOL_INFO *symbol = reinterpret_cast<SYMBOL_INFO *>(storage);
    std::memset(storage, 0, sizeof(storage));
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;
    if (!SymFromAddr(process, address, displacement, symbol)) {
        return nullptr;
    }
    return symbol->Name;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: inspect_minidump <dump> [module.exe]\n");
        return 2;
    }

    MappedDump dump = {};
    dump.file = CreateFileA(argv[1], GENERIC_READ, FILE_SHARE_READ, nullptr,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (dump.file == INVALID_HANDLE_VALUE) {
        std::perror("CreateFile");
        return 1;
    }
    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(dump.file, &file_size)) {
        return 1;
    }
    dump.size = static_cast<std::uint64_t>(file_size.QuadPart);
    dump.mapping = CreateFileMappingA(dump.file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    dump.base = static_cast<unsigned char *>(MapViewOfFile(dump.mapping, FILE_MAP_READ, 0, 0, 0));
    if (dump.base == nullptr) {
        std::fprintf(stderr, "MapViewOfFile failed: %lu\n", GetLastError());
        return 1;
    }

    void *exception_data = nullptr;
    ULONG exception_size = 0;
    if (!stream_data(&dump, ExceptionStream, &exception_data, &exception_size) ||
        exception_size < sizeof(MINIDUMP_EXCEPTION_STREAM)) {
        std::fprintf(stderr, "exception stream unavailable\n");
        return 1;
    }
    const MINIDUMP_EXCEPTION_STREAM *exception =
        static_cast<const MINIDUMP_EXCEPTION_STREAM *>(exception_data);
    std::printf("exception code=0x%08lx address=0x%08llx thread=%lu\n",
                exception->ExceptionRecord.ExceptionCode,
                static_cast<unsigned long long>(exception->ExceptionRecord.ExceptionAddress),
                exception->ThreadId);

    const void *context_data = nullptr;
    if (!dump_range(&dump, exception->ThreadContext.Rva,
                    exception->ThreadContext.DataSize, &context_data) ||
        exception->ThreadContext.DataSize < sizeof(CONTEXT)) {
        std::fprintf(stderr, "thread context unavailable\n");
        return 1;
    }
    const CONTEXT *context = static_cast<const CONTEXT *>(context_data);
    std::printf("context eip=0x%08lx esp=0x%08lx ebp=0x%08lx eax=0x%08lx ebx=0x%08lx ecx=0x%08lx edx=0x%08lx esi=0x%08lx edi=0x%08lx\n",
                context->Eip, context->Esp, context->Ebp, context->Eax,
                context->Ebx, context->Ecx, context->Edx, context->Esi,
                context->Edi);

    void *memory64_data = nullptr;
    ULONG memory64_size = 0;
    void *memory32_data = nullptr;
    ULONG memory32_size = 0;
    MINIDUMP_MEMORY64_LIST *memory64 = nullptr;
    MINIDUMP_MEMORY_LIST *memory32 = nullptr;
    if (stream_data(&dump, Memory64ListStream, &memory64_data, &memory64_size)) {
        memory64 = static_cast<MINIDUMP_MEMORY64_LIST *>(memory64_data);
    } else if (stream_data(&dump, MemoryListStream, &memory32_data, &memory32_size)) {
        memory32 = static_cast<MINIDUMP_MEMORY_LIST *>(memory32_data);
    }
    MemoryReader reader = {memory64, memory32, &dump};

    void *module_data = nullptr;
    ULONG module_size = 0;
    if (!stream_data(&dump, ModuleListStream, &module_data, &module_size)) {
        std::fprintf(stderr, "module stream unavailable\n");
        return 1;
    }
    const MINIDUMP_MODULE_LIST *modules =
        static_cast<const MINIDUMP_MODULE_LIST *>(module_data);
    HANDLE symbol_process = GetCurrentProcess();
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES);
    // The exception often belongs to a system DLL. Resolve the executable's
    // own relocated base instead of reusing the exception module's base.
    DWORD64 image_base = 0x400000;
    for (ULONG index = 0; index < modules->NumberOfModules; ++index) {
        const MINIDUMP_MODULE &module = modules->Modules[index];
        if (module.BaseOfImage >= 0x00400000 &&
            module.BaseOfImage < 0x00600000) {
            image_base = module.BaseOfImage;
            break;
        }
    }
    SymInitialize(symbol_process, argc >= 3 ? argv[2] : nullptr, FALSE);
    if (argc >= 3) {
        SymLoadModuleEx(symbol_process, nullptr, argv[2], nullptr, image_base,
                        0, nullptr, 0);
    }

    auto print_address = [&](std::uint64_t address) {
        const char *module = module_for_address(modules, address, &dump);
        DWORD64 displacement = 0;
        const char *symbol = symbol_for_address(symbol_process, address, &displacement);
        if (module != nullptr) {
            std::uint64_t image_offset = address;
            for (ULONG index = 0; index < modules->NumberOfModules; ++index) {
                const MINIDUMP_MODULE &candidate = modules->Modules[index];
                if (address >= candidate.BaseOfImage &&
                    address < candidate.BaseOfImage + candidate.SizeOfImage) {
                    image_offset = address - candidate.BaseOfImage;
                    break;
                }
            }
            std::printf("0x%08llx %s+0x%llx",
                        static_cast<unsigned long long>(address), module,
                        static_cast<unsigned long long>(image_offset));
        } else {
            std::printf("0x%08llx", static_cast<unsigned long long>(address));
        }
        if (symbol != nullptr) {
            std::printf(" [%s+0x%llx]", symbol,
                        static_cast<unsigned long long>(displacement));
        }
        std::printf("\n");
    };

    std::printf("exception address detail: ");
    print_address(exception->ExceptionRecord.ExceptionAddress);
    std::printf("stack words near esp:\n");
    for (std::uint32_t offset = 0; offset < 0x400; offset += 4) {
        std::uint32_t value = 0;
        if (!read_memory(&reader, static_cast<std::uint64_t>(context->Esp) + offset,
                         &value, sizeof(value))) {
            break;
        }
        if ((value >= 0x00400000u && value < 0x00600000u) ||
            (value >= 0x70000000u && value < 0x78000000u)) {
            std::printf("  esp+0x%03x = 0x%08lx: ", offset, value);
            print_address(value);
        }
    }

    std::printf("ebp chain:\n");
    std::uint32_t frame = context->Ebp;
    for (int depth = 0; depth < 32 && frame != 0; ++depth) {
        std::uint32_t previous = 0;
        std::uint32_t return_address = 0;
        if (!read_memory(&reader, frame, &previous, sizeof(previous)) ||
            !read_memory(&reader, static_cast<std::uint64_t>(frame) + 4,
                         &return_address, sizeof(return_address))) {
            break;
        }
        std::printf("  #%02d frame=0x%08lx return=", depth, frame);
        print_address(return_address);
        if (previous <= frame || previous - frame > 0x100000u) {
            break;
        }
        frame = previous;
    }

    SymCleanup(symbol_process);
    UnmapViewOfFile(dump.base);
    CloseHandle(dump.mapping);
    CloseHandle(dump.file);
    return 0;
}
