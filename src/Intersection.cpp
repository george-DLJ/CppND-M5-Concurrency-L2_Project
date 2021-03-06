#include <iostream>
#include <thread>
#include <chrono>
#include <future>
#include <random>

#include "Street.h"
#include "Intersection.h"
#include "Vehicle.h"

/* Implementation of class "WaitingVehicles" */

int WaitingVehicles::getSize()
{
    return _vehicles.size();
}

void WaitingVehicles::pushBack(std::shared_ptr<Vehicle> vehicle, std::promise<void> &&promise)
{
    _vehicles.push_back(vehicle);
    _promises.push_back(std::move(promise));
}

void WaitingVehicles::permitEntryToFirstInQueue()
{
    // L2.3 : First, get the entries from the front of _promises and _vehicles. 
    // Then, fulfill promise and send signal back that permission to enter has been granted.
    // Finally, remove the front elements from both queues. 
    // Alternative 1: (using front())
    //  Step 1: get entries from front of vectors _promises and _vehicles:
    std::promise<void> &prms = _promises.front();
    std::shared_ptr<Vehicle> v = _vehicles.front();

    //  Step 2: Fulfill promise:
    prms.set_value();
    
    //  Step 3: remove front elements from both queues
    _promises.erase(_promises.begin());
    _vehicles.erase(_vehicles.begin());

    // // Alternative 2: use iterator locals:
    // //  Step 1: get entries from front of vectors _promises and _vehicles:
    // auto prms_it = _promises.begin(); //prms iterator
    // auto v_it = _vehicles.begin();//vehicle iterator
    // //  Step 2: Fulfill promise:
    // prms_it->set_value();
    // //  Step 3: remove the front elements:
    // _promises.erase(prms_it);
    // _vehicles.erase(v_it);

    // // Alternative 3: way whithout locals: (Steps 1-3 together)
    // _promises.front().set_value();
    // _promises.erase(_promises.begin());
    // _vehicles.erase(_vehicles.begin());
}

/* Implementation of class "Intersection" */

Intersection::Intersection()
{
    _type = ObjectType::objectIntersection;
    _isBlocked = false;
}

void Intersection::addStreet(std::shared_ptr<Street> street)
{
    _streets.push_back(street);
}

std::vector<std::shared_ptr<Street>> Intersection::queryStreets(std::shared_ptr<Street> incoming)
{
    // store all outgoing streets in a vector ...
    std::vector<std::shared_ptr<Street>> outgoings;
    for (auto it : _streets)
    {
        if (incoming->getID() != it->getID()) // ... except the street making the inquiry
        {
            outgoings.push_back(it);
        }
    }

    return outgoings;
}

// adds a new vehicle to the queue and returns once the vehicle is allowed to enter
void Intersection::addVehicleToQueue(std::shared_ptr<Vehicle> vehicle)
{
    std::cout << "Intersection #" << _id << "::addVehicleToQueue: thread id = " << std::this_thread::get_id() << std::endl;

    // L2.2 : First, add the new vehicle to the waiting line by creating a promise, a corresponding future and then adding both to _waitingVehicles. 
    // Then, wait until the vehicle has been granted entry. 
    // NOTE: IMPORTANT! the thread will be created on Task L2.3!
    // step 1: create promise: NOTE: see pushBack signature to know the type. In this case void;
    std::promise<void> prms; 
    // step 2: create a future from that promise
    std::future<void> ftr = prms.get_future();
    // step 3: enqueue the veicle (shared_ptr) and move the promise() to _waitingVehicles vector:
    _waitingVehicles.pushBack(vehicle,std::move(prms));
    // step 4: wait until future is fulfilled:
    ftr.wait();


    std::cout << "Intersection #" << _id << ": Vehicle #" << vehicle->getID() << " is granted entry." << std::endl;
}

void Intersection::vehicleHasLeft(std::shared_ptr<Vehicle> vehicle)
{
    //std::cout << "Intersection #" << _id << ": Vehicle #" << vehicle->getID() << " has left." << std::endl;

    // unblock queue processing
    this->setIsBlocked(false);
}

void Intersection::setIsBlocked(bool isBlocked)
{
    _isBlocked = isBlocked;
    //std::cout << "Intersection #" << _id << " isBlocked=" << isBlocked << std::endl;
}

// virtual function which is executed in a thread
void Intersection::simulate() // using threads + promises/futures + exceptions
{
    // launch vehicle queue processing in a thread
    threads.emplace_back(std::thread(&Intersection::processVehicleQueue, this));
}

void Intersection::processVehicleQueue()
{
    // print id of the current thread
    //std::cout << "Intersection #" << _id << "::processVehicleQueue: thread id = " << std::this_thread::get_id() << std::endl;

    // continuously process the vehicle queue
    while (true)
    {
        // sleep at every iteration to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // only proceed when at least one vehicle is waiting in the queue
        if (_waitingVehicles.getSize() > 0 && !_isBlocked)
        {
            // set intersection to "blocked" to prevent other vehicles from entering
            this->setIsBlocked(true);

            // permit entry to first vehicle in the queue (FIFO)
            _waitingVehicles.permitEntryToFirstInQueue();
        }
    }
}
