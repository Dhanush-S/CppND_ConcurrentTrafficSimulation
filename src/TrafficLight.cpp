#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

 
template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 

    std::unique_lock<std::mutex> unqLck(_mutex);
    // wait till notify_one is called by another thread
    // check also _isDataAvailable to handle spurious wakeups
    _cond.wait(unqLck, [this]{return (!_queue.empty() && _isDataAvailable);}); 
    
    _isDataAvailable = false;
    T msg = std::move(_queue.back()); // move the message from queue to temp
    _queue.pop_back(); // pop the element which was moved from the queue
    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

    std::lock_guard<std::mutex> lockGuard(_mutex);
    _queue.push_back(std::move(msg));
    _isDataAvailable = true;
    _cond.notify_one();
}


/* Implementation of class "TrafficLight" */

// source : https://en.cppreference.com/w/cpp/numeric/random/uniform_real_distribution
float TrafficLight::getRandomCycleDuration(float startRange, float endRange)
{
    std::random_device rd;  // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
    std::uniform_real_distribution<> dis(startRange, endRange);

    return dis(gen);
}

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while(true)
    {
        auto trafficLightPhase = _messageQueue.receive();
        if (trafficLightPhase == TrafficLightPhase::green)
            return;
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::setCurrentPhase(TrafficLightPhase trafficLightPhase)
{
    _currentPhase = trafficLightPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.
    auto startTime = std::chrono::high_resolution_clock::now();
    float cycleDuration = getRandomCycleDuration(4.0,6.0);

    while(true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // compute updateTime 
        auto updateTime = std::chrono::duration_cast<std::chrono::seconds> (std::chrono::high_resolution_clock::now() - startTime).count();

        // if updateTime is greater than cycleDuration in seconds
        if(updateTime > cycleDuration)
        {
            std::unique_lock<std::mutex> unqLock(_mutex);
            (_currentPhase == TrafficLightPhase::red) ? (_currentPhase = TrafficLightPhase::green) : (_currentPhase = TrafficLightPhase::red);
            // copy to local variable in order to not remove ownership of _currentPhase 
            auto temp = _currentPhase;
            _messageQueue.send(std::move(temp));
            unqLock.unlock();

            // set random cycle duration for next traffic phase update 
            cycleDuration = getRandomCycleDuration(4.0,6.0);

            // update startTime for the next cycle
            startTime = std::chrono::high_resolution_clock::now();
        }
    } 
}