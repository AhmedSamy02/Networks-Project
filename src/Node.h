//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef __NETWORKSPROJECT_NODE_H_
#define __NETWORKSPROJECT_NODE_H_

#include <omnetpp.h>

using namespace omnetpp;

/**
 * TODO - Generated class
 */
class Node : public cSimpleModule
{
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
  public:
    int ack_expected = 0;
    int next_frame_to_send = 0;
    int frame_expected = 0;
    int too_far = 0;
    int i;
    int last_time = 0;
    int nBuffered = 0;
    cMessage* ack_timer = nullptr;
    bool no_nak = true;
    int data_length = 0;
    std::vector<cMessage*> timer_buffer;
    std::vector<bool> isArrived;
    std::vector<bool> delayed;
    std::vector<bool> duplicated;
    std::vector<bool> lossed;
    std::vector<std::string> out_buf;
    std::vector<std::string> in_buf;
    std::vector<std::pair<std::string, std::string>> data;
    bool sender = false;
};

#endif
