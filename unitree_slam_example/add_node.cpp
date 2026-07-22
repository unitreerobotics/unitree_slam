#include <unitree/robot/channel/channel_publisher.hpp>
#include <unitree/ros2_idl/QtNode_.hpp>

#define TOPIC "rt/qt_add_node"
using namespace unitree::robot;

int main(int argc, const char **argv)
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " networkInterface" << std::endl;
        exit(-1);
    }

    ChannelFactory::Instance()->Init(0, argv[1]); // argv[1]：The name of the network card with network segment 123
    ChannelPublisherPtr<unitree_interfaces::msg::dds_::QtNode_> publisher = ChannelPublisherPtr<unitree_interfaces::msg::dds_::QtNode_>(new ChannelPublisher<unitree_interfaces::msg::dds_::QtNode_>(TOPIC));
    publisher->InitChannel();
    unitree_interfaces::msg::dds_::QtNode_ send_msg;

    send_msg.seq_().data() = "index:123;";              // The value 123 is set by the user (1-10000)
    send_msg.node_().node_name_().push_back(1);         // Node name
    send_msg.node_().node_position_x_().push_back(1.0); // Point X coordinate information
    send_msg.node_().node_position_y_().push_back(1.0); // Point Y coordinate information
    send_msg.node_().node_position_z_().push_back(0);   // Point Z coordinate information
    send_msg.node_().node_yaw_().push_back(1.57);       // Point Yaw Angle Information

    send_msg.node_().node_attribute_().push_back(0); // Not open attribute, please assign a value of 0, note: cannot be empty!!! Cannot be empty!!! Cannot be empty!!!
    send_msg.node_().undefined_().push_back(0);
    send_msg.node_().node_state_2_().push_back(0);
    send_msg.node_().node_state_3_().push_back(0);

    publisher->Write(send_msg);
    std::cout << "send add node command" << std::endl;
    sleep(2);

    return 0;
}
