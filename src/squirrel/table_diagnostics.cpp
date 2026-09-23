#include "kinoko/act_host.h"
#include "kinoko/diagnostics.h"
#include "kinoko/legacy_memory.hpp"
#include <cstddef>
#include <cstdint>
#include <intrin.h>
#include <cstring>

extern "C" void retdec_trace_i32(const char*, int32_t);
namespace {
using kinoko::legacy::pointer;
struct TableHeader {
    std::byte prefix[32];
    int32_t nodes, capacity, used;
};
struct TableNode {
    int32_t value_type, value_data, key_type, key_data, next;
};
struct RefTable {
    int32_t slots, used, nodes, freelist, buckets;
};
struct RefNode {
    int32_t type, data, refs, next;
};
struct StringHash { std::byte prefix[24]; int32_t hash; };
static_assert(offsetof(TableHeader, nodes) == 32 && sizeof(TableNode) == 20);
static_assert(offsetof(RefTable, buckets) == 16 && sizeof(RefNode) == 16);
volatile LONG watch_events;
uint32_t key_hash(int32_t type, int32_t data) {
    if (type == 0x08000010 && data) return static_cast<uint32_t>(pointer<StringHash>(data)->hash);
    if (type == 0x05000004) {
        float value;
        std::memcpy(&value, &data, sizeof(value));
        return static_cast<uint32_t>(static_cast<int32_t>(value));
    }
    if (type == 0x01000008 || type == 0x05000002) return static_cast<uint32_t>(data);
    return static_cast<uint32_t>(data) >> 3;
}
}
// Diagnostic traversal of the original 20-byte SQTable bucket record.
extern "C" void retdec_trace_squirrel_table_entries(const char* label, int32_t table_address) {
    if (!label || static_cast<uint32_t>(table_address) < 0x10000u) return;
    char message[640];
    __try {
        const auto& table = *pointer<TableHeader>(table_address);
        wsprintfA(message, "%s:table=0x%08lX nodes=0x%08lX capacity=%ld used=%ld",
            label, static_cast<unsigned long>(static_cast<uint32_t>(table_address)),
            static_cast<unsigned long>(static_cast<uint32_t>(table.nodes)),
            static_cast<long>(table.capacity), static_cast<long>(table.used));
        retdec_trace(message);
        if (!table.nodes || table.capacity <= 0 || table.capacity > 4096) return;
        for (int bucket = 0; bucket < table.capacity; ++bucket) {
            int32_t entry = table.nodes + static_cast<int32_t>(sizeof(TableNode)) * bucket;
            int chain_length = 0;
            while (entry && chain_length++ < table.capacity) {
                const auto& node = *pointer<TableNode>(entry);
                const char* key_text = "";
                if (node.key_type == 0x08000010 && node.key_data)
                    key_text = reinterpret_cast<const char*>(pointer<std::byte>(node.key_data) + 28);
                wsprintfA(message,
                    "%s:bucket=%ld entry=0x%08lX key=(0x%08lX,0x%08lX) text=%s value=(0x%08lX,0x%08lX) next=0x%08lX",
                    label, static_cast<long>(bucket), static_cast<unsigned long>(static_cast<uint32_t>(entry)),
                    static_cast<unsigned long>(static_cast<uint32_t>(node.key_type)),
                    static_cast<unsigned long>(static_cast<uint32_t>(node.key_data)), key_text,
                    static_cast<unsigned long>(static_cast<uint32_t>(node.value_type)),
                    static_cast<unsigned long>(static_cast<uint32_t>(node.value_data)),
                    static_cast<unsigned long>(static_cast<uint32_t>(node.next)));
                retdec_trace(message);
                entry = node.next;
            }
            if (entry) {
                retdec_trace_i32("sq-table:cycle-or-long-chain", bucket);
                retdec_trace_i32("sq-table:chain-entry", entry);
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        retdec_trace("sq-table:snapshot-fault");
    }
}
// The shared-state RefTable starts at +24 and stores 16-byte linked nodes.
extern "C" void retdec_trace_ref_watch(const char* label, int32_t shared_state,
    int32_t type, int32_t data) {
    if (!retdec_is_release_watch_data(data)) return;
    const LONG sequence = InterlockedIncrement(&watch_events);
    if (sequence > 4096) return;
    char message[256];
    __try {
        const int32_t internal = *pointer<int32_t>(data + 4);
        const int32_t vtable = *pointer<int32_t>(data);
        int32_t node_address = 0, refs = 0;
        if (shared_state) {
            const auto& table = *pointer<RefTable>(shared_state + 24);
            if (table.slots > 0 && table.slots <= 0x100000 && table.buckets) {
                const auto bucket_address = table.buckets + 4 * static_cast<int32_t>(
                    key_hash(type, data) & static_cast<uint32_t>(table.slots - 1));
                node_address = *pointer<int32_t>(bucket_address);
                while (node_address) {
                    const auto& node = *pointer<RefNode>(node_address);
                    if (node.type == type && node.data == data) { refs = node.refs; break; }
                    node_address = node.next;
                }
            }
        }
        wsprintfA(message,
            "sq-watch:%ld %s type=0x%08lX data=0x%08lX internal=%ld vtable=0x%08lX node=0x%08lX refs=%ld caller=0x%08lX",
            static_cast<long>(sequence), label ? label : "event",
            static_cast<unsigned long>(static_cast<uint32_t>(type)),
            static_cast<unsigned long>(static_cast<uint32_t>(data)), static_cast<long>(internal),
            static_cast<unsigned long>(static_cast<uint32_t>(vtable)),
            static_cast<unsigned long>(static_cast<uint32_t>(node_address)), static_cast<long>(refs),
            static_cast<unsigned long>(reinterpret_cast<uintptr_t>(_ReturnAddress())));
        retdec_trace(message);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        retdec_trace("sq-watch:fault");
    }
}
