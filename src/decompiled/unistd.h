#pragma once

/* RetDec emits a POSIX include for a few CRT helpers even for this Win32 PE. */
#include <errno.h>
#include <io.h>

#ifndef access
#define access _access
#endif
#ifndef read
#define read _read
#endif
#ifndef write
#define write _write
#endif
