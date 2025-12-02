#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <optional>

struct libusb_context;
struct libusb_device;
struct libusb_device_handle;

struct UsbEndpoint {
  uint8_t address;
  uint8_t attributes;
  uint16_t maxPacketSize;
  bool isIn;
  bool isBulk;
};

class BotDriver {
public:
  BotDriver();
  ~BotDriver();

  bool init();
  void exit();

  std::vector<std::string> listDevices();

  bool open(uint16_t vid, uint16_t pid);
  void close();

  bool claimInterface(int iface);
  void releaseInterface();

  bool probeEndpoints();
  std::optional<UsbEndpoint> bulkIn() const;
  std::optional<UsbEndpoint> bulkOut() const;

  bool write(const uint8_t* data, int length, int timeout_ms, int* transferred);
  bool read(uint8_t* data, int length, int timeout_ms, int* transferred);

private:
  libusb_context* ctx_;
  libusb_device_handle* handle_;
  int iface_;
  std::optional<UsbEndpoint> in_;
  std::optional<UsbEndpoint> out_;
};
