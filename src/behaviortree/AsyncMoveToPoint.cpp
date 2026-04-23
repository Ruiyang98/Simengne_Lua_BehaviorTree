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
    , arrivalThreshold_(0.5) {}

BT::PortsList AsyncMoveToPoint::providedPorts() {
    return {
        BT::InputPort<double>("x", "Target X coordinate"),
        BT::InputPort<double>("y", "Target Y coordinate"),
        BT::InputPort<double>("z", 0.0, "Target Z coordinate (default 0)"),
        BT::InputPort<double>("threshold", 0.5, "Arrival threshold (default 0.5)")
    };
}

BT::NodeStatus AsyncMoveToPoint::onStart() {
    // Get target coordinates from input ports
    auto x = getInput<double>("x");
    auto y = getInput<double>("y");
    auto z = getInput<double>("z");
    auto threshold = getInput<double>("threshold");

    if (!x || !y) {
        std::cerr << "[AsyncMoveToPoint] Missing target coordinates" << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    targetX_ = x.value();
    targetY_ = y.value();
    targetZ_ = z.value();
    arrivalThreshold_ = threshold.value();

    // Get vehicle ID from blackboard
    auto blackboard = config().blackboard;
    if (!blackboard) {
        std::cerr << "[AsyncMoveToPoint] No blackboard available" << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    try {
        vehicleId_ = blackboard->get<simulation::VehicleID>(BlackboardKeys::VEHICLE_ID);
    } catch (const std::exception& e) {
        std::cerr << "[AsyncMoveToPoint] Failed to get vehicle_id from blackboard: " << e.what() << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    // Get simulation controller
    simulation::SimControlInterface* simController = getSimController();
    if (!simController) {
        std::cerr << "[AsyncMoveToPoint] SimController not available" << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    // Get current position
    double currentX, currentY, currentZ;
    if (!simController->getEntityPosition(vehicleId_, currentX, currentY, currentZ)) {
        std::cerr << "[AsyncMoveToPoint] Entity not found: vehicle=" << vehicleId_.vehicle << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    // Use getEntityDistance to check if already at destination
    double distance = simController->getEntityDistance(vehicleId_, targetX_, targetY_, targetZ_);

    std::cout << "[AsyncMoveToPoint] Starting move for entity vehicle=" << vehicleId_.vehicle
              << " from (" << currentX << ", " << currentY << ", " << currentZ << ")"
              << " to (" << targetX_ << ", " << targetY_ << ", " << targetZ_ << ")"
              << " distance: " << distance << std::endl;

    // Check if already at destination
    if (distance <= arrivalThreshold_) {
        std::cout << "[AsyncMoveToPoint] Already at destination" << std::endl;
        return BT::NodeStatus::SUCCESS;
    }

    // Calculate direction vector
    double dx = targetX_ - currentX;
    double dy = targetY_ - currentY;
    double dz = targetZ_ - currentZ;

    // Set movement direction
    if (!simController->setEntityMoveDirection(vehicleId_, dx, dy, dz)) {
        std::cerr << "[AsyncMoveToPoint] Failed to set move direction for entity: vehicle=" << vehicleId_.vehicle << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    return BT::NodeStatus::RUNNING;
}

BT::NodeStatus AsyncMoveToPoint::onRunning() {
    simulation::SimControlInterface* simController = getSimController();
    if (!simController) {
        std::cerr << "[AsyncMoveToPoint] SimController not available" << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    // Use getEntityDistance to check if arrived
    double distance = simController->getEntityDistance(vehicleId_, targetX_, targetY_, targetZ_);

    if (distance <= arrivalThreshold_) {
        std::cout << "[AsyncMoveToPoint] Entity vehicle=" << vehicleId_.vehicle << " arrived at destination ("
                  << targetX_ << ", " << targetY_ << ", " << targetZ_ << ")" << std::endl;

        // Stop movement
        simController->setEntityMoveDirection(vehicleId_, 0, 0, 0);

        return BT::NodeStatus::SUCCESS;
    }

    // Still moving towards target, continue returning RUNNING
    return BT::NodeStatus::RUNNING;
}

void AsyncMoveToPoint::onHalted() {
    std::cout << "[AsyncMoveToPoint] Movement halted for entity vehicle=" << vehicleId_.vehicle << std::endl;

    // Stop movement
    simulation::SimControlInterface* simController = getSimController();
    if (simController) {
        simController->setEntityMoveDirection(vehicleId_, 0, 0, 0);
    }
}

} // namespace behaviortree
