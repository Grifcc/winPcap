#ifndef VPSKIT_CLASS_COMMON_SINGLETON_H
#define VPSKIT_CLASS_COMMON_SINGLETON_H

#ifndef DEFINES2163A_H
#define DEFINES2163A_H

#define VPS2163A_WIDTH 27200
#define VPS2163A_HEIGHT 20480
#define VPS2163A_10BITS_WIDTH (VPS2163A_WIDTH / 25 * 32)
#define VPS2163A_16BITS_WIDTH (VPS2163A_WIDTH * 2)
#define VPS2163A_8BITS_SIZE (VPS2163A_WIDTH * VPS2163A_HEIGHT)          // 531.25MiB
#define VPS2163A_10BITS_SIZE (VPS2163A_10BITS_WIDTH * VPS2163A_HEIGHT)  // 680MiB
#define VPS2163A_16BITS_SIZE (VPS2163A_16BITS_WIDTH * VPS2163A_HEIGHT)  // 1062.5MiB

#endif  // DEFINES2163A_H

namespace vpskit {

class NonCopyable {
 protected:
  NonCopyable() = default;
  ~NonCopyable() = default;

 public:
  NonCopyable(const NonCopyable &) = delete;             // Copy construct
  NonCopyable(NonCopyable &&) = delete;                  // Move construct
  NonCopyable &operator=(const NonCopyable &) = delete;  // Copy assign
  NonCopyable &operator=(NonCopyable &&) = delete;       // Move assign
};

template <typename T>
class Singleton : public NonCopyable {
 public:
  static T &instance() {
    static T _instance;
    return _instance;
  }
};

}  // namespace vpskit

#endif  // VPSKIT_CLASS_COMMON_SINGLETON_H
