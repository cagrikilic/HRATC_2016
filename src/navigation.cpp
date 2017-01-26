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

using namespace std;

double current_x = -1000;
double current_y = -1000;  
std_msgs::Float64MultiArray Corner_points;

typedef actionlib::SimpleActionClient<move_base_msgs::MoveBaseAction> MoveBaseClient;

class RandomWalk
{
public:
    RandomWalk(ros::NodeHandle *nh);
    void sendNewGoal(double x_pos,double y_pos);
    void moveBotToTheNearCorner(int segment);
    void set_corners(double min_x, double max_x, double min_y, double max_y,double min_x2, double max_x2, double min_y2, double max_y2, double init_pos_x, double init_pos_y);
    void setLimits(double min_x, double max_x, double min_y, double max_y, double init_pos_x, double init_pos_y);

private:
    void goalDoneCallback(const actionlib::SimpleClientGoalState &state, const move_base_msgs::MoveBaseResultConstPtr &result);
    void goalActiveCallback();
    void goalFeedbackCallback(const move_base_msgs::MoveBaseFeedbackConstPtr &feedback);
    void print_all_data();
    void minepos_callback(const std_msgs::Float64MultiArray::ConstPtr& msg);
    void check_if_exploding(double x, double y);
    double distance(double x1,double y1,double x2,double y2);
    double mindistance(double d1,double d2,double d3,double d4);
    bool point_in_polygon(int nvert, double *vertx, double *verty, double testx, double testy);

    MoveBaseClient ac_;

    double min_x_;
    double max_x_;
    double min_y_;
    double max_y_;
    double min_x_2;
    double max_x_2;
    double min_y_2;
    double max_y_2;
    double init_pos_x_;
    double init_pos_y_;
    double init_time;
    int x_max_index, y_max_index, x_current_index , y_current_index;
    double *grid_x;
    double *grid_y;
    bool *covered_grid;
    bool x_plus,y_plus;
    double init_x,init_y;
    bool gole_change; 
    ros::Subscriber sub_mine_found_handle;
    ros::NodeHandle *n;
    double mine_pos_x[100];
    double mine_pos_y[100];
    int detected_mine_no;
    double deviate_pos_x;
    double deviate_pos_y;
    int total_no_of_grid;
    int covered_no_of_grid;
    int no_of_vertex;
    double *vertX;
    double *vertY;
    
};



RandomWalk::RandomWalk(ros::NodeHandle *nh) : ac_("move_base", true),n(nh)
{
    ROS_INFO("Waiting for the move_base action server to come online...");
    if(!ac_.waitForServer(ros::Duration(20.0)))
    {
        ROS_FATAL("Did you forget to launch the ROS navigation?");
        ROS_BREAK();
    }
    ROS_INFO("Found it!");
    x_plus = true;
    y_plus = true;
    sub_mine_found_handle = n->subscribe("/teamd_mine_pos", 1000, &RandomWalk::minepos_callback,this);
    detected_mine_no = 0;
    deviate_pos_x = -100;
    deviate_pos_y = -100;
    gole_change = true;
    total_no_of_grid = 0;
    covered_no_of_grid = 0;
}

void RandomWalk::set_corners(double min_x, double max_x, double min_y, double max_y,double min_x2, double max_x2, double min_y2, double max_y2, double init_pos_x, double init_pos_y)
{
	min_x_ = min_x;
    max_x_ = max_x;
    min_y_ = min_y;
    max_y_ = max_y;
    min_x_2 = min_x2;
    max_x_2 = max_x2;
    min_y_2 = min_y2;
    max_y_2 = max_y2;
    init_pos_x_ = init_pos_x;
    init_pos_y_ = init_pos_y;
	moveBotToTheNearCorner(0);
	}
