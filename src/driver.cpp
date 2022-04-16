/**
 * \file driver.cpp
 *
 * \copyright 2022 John Harwell, All rights reserved.
 *
 * This file is part of ROSBRIDGE.
 *
 * ROSBRIDGE is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * ROSBRIDGE is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ROSBRIDGE.  If not, see <http://www.gnu.org/licenses/
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "sr04us/driver.hpp"

#include <wiringPi.h>
#include <ros/ros.h>

/*******************************************************************************
 * Namespaces/Decls
 ******************************************************************************/
NS_START(rosbridge, sr04us);

/*******************************************************************************
 * Constructors/Destructor
 ******************************************************************************/
driver::driver(void)
    : ER_CLIENT_INIT("rosbridge.sr04us.driver") {
  wiringPiSetupGpio();
}

/*******************************************************************************
* Member Functions
******************************************************************************/
void driver::gpio_configure(int trig, int echo)  {
  ER_DEBUG("Configuring GPIO: trig=%d, echo=%d", trig, echo);
  pinMode(echo, INPUT);
  pinMode(trig, OUTPUT);
  ER_DEBUG("Configuring GPIO complete");
}

float driver::distance_measure(int trig, int echo)    {
  struct timeval tv1;
  struct timeval tv2;
  long start, stop;
  float dis;

  // Sends ultrasonic signal
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  //Detects ultrasonic signal
  auto send_start = ros::Time::now();
  ros::Duration timeout(0.1);
  while(!(digitalRead(echo) == 1)) {
    if (RCPPSW_UNLIKELY(ros::Time::now() - send_start >= timeout)) {
      ER_ERR("Unable to send ping: bad pin assignment?");
      return -1;
    }
  } /* while() */
  gettimeofday(&tv1, NULL);

  auto recv_start = ros::Time::now();
  while(!(digitalRead(echo) == 0)) {
    if (RCPPSW_UNLIKELY(ros::Time::now() - recv_start >= timeout)) {
      ER_ERR("Did not receive return ping: bad pin assignment?");
    }
  } /* while() */
  gettimeofday(&tv2, NULL);

  //Measure gap of time and calculate distance
  start = tv1.tv_sec * 1000000 + tv1.tv_usec;
  stop  = tv2.tv_sec * 1000000 + tv2.tv_usec;
  dis = (float)(stop - start) / 1000000 * 34000 / 2;

  return dis;
}

bool driver::report(ping::Request &req,
                    ping::Response &res)  {
  auto trig = req.trig;
  auto echo = req.echo;

  ER_INFO("Request: trig=%d, echo=%d", req.trig, req.echo);
  gpio_configure(trig, echo);

  res.reading1 = distance_measure(trig, echo);
  res.reading2 = distance_measure(trig, echo);

  ER_INFO("Response: reading1=%f, reading2=%f", res.reading1, res.reading2);
  return true;
}

NS_END(sr04us, rosbridge);