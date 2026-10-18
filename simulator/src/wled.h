#pragma once
// =============================================================================
//  wled.h  --  HOST SIMULATION SHIM
//  Replaces WLED's ESP32/Arduino platform header so that the *unmodified* FX
//  core (FX.h / FX_fcn.cpp, i.e. all 221 effects) compiles natively on Linux.
//
//  Policy:
//    * compile-time constants  -> #include the REAL const.h (no drift)
//    * render-affecting maths  -> real WLED implementations copied verbatim
//    * buses / JSON / storage -> inert stubs (the FX core only calls them to
//                                publish pixels, which we intercept ourselves)
// =============================================================================
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <vector>
#include <string>
#include <type_traits>

#include "pgmspace.h"
#include "fastled_slim.h"

#define LEDC_CHANNEL_MAX  8
#define LEDC_SPEED_MODE_MAX 2
#include "const.h"
#include <climits>
#include <cmath>          // <- real WLED constants (FIXED_PALETTE_COUNT=72, ...)

using byte = uint8_t;
using word = uint16_t;

#define PSTR(s) (s)
#define PGM_PSTR(s) (s)
#define DEBUG_PRINT(...)     do{}while(0)
#define DEBUG_PRINTLN(...)   do{}while(0)
#define DEBUG_PRINTF(...)    do{}while(0)
#define DEBUG_PRINTF_P(...)  do{}while(0)

#ifndef DEFAULT_LED_COUNT
  #define DEFAULT_LED_COUNT 30
#endif
#ifndef WLED_MAX_SEGNAME_LEN
  #define WLED_MAX_SEGNAME_LEN 64
#endif
#ifndef MAX_LEDS
  #define MAX_LEDS 4096
#endif
#ifndef MAX_LED_MEMORY
  #define MAX_LED_MEMORY (192*1024)
#endif
#ifndef CALL_MODE_WS_SEND
  #define CALL_MODE_WS_SEND 11
#endif
#ifndef JSON_LOCK_LEDMAP
  #define JSON_LOCK_LEDMAP 7
#endif
#ifndef WLED_FS
  #endif
#ifndef REALTIME_MODE_INACTIVE
  #define REALTIME_MODE_INACTIVE 0
#endif
#ifndef REALTIME_OVERRIDE_NONE
  #define REALTIME_OVERRIDE_NONE 0
#endif
#ifndef RGBW_MODE_MANUAL_ONLY
  #define RGBW_MODE_MANUAL_ONLY 0
#endif
#ifndef RGBW_MODE_DUAL
  #define RGBW_MODE_DUAL 3
#endif
#ifndef SEG_OPTION_ON
  #define SEG_OPTION_ON 2
#endif
#ifndef SEG_CAPABILITY_RGB
  #define SEG_CAPABILITY_RGB 0x01
#endif
#ifndef SEG_CAPABILITY_W
  #define SEG_CAPABILITY_W 0x02
#endif
#ifndef SEG_CAPABILITY_CCT
  #define SEG_CAPABILITY_CCT 0x04
#endif
#ifndef AW_GLOBAL_DISABLED
  #define AW_GLOBAL_DISABLED 255
#endif

// ---- buffer-allocation flags (real values from fcn_declare.h) ----
#define BFRALLOC_NOBYTEACCESS   (1 << 0)
#define BFRALLOC_PREFER_DRAM    (1 << 1)
#define BFRALLOC_ENFORCE_DRAM   (1 << 2)
#define BFRALLOC_PREFER_PSRAM   (1 << 3)
#define BFRALLOC_ENFORCE_PSRAM  (1 << 4)
#define BFRALLOC_CLEAR          (1 << 5)
#define BFRALLOC_PREFER_JSON    (1 << 6)

// ------------------------------------------------------------------ time / rng
uint32_t millis();
uint32_t micros();
uint32_t random32();
uint8_t  random8();
uint16_t random16();
float    randomf();
uint8_t  esp_random();
void     yield();
void     delay(uint32_t);

