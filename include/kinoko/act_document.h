#pragma once
#include "kinoko/act_types.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* 427530: initialize caller-owned, fresh 240-byte Win32 record storage.
   Does not replace an already initialized document or consume its storage. */
KinokoActDocument *kinoko_act_document_initialize(KinokoActDocument *document);
/* Allocate and initialize a document using the existing malloc/free ABI.
   The caller owns it; its virtual deleting destructor releases its contents. */
KinokoActDocument *kinoko_act_document_create(void);
/* 428000: borrow document and filename. Own the opened reader only for this
   call, including failure/unwind. A partial document remains caller-owned;
   this interface neither destroys it nor publishes a stage/map runtime. */
int32_t kinoko_act_document_load(KinokoActDocument *document, const char *file_name);
/* Dispatch original slot 0x18, separately from deserialization. */
int32_t kinoko_act_document_load_resources(KinokoActDocument *document, const char *prefix);
int32_t __fastcall kinoko_method_load_act_resources(KinokoActDocument *document, void *unused, const char *prefix);
int32_t __fastcall kinoko_method_suspend_act_resources(KinokoActDocument *document, void *unused);
int32_t __fastcall kinoko_method_resume_act_resources(KinokoActDocument *document, void *unused);
const char *kinoko_act_document_name(const KinokoActDocument *document);
int32_t kinoko_act_document_screen_width(const KinokoActDocument *document);
int32_t kinoko_act_document_screen_height(const KinokoActDocument *document);
#ifdef __cplusplus
}
#endif
