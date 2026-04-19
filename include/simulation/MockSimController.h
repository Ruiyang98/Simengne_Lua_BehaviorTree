#ifndef MOCK_SIM_CONTROLLER_H
#define MOCK_SIM_CONTROLLER_H

#include "simulation/SimControlInterface.h"
#include <chrono>
#include <thread>
#include <iostream>

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

private:
    void runSimulationLoop();
    void notifyStart();
    void notifyPause();
    void notifyResume();
    void notifyStop();
    void notifyReset();

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
};

} // namespace simulation

#endif // MOCK_SIM_CONTROLLER_H
