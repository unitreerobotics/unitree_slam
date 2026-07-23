#include "unitree/common/dds/dds_easy_model.hpp"
#include <unitree/robot/channel/channel_publisher.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>

#include <unitree/ros2_idl/QtCommand_.hpp>
#include <unitree/ros2_idl/String_.hpp>
#include <unitree/ros2_idl/Odometry_.hpp>
#include <unitree/ros2_idl/QtNode_.hpp>
#include <unitree/ros2_idl/QtEdge_.hpp>

#include <stdio.h>
#include <termio.h>
#include <cmath>
#include <ctime>

#define COMMANDTOPIC "rt/qt_command"
#define NOTICETOPIC "rt/qt_notice"
#define ODOMTOPIC "rt/lio_sam_ros2/mapping/re_location_odometry"
#define ADDNODETOPIC "rt/qt_add_node"
#define ADDEDGETOPIC "rt/qt_add_edge"

using namespace unitree::robot;
using namespace std;

struct nodeAttribute
{
    u_int16_t nodeName;
    float nodeX;
    float nodeY;
    float nodeZ;
    float nodeYaw;
};

struct edgeAttribute
{
    u_int16_t edgeName;
    u_int16_t edgeStart;
    u_int16_t edgeEnd;
};

class slamDemo
{
private:
    int index = 0;
    const nav_msgs::msg::dds_::Odometry_ *currentOdom;
    u_int16_t node_name = 0;

    vector<nodeAttribute> nodeAttributeList;
    vector<edgeAttribute> edgeAttributeList;

    // pub
    ChannelPublisherPtr<unitree_interfaces::msg::dds_::QtCommand_> pubQtCommand = ChannelPublisherPtr<unitree_interfaces::msg::dds_::QtCommand_>(new ChannelPublisher<unitree_interfaces::msg::dds_::QtCommand_>(COMMANDTOPIC));
    ChannelPublisherPtr<unitree_interfaces::msg::dds_::QtNode_> pubQtNode = ChannelPublisherPtr<unitree_interfaces::msg::dds_::QtNode_>(new ChannelPublisher<unitree_interfaces::msg::dds_::QtNode_>(ADDNODETOPIC));
    ChannelPublisherPtr<unitree_interfaces::msg::dds_::QtEdge_> pubQtEdge = ChannelPublisherPtr<unitree_interfaces::msg::dds_::QtEdge_>(new ChannelPublisher<unitree_interfaces::msg::dds_::QtEdge_>(ADDEDGETOPIC));
    // sub
    ChannelSubscriberPtr<std_msgs::msg::dds_::String_> subQtNotice = ChannelSubscriberPtr<std_msgs::msg::dds_::String_>(new ChannelSubscriber<std_msgs::msg::dds_::String_>(NOTICETOPIC));
    ChannelSubscriberPtr<nav_msgs::msg::dds_::Odometry_> subOdommetry = ChannelSubscriberPtr<nav_msgs::msg::dds_::Odometry_>(new ChannelSubscriber<nav_msgs::msg::dds_::Odometry_>(ODOMTOPIC));

public:
    slamDemo(const char *networkInterface);

    void qtNoticeHandler(const void *message);
    void odometryHandler(const void *message);

    void keyExecute();
    unsigned char keyDetection();

    void startMapping();
    void endMapping();
    void startRelocation();
    void initPose();
    void startNavigation();
    void defaultNavigation();
    void addNodeAndEdge();
    void addEdge(u_int16_t edge_name, u_int16_t start_node, u_int16_t end_node);
    void saveNodeAndEdge();
    void closeAllNode();
    void deleteAllNode();
    void deleteAllEdge();
    void pauseNavigation();
    void recoverNavigation();
};

slamDemo::slamDemo(const char *networkInterface)
{
    ChannelFactory::Instance()->Init(0, networkInterface); // networkInterface：The name of the network card with network segment 123

    // pub
    pubQtCommand->InitChannel();
    pubQtNode->InitChannel();
    pubQtEdge->InitChannel();

    // sub
    subQtNotice->InitChannel(std::bind(&slamDemo::qtNoticeHandler, this, std::placeholders::_1), 10);
    subOdommetry->InitChannel(std::bind(&slamDemo::odometryHandler, this, std::placeholders::_1), 1);

    cout << "***********************  Unitree SLAM Demo ***********************\n";
    cout << "------------------         q    w    e         -------------------\n";
    cout << "------------------         a    s    d         -------------------\n";
    cout << "------------------         z    x    c    v    -------------------\n";
    cout << "------------------------------------------------------------------\n";
    cout << "------------------ q: Close ROS node           -------------------\n";
    cout << "------------------ w: Start mapping            -------------------\n";
    cout << "------------------ e: End mapping              -------------------\n";
    cout << "------------------ a: Start navigation         -------------------\n";
    cout << "------------------ s: Pause navigation         -------------------\n";
    cout << "------------------ d: Recover navigation       -------------------\n";
    cout << "------------------ z: Relocation and init pose -------------------\n";
    cout << "------------------ x: Add node and edge        -------------------\n";
    cout << "------------------ c: Save node and edge       -------------------\n";
    cout << "------------------ v: Delete all node and edge -------------------\n";
    cout << "--------------- Press Ctrl + C to exit the program ---------------\n";
    cout << "------------------------------------------------------------------" << endl;
}

