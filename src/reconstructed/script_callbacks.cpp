#include "kinoko/script_callbacks.h"

#include <cstddef>

extern "C" {
extern char *g644;
extern int32_t g600[3], g601[3], g602[3];
int32_t *function_4a9500_this(int32_t *destination, int32_t source);
int32_t function_4a95c0_this(int32_t destination, int32_t source);
int32_t function_4a9d70_this(int32_t object);
int32_t function_4a9570_this(int32_t object);
int32_t function_4a97b0_this(int32_t object, int32_t key, int32_t value);
int32_t function_4a94e0_this(int32_t object);
int32_t retdec_function_45df10_impl(int32_t destination, int32_t name);
int32_t retdec_actor_step_callback(int32_t callback);
void retdec_trace(const char *message);
void retdec_trace_i32(const char *message, int32_t value);
}

namespace {
struct ScriptObject {
    int32_t vtable;
    int32_t type;
    int32_t value;
};

struct ScriptCallback {
    int32_t vm;
    ScriptObject environment;
    ScriptObject function;
};

static_assert(sizeof(void *) == 4, "Callbacks require the original Win32 ABI.");
static_assert(sizeof(ScriptObject) == 12);
static_assert(offsetof(ScriptObject, type) == 4);
static_assert(offsetof(ScriptObject, value) == 8);
static_assert(sizeof(ScriptCallback) == 28);
static_assert(offsetof(ScriptCallback, environment) == 4);
static_assert(offsetof(ScriptCallback, function) == 16);

int32_t address(const void *pointer) {
    return static_cast<int32_t>(reinterpret_cast<uintptr_t>(pointer));
}

template <typename T>
T &at(int32_t base, uint32_t offset = 0) {
    return *reinterpret_cast<T *>(static_cast<uintptr_t>(
        static_cast<uint32_t>(base) + offset));
}

// SqPlus uses sq_addref/sq_release and the VM's external reference table.
// It must not be replaced by SQObjectPtr's internal retain/release operators.
class LocalObject {
public:
    LocalObject() = default;
    explicit LocalObject(const ScriptObject &source) {
        function_4a9500_this(reinterpret_cast<int32_t *>(&value_), address(&source));
    }
    LocalObject(int32_t vtable, int32_t type, int32_t value)
        : value_{vtable, type, value} {}
    LocalObject(const LocalObject &) = delete;
    LocalObject &operator=(const LocalObject &) = delete;
    ~LocalObject() { if (!released_) release(); }

    const ScriptObject &value() const { return value_; }
    void copy_from(const ScriptObject &source) {
        function_4a9500_this(reinterpret_cast<int32_t *>(&value_), address(&source));
    }
    int32_t release() {
        released_ = true;
        return function_4a9d70_this(address(&value_));
    }

private:
    ScriptObject value_{};
    bool released_ = false;
};

void assign(ScriptCallback &destination, int32_t vm,
            const ScriptObject &environment, const ScriptObject &function) {
    destination.vm = vm;
    function_4a95c0_this(address(&destination.environment), address(&environment));
    function_4a95c0_this(address(&destination.function), address(&function));
}

void bind(ScriptCallback &destination, const ScriptObject &environment,
          const ScriptObject &function) {
    LocalObject saved_environment(environment);
    LocalObject saved_function(function);
    assign(destination, address(g644), saved_environment.value(), saved_function.value());
}

void clear(ScriptCallback &callback) {
    ScriptCallback empty{};
    retdec_function_45df10_impl(address(&empty), 0);
    assign(callback, empty.vm, empty.environment, empty.function);
    function_4a9d70_this(address(&empty.function));
    function_4a9d70_this(address(&empty.environment));
}

constexpr int32_t script_closure_type = 0x08000100;
constexpr uint32_t actor_object_offset = 44;
constexpr uint32_t actor_update_offset = 92;
constexpr uint32_t actor_collision_offset = 120;
constexpr uint32_t camera_update_offset = 12;
}

