#include "vps_pcap/ethernet.h"

namespace vpskit {

// 小端转大端或者大端转小端
unsigned short transEnd(const unsigned short &data) {
  unsigned short val = (data >> 8) | ((data & 0x00FF) << 8);
  return val;
}

// 小端转大端或者大端转小端
unsigned int transEnd(const unsigned int &data) {
  unsigned int val = ((data & 0xff000000) >> 24) | ((data & 0x00ff0000) >> 8) | ((data & 0x0000ff00) << 8) |
                     ((data & 0x000000ff) << 24);
  return val;
}

// buf1伪首部固定12字节，buf2：udp首部, buf3: udpdata，nword指的是udpdata的长度
// ！！前三个参数按大端解释
unsigned short udpChecksum(unsigned short *buf1, unsigned short *buf2, unsigned short *buf3, int nword) {
  unsigned long sum = 0;
  for (int i = 0; i < 6; i++) {
    sum += transEnd(*buf1);  // 大端转小端
    sum = (sum >> 16) + (sum & 0xffff);
    buf1++;
  }
  for (int i = 0; i < 3; i++) {
    sum += transEnd(*buf2);  // 大端转小端
    sum = (sum >> 16) + (sum & 0xffff);
    buf2++;
  }
  for (int i = 0; i < nword; i++) {
    sum += transEnd(*buf3);  // 大端转小端
    sum = (sum >> 16) + (sum & 0xffff);
    buf3++;
  }
  return (unsigned short)(~sum);
}

// buf1伪首部固定12字节，buf2：udp首部+udpdata，nword指的是udp首部+udpdata的长度
// ！！前两个参数按大端解释
unsigned short udpChecksum(unsigned short *buf1, unsigned short *buf2, int nword) {
  unsigned long sum = 0;
  for (int i = 0; i < 6; i++) {
    sum += transEnd(*buf1);  // 大端转小端
    sum = (sum >> 16) + (sum & 0xffff);
    buf1++;
  }

  for (int i = 0; i < nword; i++) {
    if (i != 3) {              // checksum自身
      sum += transEnd(*buf2);  // 大端转小端
      sum = (sum >> 16) + (sum & 0xffff);
      buf2++;
    }
  }
  return (unsigned short)(~sum);
}

}  // namespace vpskit
