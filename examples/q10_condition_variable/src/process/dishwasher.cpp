#include <iostream>
#include <cstdlib>   // atoi
#include <thread>
#include <cpen333/process/condition_variable.h>
#include <cpen333/process/mutex.h>
#include <cpen333/process/shared_memory.h>
#include "restaurant.h"

int main(int argc, char* argv[]) {

  // get info from arguments
  int id = 0;
  if (argc > 1) {
    id = atoi(argv[1]);
  }

  // grab cv/mutex/data
  cpen333::process::condition_variable cv(CONDITION_VARIABLE_RESTAURANT_CV_NAME);
  cpen333::process::mutex mutex(CONDITION_VARIABLE_RESTAURANT_MUTEX_NAME);
  cpen333::process::shared_object<DishData> dishes(CONDITION_VARIABLE_RESTAURANT_DATA_NAME);


  // protect data
  std::unique_lock<decltype(mutex)> lock(mutex);
  std::cout << "Dishwasher " << id << " arrived" << std::endl;

  // loop while it's not quittin' time
  while (!dishes->quit) {

    std::cout << "Dishwasher " << id << " taking a break" << std::endl;

    // wait until there are at least 50 dirty dishes (or it's quitting time, otherwise we may get stuck here)
    cv.wait(lock, [&dishes]() { return dishes->dirty > 50 || dishes->quit; });

    std::cout << "Dishwasher " << id << " starting to wash dishes" << std::endl;

    // we now re-acquire the lock, let's do some dishes, but let others do some as well
    // this will require some lock juggling, needing to acquire it before accessing the count
    // and releasing while doing the dish so others can do some too

    while (dishes->dirty > 0) {
      --dishes->dirty;    // grab one first, without collisions
      ++dishes->washing;
      lock.unlock();

      // wash the dish
      std::this_thread::sleep_for(std::chrono::milliseconds(10));  // man, that's fast

      // add to stack of clean dishes
      lock.lock();
      --dishes->washing;
      ++dishes->clean;
      lock.unlock();

      // notify people that there's one new clean dish
      cv.notify_all();

      lock.lock();  // need to re-lock to check condition in while loop
    }
  }

  std::cout << "Dishwasher " << id << " leaving" << std::endl;
  return 0;
}