void RandomWalk::moveBotToTheNearCorner(int segment)
{
	double minX,minY,maxX,maxY,init_pos_x,init_pos_y;
	
	if(segment == 0)
	{
		cout << "FIRST CALL." << endl;
		minX = min_x_;
		maxX = max_x_;
		minY = min_y_;
		maxY = max_y_;
		init_pos_x = init_pos_x_;
		init_pos_y = init_pos_y_;
		}
		else
		{
			cout << "SECOND CALL." << endl;
			minX = min_x_2;
			maxX = max_x_2;
			minY = min_y_2;
			maxY = max_y_2;
			init_pos_x = current_x;
			init_pos_y = current_y;
			}
			
		double d1 = distance(minX,minY,init_pos_x,init_pos_y);
        double d2 = distance(minX,maxY,init_pos_x,init_pos_y);
        double d3 = distance(maxX,minY,init_pos_x,init_pos_y);
        double d4 = distance(maxX,maxY,init_pos_x,init_pos_y);
        double d = mindistance(d1,d2,d3,d4);
        double del = 0.6;
        if(d == d1)
        {
            init_pos_x = minX; 
            init_pos_y = minY;
            setLimits(minX, maxX, minY, maxY, init_pos_x, init_pos_y);
            sendNewGoal(minX+del,minY+del);
        }
        else if( d == d2)
        {
            init_pos_x = minX; 
            init_pos_y = maxY;
            setLimits(minX, maxX, minY, maxY, init_pos_x, init_pos_y);
            sendNewGoal(minX+del,maxY-del);
        }
        else if( d == d3)
        {
            init_pos_x = maxX; 
            init_pos_y = minY;
            setLimits(minX, maxX, minY, maxY, init_pos_x, init_pos_y);
            sendNewGoal(maxX-del,minY+del);
        }
        else if( d == d4)
        {
            init_pos_x = maxX; 
            init_pos_y = maxY;
            setLimits(minX, maxX, minY, maxY, init_pos_x, init_pos_y);
            sendNewGoal(maxX-del,maxY-del);
        } 
	
	}

void RandomWalk::check_if_exploding(double x, double y)
{
    for(int i=0;i<detected_mine_no;i++)
    {
        double dx = x - mine_pos_x[i];
        double dy = y - mine_pos_y[i];
        double d = dx*dx + dy*dy;
        if(d < 0.27)
        {
            deviate_pos_x = (mine_pos_x[i] + x)/2;
            if(dy < 0)
            {
                deviate_pos_y = mine_pos_y[i] - 1.2;
            }
            else
            {
                deviate_pos_y = mine_pos_y[i] + 1.2;
            }
            return;
        }
    }
    deviate_pos_x = -100;
    deviate_pos_y = -100;
}

void RandomWalk::minepos_callback(const std_msgs::Float64MultiArray::ConstPtr& msg)
{
  //ROS_INFO("I heard:");// [%s]", msg->data.c_str());
    cout << "MINE X: "<< msg->data[0] << "   MINE Y: " << msg->data[1] << endl; 
    mine_pos_x[detected_mine_no] = msg->data[0];
    mine_pos_y[detected_mine_no] = msg->data[1];
    detected_mine_no++;

    cout << "changing goal because of mine" << endl;
    //if(gole_change)
    {
        //sendNewGoal((msg->data[0]+current_x)/2.0,(current_y-msg->data[1])<0?(msg->data[1] - 1.2):(msg->data[1] + 1.2));
        sendNewGoal(current_x,(current_y-msg->data[1])<0?(msg->data[1] - 1.2):(msg->data[1] + 1.2));
        
        //gole_change = false;
    }
}

void RandomWalk::print_all_data()
{
    cout << "X-MAX-INDX: " << x_max_index << " Y-MAX-INDX: " << y_max_index << endl << "grid_x_data: ";
    for(int i = 0; i <  x_max_index;i++)
    {
        cout << grid_x[i] << "\t" ;
    }
    cout << endl << "grid_y_data: ";
    for(int i = 0; i <  y_max_index;i++)
    {
        cout << grid_y[i] << "\t" ;
    }
    cout << endl << "cornerx: ";
    
    for(int i = 0; i <  no_of_vertex;i++)
    {
        cout << vertX[i] << "\t" ;
    }
    cout << endl << "cornery: ";
    for(int i = 0; i <  no_of_vertex;i++)
    {
        cout << vertY[i] << "\t" ;
    }
    cout << endl;
}

