#include "planner.hpp"

using namespace std::chrono_literals;

using SolvePuzzle = interfaces::action::SolvePuzzle;

Planner::Planner(const std::string &name) : LifecycleNode(name, "") {
    if (name.empty()) {
        throw std::invalid_argument("Empty node name");
    }

}

Planner::~Planner() {}

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  Planner::on_configure(const rclcpp_lifecycle::State &)
  {
    // Create IdentifyVision client callback
    callback_group_client_identify_piece_ = this->create_callback_group(
        rclcpp::CallbackGroupType::MutuallyExclusive);
    client_identify_piece_ = this->create_client<interfaces::srv::IdentifyPiece>("vision/identify_piece",
        rmw_qos_profile_services_default, callback_group_client_identify_piece_);

    // Create LocatePieces client callback
    callback_group_client_locate_pieces_ = this->create_callback_group(
        rclcpp::CallbackGroupType::MutuallyExclusive);
    client_locate_pieces_ = this->create_client<interfaces::srv::LocatePieces>("vision/locate_pieces",
        rmw_qos_profile_services_default, callback_group_client_locate_pieces_);

    // Create SolvePuzzle action callback
    callback_group_action_solve_puzzle_ = this->create_callback_group(
      rclcpp::CallbackGroupType::MutuallyExclusive);
    const rcl_action_server_options_t & options = rcl_action_server_get_default_options();

    action_solve_puzzle_ = rclcpp_action::create_server<SolvePuzzle>(
      this,
      "planner/solve_puzzle",
      std::bind(&Planner::handleGoal, this, std::placeholders::_1, std::placeholders::_2),
      std::bind(&Planner::handleCancel, this, std::placeholders::_1),
      std::bind(&Planner::solvePuzzleAccepted, this, std::placeholders::_1),
      options,
      callback_group_action_solve_puzzle_);

    RCLCPP_INFO(get_logger(), "on_configure() is called.");

    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
  }

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  Planner::on_activate(const rclcpp_lifecycle::State &)
  {

    RCLCPP_INFO(get_logger(), "on_activate() is called.");

    // Let's sleep for 2 seconds.
    // We emulate we are doing important
    // work in the activating phase.
    std::this_thread::sleep_for(2s);

    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
  }

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  Planner::on_deactivate(const rclcpp_lifecycle::State &)
  {

    RCLCPP_INFO(get_logger(), "on_deactivate() is called.");

    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
  }

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  Planner::on_cleanup(const rclcpp_lifecycle::State &)
  {

    RCLCPP_INFO(get_logger(), "on cleanup is called.");

    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
  }

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  Planner::on_error(const rclcpp_lifecycle::State &)
  {

    RCLCPP_INFO(get_logger(), "on error is called.");

    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
  }

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  Planner::on_shutdown(const rclcpp_lifecycle::State & state)
  {

    RCUTILS_LOG_INFO_NAMED(
      get_name(),
      "on shutdown is called from state %s.",
      state.label().c_str());

    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
  }

  bool Planner::requestIdentifyPiece()
  {
    if (!client_identify_piece_->wait_for_service(std::chrono::seconds(1))) {
      RCLCPP_ERROR(get_logger(), "vision/identify_piece not available after waiting");
      return false;
    }

    auto request = std::make_shared<interfaces::srv::IdentifyPiece::Request>();
    auto future_result = client_identify_piece_->async_send_request(request);

    if (future_result.wait_for(std::chrono::seconds(10)) != std::future_status::ready)
    {
      RCLCPP_ERROR(get_logger(), "vision/identify_piece no response");
      return false;
    } else {
      auto response_value = future_result.get();
      RCLCPP_INFO(get_logger(), "IdentifyPiece: %d %d", response_value->piece.piece_id,response_value->piece.piece_orientation);
      return true;
    }
  }

  bool Planner::requestLocatePieces()
  {
    if (!client_locate_pieces_->wait_for_service(std::chrono::seconds(1))) {
      RCLCPP_ERROR(get_logger(), "vision/identify_piece not available after waiting");
      return false;
    }

    auto request = std::make_shared<interfaces::srv::LocatePieces::Request>();
    auto future_result = client_locate_pieces_->async_send_request(request);

    if (future_result.wait_for(std::chrono::seconds(10)) != std::future_status::ready)
    {
      RCLCPP_ERROR(get_logger(), "vision/identify_piece no response");
      return false;
    } else {
      auto response_value = future_result.get();

      RCLCPP_INFO(get_logger(), "LocatePieces:");
      for (interfaces::msg::PiecePose i : response_value->poses) {
        RCLCPP_INFO(get_logger(), "x: %f", i.piece_x);
        RCLCPP_INFO(get_logger(), "y: %f", i.piece_y);
        RCLCPP_INFO(get_logger(), "w: %f", i.piece_w);
      }
      return true;
    }
  }

  rclcpp_action::GoalResponse Planner::handleGoal(
      const rclcpp_action::GoalUUID & uuid,
      std::shared_ptr<const SolvePuzzle::Goal> goal)
  {
    RCLCPP_INFO(get_logger(), "SolvePuzzle action handled");
    return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
  }

  rclcpp_action::CancelResponse Planner::handleCancel(
      const std::shared_ptr<rclcpp_action::ServerGoalHandle<SolvePuzzle>> goal_handle)
  {
    RCLCPP_INFO(this->get_logger(), "SolvePuzzle action canceled");
    return rclcpp_action::CancelResponse::ACCEPT;
  }

  void Planner::solvePuzzleAccepted(
      const std::shared_ptr<rclcpp_action::ServerGoalHandle<SolvePuzzle>> goal_handle)
  {
    RCLCPP_INFO(this->get_logger(), "SolvePuzzle action accepted");
    std::thread{std::bind(&Planner::executeSolvePuzzle, this, std::placeholders::_1), goal_handle}.detach();
  }

  void Planner::executeSolvePuzzle(
      const std::shared_ptr<rclcpp_action::ServerGoalHandle<SolvePuzzle>> goal_handle)
  {
    RCLCPP_INFO(get_logger(), "SolvePuzzle action executing");

    auto result = std::make_shared<SolvePuzzle::Result>();
    
    if(!requestIdentifyPiece()){
      result->success = false;
      goal_handle->succeed(result);
      return;
    }

    if(!requestLocatePieces()){
      result->success = false;
      goal_handle->succeed(result);
      return;
    }

    result->success = true;
    goal_handle->succeed(result);
    RCLCPP_INFO(get_logger(), "SolvePuzzle action completed");
    return;
  }

/**
 * A lifecycle node has the same node API
 * as a regular node. This means we can spawn a
 * node, give it a name and add it to the executor.
 */
int main(int argc, char * argv[])
{
  // force flush of the stdout buffer.
  // this ensures a correct sync of all prints
  // even when executed simultaneously within the launch file.
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  rclcpp::init(argc, argv);

  rclcpp::executors::MultiThreadedExecutor exe;

  std::shared_ptr<Planner> lc_node =
    std::make_shared<Planner>("planner");

  exe.add_node(lc_node->get_node_base_interface());

  exe.spin();

  rclcpp::shutdown();

  return 0;
}
