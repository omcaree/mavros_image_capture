/*
 * Copyright (C) 2016, Owen McAree for Sheffield Robotics
 *  
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
 
/*
 *	auto_capture
 *	Trigger a Raspberry Pi camera to take an image when specific criteria
 *	from a mavros connected autopilot are met
 */

#include <sys/stat.h>

#include "ros/ros.h"
#include "mavros_msgs/State.h"

/* 	Flag to track whether the vehicle is in auto mode or not */
bool isAuto = false;
bool isRunning = false;

/* Path to save images to */
#define IMAGE_ROOT "/home/pi/images/"
char folder[128];

/* 	Called on receipt of /mavros/state packets (1Hz by default)
 *	Track changes of flight mode in the isAuto flags
 */
void mavrosStateCallback(const mavros_msgs::State::ConstPtr& msg) {
	bool stateAuto = (msg->mode.compare("AUTO") == 0);
	if (isAuto && !stateAuto) {
		isAuto = stateAuto;
		ROS_INFO("Image capture stopped [Mode: %s]", msg->mode.c_str());
		return;
	}
	if (!isAuto && stateAuto) {
		isAuto = stateAuto;
		ROS_INFO("Image capture started [Mode: %s]", msg->mode.c_str());
		return;
	}
}

void createImageFolder() {
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	sprintf(folder, "%s%d%02d%02d%02d%02d%02d/",IMAGE_ROOT,tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	mkdir(folder,0777);
	ROS_INFO("Created folder [%s]", folder);
}

int main(int argc, char **argv) {
	ros::init(argc, argv, "auto_capture");
	ros::NodeHandle n;
	ros::Subscriber mavros_state = n.subscribe("mavros/state", 1, mavrosStateCallback);
	ros::Rate loop_rate(1);
	createImageFolder();
	while (ros::ok()) {
		if (isAuto && !isRunning) {
			char cmd[256];
			sprintf(cmd,"raspistill -o %s%%04d.jpg -t 21600000 -tl 1000", folder);
			ROS_INFO("Executing: [%s]", cmd);
			system(cmd);
			isRunning = true;
		}
		if (isRunning && !isAuto) {
			ROS_INFO("Executing: [killall raspistill]");
			system("killall raspistill");
		}
		ros::spinOnce();
		loop_rate.sleep();
	}
	return 0;
}