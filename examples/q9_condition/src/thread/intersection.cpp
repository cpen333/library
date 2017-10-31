#include <iostream>
#include <thread>

#include <cpen333/util.h>
#include <cpen333/thread/condition.h>

//
// Conditions allow threads to wait until it is notified to let them pass.  Once notified, the condition will continue
// letting threads pass until the condition is reset.
//
// In this way, Conditions act like a gate.  Once notified, the gate opens to let everyone through.  When reset, the
// gate is closed again, blocking anyone at the door.
//

// pedestrian thread function
void pedestrian(int id, cpen333::thread::condition& safetowalk) {
  // record current time for termination condition
  typedef std::chrono::steady_clock clock;
  auto start_time = clock::now();

  // loop for 60 seconds
  while(std::chrono::duration_cast<std::chrono::seconds>(clock::now()-start_time).count() < 60) {
    std::this_thread::sleep_for(std::chrono::milliseconds(600*id));		// sleep for a time period
    safetowalk.wait() ;				 // wait for walk sign
    std::cout << "P" << id << " ";   // show that I have crossed
    std::cout.flush();
  }
}

// car thread function
void car(int id, cpen333::thread::condition& safetodrive) {

  // record current time for termination condition
  typedef std::chrono::steady_clock clock;
  auto start_time = clock::now();

  // loop for 60 seconds
  while(std::chrono::duration_cast<std::chrono::seconds>(clock::now()-start_time).count() < 60) {
    std::this_thread::sleep_for(std::chrono::milliseconds(300*id));		// sleep for a time period
    safetodrive.wait();              // wait for green light
    std::cout << "C" << id << " ";   // show that I have crossed
    std::cout.flush();
  }
}

int main() {

  std::thread *pedestrians[10]; // 10 pedestrians
  std::thread *cars[10];       // 10 cars

  // conditions
  cpen333::thread::condition safetowalk;
  cpen333::thread::condition safetodrive;

  for(int i = 0; i < 10; i++) {
    pedestrians[i] = new std::thread(pedestrian, i+1,std::ref(safetowalk));
    cars[i]  = new std::thread(car, i+1, std::ref(safetodrive));
  }

  // loop for about a minute
  typedef std::chrono::steady_clock clock;
  auto start_time = clock::now();
  while(std::chrono::duration_cast<std::chrono::seconds>(clock::now()-start_time).count() < 60)	{
    std::cout << std::endl << "Green Light" << std::endl;
    safetodrive.notify();                                   // allow cars to drive over pedestrian crossing
    std::this_thread::sleep_for(std::chrono::seconds(10));  // wait a suitable time delay between lights changing
    safetodrive.reset();                                    // stop cars
    std::cout << std::endl << "Red Light" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));   // wait a suitable time delay between lights changing
    std::cout << std::endl << "Walk on" << std::endl;
    safetowalk.notify();                                    // allow pedestrians to cross
    std::this_thread::sleep_for(std::chrono::seconds(15));  // allow pedestrians to cross for 15 seconds
    safetowalk.reset();                                     // stop pedestrians
    std::cout << std::endl <<  "No Walking" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));   // wait a suitable time delay between lights changing

  }

  // clean up memory
  for (int i=0; i<10; ++i) {
    pedestrians[i]->join();
    cars[i]->join();
    delete pedestrians[i];
    delete cars[i];
  }

  std::cout << "Done." << std::endl;

  cpen333::pause();

  return 0;
}