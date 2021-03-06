/*******************************************************************
*
* Author: Mrinmoy Sarkar
* email: sarkar.mrinmoy.bd@ieee.org
*
*********************************************************************/

#include <ros/ros.h>
#include <move_base_msgs/MoveBaseAction.h>
#include <actionlib/client/simple_action_client.h>
#include <tf/tf.h>
#include <trajectory_msgs/JointTrajectory.h>
#include <std_msgs/Float64MultiArray.h>
#include <geometry_msgs/PoseWithCovarianceStamped.h>
#include <iostream>
#include <math.h>
#include <std_msgs/Bool.h>


#include <actionlib/client/terminal_state.h>
#include <control_msgs/FollowJointTrajectoryAction.h>

int main(int argc, char** argv)
{
    ros::init(argc, argv, "teamd_move_tilt");

    ROS_INFO("HRATC 2016 Team DISHARI‬ tilt move Node");
    ros::NodeHandle pn("~");

    double tilt_speed;
    pn.param("tilt_speed", tilt_speed, 0.8);

    double lower_tilt;
    pn.param("lower_tilt", lower_tilt, -0.5);

    double upper_tilt;
    pn.param("upper_tilt", upper_tilt, 0.5);

    double timeout;
    pn.param("timeout", timeout, 10.0);

    actionlib::SimpleActionClient<control_msgs::FollowJointTrajectoryAction> tilt_ac("/tilt_controller/follow_joint_trajectory", true);
    //ROS_INFO("PointCloud Generator -- Waiting for tilt action server to start...");
    tilt_ac.waitForServer();
    

    
    // Move laser to the start position
    double tilt = upper_tilt;

    control_msgs::FollowJointTrajectoryGoal goal;
    goal.trajectory.header.stamp = ros::Time::now();
    goal.trajectory.joint_names.resize(1);
    goal.trajectory.points.resize(1);
    goal.trajectory.joint_names[0] = "tilt_joint";
    goal.trajectory.points[0].positions.push_back(tilt);
    goal.trajectory.points[0].velocities.push_back(tilt_speed);
    goal.trajectory.points[0].time_from_start = ros::Duration(0.5);
    goal.goal_tolerance.resize(1);
    goal.goal_tolerance[0].name = "tilt_joint";
    goal.goal_tolerance[0].position = 0.01;
    goal.goal_time_tolerance = ros::Duration(0.5);

    tilt_ac.sendGoal(goal);

    // Wait for the action to return
    bool finished_before_timeout = tilt_ac.waitForResult(ros::Duration(timeout));

    if(!finished_before_timeout)
    {
        ROS_FATAL("PointCloud Generator -- Unable to move the laser to the start position!");
        ROS_BREAK();
    }
    ros::spin();

    return 0;
}
