#include "kinoko/squirrel_api_types.h"
#include "kinoko/csv_bridge.h"
#include <windows.h>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqtable.h"
#include "sqstring.h"

extern "C" {
int32_t function_48a600(int32_t vm);
extern char *g644, *g767;
extern char g874;
void*  kinoko_sqplus_object_destroy(void * object);
int32_t kinoko_csv_load_bytes(const char *path, char **bytes);
}
namespace {
using Row = std::vector<std::string>;
using Rows = std::vector<Row>;
template<class T> T &at(int32_t address) {
    return *reinterpret_cast<T *>(static_cast<uintptr_t>(static_cast<uint32_t>(address)));
}
// Original 40C010. This format is deliberately not RFC CSV: commas split even
// inside quotes; quotes preserve newlines; # suppresses input until an unquoted
// newline. Only a newline commits a row (no implicit final row at EOF).
Rows parse(const char *text) {
    Rows rows;
    Row row;
    std::string field;
    bool comment = false, quoted = false;
    for (const char *cursor = text; *cursor;) {
        const char ch = *cursor;
        switch (ch) {
        case '"': quoted = !quoted; ++cursor; break;
        case '#': comment = true; ++cursor; break;
        case ',':
            if (!comment) { row.push_back(field); field.clear(); }
            ++cursor; break;
        case '\r':
            if (quoted && !comment) field += ch;
            ++cursor; break;
        case '\n':
            if (quoted) { if (!comment) field += ch; }
            else {
                if (!comment) {
                    row.push_back(field); field.clear();
                    rows.push_back(row); row.clear();
                }
                comment = false;
            }
            ++cursor; break;
        default:
            const char *next = CharNextA(cursor);
            if (!comment) field.append(cursor, next - cursor);
            cursor = next; break;
        }
    }
    return rows;
}
const char *cell(const Rows &rows, size_t row, size_t column) {
    return row < rows.size() && column < rows[row].size() ? rows[row][column].c_str() : "";
}
// 40C4B0 uses the original 256-byte secure-copy buffer for textual fields.
std::string text_cell(const Rows &rows, size_t row, size_t column) {
    char buffer[256];
    strcpy_s(buffer, sizeof buffer, cell(rows, row, column));
    return buffer;
}
SQObjectPtr string(SQVM &vm, const std::string &text) {
    return SQObjectPtr(SQString::Create(_ss(&vm), text.c_str()));
}
struct WrapperOwner {
    int32_t *object;
    ~WrapperOwner() { (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(static_cast<int32_t>(reinterpret_cast<uintptr_t>(object))))); }
};
}
extern "C" int32_t kinoko_csv_populate(int32_t address, const char *text, const int32_t object[2]) {
    SQVM &vm = at<SQVM>(address);
    SQObjectPtr table = *reinterpret_cast<const SQObject *>(object);
    if (type(table) != OT_TABLE) return 1;
    const auto rows = parse(text);
    std::vector<SQObjectPtr> columns;
    std::vector<char> types;
    for (size_t i = 1;; ++i) {
        auto name = text_cell(rows, 0, i);
        if (name.empty()) break;
        columns.push_back(string(vm, name));
    }
    for (size_t i = 1;; ++i) {
        auto name = text_cell(rows, 1, i);
        if (name.empty()) break;
        types.push_back(name[0]);
    }
    if (types.size() != columns.size()) return 2;
    for (size_t r = 2; r < rows.size(); ++r) {
        auto key = text_cell(rows, r, 0);
        if (key.empty()) continue;
        // Keep the game's table vtable so its GC recognizes the new row.
        sq_newtable(kinoko_vm(address));
        SQObjectPtr row = vm.GetUp(-1);
        vm.Pop();
        for (size_t c = 0; c < columns.size(); ++c) {
            SQObjectPtr value;
            switch (types[c]) {
            case 'i': value = static_cast<SQInteger>(std::atoi(cell(rows, r, c + 1))); break;
            case 'f': value = static_cast<SQFloat>(std::atof(cell(rows, r, c + 1))); break;
            case 'b': {
                auto token = text_cell(rows, r, c + 1);
                value = SQObjectPtr(!token.empty() && token[0] == 't'); break;
            }
            default: value = string(vm, text_cell(rows, r, c + 1)); break;
            }
            // 4A97B0 uses sq_rawset, so no _newslot metamethod is invoked.
            _table(row)->NewSlot(columns[c], value);
        }
        _table(table)->NewSlot(string(vm, key), row);
    }
    return 0;
}
extern "C" int32_t kinoko_read_csv(int32_t vm, int32_t window, const char *path,
    int32_t object[3], int32_t encoded) {
    WrapperOwner owner{object}; // consume the original by-value SquirrelObject
    const char *error = nullptr;
    if (object[1] != OT_TABLE) error = "SqReadCSV-ObjectNotTable";
    else {
        std::string filename(path);
        if (encoded && filename.size() >= 4) filename.replace(filename.size() - 4, 4, ".cv1");
        char *raw = nullptr;
        if (!kinoko_csv_load_bytes(filename.c_str(), &raw)) error = "SqReadCSV-FileNotFound";
        else {
            std::unique_ptr<char, decltype(&std::free)> bytes(raw, &std::free);
            // The byte transform is performed by the reader bridge, which
            // knows the exact file length (including embedded zero bytes).
            if (kinoko_csv_populate(vm, raw, object + 1) == 2) error = "SqReadCSV-DefineError";
        }
    }
    if (!error) return 1;
    MessageBoxA(reinterpret_cast<HWND>(static_cast<uintptr_t>(static_cast<uint32_t>(window))), path, error, 0);
    return 0;
}

// Original 403000: ReadCSV receives the path and a by-value SqPlus object.
// Keep the three-word object and borrowed primary VM/window at this boundary.
extern "C" int32_t kinoko_script_read_csv(const char* path, int32_t vtable,
    int32_t type, int32_t data) {
    int32_t object[3] = {vtable, type, data};
    return kinoko_read_csv(static_cast<int32_t>(reinterpret_cast<uintptr_t>(g644)),
        static_cast<int32_t>(reinterpret_cast<uintptr_t>(g767)), path, object, g874 != 0);
}
