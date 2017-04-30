#ifndef CPEN333_THREAD_SHARED_MUTEX_H
#define CPEN333_THREAD_SHARED_MUTEX_H

#include <shared_mutex>
#include "cpen333/thread/impl/shared_mutex_shared.h"
#include "cpen333/thread/impl/shared_mutex_exclusive.h"

namespace cpen333 {
namespace thread {

using shared_mutex_fair = std::shared_timed_mutex;
using shared_timed_mutex_fair = std::shared_timed_mutex;

using shared_mutex = std::shared_timed_mutex;
using shared_timed_mutex = std::shared_timed_mutex;

}  // thread
}  // cpen333

#endif //CPEN333_THREAD_SHARED_MUTEX_H