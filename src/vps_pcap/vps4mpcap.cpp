#include "vps_pcap/vps4mpcap.h"

#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <queue>

#include "vps_pcap/airimgrecv.h"
#include "vps_pcap/ethernet.h"
using namespace std;

// pcap open device错误
#define RTN_ERR_FIND_DEVICES (-3)
#define RTN_ERR_OPEN_ADPTER (-4)
#define RTN_ERR_DLT_EN10MB (-5)
#define RTN_OK (0)

VPS4MPcap::VPS4MPcap()
{
  m_ethHandle = NULL;
  m_ethIndex = -1;
  m_info = new VPSEthernetInfo;
  m_packet_handler = NULL;
  imgrev_ = new AirImgRecv();
}

VPS4MPcap::~VPS4MPcap()
{
  closeAll();
  delete imgrev_;
}

void VPS4MPcap::closeAll()
{
  if (m_info)
  {
    delete m_info;
    m_info = NULL;
  }
  if (opened_)
  {
    pcapClose();
  }
}

int g_pcapKernelBufferSize = 20 * 1024 * 1024;
int g_pcapCommitSize = 5 * 1024 * 1024;
int g_pcapUserBufferSize = 15 * 1024 * 1024;

bool recvCheck(uint8_t *buf, int len) // 16
{
  if (len > 1 && len < 2048)
  {
    uint8_t sum = 0;
    for (int i = 0; i < len - 1; i++)
      sum += buf[i];
    return sum == buf[len - 1];
  }
  return false;
}

// pcap回调，注意这里是跨线程调用
void packet_handler(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data)
{
  void *out_data = nullptr;
  if (!pkt_data || header->len < 60 || header->len > 1514)
    return;

  if (header->len == 1083 && pkt_data[42] == 0x81)
  {
    if (recvCheck((uint8_t *)pkt_data + 42, 1041))
    {
      VPS4MPcap::instance().recvPack((uint8_t *)pkt_data + 42, out_data);
    }
  }
}

void *VPS4MPcap::recvPack(void *vdata, void *out_data) { return imgrev_->imgRecv((uint8_t *)vdata, out_data); }

bool VPS4MPcap::init10g()
{
  pcapClose();

  pcapQueryEthernet();

  int cnt = getEthernetCnt();
  for (int i = 0; i < cnt; i++)
  {
    int res = strcmp(m_info->Ethernet[i], "eth0");
    if (res == 0)
    {
      // defalut callback handler
      pcapSetHandler(packet_handler);
      if (!pcapInit(i))
      {
        pcapClose();
        return false;
      }
      usleep(250 * 1000);
      return true;
    }
  }

  return false;
}

bool VPS4MPcap::pcapQueryEthernet()
{
  m_info->EthernetCnt = 0;

  pcap_if_t *d;
  pcap_if_t *alldevs;
  char errbuf[PCAP_ERRBUF_SIZE];
  if (pcap_findalldevs(&alldevs, errbuf) == -1)
  {
    return false;
  }

  int inum = 0;
  for (d = alldevs; d; d = d->next)
  {
    if (d->description != NULL)
    {
      string description(d->description);
      if (description.find("virtual") != string::npos || description.find("Virtual") != string::npos ||
          description.find("wireless") != string::npos || description.find("Wireless") != string::npos)
        continue;
    }

    strcpy(m_info->Ethernet[inum], d->name);

    inum++;
  }
  m_info->EthernetCnt = inum;
  pcap_freealldevs(alldevs);

  return inum > 0;
}

int VPS4MPcap::getEthernetCnt() { return m_info->EthernetCnt; }

VPSEthernetInfo VPS4MPcap::getEthernetScanResult() { return *m_info; }

int VPS4MPcap::openCapture(const char *filter_exp, uint8_t *custom_param)
{
  if (m_ethIndex < 0 || m_ethIndex > m_info->EthernetCnt - 1)
  {
    return RTN_ERR_FIND_DEVICES;
  }

  char errbuf[PCAP_ERRBUF_SIZE]; // 错误缓冲区
  struct bpf_program fp;         // BPF过滤器程序
  // char filter_exp[] = "udp and port 8189";  // 过滤规则

  // 打开网卡设备
  m_ethHandle = pcap_open_live(m_info->Ethernet[m_ethIndex], 1200, 1, 1000, errbuf);
  if (m_ethHandle == NULL)
  {
    fprintf(stderr, "无法打开网卡设备: %s\n", errbuf);
    return 1;
  }

  // 编译并设置过滤规则
  if (pcap_compile(m_ethHandle, &fp, filter_exp, 0, PCAP_NETMASK_UNKNOWN) == -1)
  {
    fprintf(stderr, "无法编译过滤规则: %s\n", pcap_geterr(m_ethHandle));
    return 1;
  }
  if (pcap_setfilter(m_ethHandle, &fp) == -1)
  {
    fprintf(stderr, "无法设置过滤规则: %s\n", pcap_geterr(m_ethHandle));
    return 1;
  }
  opened_ = true;
  // // 开始捕获数据包
  pcap_loop(m_ethHandle, 0, m_packet_handler, custom_param);

  return RTN_OK;
}

void VPS4MPcap::pcapSetHandler(VPSMPcapHandler *packet_handler) { m_packet_handler = packet_handler; }

bool VPS4MPcap::pcapInit(int index)
{
  if (m_ethHandle != NULL)
  {
    pcapClose();
  }

  m_ethIndex = index;

  return true;
}

void VPS4MPcap::pcapClose()
{
  if(!opened_)
  {
    return;
  }
  if (m_ethHandle)
  {
    pcap_breakloop(m_ethHandle);
    pcap_close(m_ethHandle);
  }
  opened_ = false;
}
