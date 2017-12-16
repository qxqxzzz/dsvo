#include "state.hpp"
#include <iostream>

State::State() {
	pose.position = Eigen::Vector3d::Zero();
	pose.orientation = Eigen::Quaterniond::Identity();
	velocity = Eigen::Vector3d::Zero();
	imu_bias.acceleration = Eigen::Vector3d::Zero();
	imu_bias.rotation = Eigen::Vector3d::Zero();

	covariance = Eigen::Matrix<double, 16, 16>::Identity();
	   
	pose_pub = nh.advertise<geometry_msgs::PoseStamped>("cam_pose", 1000);
    pcl_pub = nh.advertise<PointCloud> ("points2", 1);
}

void State::showPose() {
	std::cout<<"Position: ["<<this->pose.position[0]<<", "<<this->pose.position[1]<<", "<<this->pose.position[2]<<"]"<<std::endl;
	std::cout<<"orientation: ["<<this->pose.orientation.w()<<", "<<this->pose.orientation.x()<<", "<<this->pose.orientation.y()<<", "<<this->pose.orientation.z()<<"]"<<std::endl;
	std::cout<<"Velocity: ["<<this->velocity[0]<<", "<<this->velocity[1]<<", "<<this->velocity[2]<<"]"<<std::endl;
	std::cout<<"Landmark size: "<<this->landmarks.size()<<std::endl;
	std::cout<<"Uncertainty: "<<this->covariance.norm()<<std::endl;

	// publish pose
	geometry_msgs::PoseStamped pose;

	pose.header.frame_id = "/map";

	pose.pose.position.x = this->pose.position[0];
	pose.pose.position.y = this->pose.position[1];
	pose.pose.position.z = this->pose.position[2];

	pose.pose.orientation.w = this->pose.orientation.w(); 
	pose.pose.orientation.x = this->pose.orientation.x();
	pose.pose.orientation.y = this->pose.orientation.y();
	pose.pose.orientation.z = this->pose.orientation.z();

	pose_pub.publish(pose);

    PointCloud::Ptr point_cloud (new PointCloud);

    for(int i=0; i<this->landmarks.size(); i++)
    {
        pcl::PointXYZ p;
        p.x = landmarks[i].x;
        p.y = landmarks[i].y;
        p.z = landmarks[i].z;
        point_cloud->push_back(p);
    }

    point_cloud->header.frame_id = "/map";
    pcl_pub.publish(point_cloud);
}