void slamDemo::odometryHandler(const void *message)
{
    currentOdom = (const nav_msgs::msg::dds_::Odometry_ *)message;
}

void slamDemo::qtNoticeHandler(const void *message)
{
    int index_, begin_, end_, feedback_, arrive_;
    const std_msgs::msg::dds_::String_ *seq = (const std_msgs::msg::dds_::String_ *)message;
    string str_, notice_;

    begin_ = seq->data().find("index:", 0); // Instruction unique identifier
    end_ = seq->data().find(";", begin_);
    str_ = seq->data().substr(begin_ + 6, end_ - begin_ - 6);
    index_ = atoi(str_.c_str());

    begin_ = seq->data().find("notice:", 0); // Prompt message
    end_ = seq->data().find(";", begin_);
    notice_ = seq->data().substr(begin_ + 7, end_ - begin_ - 7);

    if (index_ <= 10000)
    { // Command execution feedback
        begin_ = seq->data().find("feedback:", 0);
        end_ = seq->data().find(";", begin_);
        str_ = seq->data().substr(begin_ + 9, end_ - begin_ - 9);
        feedback_ = atoi(str_.c_str());
        if (feedback_ == 0 || feedback_ == -1)
            cout << "\033[1;31m"
                 << "Command execution failed with index = " << index_ << "."
                 << "\033[0m";
        cout << notice_ << endl;
    }
    else if (index_ == 10001)
    { // Navigation feedback
        begin_ = seq->data().find("arrive:", 0);
        end_ = seq->data().find(";", begin_);
        str_ = seq->data().substr(begin_ + 7, end_ - begin_ - 7);
        arrive_ = atoi(str_.c_str());
        cout << " I arrived node " << arrive_ << ". " << notice_ << endl;
    }
}

void slamDemo::startMapping()
{
    index++;
    unitree_interfaces::msg::dds_::QtCommand_ send_msg;
    send_msg.seq_().data() = "index:" + to_string(index) + ";";
    send_msg.command_() = 3; // 3 is to start mapping command
    send_msg.attribute_() = 1; //When this value is 1, the XT16 LiDAR node is enabled, and when it is 2, the MID360 LiDAR node is enabled. Please confirm the value based on the actual LiDAR being
    pubQtCommand->Write(send_msg);
}

void slamDemo::endMapping()
{
    index++;
    unitree_interfaces::msg::dds_::QtCommand_ send_msg;
    send_msg.command_() = 4; // 4 is the end mapping command
    send_msg.attribute_() = 0;
    send_msg.seq_().data() = "index:" + to_string(index) + ";";
    send_msg.floor_index_().push_back(0);  // Floor number，just give a fixed value of 0.
    send_msg.pcdmap_index_().push_back(0); // PCD Map number，just give a fixed value of 0.
    pubQtCommand->Write(send_msg);
}

void slamDemo::startRelocation()
{
    index++;
    unitree_interfaces::msg::dds_::QtCommand_ send_msg;
    send_msg.seq_().data() = "index:" + to_string(index) + ";";
    send_msg.command_() = 6; // 6 is to start relocation command
    send_msg.attribute_() = 1; //When this value is 1, the XT16 LiDAR node is enabled, and when it is 2, the MID360 LiDAR node is enabled. Please confirm the value based on the actual LiDAR being
    pubQtCommand->Write(send_msg);
}

void slamDemo::initPose()
{
    index++;
    unitree_interfaces::msg::dds_::QtCommand_ send_msg;
    send_msg.seq_().data() = "index:" + to_string(index) + ";";
    send_msg.command_() = 7;      // 7 is the pose initialization instruction
    send_msg.quaternion_x_() = 0; // Quaternion
    send_msg.quaternion_y_() = 0;
    send_msg.quaternion_z_() = 0;
    send_msg.quaternion_w_() = 1;
    send_msg.translation_x_() = 0; // Translation
    send_msg.translation_y_() = 0;
    send_msg.translation_z_() = 0;
    pubQtCommand->Write(send_msg);
}