// 45E120/45E180: call the current update; a failed call clears the callback.
// Keep original error handling while investigating the preceding runtime fault.
extern "C" int32_t kinoko_actor_step_callback(int32_t actor) {
    auto &current = at<ScriptCallback>(actor, actor_update_offset);
    const int32_t result = retdec_actor_step_callback(address(&current));
    if (result < 0)
        clear(current);
    return result;
}

// Original 45FB90: clear native callbacks, reset Actor CLASS defaults, then
// release the native owner's instance reference. Existing script instances
// retain their own user/step fields until their remaining references expire.
extern "C" int32_t kinoko_actor_clear_script(int32_t actor) {
    auto &instance = at<ScriptObject>(actor, actor_object_offset);
    if (instance.type == 0x0a008000) {
        clear(at<ScriptCallback>(actor, actor_update_offset));
        clear(at<ScriptCallback>(actor, actor_collision_offset));
        ScriptObject empty{};
        function_4a94e0_this(address(&empty));
        // 45FC7A and 45FC94 both load ECX=513C58 (g602), not actor+44.
        function_4a97b0_this(address(g602), address(g601), address(&empty));
        function_4a97b0_this(address(g602), address(g600), address(&empty));
        function_4a9d70_this(address(&empty));
    }
    return function_4a9570_this(address(&instance));
}

// 45FCD0: Actor.SetUpdateFunction.
extern "C" int32_t __fastcall kinoko_actor_set_update_callback(
    int32_t actor, void *, int32_t vtable, int32_t type, int32_t value) {
    LocalObject incoming(vtable, type, value);
    bind(at<ScriptCallback>(actor, actor_update_offset),
         at<ScriptObject>(actor, actor_object_offset), incoming.value());
    return incoming.release();
}

// 45FD80: non-script closures select the original empty SquirrelFunction.
extern "C" int32_t __fastcall kinoko_actor_set_collision_callback(
    int32_t actor, void *, int32_t vtable, int32_t type, int32_t value) {
    LocalObject incoming(vtable, type, value);
    {
        LocalObject environment;
        LocalObject function;
        auto &destination = at<ScriptCallback>(actor, actor_collision_offset);
        if (type == script_closure_type) {
            environment.copy_from(at<ScriptObject>(actor, actor_object_offset));
            function.copy_from(incoming.value());
            assign(destination, address(g644), environment.value(), function.value());
        } else {
            ScriptCallback empty{};
            retdec_function_45df10_impl(address(&empty), 0);
            assign(destination, empty.vm, empty.environment, empty.function);
            function_4a9d70_this(address(&empty.function));
            function_4a9d70_this(address(&empty.environment));
        }
        // Keep the existing compatibility cleanup of both empty temporaries.
    }
    return incoming.release();
}

// 4663C0: Camera.SetUpdateFunction uses the same binding with different offsets.
extern "C" int32_t __fastcall kinoko_camera_set_update_callback(
    int32_t camera, void *, int32_t vtable, int32_t type, int32_t value) {
    if (!camera)
        return 0;
    LocalObject incoming(vtable, type, value);
    retdec_trace("4663c0:begin");
    retdec_trace_i32("4663c0:this", camera);
    retdec_trace_i32("4663c0:argument-type", vtable);
    retdec_trace_i32("4663c0:argument-data", type);
    retdec_trace_i32("4663c0:argument-aux", value);
    bind(at<ScriptCallback>(camera, camera_update_offset),
         at<ScriptObject>(camera), incoming.value());
    incoming.release();
    retdec_trace("4663c0:end");
    return 0;
}

// 466470: native closures and empty callbacks are not dispatched here.
extern "C" int32_t __fastcall kinoko_camera_update(int32_t camera, void *) {
    const auto &callback = at<ScriptCallback>(camera, camera_update_offset);
    if (callback.function.type != script_closure_type)
        return callback.function.type;
    return retdec_actor_step_callback(address(&callback));
}
