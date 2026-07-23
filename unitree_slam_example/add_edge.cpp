#include <unitree/robot/channel/channel_publisher.hpp>
#include <unitree/ros2_idl/QtEdge_.hpp>

#define TOPIC "rt/qt_add_edge"
using namespace unitree::robot;

int main(int argc, const char **argv)
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " networkInterface" << std::endl;
        exit(-1);
    }

    ChannelFactory::Instance()->Init(0, argv[1]); // argv[1]：The name of the network card with network segment 123
    ChannelPublisherPtr<unitree_interfaces::msg::dds_::QtEdge_> publisher = ChannelPublisherPtr<unitree_interfaces::msg::dds_::QtEdge_>(new ChannelPublisher<unitree_interfaces::msg::dds_::QtEdge_>(TOPIC));
    publisher->InitChannel();
    unitree_interfaces::msg::dds_::QtEdge_ send_msg;

    // Build an edge connecting topology node 0 and topology node 1. Before adding edges, ensure that the start and end node exist
    send_msg.seq_().data() = "index:123;";            // The value 123 is set by the user (1-10000)
    send_msg.edge_().edge_name_().push_back(1);       // Edge name
    send_msg.edge_().start_node_name_().push_back(0); // Start node name
    send_msg.edge_().end_node_name_().push_back(1);   // End node name
    send_msg.edge_().dog_speed_().push_back(1);       // The speed of the dog walking this edge (0-1), it is recommended to assign a value of 1
    send_msg.edge_().edge_state_2_().push_back(0);    // 0：Stop 1：Avoid 3：Replan   when encountering obstacles.Suggest assigning a value of 0.

    send_msg.edge_().dog_stats_().push_back(0); // Not open attribute, please assign a value of 0, note: cannot be empty!!! Cannot be empty!!! Cannot be empty!!!
    send_msg.edge_().edge_length_().push_back(0);
    send_msg.edge_().dog_back_stats_().push_back(0);
    send_msg.edge_().edge_state_().push_back(0);
    send_msg.edge_().edge_state_1_().push_back(0);
    send_msg.edge_().edge_state_3_().push_back(0);
    send_msg.edge_().edge_state_4_().push_back(0);

    publisher->Write(send_msg);
    std::cout << "send add edge command" << std::endl;
    sleep(2);

    return 0;
}
