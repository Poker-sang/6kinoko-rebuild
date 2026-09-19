REPLACEMENTS = {}
def body(name, text): REPLACEMENTS[name] = text.strip()+'\n'
body('retdec_audio_manager_this', '''
static ManagerRecord* retdec_audio_manager_this() noexcept {
    return &g_retdec_audio_manager_state;
}
''')
body('retdec_audio_manager_list_init', '''
static bool retdec_audio_manager_list_init(QueueRecord& queue) {
    auto* node = static_cast<QueueNode*>(std::malloc(sizeof(QueueNode)));
    if (!node) return false;
    node->next = node;
    node->previous = node;
    node->handle = 0;
    queue.head = node;
    return true;
}
''')
body('retdec_audio_list_push', '''
static void retdec_audio_list_push(QueueNode* list, std::uint32_t value) {
    if (!list) return;
    auto* node = static_cast<QueueNode*>(std::malloc(sizeof(QueueNode)));
    if (!node) return;
    node->next = list;
    node->previous = list->previous;
    node->handle = value;
    node->previous->next = node;
    list->previous = node;
}
''')
body('retdec_audio_list_pop', '''
static bool retdec_audio_list_pop(QueueNode* list, std::uint32_t* value) {
    if (!list || list->next == list) return false;
    auto* node = list->next;
    node->previous->next = node->next;
    node->next->previous = node->previous;
    if (value) *value = node->handle;
    std::free(node);
    return true;
}
''')
body('retdec_audio_buffer_path', '''
static const char* retdec_audio_buffer_path(const BufferRecord* buffer) noexcept {
    return buffer ? buffer->path.c_str() : nullptr;
}
''')
body('retdec_audio_buffer_initialize', '''
static void retdec_audio_buffer_initialize(BufferRecord* buffer) noexcept {
    // Placement construction gives the malloc-backed native record its C++
    // lifetime without changing any recovered bytes or decoder identity.
    new (buffer) BufferRecord{};
    buffer->path.capacity = sizeof(buffer->path.storage.inline_text) - 1;
    buffer->decoder_vtable = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(kinoko_audio_host_symbols()->decoder_vtable));
    buffer->gain = 1.0f;
    buffer->playback_state = 3;
}
''')
body('retdec_audio_handle_lookup', '''
static BufferRecord* retdec_audio_handle_lookup(HandleTable* manager,
                                               std::uint32_t handle) {
    if (!manager) return nullptr;
    const auto index = handle & 0xffffu;
    const auto generation = handle >> 16;
    const auto begin = reinterpret_cast<std::uintptr_t>(manager->buffers_begin);
    const auto end = reinterpret_cast<std::uintptr_t>(manager->buffers_end);
    auto* generations = manager->generations_begin;
    retdec_trace_i32("audio:lookup-manager", address(manager));
    retdec_trace_i32("audio:lookup-handle", static_cast<std::int32_t>(handle));
    retdec_trace_i32("audio:lookup-begin", static_cast<std::int32_t>(begin));
    retdec_trace_i32("audio:lookup-end", static_cast<std::int32_t>(end));
    retdec_trace_i32("audio:lookup-generations", address(generations));
    if (!begin || end < begin || index >= (end - begin) / sizeof(BufferRecord*))
        return nullptr;
    if (!generations || generations[index] != generation) {
        retdec_trace_i32("audio:lookup-generation", generations
            ? static_cast<std::int32_t>(generations[index]) : 0);
        return nullptr;
    }
    auto* buffer = manager->buffers_begin[index];
    retdec_trace_i32("audio:lookup-buffer", address(buffer));
    return buffer;
}
''')
body('retdec_audio_handle_create', '''
static bool retdec_audio_handle_create(HandleTable* manager, std::uint32_t* output) {
    if (!manager || !output) return false;
    const auto begin = reinterpret_cast<std::uintptr_t>(manager->buffers_begin);
    const auto end = reinterpret_cast<std::uintptr_t>(manager->buffers_end);
    const auto count = (!begin || end < begin) ? std::size_t{0}
        : (end - begin) / sizeof(BufferRecord*);
    retdec_trace_i32("audio:create-manager", address(manager));
    retdec_trace_i32("audio:create-count", static_cast<std::int32_t>(count));
    // The low half of the original handle is the slot index. Never allocate a
    // slot that would alias another handle or overflow the allocation size.
    if (count > 0xffffu) return false;
    kinoko::legacy::Allocation<BufferRecord> buffer(
        static_cast<BufferRecord*>(std::malloc(sizeof(BufferRecord))));
    if (!buffer) return false;
    retdec_audio_buffer_initialize(buffer.get());
    kinoko::legacy::Allocation<BufferRecord*> buffers(
        static_cast<BufferRecord**>(std::malloc((count + 1) * sizeof(BufferRecord*))));
    kinoko::legacy::Allocation<std::uint32_t> generations(
        static_cast<std::uint32_t*>(std::malloc((count + 1) * sizeof(std::uint32_t))));
    if (!buffers || !generations) return false;
    if (count) {
        std::memcpy(buffers.get(), manager->buffers_begin, count * sizeof(BufferRecord*));
        const auto first = reinterpret_cast<std::uintptr_t>(manager->generations_begin);
        const auto last = reinterpret_cast<std::uintptr_t>(manager->generations_end);
        if (first && last >= first) {
            // Reject a truncated generation table instead of reading past it.
            if ((last - first) / sizeof(std::uint32_t) < count) return false;
            std::memcpy(generations.get(), manager->generations_begin,
                        count * sizeof(std::uint32_t));
        } else {
            std::memset(generations.get(), 0, count * sizeof(std::uint32_t));
        }
    }
    auto generation = (manager->next_generation + 1) & 0xffffu;
    if (!generation) generation = 1;
    const auto handle = static_cast<std::uint32_t>(count) | (generation << 16);
    buffers.get()[count] = buffer.get();
    generations.get()[count] = generation;
    retdec_trace_i32("audio:create-handle", static_cast<std::int32_t>(handle));
    retdec_trace_i32("audio:create-buffer", address(buffer.get()));
    retdec_trace_i32("audio:create-buffer-table", address(buffers.get()));
    retdec_trace_i32("audio:create-generation-table", address(generations.get()));
    std::free(manager->buffers_begin);
    std::free(manager->generations_begin);
    manager->buffers_begin = buffers.release();
    manager->buffers_end = manager->buffers_begin + count + 1;
    manager->buffers_capacity = manager->buffers_end;
    manager->generations_begin = generations.release();
    manager->generations_end = manager->generations_begin + count + 1;
    manager->generations_capacity = manager->generations_end;
    manager->next_generation = generation;
    buffer.release(); // exclusive ownership transfers to the manager table
    retdec_audio_list_push(manager->live_handles, handle);
    *output = handle;
    return true;
}
''')
body('function_40adb0', '''
int32_t function_40adb0(int32_t* storage) {
    auto* table = reinterpret_cast<HandleTable*>(storage);
    // The outer manager has already value-initialized the whole record.
    const auto* symbols = kinoko_audio_host_symbols();
    table->vtable = symbols->handle_table_vtable;
    table->buffers_begin = table->buffers_end = table->buffers_capacity = nullptr;
    table->generations_begin = table->generations_end = table->generations_capacity = nullptr;
    table->live_count = 0;
    auto* node = static_cast<QueueNode*>(std::malloc(sizeof(QueueNode)));
    if (!node) throw std::bad_alloc();
    node->next = node->previous = node;
    node->handle = 0;
    table->live_handles = node;
    table->next_generation = 0;
    table->lock_vtable = symbols->critical_section_vtable;
    InitializeCriticalSection(&table->lock);
    return address(storage);
}
''')
body('retdec_audio_manager_construct', '''
static void retdec_audio_manager_construct() {
    if (g_retdec_audio_manager_initialized) return;
    auto& state = g_retdec_audio_manager_state;
    state = ManagerRecord{};
    state.lock_vtable = kinoko_audio_host_symbols()->critical_section_vtable;
    InitializeCriticalSection(&state.lock);
    function_40adb0(reinterpret_cast<int32_t*>(&state.handles));
    if (!retdec_audio_manager_list_init(state.active) ||
        !retdec_audio_manager_list_init(state.pending) ||
        !retdec_audio_manager_list_init(state.retired)) return;
    state.master_gain = 1.0f;
    state.stream_gain = 1.0f;
    g_retdec_audio_manager_initialized = 1;
}
''')
body('retdec_audio_method_40a5d0', '''
static std::uint32_t* retdec_audio_method_40a5d0(ManagerRecord* manager,
                                              std::uint32_t* output) {
    if (!output) return nullptr;
    *output = 0;
    if (!g_retdec_audio_manager_initialized) retdec_audio_manager_construct();
    if (g_retdec_audio_manager_initialized)
        retdec_audio_handle_create(&manager->handles, output);
    return output;
}
''')
body('retdec_release_dsound_buffer', '''
static void retdec_release_dsound_buffer(IDirectSoundBuffer* buffer) noexcept {
    if (buffer) buffer->Release();
}
''')
body('retdec_create_secondary_buffer', '''
static int retdec_create_secondary_buffer(const WAVEFORMATEX* format,
                                          DWORD buffer_bytes,
                                          IDirectSoundBuffer** result) {
    if (!result) return 0;
    *result = nullptr;
    auto* device = g_audio_device.device.get();
    if (!device || !format || !buffer_bytes) return 0;
    DSBUFFERDESC description{};
    description.dwSize = sizeof(description);
    description.dwFlags = DSBCAPS_LOCSOFTWARE | DSBCAPS_CTRLVOLUME |
        DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2;
    description.dwBufferBytes = buffer_bytes;
    description.lpwfxFormat = const_cast<WAVEFORMATEX*>(format);
    retdec_trace_i32("audio:create-bytes", static_cast<int32_t>(buffer_bytes));
    retdec_trace_i32("audio:create-rate", static_cast<int32_t>(format->nSamplesPerSec));
    retdec_trace_i32("audio:create-channels", static_cast<int32_t>(format->nChannels));
    const auto hr = device->CreateSoundBuffer(&description, result, nullptr);
    retdec_trace_hresult("audio:secondary-create-hr", hr);
    if (FAILED(hr) || !*result) {
        *result = nullptr;
        return 0;
    }
    return 1;
}
''')
body('retdec_set_dsound_volume', '''
static void retdec_set_dsound_volume(IDirectSoundBuffer* buffer, float gain) {
    if (buffer) buffer->SetVolume(retdec_audio_volume_db(gain));
}
''')
body('function_411d80', '''
int32_t function_411d80(HWND hwnd, int32_t options) {
    g_audio_device.reset();
    sync_audio_device_aliases();
    auto& owner = g_audio_device;
    retdec_trace("411d80:pre-cocreate");
    // Use the SDK GUID objects, not the first DWORD of a split RetDec global.
    auto hr = CoCreateInstance(CLSID_DirectSound8, nullptr, CLSCTX_INPROC_SERVER,
        IID_IDirectSound8, reinterpret_cast<void**>(owner.device.put()));
    retdec_trace_hresult("411d80:cocreate-hr", hr);
    bool initialized = false;
    if (FAILED(hr) || !owner.device) {
        retdec_trace("411d80:pre-directsoundcreate8");
        owner.module = LoadLibraryA("dsound.dll");
        using Create = HRESULT (WINAPI*)(LPCGUID, LPDIRECTSOUND8*, LPUNKNOWN);
        const auto create = owner.module ? reinterpret_cast<Create>(
            GetProcAddress(owner.module, "DirectSoundCreate8")) : nullptr;
        hr = create ? create(nullptr, owner.device.put(), nullptr) : E_FAIL;
        retdec_trace_hresult("411d80:directsoundcreate8-hr", hr);
        initialized = SUCCEEDED(hr) && static_cast<bool>(owner.device);
    }
    if (FAILED(hr) || !owner.device) {
        MessageBoxA(nullptr, kinoko_audio_host_symbols()->device_error_message,
                    "DSound-Error", MB_OK);
        owner.reset();
        return 0;
    }
    if (!initialized) {
        retdec_trace("411d80:pre-initialize");
        hr = owner.device->Initialize(nullptr);
        retdec_trace_hresult("411d80:initialize-hr", hr);
        if (FAILED(hr)) { owner.reset(); return 0; }
    } else {
        retdec_trace("411d80:initialize-skipped");
    }
    retdec_trace("411d80:pre-cooperative-level");
    hr = owner.device->SetCooperativeLevel(hwnd, DSSCL_PRIORITY);
    if (FAILED(hr)) hr = owner.device->SetCooperativeLevel(hwnd, DSSCL_NORMAL);
    retdec_trace_hresult("411d80:cooperative-level-hr", hr);
    if (FAILED(hr)) { owner.reset(); return 0; }
    DSCAPS caps{};
    caps.dwSize = sizeof(caps);
    owner.device->GetCaps(&caps);
    DSBUFFERDESC description{};
    description.dwSize = sizeof(description);
    description.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_LOCSOFTWARE;
    if (options & 1) description.dwFlags |= DSBCAPS_CTRL3D | DSBCAPS_CTRLVOLUME;
    retdec_trace("411d80:pre-create-primary");
    hr = owner.device->CreateSoundBuffer(&description, owner.primary.put(), nullptr);
    retdec_trace_hresult("411d80:create-primary-hr", hr);
    if (FAILED(hr) || !owner.primary) { owner.reset(); return 0; }
    if (options & 1) {
        hr = owner.primary->QueryInterface(IID_IDirectSound3DListener,
            reinterpret_cast<void**>(owner.listener.put()));
        retdec_trace_hresult("411d80:listener-hr", hr);
        if (FAILED(hr) || !owner.listener) { owner.reset(); return 0; }
    }
    retdec_trace("411d80:pre-play-primary");
    hr = owner.primary->Play(0, 0, DSBPLAY_LOOPING);
    retdec_trace_hresult("411d80:play-primary-hr", hr);
    sync_audio_device_aliases();
    retdec_trace("411d80:done");
    return 1;
}
''')
body('function_411f90', '''
int32_t function_411f90(void) {
    const auto result = address(g_audio_device.device.get());
    // Shutdown has already joined the audio workers. Release the listener,
    // primary buffer and device in that order before unloading the module.
    g_audio_device.reset();
    sync_audio_device_aliases();
    return result;
}
''')
body('function_470980', '''
int32_t function_470980(int32_t id) {
    retdec_trace_i32("470980:se-id", id);
    for (int index = 0; index < g_retdec_se_entry_count; ++index) {
        const auto& entry = g_retdec_se_entries[index];
        if (entry.id != id) continue;
        auto* buffer = entry.buffer.get();
        if (!buffer) return 0;
        DWORD status = 0;
        buffer->GetStatus(&status);
        if (status & DSBSTATUS_PLAYING) buffer->Stop();
        buffer->SetCurrentPosition(0);
        retdec_trace_i32("470980:se-bytes", static_cast<int32_t>(entry.buffer_bytes));
        // No process-memory dump of a COM implementation on the sound path.
        const auto hr = buffer->Play(0, 0, 0);
        retdec_trace_hresult("470980:play-hr", hr);
        return SUCCEEDED(hr) ? 1 : 0;
    }
    return 0;
}
''')
