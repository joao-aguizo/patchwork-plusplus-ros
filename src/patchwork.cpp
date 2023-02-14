#include <iostream>
// For disable PCL complile lib, to use PointXYZIR
#define PCL_NO_PRECOMPILE

#include <ros/ros.h>
#include <signal.h>
#include <sensor_msgs/PointCloud2.h>
#include <pcl_conversions/pcl_conversions.h>

#include "patchworkpp/patchworkpp.hpp"

using PointType = pcl::PointXYZI;
using namespace std;

boost::shared_ptr<PatchWorkpp<PointType>> PatchworkppGroundSeg;

ros::Publisher pub_cloud;
ros::Publisher pub_ground;
ros::Publisher pub_non_ground;

bool enable_cloud_republish;

template<typename T>
sensor_msgs::PointCloud2 cloud2msg(pcl::PointCloud<T> cloud, std::string frame_id = "map") {
    sensor_msgs::PointCloud2 cloud_ROS;
    pcl::toROSMsg(cloud, cloud_ROS);
    cloud_ROS.header.frame_id = frame_id;
    return cloud_ROS;
}

void callbackCloud(const sensor_msgs::PointCloud2::Ptr &cloud_msg)
{
    double time_taken;

    pcl::PointCloud<PointType> pc_curr;
    pcl::PointCloud<PointType> pc_ground;
    pcl::PointCloud<PointType> pc_non_ground;
    
    pcl::fromROSMsg(*cloud_msg, pc_curr);

    PatchworkppGroundSeg->estimate_ground(pc_curr, pc_ground, pc_non_ground, time_taken);

    cout << "\033[1;32m" << "Result: Input PointCloud: " << pc_curr.size() << " -> Ground: " << pc_ground.size() <<  "/ NonGround: " << pc_non_ground.size()
         << " (running_time: " << time_taken << " sec)" << "\033[0m" << endl;


    if (enable_cloud_republish){
        pub_cloud.publish(cloud2msg(pc_curr, cloud_msg->header.frame_id));
    }

    pub_ground.publish(cloud2msg(pc_ground, cloud_msg->header.frame_id));
    pub_non_ground.publish(cloud2msg(pc_non_ground, cloud_msg->header.frame_id));
}

int main(int argc, char**argv) {

    ros::init(argc, argv, "PatchworkNode");
    ros::NodeHandle nh;
    
    // determines if original cloud is republished
    nh.param<bool>("enable_cloud_republish", enable_cloud_republish, false);

    cout << "Operating patchwork++..." << endl;
    PatchworkppGroundSeg.reset(new PatchWorkpp<PointType>(&nh));

    if (enable_cloud_republish){
        pub_cloud = nh.advertise<sensor_msgs::PointCloud2>("patchwork/cloud", 100, true);
    }

    pub_ground      = nh.advertise<sensor_msgs::PointCloud2>("patchwork/ground", 100, true);
    pub_non_ground  = nh.advertise<sensor_msgs::PointCloud2>("patchwork/nonground", 100, true);
    
    ros::Subscriber sub_cloud = nh.subscribe("pointcloud", 100, callbackCloud);


    while (ros::ok())
    {
        ros::spinOnce();
    }
    
    return 0;
}
