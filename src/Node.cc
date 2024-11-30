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
#include "Node.h"
#include "MyMessage_m.h"
#include<string>
#define MAX_SEQ 7
#define WINDOW_SIZE 4
bool no_nak = true;
const char FLAG = '$';
const char ESC = '\\';
long long time  = 0;

int ack_expected;
int next_frame_to_send;
int frame_expected;
int too_far;
int i;


string framing(string s)
{
    string s1 = "";
    s1 += FLAG;
    for (size_t i = 0; i < s.length(); i++)
    {
        if (s[i] == FLAG || s[i] == ESC)
        {
            s1 += ESC;
        }
        s1 += s[i];
    }
    s1 += FLAG;
    return s1;
}
static bool between(int a,int b,int c){
    return ((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a));
}
static void send_frame(int type, int frame_number,int frame_expected, string buffer[],MyMessage_Base* msg){
    msg->setType(type);
    if (type==2){
        msg->setPayload( buffer[frame_number%WINDOW_SIZE]);
    }
    msg->setAck_number((frame_expected+MAX_SEQ)%(MAX_SEQ+1));
    if (type == 0){
        no_nak = false;
    }
    send(msg,'out');
    if (type==2){
        //TODO : Start timer

    }
    //TODO : stop timer
}

void protcol(){

}

Define_Module(Node);

void Node::initialize()
{
    time = simTime();
    if (strcmp(this->getName() , 'node0') == 0){
        MyMessage_Base* msg = new MyMessage_Base();
        msg->setAck_number(0);
        msg->setHeader(0);
        msg->setPayload("Hello");
        msg->setType(0);
        msg->setTrailer('A');

        send(msg,'out');
    }
}

void Node::handleMessage(cMessage *msg)
{
    MyMessage_Base *mmsg = check_and_cast<MyMessage_Base *>(msg);
    auto payload = mmsg->getPayload();
    EV<< payload<<endl;


}
