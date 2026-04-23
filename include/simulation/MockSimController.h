#ifndef MOCK_SIM_CONTROLLER_H
#define MOCK_SIM_CONTROLLER_H

#include "simulation/SimControlInterface.h"
#include "scripting/EntityScriptManager.h"
#include <behaviortree_cpp_v3/bt_factory.h>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <functional>
#include <vector>
#include <string>
#include <atomic>

namespace scripting {
    class EntityScriptManager;
}

// Hash function for VehicleID
struct VehicleIDHash {
    std::size_t operator()(const VehicleID& vid) const {
        return std::hash<int>()(vid.address.site) ^
               (std::hash<int>()(vid.address.host) << 1) ^
               (std::hash<int>()(vid.vehicle) << 2);
    }
};

// Extended Entity structure with movement direction
struct EntityExt : public Entity {
    double dx, dy, dz;  // Movement direction
    
    EntityExt() : Entity(), dx(0), dy(0), dz(0) {}
    EntityExt(const VehicleID& vid, const std::string& t, double xPos, double yPos, double zPos)
        : Entity(vid, t, xPos, yPos, zPos), dx(0), dy(0), dz(0) {}
};

// Mock simulation controller implementing the SimControlInterface
class MockSimController : public SimControlInterface {
public:
    MockSimController();
    ~MockSimController();

    // SimControlInterface implementation
    bool start() override;
    bool pause() override;
    bool resume() override;
    bool stop() override;
    bool reset() override;

    SimState getState() const override;
    bool isRunning() const override;
    bool isPaused() const override;
    bool isStopped() const override;

    double getSimTime() const override;
    double getTimeStep() const override;
    void setTimeScale(double scale) override;
    double getTimeScale() const override;

    VehicleID addEntity(const std::string& type, double x, double y, double z) override;
    bool removeEntity(const VehicleID& vehicleId) override;
    bool moveEntity(const VehicleID& vehicleId, double x, double y, double z) override;
    bool getEntityPosition(const VehicleID& vehicleId, double& x, double& y, double& z) override;
    std::vector<Entity> getAllEntities() override;
    size_t getEntityCount() override;

    bool setEntityMoveDirection(const VehicleID& vehicleId, double dx, double dy, double dz) override;
    double getEntityDistance(const VehicleID& vehicleId, double x, double y, double z) override;

    // Set callbacks
    void setOnStartCallback(std::function<void()> callback) override;
    void setOnPauseCallback(std::function<void()> callback) override;
    void setOnResumeCallback(std::function<void()> callback) override;
    void setOnStopCallback(std::function<void()> callback) override;
    void setOnResetCallback(std::function<void()> callback) override;

    // Additional mock-specific methods
    void setVerbose(bool verbose);
    bool isVerbose() const;

    // Entity script manager methods (C++ layer only)
    // Dependencies are obtained from singletons (LuaSimBinding and BehaviorTreeExecutor)
    std::shared_ptr<scripting::EntityScriptManager> createScriptManager(const std::string& entityId);
    bool removeScriptManager(const std::string& entityId);
    std::shared_ptr<scripting::EntityScriptManager> getScriptManager(const std::string& entityId) const;
    bool hasScriptManager(const std::string& entityId) const;

    // Script management methods (C++ layer)
    bool addScriptToEntity(const std::string& entityId, 
                           const std::string& scriptName,
                           const std::string& scriptCode);
    bool addScriptToEntityFromFile(const std::string& entityId,
                                    const std::string& scriptName,
                                    const std::string& filePath);
    bool removeScriptFromEntity(const std::string& entityId,
                                 const std::string& scriptName);
    bool enableEntityScript(const std::string& entityId,
                            const std::string& scriptName);
    bool disableEntityScript(const std::string& entityId,
                             const std::string& scriptName);
    std::vector<std::string> getEntityScriptNames(const std::string& entityId) const;

private:
    VehicleID generateVehicleId();
    void executeAllEntityScripts();

    mutable std::mutex mutex_;
    SimState state_;
    double simTime_;
    double timeStep_;
    double timeScale_;
    bool verbose_;

    std::unordered_map<VehicleID, EntityExt, VehicleIDHash> entities_;
    std::atomic<int> nextVehicleId_;

    std::function<void()> onStartCallback_;
    std::function<void()> onPauseCallback_;
    std::function<void()> onResumeCallback_;
    std::function<void()> onStopCallback_;
    std::function<void()> onResetCallback_;

    bool running_;
    bool paused_;

    std::unordered_map<std::string, std::shared_ptr<scripting::EntityScriptManager>> entityScriptManagers_;
};

#endif // MOCK_SIM_CONTROLLER_H
