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
#include <vector>
#include "crc.h"
#include "framing.h"
#include "file_reader.h"
#include<iostream>
#define MAX_SEQ 7
#define WINDOW_SIZE 4
#define TIMEOUT_THRESHOLD 5     // seconds
#define ACK_TIMEOUT_THRESHOLD 8 // seconds

enum DATA_KIND {
    NACK, ACK, DATA
};
int ack_expected = 0;
int next_frame_to_send = 0;
int frame_expected = 0;
int too_far = WINDOW_SIZE;
int i;
bool sender = 0;
int nBuffered = 0;
bool ack_timer = false;
bool no_nak = true;

std::vector<bool> timer_buffer;
std::vector<bool> isArrived;
std::vector<std::string> out_buf;
std::vector<std::string> in_buf;
std::vector<std::pair<std::string, std::string>> data;
void inc(int &k) {
    if (k < MAX_SEQ)
        k = k + 1;
    else
        k = 0;
}
void start_timer(int num, Node *node) {
    timer_buffer[num] = true;
    char c = char(num);
    cMessage *selfMessage = new cMessage(&c, 0);
    node->scheduleAt(simTime() + TIMEOUT_THRESHOLD, selfMessage);
}
void stop_timer(int num) {
    timer_buffer[num] = -1;
}
void start_ack_timer(Node *node) {
    ack_timer = true;
    cMessage *selfMessage = new cMessage("", 1);
    node->scheduleAt(simTime() + ACK_TIMEOUT_THRESHOLD, selfMessage);
}
void stop_ack_timer() {
    ack_timer = false;
}
static bool between(int a, int b, int c) {
    return ((a <= b) && (b < c)) || ((c < a) && (a <= b))
            || ((b < c) && (c < a));
}
static void send_frame(DATA_KIND type, int frame_number, int frame_expected,
        std::vector<std::string> buffer, Node *node) {
    MyMessage_Base *msg = new MyMessage_Base();
    msg->setType(type);
    if (type == DATA) // Data
            {
        std::string frame = buffer[frame_number % WINDOW_SIZE];
        msg->setTrailer(binaryToMessage(computeCRC(messageToBinary(frame)))[0]);
        msg->setPayload(frame.c_str());
    }
    msg->setHeader(frame_number);
    msg->setAck_number((frame_expected + MAX_SEQ) % (MAX_SEQ + 1));
    if (type == NACK) // NACK
            {
        no_nak = false;
    }
    node->send(msg, "out");
    if (type == DATA) {
        start_timer(frame_number % WINDOW_SIZE, node);
    }
    stop_ack_timer();
}
void handle_message_timeout(cMessage *msg, Node *node) {
    int num = int(msg->getName()[0]);
    if (timer_buffer[num]) {
        send_frame(DATA, num, frame_expected, out_buf, node);
    }
}
void handle_ack_timeout(Node *node) {
    send_frame(ACK, 0, frame_expected, out_buf, node);
}

Define_Module(Node);

void Node::initialize() {
    // Timer initialization
    timer_buffer = std::vector<bool>(WINDOW_SIZE, false);
    isArrived = std::vector<bool>(WINDOW_SIZE, false);
    out_buf = std::vector<std::string>(WINDOW_SIZE);
    in_buf = std::vector<std::string>(WINDOW_SIZE);
    auto number = this->getName()[5] == '0' ? false : true;

    if (!number) { //TODO handle who is the receiver
        auto delay_time = 1; //must be given from the coordinator
        data = read_file(number);
        sender = true;
        scheduleAt(simTime() + delay_time, new cMessage("", 2));
        i = 0;
    }

}

void Node::handleMessage(cMessage *msg) {
    if (msg->isSelfMessage()) {

        auto kind = msg->getKind();
        // Either ack or message timeout
        if (kind == 0) {
            handle_message_timeout(msg, this);
            cancelAndDelete(msg);
            return;
        }

        else if (kind == 1) {
            handle_ack_timeout(this);
            cancelAndDelete(msg);
            return;
        } else {
            if (sender) {

                if (nBuffered < WINDOW_SIZE) {
                    nBuffered++;
                    // TODO: Handle delays here
                    out_buf[next_frame_to_send % WINDOW_SIZE] = framing(
                            data[i].second);

                    send_frame(DATA, next_frame_to_send, frame_expected,
                            out_buf, this);
                    inc(next_frame_to_send);
                    cancelAndDelete(msg);
                    return;
                }
            }
        }

    }

    MyMessage_Base *mmsg = check_and_cast<MyMessage_Base*>(msg);
    int kind = mmsg->getType();
    int ack_number = mmsg->getAck_number();
    int seqNum = mmsg->getHeader();
    std::string payload = mmsg->getPayload();
    char trailer = mmsg->getTrailer();
    cancelAndDelete(mmsg);
    if (kind == DATA) {

        if (seqNum != frame_expected && no_nak) {
            send_frame(NACK, 0, frame_expected, out_buf, this);
        } else {
            start_ack_timer(this);
        }
        // Message check


        if (!checkMessage(payload + std::to_string(trailer))) {
            if (no_nak) {
                send_frame(NACK, 0, frame_expected, out_buf, this);
                return;
            }
        }
        if (between(frame_expected, seqNum, too_far)
                && !isArrived[seqNum % WINDOW_SIZE]) {
            isArrived[seqNum % WINDOW_SIZE] = true;
            std::string receivedFrame = binaryToMessage(
                    getMessageFromEncoded(messageToBinary(payload + trailer)));
            in_buf[seqNum % WINDOW_SIZE] = receivedFrame;
            while (isArrived[frame_expected % WINDOW_SIZE]) {
                std::string receivedMessage = deframing(receivedFrame);
                // TODO: Do what you want with this message
                EV << receivedFrame << endl;
                no_nak = true;
                isArrived[frame_expected % WINDOW_SIZE] = false;
                inc(frame_expected);
                inc(too_far);

                start_ack_timer(this);
            }
        }
    }

    if (kind == NACK
            && between(ack_expected, (ack_number + 1) % (MAX_SEQ + 1),
                    next_frame_to_send)) {
        send_frame(DATA, (ack_number + 1) % (MAX_SEQ + 1), frame_expected,
                out_buf, this);
    }

    while (between(ack_expected, ack_number, next_frame_to_send)) {
        nBuffered--;
        stop_timer(ack_expected % WINDOW_SIZE);
        inc(ack_expected);
    }
    //Send message if any
    if (sender) {
        if (nBuffered < WINDOW_SIZE) {
            nBuffered++;
            i++;
            // TODO: Handle delays here
            out_buf[next_frame_to_send % WINDOW_SIZE] = framing(data[i].second);
            send_frame(DATA, next_frame_to_send, frame_expected, out_buf, this);
            inc(next_frame_to_send);
        }
    }

}