void RandomWalk::sendNewGoal(double x_pos,double y_pos)
{
    ros::Duration(1.5).sleep();
    move_base_msgs::MoveBaseGoal goal;

    goal.target_pose.header.frame_id = "minefield";
    goal.target_pose.header.stamp = ros::Time::now();

    srand(time(NULL));
   // print_all_data();

    double x;  
    double y; 
    /*
    if(x_pos == 0 && y_pos == 0)
    {
        x = grid_x[x_current_index] + init_pos_x_;
        y = grid_y[y_current_index] + init_pos_y_;
        check_if_exploding(x,y);
        if(deviate_pos_x != -100)
        {
           cout << "changing goal destination because of mine" << endl;
           x=deviate_pos_x;
           y=deviate_pos_y;
        }
    }*/
    //else
    //{
        x = x_pos;
        y = y_pos;
    //}
    
    
    double yaw = x_plus?0:M_PI;

    goal.target_pose.pose.position.x = x;
    goal.target_pose.pose.position.y = y;
    goal.target_pose.pose.orientation = tf::createQuaternionMsgFromYaw(yaw);
    cout << "GOAL X: " << x << " GOAL Y: " << y << endl;

    //ROS_INFO("Sending a new goal to move_base x %lf y %lf yaw %lf", x, y, yaw);
    
    //cout << "CUR-X: " << x_current_index << "  CUR-Y: " << y_current_index << endl;
    init_x = x;
    init_y = y;    


    init_time = ros::Time::now().toSec();
    ac_.sendGoal(goal, boost::bind(&RandomWalk::goalDoneCallback, this, _1, _2), boost::bind(&RandomWalk::goalActiveCallback, this), boost::bind(&RandomWalk::goalFeedbackCallback, this, _1));
}

void RandomWalk::goalDoneCallback(const actionlib::SimpleClientGoalState &state, const move_base_msgs::MoveBaseResultConstPtr &result)
{

    if(state.state_ == actionlib::SimpleClientGoalState::SUCCEEDED)
    {
        //ROS_INFO("The goal was reached!");
    }

    if(state.state_ == actionlib::SimpleClientGoalState::ABORTED)
    {
        //ROS_WARN("Failed to reach the goal...");
    }

    
    cout <<"gole done"<<endl;
    //covered_grid[y_current_index*x_max_index + x_current_index] = false;
    label:
    if(x_plus)
    {
        x_current_index++;
    }
    else
    {
        x_current_index--;
    }
    if(x_current_index == x_max_index)
    {
        x_current_index--;
        if(y_plus)
        {
            y_current_index++;
        }
        else
        {
            y_current_index--;
        }
        x_plus = false;
        if(y_current_index == -1)
        {
            y_current_index += 2;
            y_plus = true;
        }
        else if(y_current_index == y_max_index)
        {
            y_current_index -= 2;
            y_plus = false;
        }
    }
    else if(x_current_index == -1)
    {
        x_current_index++;
        if(y_plus)
        {
            y_current_index++;
        }
        else
        {
            y_current_index--;
        }
        x_plus = true;
        if(y_current_index == -1)
        {
            y_current_index += 2;
            y_plus = true;
        }
        else if(y_current_index == y_max_index)
        {
            y_current_index -= 2;
            y_plus = false;
        }
    }
    //gole_change = true;
    
    covered_no_of_grid++;
    //if(covered_no_of_grid  > (total_no_of_grid+2))return;
    //{
	//	moveBotToTheNearCorner(1);
	//}
    //else
    //{
		//sendNewGoal(0,0);
	//}
	
	double x;  
    double y; 
        
    check_if_exploding(x,y);
    if(deviate_pos_x != -100)
    {
       cout << "changing goal destination because of mine" << endl;
       x=deviate_pos_x;
       y=deviate_pos_y;
       sendNewGoal(x,y);
    }
    else
    {
		x = grid_x[x_current_index];// + init_pos_x_;
		y = grid_y[y_current_index];// + init_pos_y_;
		//check if x,y is in the field
		if(point_in_polygon(no_of_vertex,vertX,vertY,x,y))
		{
			//cout << "JUMPING" << endl;
			goto label;
		}
		else
		{
			sendNewGoal(x,y);
		}
	}
}

void RandomWalk::goalActiveCallback()
{
    //ROS_INFO("The new goal is active!");
}

