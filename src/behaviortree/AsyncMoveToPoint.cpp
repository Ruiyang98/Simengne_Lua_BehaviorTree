#include "behaviortree/AsyncMoveToPoint.h"
#include "behaviortree/SimControllerPtr.h"
#include "behaviortree/BlackboardKeys.h"
#include <iostream>
#include <cmath>

namespace behaviortree {

AsyncMoveToPoint::AsyncMoveToPoint(const std::string& name, const BT::NodeConfiguration& config)
    : BT::StatefulActionNode(name, config)
    , targetX_(0.0)
    , targetY_(0.0)
    , targetZ_(0.0)
    , speed_(1.0)
    , arrivalThreshold_(0.5) {}

BT::PortsList AsyncMoveToPoint::providedPorts() {
    return {
        BT::InputPort<double>("x", "Target X coordinate"),
        BT::InputPort<double>("y", "Target Y coordinate"),
        BT::InputPort<double>("z", 0.0, "Target Z coordinate (default 0)"),
        BT::InputPort<double>("speed", 1.0, "Movement speed per tick (default 1.0)"),
        BT::InputPort<double>("threshold", 0.5, "Arrival threshold (default 0.5)")
    };
}

BT::NodeStatus AsyncMoveToPoint::onStart() {
    // Get target coordinates from input ports
    auto x = getInput<double>("x");
    auto y = getInput<double>("y");
    auto z = getInput<double>("z");
    auto speed = getInput<double>("speed");
    auto threshold = getInput<double>("threshold");

    if (!x || !y) {
        std::cerr << "[AsyncMoveToPoint] Missing target coordinates" << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    targetX_ = x.value();
    targetY_ = y.value();
    targetZ_ = z.value();
    speed_ = speed.value();
    arrivalThreshold_ = threshold.value();

    // Get entity ID from blackboard
    auto blackboard = config().blackboard;
    if (!blackboard) {
        std::cerr << "[AsyncMoveToPoint] No blackboard available" << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    try {
        entityId_ = blackboard->get<std::string>(BlackboardKeys::ENTITY_ID);
    } catch (const std::exception& e) {
        std::cerr << "[AsyncMoveToPoint] Failed to get entity_id from blackboard: " << e.what() << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    // Verify entity exists
    simulation::SimControlInterface* simController = getSimController();
    if (!simController) {
        std::cerr << "[AsyncMoveToPoint] SimController not available" << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    double currentX, currentY, currentZ;
    if (!simController->getEntityPosition(entityId_, currentX, currentY, currentZ)) {
        std::cerr << "[AsyncMoveToPoint] Entity not found: " << entityId_ << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    std::cout << "[AsyncMoveToPoint] Starting move for entity " << entityId_
              << " from (" << currentX << ", " << currentY << ", " << currentZ << ")"
              << " to (" << targetX_ << ", " << targetY_ << ", " << targetZ_ << ")"
              << " with speed " << speed_ << std::endl;

    // Check if already at destination
    if (hasArrived(currentX, currentY, currentZ)) {
        std::cout << "[AsyncMoveToPoint] Already at destination" << std::endl;
        return BT::NodeStatus::SUCCESS;
    }

    return BT::NodeStatus::RUNNING;
}

BT::NodeStatus AsyncMoveToPoint::onRunning() {
    simulation::SimControlInterface* simController = getSimController();
    if (!simController) {
        std::cerr << "[AsyncMoveToPoint] SimController not available" << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    // Get current position
    double currentX, currentY, currentZ;
    if (!simController->getEntityPosition(entityId_, currentX, currentY, currentZ)) {
        std::cerr << "[AsyncMoveToPoint] Entity not found during movement: " << entityId_ << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    // Check if arrived
    if (hasArrived(currentX, currentY, currentZ)) {
        std::cout << "[AsyncMoveToPoint] Entity " << entityId_ << " arrived at destination ("
                  << targetX_ << ", " << targetY_ << ", " << targetZ_ << ")" << std::endl;
        return BT::NodeStatus::SUCCESS;
    }

    // Move towards target
    moveTowards(currentX, currentY, currentZ);

    // Update entity position
    simController->moveEntity(entityId_, currentX, currentY, currentZ);

    return BT::NodeStatus::RUNNING;
}

void AsyncMoveToPoint::onHalted() {
    std::cout << "[AsyncMoveToPoint] Movement halted for entity " << entityId_ << std::endl;
}

bool AsyncMoveToPoint::hasArrived(double currentX, double currentY, double currentZ) const {
    double dx = targetX_ - currentX;
    double dy = targetY_ - currentY;
    double dz = targetZ_ - currentZ;
    double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    return distance <= arrivalThreshold_;
}

void AsyncMoveToPoint::moveTowards(double& currentX, double& currentY, double& currentZ) const {
    double dx = targetX_ - currentX;
    double dy = targetY_ - currentY;
    double dz = targetZ_ - currentZ;
    double distance = std::sqrt(dx * dx + dy * dy + dz * dz);

    if (distance <= speed_) {
        // Can reach target in this tick
        currentX = targetX_;
        currentY = targetY_;
        currentZ = targetZ_;
    } else {
        // Move speed_ units towards target
        double ratio = speed_ / distance;
        currentX += dx * ratio;
        currentY += dy * ratio;
        currentZ += dz * ratio;
    }
}

} // namespace behaviortree