void slamDemo::startNavigation()
{
    index++;
    unitree_interfaces::msg::dds_::QtCommand_ send_msg;
    send_msg.seq_().data() = "index:" + to_string(index) + ";";
    send_msg.command_() = 8; // 8 is the command to start navigation
    pubQtCommand->Write(send_msg);
}

void slamDemo::defaultNavigation()
{
    index++;
    unitree_interfaces::msg::dds_::QtCommand_ send_msg;
    send_msg.seq_().data() = "index:" + to_string(index) + ";";
    send_msg.command_() = 10; // 10 Multi nodes loop navigation(default) command
    pubQtCommand->Write(send_msg);
}

void slamDemo::closeAllNode()
{
    index++;
    unitree_interfaces::msg::dds_::QtCommand_ send_msg;
    send_msg.seq_().data() = "index:" + to_string(index) + ";";
    send_msg.command_() = 99; // 99 Close all nodes command
    pubQtCommand->Write(send_msg);
}

void slamDemo::addNodeAndEdge()
{
    nodeAttribute nodeTemp;
    float siny_cosp, cosy_cosp;

    siny_cosp = 2 * (currentOdom->pose().pose().orientation().w() * currentOdom->pose().pose().orientation().z() + currentOdom->pose().pose().orientation().x() * currentOdom->pose().pose().orientation().y());
    cosy_cosp = 1 - 2 * (currentOdom->pose().pose().orientation().y() * currentOdom->pose().pose().orientation().y() + currentOdom->pose().pose().orientation().z() * currentOdom->pose().pose().orientation().z());
    node_name++;

    nodeTemp.nodeX = currentOdom->pose().pose().position().x();
    nodeTemp.nodeY = currentOdom->pose().pose().position().y();
    nodeTemp.nodeZ = currentOdom->pose().pose().position().z();
    nodeTemp.nodeYaw = std::atan2(siny_cosp, cosy_cosp);
    nodeTemp.nodeName = node_name;

    nodeAttributeList.push_back(nodeTemp);

    cout << "Add Node.   Name: " << node_name << "  X:" << nodeTemp.nodeX << "  Y:" << nodeTemp.nodeY << "  Z:" << nodeTemp.nodeY << "  Yaw:" << nodeTemp.nodeYaw << endl;
    if (node_name >= 2)
        addEdge(node_name - 1, node_name - 1, node_name); // Sequential connection nodes
}

void slamDemo::addEdge(u_int16_t edge_name, u_int16_t start_node, u_int16_t end_node)
{
    edgeAttribute edgeTemp;

    edgeTemp.edgeName = edge_name;
    edgeTemp.edgeStart = start_node;
    edgeTemp.edgeEnd = end_node;
    edgeAttributeList.push_back(edgeTemp);

    cout << "Add Edge.   Name: " << edge_name << "  Start node:" << start_node << "  End node:" << end_node << endl;
    cout << "--------------------------------------------------------------------------" << endl;
}

void slamDemo::saveNodeAndEdge()
{
    unitree_interfaces::msg::dds_::QtNode_ nodeMsg;
    unitree_interfaces::msg::dds_::QtEdge_ edgeMsg;
    if (edgeAttributeList.size() == 0)
    {
        cout << "\033[1;31m"
             << "The number of edges in the topology graph is 0."
             << "\033[0m" << endl;
        return;
    }

    index++;
    nodeMsg.seq_().data() = "index:" + to_string(index) + ";";
    for (int i = 0; i < nodeAttributeList.size(); i++)
    {
        nodeMsg.node_().node_name_().push_back(nodeAttributeList[i].nodeName);    // Node name
        nodeMsg.node_().node_position_x_().push_back(nodeAttributeList[i].nodeX); // Point X coordinate information
        nodeMsg.node_().node_position_y_().push_back(nodeAttributeList[i].nodeY); // Point Y coordinate information
        nodeMsg.node_().node_position_z_().push_back(nodeAttributeList[i].nodeZ); // Point Z coordinate information
        nodeMsg.node_().node_yaw_().push_back(nodeAttributeList[i].nodeYaw);      // Point Yaw Angle Information
        nodeMsg.node_().node_attribute_().push_back(0);                           // Not open attribute, please assign a value of 0, note: cannot be empty!!! Cannot be empty!!! Cannot be empty!!!
        nodeMsg.node_().undefined_().push_back(0);
        nodeMsg.node_().node_state_2_().push_back(0);
        nodeMsg.node_().node_state_3_().push_back(0);
    }
    pubQtNode->Write(nodeMsg);

    index++;
    edgeMsg.seq_().data() = "index:" + to_string(index) + ";";
    for (int i = 0; i < edgeAttributeList.size(); i++)
    {
        edgeMsg.edge_().edge_name_().push_back(edgeAttributeList[i].edgeName);        // Edge name
        edgeMsg.edge_().start_node_name_().push_back(edgeAttributeList[i].edgeStart); // Start node name
        edgeMsg.edge_().end_node_name_().push_back(edgeAttributeList[i].edgeEnd);     // End node name
        edgeMsg.edge_().edge_length_().push_back(0);
        edgeMsg.edge_().dog_speed_().push_back(1);    // The speed of the dog walking this edge (0-1)
        edgeMsg.edge_().edge_state_2_().push_back(0); // 0：Stop 1：Avoid 3：Replan  when encountering obstacles. Suggest assigning a value of 0.

        edgeMsg.edge_().dog_stats_().push_back(0); // Not open attribute, please assign a value of 0, note: cannot be empty!!! Cannot be empty!!! Cannot be empty!!!
        edgeMsg.edge_().dog_back_stats_().push_back(0);
        edgeMsg.edge_().edge_state_().push_back(0);
        edgeMsg.edge_().edge_state_1_().push_back(0);
        edgeMsg.edge_().edge_state_3_().push_back(0);
        edgeMsg.edge_().edge_state_4_().push_back(0);
    }
    pubQtEdge->Write(edgeMsg);
}

