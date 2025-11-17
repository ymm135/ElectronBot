#include "botdriver.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// 简易命令行工具：
// - list  列出系统中的 USB 设备
// - probe <vid> <pid>  打开设备并打印首个 Bulk IN/OUT 端点信息
// - xfer  <vid> <pid> <hex-cmd> <float>  发送 5 字节帧并读取 5 字节响应
// 注意：Windows 需将目标设备绑定到 WinUSB/libusbK/libusb-win32 才能被 libusb 访问

static uint16_t parse_hex(const char* s) {
  unsigned v = 0; sscanf(s, "%x", &v); return (uint16_t)v; }

int main(int argc, char** argv) {
  uint16_t vid = 0x1001;
  uint16_t pid = 0x8023;
  std::string cmd = argc > 1 ? argv[1] : "list";
  if (cmd == "help") {
    printf("Usage:\n");
    printf("  botdriver list\n");
    printf("  botdriver probe <vid> <pid>\n");
    printf("  botdriver xfer <vid> <pid> <hex-cmd> <float-value>\n");
    printf("\n数据帧约定：TX=[cmd(1B)][float(4B, 小端)]，RX 同样为 5 字节\n");
    return 0;
  }

  BotDriver drv;
  if (!drv.init()) { fprintf(stderr, "libusb init failed\n"); return 1; }

  if (cmd == "list") {
    auto devs = drv.listDevices();
    for (auto& s : devs) printf("%s\n", s.c_str());
    return 0;
  }

  if (cmd == "probe" && argc >= 4) {
    vid = parse_hex(argv[2]); pid = parse_hex(argv[3]);
    if (!drv.open(vid, pid)) { fprintf(stderr, "open failed\n"); return 2; }
    if (!drv.probeEndpoints()) { fprintf(stderr, "endpoint probe failed\n"); return 3; }
    auto in = drv.bulkIn(); auto out = drv.bulkOut();
    if (in.has_value()) printf("IN ep=0x%02X maxpkt=%u\n", in->address, in->maxPacketSize);
    if (out.has_value()) printf("OUT ep=0x%02X maxpkt=%u\n", out->address, out->maxPacketSize);
    return 0;
  }

  if (cmd == "xfer" && argc >= 6) {
    vid = parse_hex(argv[2]); pid = parse_hex(argv[3]);
    unsigned cmdByte = 0; sscanf(argv[4], "%x", &cmdByte);
    float f = (float)atof(argv[5]);
    if (!drv.open(vid, pid)) { fprintf(stderr, "open failed\n"); return 2; }
    if (!drv.probeEndpoints()) { fprintf(stderr, "endpoint probe failed\n"); return 3; }
    uint8_t tx[5];
    tx[0] = (uint8_t)cmdByte;
    std::memcpy(tx + 1, &f, 4);
    int sent = 0;
    if (!drv.write(tx, 5, 3000, &sent) || sent != 5) { fprintf(stderr, "write failed\n"); return 4; }
    uint8_t rx[5]{}; int got = 0;
    if (!drv.read(rx, 5, 3000, &got) || got != 5) { fprintf(stderr, "read failed\n"); return 5; }
    float resp = 0; std::memcpy(&resp, rx + 1, 4);
    printf("resp cmd=0x%02X val=%.6f\n", rx[0], resp);
    return 0;
  }

  fprintf(stderr, "unknown command\n");
  return 1;
}