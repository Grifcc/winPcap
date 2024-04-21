#include <thread>

#include "utils/logger.h"
#include "vps_pcap/imgmemorymanager.h"
#include "vps_pcap/vps4mpcap.h"
#include <opencv2/video.hpp>
#include <opencv2/opencv.hpp>
#include "utils/threadsafe_queue.h"

void packet_handler_t(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data)
{
  void *out_data = nullptr;
  if (!pkt_data || header->len < 60 || header->len > 1514)
    return;
  if (header->len == 1083 && pkt_data[42] == 0x81)
  {
    if (recvCheck((uint8_t *)pkt_data + 42, 1041))
    {
      out_data = VPS4MPcap::instance().recvPack((uint8_t *)pkt_data + 42, NULL);
      if (out_data)
      {
        reinterpret_cast<threadsafe_queue<void *> *>(param)->push(out_data);
      }
    }
  }
}

int main()
{
  int port = 8189;
  int win_idx = 0;
  threadsafe_queue<void *> q;
  bool ret = VPS4MPcap::instance().init10g();
  if (ret)
  {
    ret = ImgMemoryManager::instance().mallocMemoryPool(512 * 512 *
                                                        static_cast<uint64_t>(10000));
    if (!ret)
    {
      MLOG_ERROR("can't allocate mem");
      throw std::runtime_error("can't allocate mem");
    }
    else
    {
      MLOG_INFO(" allocate mem success");
    }
  }
  else
  {
    MLOG_ERROR("init pcap err.");
    throw std::runtime_error("init pcap err");
  }
  MLOG_INFO("start to capture udp pack.");
  VPS4MPcap::instance().pcapSetHandler(packet_handler_t);
  VPS4MPcap::instance().set_WinIdx(win_idx);

  std::thread([&]()
              { VPS4MPcap::instance().openCapture(("udp and port " + std::to_string(port)).c_str(),
                                                  reinterpret_cast<uint8_t *>(&(q))); }).detach();
  while (true)
  {
    if (!q.empty())
    {
      AirImg *Airpack = reinterpret_cast<AirImg *>(q.wait_and_pop());  // 必须

      //process
      MLOG_INFO("rev paket:%d", Airpack->frame_id);
      cv::Mat img(512, 512, CV_8UC1, Airpack->frame);
      // cv::imwrite("rev_img/" + std::to_string(Airpack->frame_id) + "_" + std::to_string(Airpack->win_idx) + ".jpg", img);
      
      ImgMemoryManager::instance().setFree(Airpack->mempool_point);  //必须的
      delete Airpack;  // 必须的
    }
  }
}