void RandomWalk::goalFeedbackCallback(const move_base_msgs::MoveBaseFeedbackConstPtr &feedback)
{
	//ac_.cancelGoal();
    //ROS_INFO("Getting feedback! How cool is that?");
    //cout << feedback << endl;
    double tim = ros::Time::now().toSec() - init_time;
    double distance = (init_x - current_x) * (init_x - current_x) + (init_y - current_y) * (init_y - current_y);
    distance = sqrt(distance);
    if(tim > (covered_no_of_grid == 0?30.0:15.0) || distance < 0.1)
    {
        
       init_time = ros::Time::now().toSec();
       
       //cout << "CANCELLING: " << tim << endl;
       //cout << " x: "<< current_x << " y: " << current_y << endl;
       //cout << "Distance: " << distance << endl;
       ac_.cancelGoal();
       //gole_change = true;
    }
}

void RandomWalk::setLimits(double min_x, double max_x, double min_y, double max_y, double init_pos_x, double init_pos_y)
{
	
	no_of_vertex = Corner_points.data.size()/2;
    vertX = new double[no_of_vertex];
    vertY = new double[no_of_vertex];
    for(int i = 0; i < no_of_vertex;++i)
	{
		int indx = i*2;
		vertX[i] = Corner_points.data[indx];
		vertY[i] = Corner_points.data[indx+1];
	}
    
	
    double del = 0.7;
    double grid_length = 1.3;
    double grid_width = 1.2;
    min_x_ = min_x + del;
    max_x_ = max_x - del;
    min_y_ = min_y + del;
    max_y_ = max_y - del;
    init_pos_x_ = init_pos_x;
    init_pos_y_ = init_pos_y;

    //min_x_ -= init_pos_x;
    //max_x_ -= init_pos_x;
    //min_y_ -= init_pos_y;
    //max_y_ -= init_pos_y;
    double length = abs(min_x_) + abs(max_x_);
    double width = abs(min_y_) + abs(max_y_); 
    x_max_index = int(length / grid_length);
    y_max_index = int(width / grid_width);
    cout << "x_max_index: " << x_max_index << " y_max_index " << y_max_index << endl;
    //double grid_x_coordinate[x_max_index];
    //double grid_y_coordinate[y_max_index];
    grid_x = new double[x_max_index];
    grid_y = new double[y_max_index];
    grid_x[0] = min_x_;
    x_current_index = 0;
    double old_d;
    double new_d; 
    old_d = abs(grid_x[0] - init_pos_x);
    for(int i = 1;i<x_max_index;i++)
    {
        grid_x[i] = grid_x[i-1] + grid_length;
        new_d = abs(grid_x[i] - init_pos_x);
        if(old_d > new_d)
        {
            x_current_index = i;
            old_d = new_d;
        }
    }
    grid_x[x_max_index-1] = max_x_ - del; 
    grid_y[0] = min_y_;
    y_current_index = 0;
    old_d = abs(grid_y[0] - init_pos_y);
    for(int i = 1;i<y_max_index;i++)
    {
        grid_y[i] = grid_y[i-1] + grid_width;
        new_d = abs(grid_y[i] - init_pos_y);
        if(old_d > new_d)
        {
            y_current_index = i;
            old_d = new_d;
        }      
    }
    grid_y[y_max_index-1] = max_y_ - del;
    cout << "x_current_index " << x_current_index << " y_current_index " << y_current_index << endl;
    cout << "CnpX: " << current_x << "  CnpY: " << current_y << endl;

    //covered_grid = new bool[x_max_index*y_max_index];
    //bool cov_grid[x_max_index*y_max_index];   
    //for(int i=0;i < x_max_index*y_max_index;i++)
    //{
     //   covered_grid[i] = true;       
    //}
    //covered_grid = cov_grid;
    total_no_of_grid = x_max_index*y_max_index;
    covered_no_of_grid = 0;
    //print_all_data();
}

bool RandomWalk::point_in_polygon(int nvert, double *vertx, double *verty, double testx, double testy)
{
  int i, j;
  bool c = false;
  
  for (i = 0, j = nvert-1; i < nvert; j = i++) {
    if ( ((verty[i]>testy) != (verty[j]>testy)) &&
     (testx < (vertx[j]-vertx[i]) * (testy-verty[i]) / (verty[j]-verty[i]) + vertx[i]) )
       c = !c;
  }
  return c;
}

double RandomWalk::distance(double x1,double y1,double x2,double y2)
{
    return sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
}

