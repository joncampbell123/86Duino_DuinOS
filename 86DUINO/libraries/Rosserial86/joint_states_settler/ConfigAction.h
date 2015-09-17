#ifndef _ROS_joint_states_settler_ConfigAction_h
#define _ROS_joint_states_settler_ConfigAction_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "joint_states_settler/ConfigActionGoal.h"
#include "joint_states_settler/ConfigActionResult.h"
#include "joint_states_settler/ConfigActionFeedback.h"

namespace joint_states_settler
{

  class ConfigAction : public ros::Msg
  {
    public:
      joint_states_settler::ConfigActionGoal action_goal;
      joint_states_settler::ConfigActionResult action_result;
      joint_states_settler::ConfigActionFeedback action_feedback;

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      offset += this->action_goal.serialize(outbuffer + offset);
      offset += this->action_result.serialize(outbuffer + offset);
      offset += this->action_feedback.serialize(outbuffer + offset);
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      offset += this->action_goal.deserialize(inbuffer + offset);
      offset += this->action_result.deserialize(inbuffer + offset);
      offset += this->action_feedback.deserialize(inbuffer + offset);
     return offset;
    }

    const char * getType(){ return "joint_states_settler/ConfigAction"; };
    const char * getMD5(){ return "baa9ae56369a6c832695f8fd10a504f5"; };

  };

}
#endif