#ifndef SIM_CONTROL_INTERFACE_H
#define SIM_CONTROL_INTERFACE_H

#include <functional>
#include <string>
#include <vector>

// Simulation state enum
enum SimState {
    STOPPED = 0,
    RUNNING = 1,
    PAUSED = 2
};

// Event callback type
typedef std::function<void()> SimEventCallback;

struct SimAddress {
    int site;
    int host;
};


struct VehicleID {
    SimAddress address;
    int vehicle;
};

// Comparison operators for VehicleID
inline bool operator==(const VehicleID& lhs, const VehicleID& rhs) {
    return lhs.address.site == rhs.address.site &&
           lhs.address.host == rhs.address.host &&
           lhs.vehicle == rhs.vehicle;
}

inline bool operator<(const VehicleID& lhs, const VehicleID& rhs) {
    if (lhs.address.site != rhs.address.site) return lhs.address.site < rhs.address.site;
    if (lhs.address.host != rhs.address.host) return lhs.address.host < rhs.address.host;
    return lhs.vehicle < rhs.vehicle;
}

inline bool operator!=(const VehicleID& lhs, const VehicleID& rhs) {
    return !(lhs == rhs);
}


// Entity structure for simulation entities
struct Entity {
    VehicleID id;
    std::string type;
    double x;
    double y;
    double z;
    
    Entity() : x(0.0), y(0.0), z(0.0) {}
    Entity(const VehicleID& entityId, const std::string& entityType, double px, double py, double pz)
        : id(entityId), type(entityType), x(px), y(py), z(pz) {}
};

class SimControlInterface {
public:
    virtual ~SimControlInterface() {}

    // Control commands
    virtual bool start() = 0;
    virtual bool pause() = 0;
    virtual bool resume() = 0;
    virtual bool stop() = 0;
    virtual bool reset() = 0;

    // State queries
    virtual SimState getState() const = 0;
    virtual bool isRunning() const = 0;
    virtual bool isPaused() const = 0;
    virtual bool isStopped() const = 0;
    virtual double getSimTime() const = 0;
    virtual double getTimeStep() const = 0;

    // Speed control
    virtual void setTimeScale(double scale) = 0;
    virtual double getTimeScale() const = 0;

    // Event callbacks
    virtual void setOnStartCallback(SimEventCallback callback) = 0;
    virtual void setOnPauseCallback(SimEventCallback callback) = 0;
    virtual void setOnResumeCallback(SimEventCallback callback) = 0;
    virtual void setOnStopCallback(SimEventCallback callback) = 0;
    virtual void setOnResetCallback(SimEventCallback callback) = 0;

    // Entity management
    virtual VehicleID addEntity(const std::string& type, double x, double y, double z) = 0;
    virtual bool removeEntity(const VehicleID& entityId) = 0;
    virtual bool moveEntity(const VehicleID& entityId, double x, double y, double z) = 0;
    virtual bool getEntityPosition(const VehicleID& entityId, double& x, double& y, double& z) = 0;
    virtual std::vector<Entity> getAllEntities() = 0;
    virtual size_t getEntityCount() = 0;

    // Set entity movement direction
    virtual bool setEntityMoveDirection(const VehicleID& entityId, double dx, double dy, double dz) = 0;

    // Get distance from entity to target point
    virtual double getEntityDistance(const VehicleID& entityId, double x, double y, double z) = 0;

    // Utility
    static std::string stateToString(SimState state);

    // Singleton access - returns the implementation instance
    static SimControlInterface* getInstance();
};

#endif // SIM_CONTROL_INTERFACE_H
