#include "botdriver.h"
#include <libusb.h>
#include <sstream>

// 构造与析构：确保资源在退出时释放
BotDriver::BotDriver() : ctx_(nullptr), handle_(nullptr), iface_(-1) {}

BotDriver::~BotDriver() { close(); exit(); }

// 初始化 libusb（必须在其它操作前调用）
bool BotDriver::init() {
  return libusb_init(&ctx_) == 0;
}

void BotDriver::exit() {
  if (ctx_) { libusb_exit(ctx_); ctx_ = nullptr; }
}

// 列举设备并打印基本信息（VID:PID、总线与地址）
std::vector<std::string> BotDriver::listDevices() {
  std::vector<std::string> out;
  libusb_device** list = nullptr;
  ssize_t cnt = libusb_get_device_list(ctx_, &list);
  for (ssize_t i = 0; i < cnt; ++i) {
    libusb_device* dev = list[i];
    libusb_device_descriptor desc{};
    if (libusb_get_device_descriptor(dev, &desc) == 0) {
      std::ostringstream ss;
      ss << "VID:PID=" << std::hex << std::uppercase << (int)desc.idVendor << ":" << (int)desc.idProduct
         << " bus=" << std::dec << (int)libusb_get_bus_number(dev)
         << " addr=" << (int)libusb_get_device_address(dev);
      out.push_back(ss.str());
    }
  }
  libusb_free_device_list(list, 1);
  return out;
}

// 按 VID/PID 打开设备（需驱动绑定到通用后端，如 WinUSB）
bool BotDriver::open(uint16_t vid, uint16_t pid) {
  handle_ = libusb_open_device_with_vid_pid(ctx_, vid, pid);
  return handle_ != nullptr;
}

void BotDriver::close() {
  if (handle_) {
    if (iface_ >= 0) { libusb_release_interface(handle_, iface_); iface_ = -1; }
    libusb_close(handle_);
    handle_ = nullptr;
  }
}

// 声明接口：Windows 直接 claim；Linux 需尝试 detach 内核驱动
bool BotDriver::claimInterface(int iface) {
  if (!handle_) return false;
  iface_ = iface;
  int r = 0;
#ifdef _WIN32
  r = libusb_claim_interface(handle_, iface_);
#else
  if (libusb_kernel_driver_active(handle_, iface_)) libusb_detach_kernel_driver(handle_, iface_);
  r = libusb_claim_interface(handle_, iface_);
#endif
  return r == 0;
}

void BotDriver::releaseInterface() {
  if (handle_ && iface_ >= 0) { libusb_release_interface(handle_, iface_); iface_ = -1; }
}

// 探测当前激活配置：记录首个 Bulk IN/OUT 端点，并保存所属接口号
bool BotDriver::probeEndpoints() {
  if (!handle_) return false;
  libusb_device* dev = libusb_get_device(handle_);
  libusb_config_descriptor* config = nullptr;
  if (libusb_get_active_config_descriptor(dev, &config) != 0) return false;
  bool ok = false;
  for (int i = 0; i < config->bNumInterfaces && !ok; ++i) {
    const libusb_interface& iface = config->interface[i];
    for (int a = 0; a < iface.num_altsetting && !ok; ++a) {
      const libusb_interface_descriptor& alt = iface.altsetting[a];
      for (int e = 0; e < alt.bNumEndpoints; ++e) {
        const libusb_endpoint_descriptor& ep = alt.endpoint[e];
        UsbEndpoint epinfo{};
        epinfo.address = ep.bEndpointAddress;
        epinfo.attributes = ep.bmAttributes;
        epinfo.maxPacketSize = ep.wMaxPacketSize;
        epinfo.isIn = (ep.bEndpointAddress & LIBUSB_ENDPOINT_IN) != 0;
        epinfo.isBulk = ((ep.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_BULK);
        if (epinfo.isBulk) {
          if (epinfo.isIn && !in_.has_value()) in_ = epinfo;   // 记录首个 Bulk IN
          if (!epinfo.isIn && !out_.has_value()) out_ = epinfo;// 记录首个 Bulk OUT
        }
      }
      if (in_.has_value() && out_.has_value()) { ok = true; iface_ = alt.bInterfaceNumber; }
    }
  }
  libusb_free_config_descriptor(config);
  return ok;
}

std::optional<UsbEndpoint> BotDriver::bulkIn() const { return in_; }
std::optional<UsbEndpoint> BotDriver::bulkOut() const { return out_; }

// Bulk 写：向 OUT 端点发送数据（阻塞，带超时）
bool BotDriver::write(const uint8_t* data, int length, int timeout_ms, int* transferred) {
  if (!handle_ || !out_.has_value()) return false;
  int xfer = 0;
  int r = libusb_bulk_transfer(handle_, out_->address, const_cast<unsigned char*>(data), length, &xfer, timeout_ms);
  if (transferred) *transferred = xfer;
  return r == 0;
}

// Bulk 读：从 IN 端点接收数据（阻塞，带超时）
bool BotDriver::read(uint8_t* data, int length, int timeout_ms, int* transferred) {
  if (!handle_ || !in_.has_value()) return false;
  int xfer = 0;
  int r = libusb_bulk_transfer(handle_, in_->address, reinterpret_cast<unsigned char*>(data), length, &xfer, timeout_ms);
  if (transferred) *transferred = xfer;
  return r == 0;
}