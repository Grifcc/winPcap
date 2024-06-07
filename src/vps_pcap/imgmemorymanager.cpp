#include "vps_pcap/imgmemorymanager.h"

#include <cstring>
#include <sys/mman.h>
#include <unistd.h>

ImgMemoryManager::~ImgMemoryManager() { releaseMemory(); }

bool ImgMemoryManager::mallocMemoryPool(size_t mem_pool_size)
{
  mem_pool_size_ = mem_pool_size;
  img_nums_ = mem_pool_size_ / static_cast<uint64_t>(WINMODEWIDTH * WINMODEHEIGHT);
  busy_flags_ = new std::atomic<bool>[img_nums_];
  m_pool = new unsigned char *[img_nums_];
  for (int i = 0; i < img_nums_; i++)
  {
    m_pool[i] = new unsigned char[WINMODEWIDTH * WINMODEHEIGHT];
    memset(m_pool[i], 0, WINMODEWIDTH * WINMODEHEIGHT);
    mlock(m_pool[i], WINMODEWIDTH * WINMODEHEIGHT);
    busy_flags_[i].store(false);
  }
  return true;
}

void ImgMemoryManager::setBusy()
{
  busy_flags_[using_point_].store(true);
  using_point_ = -1;
}
void ImgMemoryManager::setFree(int8_t point)
{
  memset(m_pool[point], 0, WINMODEWIDTH * WINMODEHEIGHT);
  busy_flags_[point].store(false);
}

unsigned char *ImgMemoryManager::getFreeMem(int offset)
{
  if (using_point_ == -2) // no free
    return nullptr;
  else if (using_point_ < img_nums_ && using_point_ != -1)
  { // lock
    return m_pool[using_point_];
  }
  else if (using_point_ == -1)
  {
    for (int i = 0; i < img_nums_; i++)
    {
      if (!busy_flags_[i].load())
      {
        using_point_ = i;
        return m_pool[using_point_];
      }
    }
    using_point_ = -2;
  }
  return nullptr;
}
void ImgMemoryManager::releaseMemory()
{
  if (m_pool)
  {
    for (int i = 0; i < img_nums_; i++)
    {
      munlock(m_pool[i], WINMODEWIDTH * WINMODEHEIGHT);
      delete[] m_pool[i];
    }
    delete[] m_pool;
    delete[] busy_flags_;
  }
}