template<typename T> inline T max(T a, T b){ return a>b?a:b; }
template<typename T> inline T min(T a, T b){ return a<b?a:b; }
template<typename T> inline T abs(T a){ return a<0?-a:a; }
#ifndef constrain
  #define constrain(amt,lo,hi) ((amt)<(lo)?(lo):((amt)>(hi)?(hi):(amt)))
#endif
#ifndef lerb
  #define lerb(a,b,t) ((uint8_t)((((uint16_t)(a))*((255)-((uint16_t)(t)))+((uint16_t)(b))*((uint16_t)(t)))/255))
#endif
#ifndef scale8
  #define scale8(a,b) (((uint16_t)(a)*(b)+127)>>8)
#endif
#ifndef byteswap2
  #define byteswap2(a) (a)
#endif


// ---- globals declared by real wled.h that FX_fcn.cpp uses -----------------
inline byte     errorFlag = 0;            // wled.h:774
inline uint8_t  paletteBlend = 0;         // wled.h:593
inline byte     bri = 255;                // wled.h:619
inline uint8_t  briMultiplier = 100;      // wled.h:408

// ---- FastLED-compatible fixed-point trig: int16 result, -0x7FFF..+0x7FFF ----
inline int16_t sin16_t(uint16_t i) { return (int16_t)(sin((double)i / 65536.0 * 2.0 * M_PI) * 32767.0); }
inline int16_t cos16_t(uint16_t i) { return (int16_t)(cos((double)i / 65536.0 * 2.0 * M_PI) * 32767.0); }

// ---- misc helpers the FX core calls -------------------------------------
inline uint32_t hashInt(uint32_t x) {   // xorshift, same spirit as WLED's
  x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16; return x;
}
inline void enumerateLedmaps() {}
inline void refreshLedmaps() {}
#define F(x) (x)
#define strncmp_P strncmp
#define PSTR_(s)  (s)

// ------------------------------------------------- ArduinoJSON / FS surface
struct JsonArray { template<typename T> int size() const { return 0; } };
struct JsonProxy {
  bool _b = false; int _i = 0; unsigned _u = 0; double _d = 0; char _s[64] = {0};
  // 显式转换，避免 filter["x"] = true 这类赋值与数值比较产生歧义
  template<typename T> explicit operator T() const { return T{}; }
  const char* c_str() const { return _s; }
  template<typename T> JsonProxy& operator=(T v) {
    using V = std::decay_t<T>;
    if constexpr (std::is_same_v<V, bool>) _b = v;
    else if constexpr (std::is_floating_point_v<V>) _d = v;
    else if constexpr (std::is_signed_v<V>) _i = (int)v;
    else _u = (unsigned)v;
    return *this;
  }
  template<typename T> JsonProxy& operator=(const char* v) { strncpy(_s, v, 63); return *this; }
  JsonProxy& operator=(const JsonProxy& o) { return *this = (int)o; }
  template<typename T> T as() const { return T{}; }
  template<typename T> bool is() const { return false; }
  bool isNull() const { return true; }
  template<typename T> JsonProxy& createNestedArray(const char*) { return *this; }
  template<typename T> JsonProxy& createNestedObject(const char*) { return *this; }
  bool containsKey(const char*) const { return false; }
  size_t size() const { return 0; }
  JsonProxy& add(JsonProxy&) { return *this; }
};
struct JsonObject : JsonProxy {
  JsonProxy operator[](const char*) { return JsonProxy{}; }
  JsonProxy operator[](int) { return JsonProxy{}; }
  using JsonProxy::as;
  using JsonProxy::isNull;
};
struct JsonDocument {};
struct StaticJsonDocumentBase : JsonDocument {
  JsonProxy operator[](const char*) { return JsonProxy{}; }
  JsonProxy operator[](int) { return JsonProxy{}; }
  template<typename T> T as() { return T{}; }
  template<typename T> bool deserializeJson(T&&...) { return false; }
  bool isNull() const { return true; }
};
template<unsigned N> struct StaticJsonDocumentT : StaticJsonDocumentBase {};
template<unsigned N> struct StaticJsonDocument : StaticJsonDocumentBase {
  StaticJsonDocument() {}
  StaticJsonDocument(int) {}
  using StaticJsonDocumentBase::operator[];
};
struct String {
  String() {} String(const char*) {} String(int) {}
  const char* c_str() const { return ""; }
  unsigned length() const { return 0; }
};
struct File {
  bool read(char*) { return false; }
  int  readLine(char*, int) { return 0; }
  int  readInt() { return 0; }
  void rewind() {}
  bool isEnd() { return true; }
  bool exists() { return false; }
  size_t size() { return 0; }
  size_t read(uint8_t*, size_t) { return 0; }
  size_t write(const uint8_t*, size_t) { return 0; }
  size_t seek(uint32_t) { return 0; }
  bool find(const char*) { return false; }
  void close() {}
  explicit operator bool() const { return false; }
};
struct Dir {
  void rewinddir() {}
  struct Ent { bool isDir() { return false; } bool isFile() { return false; } char name[64]; };
  Ent* readNext() { return nullptr; }
  void close() {}
};
struct FSStub {
  bool exists(const char*) { return false; }
  Dir* opendir(const char*) { return nullptr; }
  File open(const char*, const char*) { return File(); }
};
struct DebugOutStub {
  template<typename...A> void print(A&&...){}
  template<typename...A> void println(A&&...){}
  template<typename...A> void printf(A&&...){}
  explicit operator bool(){ return false; }
};
extern DebugOutStub Serial;