void slamDemo::deleteAllNode()
{
    index++;
    unitree_interfaces::msg::dds_::QtCommand_ send_msg;
    send_msg.seq_().data() = "index:" + to_string(index) + ";";
    send_msg.command_() = 1;   // 1 is delete instruction
    send_msg.attribute_() = 1; // 1 is select delete node
    send_msg.node_edge_name_().push_back(999);

    pubQtCommand->Write(send_msg);
}

void slamDemo::deleteAllEdge()
{
    index++;
    unitree_interfaces::msg::dds_::QtCommand_ send_msg;
    send_msg.seq_().data() = "index:" + to_string(index) + ";";
    send_msg.command_() = 1;   // 1 is delete instruction
    send_msg.attribute_() = 2; // 2 is select delete edge
    send_msg.node_edge_name_().push_back(999);
    pubQtCommand->Write(send_msg);
}

void slamDemo::pauseNavigation()
{
    index++;
    unitree_interfaces::msg::dds_::QtCommand_ send_msg;
    send_msg.seq_().data() = "index:" + to_string(index) + ";";
    send_msg.command_() = 13; // 13 is the pause navigation command
    pubQtCommand->Write(send_msg);
}

void slamDemo::recoverNavigation()
{
    index++;
    unitree_interfaces::msg::dds_::QtCommand_ send_msg;
    send_msg.seq_().data() = "index:" + to_string(index) + ";";
    send_msg.command_() = 14; // 14 to recover navigation commands
    pubQtCommand->Write(send_msg);
}

// Key detection
unsigned char slamDemo::keyDetection()
{
    termios tms_old, tms_new;
    tcgetattr(0, &tms_old);
    tms_new = tms_old;
    tms_new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(0, TCSANOW, &tms_new);
    unsigned char ch = getchar();
    tcsetattr(0, TCSANOW, &tms_old);
    cout << "\033[1;32m"
         << "Key " << ch << " pressed."
         << "\033[0m" << endl;
    return ch;
}

// Execute
void slamDemo::keyExecute()
{
    unsigned char currentKey;
    while (true)
    {
        currentKey = keyDetection();
        switch (currentKey)
        {
        case 'q': // Close all nodes
            closeAllNode();
            break;
        case 'w': // Start mapping (default to clearing node/edge information)
            deleteAllNode();
            deleteAllEdge();
            nodeAttributeList.clear();
            edgeAttributeList.clear();
            startMapping();
            node_name = 0;
            break;
        case 'e': // End mapping
            endMapping();
            break;
        case 'a': // Start navigation (default)
            startRelocation();
            startNavigation();
            initPose();
            defaultNavigation();
            break;
        case 's': // Pause navigation
            pauseNavigation();
            break;
        case 'd': // Recover navigation
            recoverNavigation();
            break;
        case 'z': // Start relocation and navigation to prepare for collecting node/edge information. (Default clearing of node/edge information)
            deleteAllNode();
            deleteAllEdge();
            startRelocation();
            startNavigation();
            initPose();
            break;
        case 'x': // Collect ndoe/edge information
            addNodeAndEdge();
            break;
        case 'c': // Save ndoe/edge information
            saveNodeAndEdge();
            nodeAttributeList.clear();
            edgeAttributeList.clear();
            node_name = 0;
            break;
        case 'v': // Clear node/edge information
            deleteAllNode();
            deleteAllEdge();
            node_name = 0;
            break;
        default:
            break;
        }
    }
}

int main(int argc, const char **argv)
{
    slamDemo slamTest(argv[1]); //// argv[1]：The name of the network card with network segment 123
    slamTest.keyExecute();
    return 0;
}
