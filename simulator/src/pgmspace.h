#pragma once
// Stub for Arduino-ESP32 flash-memory macros. On host (simulator) all "flash"
// is just normal RAM, so every accessor is a plain dereference.
#include <stdint.h>
#include <string.h>
#define PROGMEM
#define PGM_P           const char*
#define PGM_VOID_P      const void*
#define PGM_STR         const char*
#define PGM_READ(addr)  (*(const unsigned char*)(addr))
#define PGM_READ_BYTE(addr)  PGM_READ(addr)
#define PGM_READ_WORD(addr)  ((uint16_t)PGM_READ(addr) | ((uint16_t)PGM_READ((const char*)(addr)+1) << 8))
#define PGM_READ_DWORD(addr) ((uint32_t)PGM_READ_WORD(addr) | ((uint32_t)PGM_READ_WORD((const char*)(addr)+2) << 16))
#define pgm_read_byte(addr)   PGM_READ_BYTE(addr)
#define pgm_read_word(addr)   PGM_READ_WORD(addr)
#define pgm_read_dword(addr)  PGM_READ_DWORD(addr)
#define pgm_read_ptr(addr)    (*(void* const*)(addr))
#define memcpy_P   memcpy
#define strlen_P   strlen
#define strcpy_P   strcpy
#define strncpy_P  strncpy
#define strcat_P   strcat
#define strcmp_P   strcmp
#define snprintf_P snprintf
#define vsnprintf_P vsnprintf
