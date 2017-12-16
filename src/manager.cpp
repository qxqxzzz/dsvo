#include "manager.hpp"

Manager::Manager() {
	ros::NodeHandle nhPriv("~");
	std::vector<double> T_BS0, K0, frame_size0, dist_coeff0;
	std::vector<double> T_BS1, K1, frame_size1, dist_coeff1;
	// cameras model
	if(!nhPriv.getParam("/cam0/T_BS/data", T_BS0) 
	|| !nhPriv.getParam("/cam0/intrinsics", K0) 
	|| !nhPriv.getParam("/cam0/resolution", frame_size0) 
	|| !nhPriv.getParam("/cam0/distortion_coefficients", dist_coeff0)
	|| !nhPriv.getParam("/cam1/T_BS/data", T_BS1) 
	|| !nhPriv.getParam("/cam1/intrinsics", K1) 
	|| !nhPriv.getParam("/cam1/resolution", frame_size1) 
	|| !nhPriv.getParam("/cam1/distortion_coefficients", dist_coeff1) 
    )
	{
		ROS_INFO("Fail to get cameras parameters, exit.");
        return;
	}
	stereo_cam = boost::shared_ptr<StereoCamera>(new StereoCamera(T_BS0, K0, frame_size0, dist_coeff0,
																  T_BS1, K1, frame_size1, dist_coeff1));

	// imu model
	std::vector<double> T_BS;
	double gyroscope_noise_density, gyroscope_random_walk, accelerometer_noise_density, accelerometer_random_walk;
	if(!nhPriv.getParam("/imu/T_BS/data", T_BS) 
	|| !nhPriv.getParam("/imu/gyroscope_noise_density", gyroscope_noise_density) 
	|| !nhPriv.getParam("/imu/gyroscope_random_walk", gyroscope_random_walk) 
	|| !nhPriv.getParam("/imu/accelerometer_noise_density", accelerometer_noise_density) 
	|| !nhPriv.getParam("/imu/accelerometer_random_walk", accelerometer_random_walk) 
    )
	{
		ROS_INFO("Fail to get cam1 parameters, exit.");
        return;
	}
	imu = boost::shared_ptr<IMU>(new IMU(T_BS, 
										 gyroscope_noise_density, 
										 gyroscope_random_walk, 
										 accelerometer_noise_density, 
										 accelerometer_random_walk));

	// sensor topics
	std::string cam0_topic, cam1_topic, imu_topic;
	if(!nhPriv.getParam("cam0_topic", cam0_topic) 
	|| !nhPriv.getParam("cam1_topic", cam1_topic) 
	|| !nhPriv.getParam("imu_topic", imu_topic) )
	{
		ROS_INFO("Fail to get sensor topics, exit.");
        return;
	}

	cam0_sub = new message_filters::Subscriber<sensor_msgs::Image>(nh, cam0_topic, 1);
	cam1_sub = new message_filters::Subscriber<sensor_msgs::Image>(nh, cam1_topic, 1);

	sync = new message_filters::Synchronizer<StereoSyncPolicy>(StereoSyncPolicy(10), *cam0_sub, *cam1_sub);
	sync->registerCallback(boost::bind(&Manager::imageMessageCallback, this, _1, _2));
	
	imu_sub = nh.subscribe(imu_topic, 10, &Manager::imuMessageCallback, this);	

	std::string pos_topic = "";
	nhPriv.getParam("pos_topic", pos_topic);
	pos_sub = nh.subscribe(pos_topic, 10, &Manager::posMessageCallback, this);	

	last_pos_set = false;
}


void Manager::imageMessageCallback(const sensor_msgs::ImageConstPtr& img0_cptr, const sensor_msgs::ImageConstPtr& img1_cptr){
	cv_bridge::CvImageConstPtr img0_ptr, img1_ptr;
	try
	{
		img0_ptr = cv_bridge::toCvShare(img0_cptr, img0_cptr->encoding);
		img1_ptr = cv_bridge::toCvShare(img1_cptr, img1_cptr->encoding);
	}
	catch (cv_bridge::Exception& e)
	{
		ROS_ERROR("cv_bridge exception: %s", e.what());
		return;
	}

	//image preparation
	cv::Mat cur_img0, cur_img1;
	img0_ptr->image.copyTo(cur_img0);
	img1_ptr->image.copyTo(cur_img1);

	stereo_cam->stereoTrack(cur_img0, cur_img1, state);

	return;
}

void Manager::imuMessageCallback(const sensor_msgs::ImuConstPtr& imu_cptr) {
	Eigen::Vector3d rot_vel(imu_cptr->angular_velocity.x, imu_cptr->angular_velocity.y, imu_cptr->angular_velocity.z);
	Eigen::Vector3d lin_acc(imu_cptr->linear_acceleration.x, imu_cptr->linear_acceleration.y, imu_cptr->linear_acceleration.z);
	imu->imu_propagate(state, rot_vel, lin_acc, imu_cptr->header.stamp.toSec());
}

void Manager::posMessageCallback(const geometry_msgs::PointStampedConstPtr& pos_cptr) {
	Eigen::Vector4d cur_pos = Eigen::Vector4d(pos_cptr->header.stamp.toSec(),
			                      				pos_cptr->point.x,
			                       				pos_cptr->point.y,
			                       				pos_cptr->point.z);
	if(last_pos_set) {
		last_pos = cur_pos;
		last_pos_set = true;
		return;
	}

	double dt = cur_pos(0) - last_pos(0);
	double dx = cur_pos(1) - last_pos(1);
	double dy = cur_pos(2) - last_pos(2);
	double dz = cur_pos(3) - last_pos(3);

	std::cout<<"True Velocity: ["<<dx/dt<<", "<<dy/dt<<", "<<dz/dt<<"]"<<std::endl;

	last_pos = cur_pos;
}	