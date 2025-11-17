#include "botdriver_lib.h"
#include "botdriver.h"
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>

extern "C" {
void* BD_Create() {
  return new BotDriver();
}
void BD_Destroy(void* h) {
  if (!h) return;
  delete static_cast<BotDriver*>(h);
}
int BD_Init(void* h) {
  if (!h) return 0;
  return static_cast<BotDriver*>(h)->init() ? 1 : 0;
}
void BD_Exit(void* h) {
  if (!h) return;
  static_cast<BotDriver*>(h)->exit();
}
int BD_Open(void* h, uint16_t vid, uint16_t pid) {
  if (!h) return 0;
  return static_cast<BotDriver*>(h)->open(vid, pid) ? 1 : 0;
}
void BD_Close(void* h) {
  if (!h) return;
  static_cast<BotDriver*>(h)->close();
}
int BD_ProbeEndpoints(void* h) {
  if (!h) return 0;
  return static_cast<BotDriver*>(h)->probeEndpoints() ? 1 : 0;
}
int BD_Write(void* h, const uint8_t* data, int length, int timeout_ms, int* transferred) {
  if (!h) return 0;
  int x = 0;
  bool ok = static_cast<BotDriver*>(h)->write(data, length, timeout_ms, &x);
  if (transferred) *transferred = x;
  return ok ? 1 : 0;
}
int BD_Read(void* h, uint8_t* data, int length, int timeout_ms, int* transferred) {
  if (!h) return 0;
  int x = 0;
  bool ok = static_cast<BotDriver*>(h)->read(data, length, timeout_ms, &x);
  if (transferred) *transferred = x;
  return ok ? 1 : 0;
}
char* BD_ListDevices() {
  BotDriver tmp;
  if (!tmp.init()) return nullptr;
  std::vector<std::string> v = tmp.listDevices();
  tmp.exit();
  std::string s;
  for (size_t i = 0; i < v.size(); ++i) {
    s += v[i];
    if (i + 1 < v.size()) s += "\n";
  }
  char* out = static_cast<char*>(std::malloc(s.size() + 1));
  if (!out) return nullptr;
  std::memcpy(out, s.c_str(), s.size() + 1);
  return out;
}
void BD_Free(void* p) {
  std::free(p);
}
}