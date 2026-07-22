#include <unitree/robot/channel/channel_publisher.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>
#include <unitree/ros2_idl/PointCloud2_.hpp>

#define TOPIC "rt/rslidar_points"
using namespace unitree::robot;

void lidarHandler(const void *msg)
{
    const sensor_msgs::msg::dds_::PointCloud2_ *seq = (const sensor_msgs::msg::dds_::PointCloud2_ *)msg;
    std::cout << "sec:" << seq->header().stamp().sec() << "  nanosec:" << seq->header().stamp().nanosec() << std::endl;
}

int main(int argc, const char **argv)
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " networkInterface" << std::endl;
        exit(-1);
    }
    ChannelFactory::Instance()->Init(0, argv[1]); // argv[1]：The name of the network card with network segment 123
    ChannelSubscriberPtr<sensor_msgs::msg::dds_::PointCloud2_> sublidar = ChannelSubscriberPtr<sensor_msgs::msg::dds_::PointCloud2_>(new ChannelSubscriber<sensor_msgs::msg::dds_::PointCloud2_>(TOPIC));
    sublidar->InitChannel(std::bind(lidarHandler, std::placeholders::_1), 10);

    while (true)
    {
        sleep(1);
    }
    return 0;
}
