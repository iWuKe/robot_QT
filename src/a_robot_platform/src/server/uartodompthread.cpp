#include "uartodompthread.h"
#include "uartodom.h"

extern zw::UartOdom m_uartOdom;

namespace zw {


UartOdomPthread::UartOdomPthread()
{
  pthread_create(&id,NULL,MyPthread,(void *)this);
}

UartOdomPthread::~UartOdomPthread()
{
  pthread_detach(id);
}

void *UartOdomPthread::MyPthread(void *temp)
{
  UartOdomPthread *t=(UartOdomPthread *)temp;
  t->DoPthread();
}

void *UartOdomPthread::DoPthread(void)
{
  ros::NodeHandle n;
  ros::Publisher odom_pub = n.advertise<nav_msgs::Odometry>("odom", 100);
  ros::Publisher imu_pub =n.advertise<sensor_msgs::Imu>("imu",100);

  ros::Time starts = ros::Time::now();
  ros::Time ends = ros::Time::now();

  sensor_msgs::Imu  imu;

  float linear_acceleration_stddev=0;
  float linear_velocity_stddev=0;
  float orientation_stddev =0;

  imu.linear_acceleration_covariance[0]= linear_acceleration_stddev ;
  imu.linear_acceleration_covariance[4]= linear_acceleration_stddev ;
  imu.linear_acceleration_covariance[8]= linear_acceleration_stddev ;
  imu.angular_velocity_covariance[0]= linear_velocity_stddev ;
  imu.angular_velocity_covariance[4]= linear_velocity_stddev ;
  imu.angular_velocity_covariance[8]= linear_velocity_stddev ;
  imu.orientation_covariance[0]=orientation_stddev;
  imu.orientation_covariance[4]=orientation_stddev;
  imu.orientation_covariance[8]=orientation_stddev;

  double dt ;
  double x=0.0;
  double y=0.0;
  double th=0.0;
  double lth =0.0;

  double vx;
  double vy;
  double vth;
  double delta_x;
  double delta_y ;
  double delta_th ;
  tf:: TransformBroadcaster odom_broadcaster;

  ros::Rate loop_rate(50);
  while(ros::ok())
  {
    ros::spinOnce();
    ends = ros::Time::now();
    dt =(ends-starts).toSec();

    int32_t car_msg[6];
    ParaGetSet car_para={R_HOLDING_REGISTER,2,MSG_CONTROL,car_msg};
    modbus.GetAddressValue(car_para);
    zw::Float2Int32 mf;
    mf.i= car_msg[0];
    vx =(double)mf.f;

    car_para={R_HOLDING_REGISTER,6,MSG_IMU,car_msg};
    modbus.GetAddressValue(car_para);
    vth = (double)car_msg[5] * Gyro_Gr;    //gyr_z
    delta_th = vth*dt;
    th += delta_th;
    delta_x = (vx * cos(th) -vy *sin(th)) * dt;
    delta_y = (vx * sin(th) -vy *cos(th)) * dt;

    x +=delta_x ;
    y +=delta_y ;

    geometry_msgs :: Quaternion odom_quat =tf::createQuaternionMsgFromYaw(th);

    geometry_msgs :: TransformStamped odom_trans;
    odom_trans.header.stamp = ends;
    odom_trans.header.frame_id = "odom";
    odom_trans.child_frame_id ="base_link";

    odom_trans.transform.translation.x =x;
    odom_trans.transform.translation.y =y;
    odom_trans.transform.translation.z =0.0;
    odom_trans.transform.rotation =odom_quat;
    odom_broadcaster.sendTransform(odom_trans);

    nav_msgs ::Odometry odom;
    odom.header.stamp =ends;
    odom.header.frame_id = "odom" ;

    odom.pose.pose.position.x =x;
    odom.pose.pose.position.y =y;
    odom.pose.pose.position.z =0.0;
    odom.pose.pose.orientation = odom_quat ;
    odom.child_frame_id = "base_link" ;
    odom.twist.twist.linear.x =vx;
    odom.twist.twist.linear.y =vy;
    odom.twist.twist.angular.z = vth ;
    //publish odom
    odom_pub.publish(odom);

    float xf=car_msg[0] *Acc_Mss;    //acc_x   m/s^2
    float yf=car_msg[1] *Acc_Mss;
    float zf=car_msg[2] *Acc_Mss;
    imu.linear_acceleration.x =xf;
    imu.linear_acceleration.y =yf;
    imu.linear_acceleration.z =zf;
    xf= car_msg[3] * Gyro_Gr;   //gyr_x     rad/s
    yf= car_msg[4] * Gyro_Gr;
    zf= car_msg[5] * Gyro_Gr;
    imu.angular_velocity.x =xf;
    imu.angular_velocity.y =yf;
    imu.angular_velocity.z =zf;
    imu.header.stamp= ends;
    imu.header.frame_id = "imu_link";
    //publish imu
    imu_pub.publish(imu);

    starts = ends;
    loop_rate.sleep();
  }
}


}
