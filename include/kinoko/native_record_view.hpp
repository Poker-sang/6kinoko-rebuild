#pragma once
#include <cstddef>
#include <cstring>
#include <type_traits>

namespace kinoko::native {
// A non-owning, alignment-independent view of an existing native byte record.
// The schema describes layout only; it is never constructed over malloc or C
// storage. Squirrel objects are accessed by ObjectView/the public API instead.
template<class Record>
class RecordView final {
    static_assert(std::is_standard_layout_v<Record>);
    static_assert(std::is_trivially_copyable_v<Record>);
    unsigned char* storage_;
    inline static const Record layout_{};

    template<class Member>
    static std::size_t offset(Member Record::* member) noexcept {
        // Compute a real member's displacement in a real schema object. No
        // null-object member access, integer address guessing or type-punning.
        return static_cast<std::size_t>(
            reinterpret_cast<const unsigned char*>(&(layout_.*member)) -
            reinterpret_cast<const unsigned char*>(&layout_));
    }
public:
    explicit RecordView(void* storage) noexcept
        : storage_(static_cast<unsigned char*>(storage)) {}
    unsigned char* data() const noexcept { return storage_; }
    template<class Member>
    unsigned char* bytes(Member Record::* member) const noexcept {
        return storage_ + offset(member);
    }
    template<class Member>
    Member get(Member Record::* member) const noexcept {
        static_assert(std::is_trivially_copyable_v<Member>);
        Member value;
        std::memcpy(&value, bytes(member), sizeof(value));
        return value;
    }
    template<class Member>
    void set(Member Record::* member, const Member& value) const noexcept {
        static_assert(std::is_trivially_copyable_v<Member>);
        std::memcpy(bytes(member), &value, sizeof(value));
    }
    template<class Member>
    RecordView<Member> view(Member Record::* member) const noexcept {
        return RecordView<Member>(bytes(member));
    }
    Record load() const noexcept {
        Record value;
        std::memcpy(&value, storage_, sizeof(value));
        return value;
    }
    void clear() const noexcept { std::memset(storage_, 0, sizeof(Record)); }
};
}