// ------------------------------------------------- REAL WLED maths (verbatim)
uint32_t sqrt32_bw(uint32_t x) {          // wled_math.cpp:225
  uint32_t res = 0, bit, num = x;
  if      (num < (1u << 10)) bit = 1u << 10;
  else if (num < (1u << 20)) bit = 1u << 20;
  else                       bit = 1u << 30;
  while (bit > num) bit >>= 2;
  while (bit != 0) {
    if (num >= res + bit) { num -= res + bit; res = (res >> 1) + bit; }
    else                  { res >>= 1; }
    bit >>= 2;
  }
  return res;
}
extern uint8_t briMultiplier;              // led.cpp
byte scaledBri(byte in) {                  // led.cpp:58
  unsigned val = ((unsigned)in * briMultiplier) / 100;
  if (val > 255) val = 255;
  return (byte)val;
}

// ------------------------------------------------------- buses (inert stubs)
struct BusManager;
struct Bus {
  int  type = 0, iType = 0, count = 0, driverType = 0, _len = 0;
  bool _ok = true;
  bool isOk() const { return _ok; }
  bool containsPixel(unsigned) const { return true; }
  bool hasRGB() const { return true; }
  bool hasCCT() const { return false; }
  bool hasWhite() const { return false; }
  static bool isDigital(int){ return true; }
  static bool is2Pin(int){ return true; }
  bool isPlaceholder() const { return false; }
  bool isOffRefreshRequired() const { return false; }
  bool isPWM() const { return false; }
  void begin(){}
  void setPixelColor(uint16_t, uint32_t){}
  unsigned getBusSize() const { return 0; }
  int  getStart() const { return 0; }
  int  getLength() const { return _len; }
  int  getAutoWhiteMode() const { return AW_GLOBAL_DISABLED; }
  int  getLengthTotal() const { return _len; }
  void setLength(int n) { _len = n; }
  void setBrightness(uint8_t){}
  static void setCCT(int){}
  static uint16_t getCCT(){ return 0; }
  void setCCT(uint16_t,int){}
  void setColorOrder(int){}
  static uint8_t getGlobalAWMode();
};
struct BusConfig {
  int gpio = -1, type = 0, iType = 0, count = 0, start = 0, pixels = 0, pins = 0, driverType = 0;
  unsigned memUsage() const { return 0; }   // real WLED method, not a field
};

