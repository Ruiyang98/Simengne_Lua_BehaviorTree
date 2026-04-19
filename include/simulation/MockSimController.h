#ifndef MOCK_SIM_CONTROLLER_H
#define MOCK_SIM_CONTROLLER_H

#include "simulation/SimControlInterface.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <map>
#include <atomic>

namespace simulation {

class MockSimController : public SimControlInterface {
public:
    MockSimController();
    ~MockSimController();

    // Control commands
    bool start();
    bool pause();
    bool resume();
    bool stop();
    bool reset();

    // State queries
    SimState getState() const;
    bool isRunning() const;
    bool isPaused() const;
    bool isStopped() const;
    double getSimTime() const;
    double getTimeStep() const;

    // Speed control
    void setTimeScale(double scale);
    double getTimeScale() const;

    // Event callbacks
    void setOnStartCallback(SimEventCallback callback);
    void setOnPauseCallback(SimEventCallback callback);
    void setOnResumeCallback(SimEventCallback callback);
    void setOnStopCallback(SimEventCallback callback);
    void setOnResetCallback(SimEventCallback callback);

    // MockSimController specific methods
    void update(double deltaTime);
    void setAutoUpdate(bool enable);
    void setVerbose(bool verbose);

    // Entity management implementation
    std::string addEntity(const std::string& type, double x, double y, double z);
    bool removeEntity(const std::string& entityId);
    bool moveEntity(const std::string& entityId, double x, double y, double z);
    bool getEntityPosition(const std::string& entityId, double& x, double& y, double& z);
    std::vector<Entity> getAllEntities();
    size_t getEntityCount();

private:
    void runSimulationLoop();
    void notifyStart();
    void notifyPause();
    void notifyResume();
    void notifyStop();
    void notifyReset();
    std::string generateEntityId();

    // State
    volatile int state_;
    volatile double simTime_;
    volatile double timeScale_;
    double timeStep_;

    // Callbacks
    SimEventCallback onStartCallback_;
    SimEventCallback onPauseCallback_;
    SimEventCallback onResumeCallback_;
    SimEventCallback onStopCallback_;
    SimEventCallback onResetCallback_;

    // Auto update thread
    volatile bool autoUpdate_;
    volatile bool running_;
    std::thread updateThread_;

    // Config
    bool verbose_;

    // Entity storage
    std::map<std::string, Entity> entities_;
    std::atomic<uint64_t> nextEntityId_;
};

} // namespace simulation

#endif // MOCK_SIM_CONTROLLER_H
