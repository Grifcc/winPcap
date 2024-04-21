

#include "vps_pcap/airimgrecv.h"

#include <unistd.h>

#include "opencv2/opencv.hpp"
#include "vps_pcap/ethernet.h"
#include "vps_pcap/imgmemorymanager.h"

using namespace vpskit;

namespace
{

  constexpr int n_w = 512;
  constexpr int n_h = 512;

  AirLastPacketCheck n_frameCompleteCheck;
  AirImgPacket n_airImgPacket; // current packet
} // namespace

// #define PACKET_LOSS_TEST

AirImgRecv::AirImgRecv()
{
  static const uint16_t pktCnt[5] = {15, 8, 4, 2, 1};
  static const uint16_t lineCnt[5] = {17024, 8512, 4256, 2128, 1064};

  AirImgPacket packet;
  packet.model = 5;
  for (int quad = 0; quad < 4; quad++)
  {
    packet.quad = 2 * quad + 1;
    packet.pkt = 255;
    n_frameCompleteCheck.model[5].win8_16[quad] = packet;
  }

  packet.model = 5;
  for (int quad = 0; quad < 4; quad++)
  {
    packet.quad = 2 * quad + 1 + 8;
    packet.pkt = 255;
    n_frameCompleteCheck.model[5].win8_16[quad + 4] = packet;
  }
}

// quadx       WIN_IDX
// quad0    0  1  8   9
// quad1    2  3  10  11
// quad2    4  5  12  13
// quad3    6  7  14  15

AirImgRecv::~AirImgRecv() {}

bool AirImgRecv::isLastPacket(uint8_t *data)
{
  if (!data)
    return false;

  uint8_t model = data[1];
  uint16_t pkt = vpskit::transEnd(*(uint16_t *)((data + 6)));

  if (model == 5)
  {
    return pkt == 255;
  }
  return false;
}

void *AirImgRecv::imgRecv(uint8_t *data, void *out_data)
{

  if (!m_shouldRecv || !data)
    return nullptr;
  if (data[1] != 0x05)
    return nullptr;

  if (data[1] == 0x05)
  {
    uint8_t *dst = nullptr;
    int offset = 0;

    uint8_t fun = data[0];
    uint8_t model = data[1];
    uint8_t frame = data[2];
    uint8_t win_idx = data[3];
    uint16_t pkt = vpskit::transEnd(*(uint16_t *)((data + 6)));
    uint16_t winX = vpskit::transEnd(*(uint16_t *)((data + 8)));
    uint16_t winY = vpskit::transEnd(*(uint16_t *)((data + 10)));
    if (win_idx != win_idx_)
      return nullptr;
    n_airImgPacket.fun = fun;
    n_airImgPacket.model = model;
    n_airImgPacket.frame = frame;
    n_airImgPacket.quad = win_idx; // check
    n_airImgPacket.pkt = pkt;
    n_airImgPacket.winX = winX;
    n_airImgPacket.winY = winY;

    // 全幅图像
    // 窗口
    if (data[1] == 0x05)
    {
      offset = pkt * 1024;
    }

    dst = ImgMemoryManager::instance().getFreeMem(n_w * n_h * 2) + offset;
    if (dst == nullptr)
    {
      printf("can't get free mem\n");
      return nullptr;
    }

    if (data[1] == 0x05)
    {
      memcpy(dst, data + 16, 1024);
    }

    if (isLastPacket(data))
    {
      auto out_ptr = ImgMemoryManager::instance().getFreeMem(n_w * n_h);
      ImgMemoryManager::instance().setBusy();
      AirImg *n_retImg = new AirImg;
      n_retImg->frame_id = frame;
      n_retImg->mempool_point = *(out_ptr - 1);
      n_retImg->win_idx = win_idx;
      n_retImg->winX = winX;
      n_retImg->winY = winY;
      n_retImg->frame = out_ptr;
      return reinterpret_cast<void *>(n_retImg);
    }
  }
  return nullptr;
}