struct BusManager {
  static std::vector<Bus*> busses;
  static std::vector<Bus>  _pool;
  static Bus* _single;
  static uint8_t _gAWM;
  static bool _isServicing, _suspend;

  static uint8_t  getGlobalAWMode() { return _gAWM; }
  static void     setGlobalAWMode(uint8_t m) { _gAWM = (m < 5) ? m : AW_GLOBAL_DISABLED; }
  static int      getNumBusses() { return (int)busses.size(); }
  static int      getTotalLength(bool = false) { int t = 0; for (auto* b : busses) t += b->getLengthTotal(); return t; }
  static Bus*     getBus(int i) { if (i >= 0 && i < (int)busses.size()) return busses[i]; return nullptr; }
  static int      getI(int,int,int){ return 0; }
  static int      add(const BusConfig&, bool){ Bus* b = &_pool[_pool.size()]; _pool.push_back(*b); busses.push_back(b); return (int)busses.size()-1; }
  static Bus*     allocBus(){ if(!_single){ static Bus tmp; _single=&tmp; } return _single; }
  static bool     getBusId(int, int&) { return false; }
  static int      getI(int, int) { return 0; }
  static void     setBrightness(uint8_t){}
  static void     setPixelColor(int,int,uint32_t){}
  template<typename T> static void setPixelColor(uint16_t, T){}
  static void     setSegmentCCT(int){}
  static void     setSegmentCCT(int,int){}
  static void     show(){}
  static void     add(const BusConfig&, int){}
  static void     removeAll(){ busses.clear(); _pool.clear(); }
  static void     initializeABL(){}
  static bool     canAllShow() { return true; }
  static bool     isServicing() { return _isServicing; }
  static void     setServicing(){ _isServicing = true; }
  static void     desetServicing(){ _isServicing = false; }
  static void     setPixelBuffer(){}
};

inline uint8_t Bus::getGlobalAWMode() { return BusManager::_gAWM; }

// --------------------------------------------------------- WLED globals
inline uint8_t blendingStyle = 0;
inline uint8_t randomPaletteChangeTime = 5;
inline bool useHarmonicRandomPalette = true;
inline bool gammaCorrectBri = false;
inline bool arlsDisableGammaCorrection = true;
inline byte realtimeOverride = REALTIME_OVERRIDE_NONE;
inline bool useMainSegmentOnly = false;
inline uint8_t currentLedmap = 0;
inline bool      stateChanged = false;
inline byte      realtimeMode = REALTIME_MODE_INACTIVE;
inline bool      arlsForceMaxBri = false;
inline byte      briT = 255;
inline std::vector<BusConfig> busConfigs;

template<typename... A> inline void* d_malloc(size_t n, A&&...)      { return calloc(1, n); }
template<typename... A> inline void* allocate_buffer(size_t n, A&&...){ return calloc(1, n); }
template<typename... A> inline void* p_malloc(size_t n, A&&...)      { return malloc(n); }
inline void p_free(void* p){ free(p); }
inline void d_free(void* p){ free(p); }
inline void* requestJSONBufferLock(uint8_t lockType = 0){ (void)lockType; return calloc(1, 2048); }
template<typename... A> inline void releaseJSONBufferLock(A&&...) {}
template<typename... A> inline uint8_t extractModeDefaults(A&&...){ return 0; }
template<typename... A> inline int readNextIntFromFile(A&&...) { return 0; }
template<typename... A> inline bool readObjectFromFile(A&&...) { return false; }
inline byte interfaceUpdateCallMode = 0;   // scheduled notifier call mode
template<typename... A> inline void setSegmentCCT(A&&...) {}

#include "FX.h"

extern WS2812FX strip;
inline FSStub WLED_FS;
struct JsonObjectPtr { template<typename T> T as() { return T{}; } JsonObject* operator->() { static JsonObject o; return &o; } };
inline JsonObjectPtr pDoc;
