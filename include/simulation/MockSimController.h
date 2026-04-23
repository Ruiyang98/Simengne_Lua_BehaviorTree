#ifndef MOCK_SIM_CONTROLLER_H
#define MOCK_SIM_CONTROLLER_H

#include "simulation/SimControlInterface.h"
#include <behaviortree_cpp_v3/bt_factory.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <map>
#include <atomic>
#include <unordered_map>
#include <memory>

namespace scripting {
    class EntityScriptManager;
}

class MockSimController : public SimControlInterface {
public:
    // Singleton access
    static MockSimController* getInstance();
    static void setInstance(MockSimController* instance);
    
    // Create instance (for initial setup)
    static MockSimController* createInstance();
    static void destroyInstance();

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
    VehicleID addEntity(const std::string& type, double x, double y, double z);
    bool removeEntity(const VehicleID& entityId);
    bool moveEntity(const VehicleID& entityId, double x, double y, double z);
    bool getEntityPosition(const VehicleID& entityId, double& x, double& y, double& z);
    std::vector<Entity> getAllEntities();
    size_t getEntityCount();

    // New movement interface
    bool setEntityMoveDirection(const VehicleID& entityId, double dx, double dy, double dz);
    double getEntityDistance(const VehicleID& entityId, double x, double y, double z);

    // Script manager methods
    void setBehaviorTreeFactory(BT::BehaviorTreeFactory* factory);
    std::shared_ptr<scripting::EntityScriptManager> createScriptManager(const std::string& entityId);
    bool removeScriptManager(const std::string& entityId);
    std::shared_ptr<scripting::EntityScriptManager> getScriptManager(const std::string& entityId);
    bool hasScriptManager(const std::string& entityId) const;
    std::vector<std::string> getManagedEntityIds() const;

private:
    // Private constructor for singleton
    MockSimController();
    ~MockSimController();

    // Disable copy and assignment
    MockSimController(const MockSimController&) = delete;
    MockSimController& operator=(const MockSimController&) = delete;

    void runSimulationLoop();
    void notifyStart();
    void notifyPause();
    void notifyResume();
    void notifyStop();
    void notifyReset();
    VehicleID generateVehicleId();

    // Singleton instance
    static MockSimController* instance_;

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
    std::map<VehicleID, Entity> entities_;
    std::atomic<int> nextVehicleId_;

    // Script managers
    std::unordered_map<std::string, std::shared_ptr<scripting::EntityScriptManager>> entityScriptManagers_;
    BT::BehaviorTreeFactory* btFactory_;

    // Script update
    void updateScripts(double deltaTime);
    double scriptUpdateAccumulator_;
    static constexpr double SCRIPT_UPDATE_INTERVAL = 0.5; // 500ms
};

#endif // MOCK_SIM_CONTROLLER_H