double RandomWalk::mindistance(double d1,double d2,double d3,double d4)
{
    double d = d1>d2?d2:d1;
    d = d>d3?d3:d;
    d = d>d4?d4:d;
    return d;
}



//////////////////////////////////////////////////////////////////////////////////

class Grid_Generation
{
public:
    Grid_Generation(ros::NodeHandle *nh, RandomWalk *rw);
    
   
private:
    ros::NodeHandle* n;
    ros::Subscriber sub_corners;
    ros::Subscriber sub_current_pos;
    bool initial_pos;
    bool corner_pos;
    void corner_callback(const std_msgs::Float64MultiArray::ConstPtr & corners);
    void current_position_callback(const geometry_msgs::PoseWithCovarianceStamped::ConstPtr & current_pos);
    double distance(double x1,double y1,double x2,double y2);
    double mindistance(double d1,double d2,double d3,double d4);
    void get_nearest_corner_point(double *point, double init_x, double init_y);
    double init_pos_x;
    double init_pos_y;
    double minX,minY,maxX,maxY;//,minX2,minY2,maxX2,maxY2;
    double centreX;
    double centreY;
    RandomWalk* r_w;
    double near_point[2];
    bool rw_call;
};

Grid_Generation::Grid_Generation(ros::NodeHandle *nh, RandomWalk *rw) : n(nh), r_w(rw)
{
    sub_corners = nh->subscribe("/corner_and_dimension", 1000, &Grid_Generation::corner_callback,this);
    sub_current_pos = nh->subscribe("/robot_pose_ekf/odom", 1000, &Grid_Generation::current_position_callback,this);
    initial_pos = false;
    corner_pos = false;
    rw_call = false;
    init_pos_x = 0.0;
    init_pos_y = 0.0;
    
}

void Grid_Generation::corner_callback(const std_msgs::Float64MultiArray::ConstPtr & corners)
{
    if (corner_pos) return;
    //ROS_INFO("CENTRE X: %lf and CENTRE Y: %lf",corners->data[0],corners->data[1]);
    if (init_pos_x != 0.0)
    {
        centreX = corners->data[0];
        centreY = corners->data[1];
        init_pos_x -= corners->data[0];
        init_pos_y -= corners->data[1];
        minX = corners->data[2];
        maxX = corners->data[3];
        minY = corners->data[4];
        maxY = corners->data[5];

        //minX2 = corners->data[8];
       // maxX2 = corners->data[9];
        //minY2= corners->data[10];
       // maxY2 = corners->data[11];
       //cout << "size : " << corners->data.size() << endl; 
       Corner_points.data.resize(corners->data.size() - 6);
       int total_corner_point = (corners->data.size() - 6);
       for(int i = 0;i < total_corner_point;i++)
       {
		   Corner_points.data[i] = corners->data[6+i];	
		   //cout<<   Corner_points.data[i]<<endl;
		}
		
        get_nearest_corner_point(near_point,init_pos_x,init_pos_y);
        
        
        //cout << "CURX: " << init_pos_x << "  CURY: " << init_pos_y << endl;
        
        double del1 = init_pos_x > 0?-0.6:0.6;
        double del2 = init_pos_y > 0?-0.6:0.6;
        near_point[0] += del1;
        near_point[1] += del2;
        init_pos_x = near_point[0]; 
        init_pos_y = near_point[1];
        corner_pos = true;
        rw_call = true;
        //cout << "MINX: " << minX << "  MAXX: " << maxX << "  MINY: " << minY << "  MAXY: " << maxY << endl;
        //cout << "CURDX: " << init_pos_x << "  CURDY: " << init_pos_y << endl;
        //cout << "CnpX: " << near_point[0] << "  CnpY: " << near_point[1] << endl;
        
        //r_w->setLimits(minX, maxX, minY, maxY, init_pos_x, init_pos_y);
        //r_w->sendNewGoal(near_point[0],near_point[1]);
        
        
        //ROS_INFO("minx:%lf miny:%lf maxx: %lf maxy:%lf curx:%lf cury:%lf minx2:%lf miny2:%lf maxx2: %lf maxy2:%lf",minX,minY,maxX,maxY,init_pos_x,init_pos_y,minX2,minY2,maxX2,maxY2);
        
        //r_w->set_corners(minX,maxX,minY,maxY,minX2,maxX2,minY2,maxY2,init_pos_x,init_pos_y);

        
        /*
        double d1 = distance(minX,minY,init_pos_x,init_pos_y);
        double d2 = distance(minX,maxY,init_pos_x,init_pos_y);
        double d3 = distance(maxX,minY,init_pos_x,init_pos_y);
        double d4 = distance(maxX,maxY,init_pos_x,init_pos_y);
        double d = mindistance(d1,d2,d3,d4);
        double del = 0.6;
        if(d == d1)
        {
            init_pos_x = minX; 
            init_pos_y = minY;
            r_w->setLimits(minX, maxX, minY, maxY, init_pos_x, init_pos_y);
            r_w->sendNewGoal(minX+del,minY+del);
        }
        else if( d == d2)
        {
            init_pos_x = minX; 
            init_pos_y = maxY;
            r_w->setLimits(minX, maxX, minY, maxY, init_pos_x, init_pos_y);
            r_w->sendNewGoal(minX+del,maxY-del);
        }
        else if( d == d3)
        {
            init_pos_x = maxX; 
            init_pos_y = minY;
            r_w->setLimits(minX, maxX, minY, maxY, init_pos_x, init_pos_y);
            r_w->sendNewGoal(maxX-del,minY+del);
        }
        else if( d == d4)
        {
            init_pos_x = maxX; 
            init_pos_y = maxY;
            r_w->setLimits(minX, maxX, minY, maxY, init_pos_x, init_pos_y);
            r_w->sendNewGoal(maxX-del,maxY-del);
        } */
    //cout << "INIT X: " <<  init_pos_x << "  INIT Y: " << init_pos_y;     
    }
}

