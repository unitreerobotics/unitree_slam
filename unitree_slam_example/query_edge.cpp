#include <unitree/robot/channel/channel_publisher.hpp>
#include <unitree/ros2_idl/QtCommand_.hpp>

#define TOPIC "rt/qt_command"
using namespace unitree::robot;

int main(int argc, const char **argv)
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " networkInterface" << std::endl;
        exit(-1);
    }
    ChannelFactory::Instance()->Init(0, argv[1]); // argv[1]：The name of the network card with network segment 123
    ChannelPublisherPtr<unitree_interfaces::msg::dds_::QtCommand_> publisher = ChannelPublisherPtr<unitree_interfaces::msg::dds_::QtCommand_>(new ChannelPublisher<unitree_interfaces::msg::dds_::QtCommand_>(TOPIC));
    publisher->InitChannel();
    unitree_interfaces::msg::dds_::QtCommand_ send_msg;

    // Query all edge information
    send_msg.seq_().data() = "index:123;";     // The value 123 is set by the user (1-10000).
    send_msg.command_() = 2;                   // 2 is a query instruction
    send_msg.attribute_() = 2;                 // 2 is select query edge
    send_msg.floor_index_().push_back(999);    // Floor differentiation, please give a fixed value of 999
    send_msg.node_edge_name_().push_back(999); // The list of names of the queried edges. When node_edge_name[0]=999, all edge information will be queried.

    publisher->Write(send_msg);
    std::cout << "send query edge command" << std::endl;
    sleep(2);

    return 0;
}
