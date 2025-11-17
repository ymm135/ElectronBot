#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <optional>

struct libusb_context;
struct libusb_device;
struct libusb_device_handle;

// 端点描述：记录地址、类型与方向等信息
struct UsbEndpoint {
  uint8_t address;       // 端点地址（含方向位）
  uint8_t attributes;    // 端点属性（传输类型等）
  uint16_t maxPacketSize;// 最大包长
  bool isIn;             // 是否为 IN 端点（设备 → 主机）
  bool isBulk;           // 是否为 Bulk 传输类型
};

// BotDriver：封装 libusb 初始化、设备打开、接口声明、端点探测与读写
class BotDriver {
public:
  BotDriver();
  ~BotDriver();

  bool init();
  void exit();

  // 列举当前系统上的 USB 设备（输出基本信息）
  std::vector<std::string> listDevices();

  // 打开指定 VID/PID 的设备（需先安装 WinUSB/libusbK/libusb-win32 驱动绑定）
  bool open(uint16_t vid, uint16_t pid);
  void close();

  // 声明与释放接口（Windows 下通常直接 claim；Linux 需 detach 内核驱动）
  bool claimInterface(int iface);
  void releaseInterface();

  // 探测当前配置下的 Bulk IN/OUT 端点，保存首个匹配的端点与接口号
  bool probeEndpoints();
  std::optional<UsbEndpoint> bulkIn() const;
  std::optional<UsbEndpoint> bulkOut() const;

  // Bulk 读写（阻塞式），返回是否成功，并输出实际传输字节数
  bool write(const uint8_t* data, int length, int timeout_ms, int* transferred);
  bool read(uint8_t* data, int length, int timeout_ms, int* transferred);

private:
  libusb_context* ctx_;                 // libusb 上下文
  libusb_device_handle* handle_;        // 打开的设备句柄
  int iface_;                           // 已声明的接口号（探测得到）
  std::optional<UsbEndpoint> in_;       // Bulk IN 端点
  std::optional<UsbEndpoint> out_;      // Bulk OUT 端点
};