void Grid_Generation::current_position_callback(const geometry_msgs::PoseWithCovarianceStamped::ConstPtr & current_pos)
{
    if(initial_pos)
    {
        current_x = current_pos->pose.pose.position.x - centreX;
        current_y = current_pos->pose.pose.position.y - centreY;
        //cout << "calling" << endl;
        //cout << "CnpX: " << current_x << "  CnpY: " << current_y << endl;
        if(rw_call && corner_pos)
        {
			rw_call = false;
			r_w->setLimits(minX, maxX, minY, maxY, init_pos_x, init_pos_y);
            r_w->sendNewGoal(near_point[0],near_point[1]);
		}
        return;
    }
    //ROS_INFO("CURRENT POS X: %lf and CURRENT POS Y: %lf",current_pos->pose.pose.position.x,current_pos->pose.pose.position.y);
    initial_pos = true;
    init_pos_x = current_pos->pose.pose.position.x;
    init_pos_y = current_pos->pose.pose.position.y;
    current_x = init_pos_x;
    current_y = init_pos_y;
}


void Grid_Generation::get_nearest_corner_point(double *point, double init_x, double init_y)
{
	double d = 1000;
	int l = Corner_points.data.size()/2;
	for(int i = 0; i < l;++i)
	{
		int indx = i*2;
		//cout << indx << "   " << (indx+1) << endl;
		//cout << Corner_points.data[indx] << "   " << Corner_points.data[indx+1] << endl;
		double d2 = distance(Corner_points.data[indx],Corner_points.data[indx+1],init_x,init_y);
		if(d2 < d)
		{
			d = d2;
			point[0] = Corner_points.data[indx];
			point[1] = Corner_points.data[indx+1];
		}
	}
}


double Grid_Generation::distance(double x1,double y1,double x2,double y2)
{
    return sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
}

double Grid_Generation::mindistance(double d1,double d2,double d3,double d4)
{
    double d = d1>d2?d2:d1;
    d = d>d3?d3:d;
    d = d>d4?d4:d;
    return d;
}

///////////////////////////////////////////////////////////////////////////////////





int main(int argc, char** argv)
{
    ros::init(argc, argv, "teamd_navigation");

    ROS_INFO("HRATC 2016 Team DISHARI‬ Navigation Node");
    

    ros::NodeHandle n;
    ros::NodeHandle pn("~");   

    RandomWalk rw(&n);
    Grid_Generation grid_g(&n, &rw);
    ros::spin();

    return 0;
}


