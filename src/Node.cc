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
#include "file_writer.h"
#include <iostream>
#define MAX_SEQ 7
#define WINDOW_SIZE ((MAX_SEQ + 1) / 2)
#define TIMEOUT_THRESHOLD 15      // seconds
#define ACK_TIMEOUT_THRESHOLD 20 // seconds

enum DATA_KIND {
    NACK, ACK, DATA
};
int ack_expected = 0;
int next_frame_to_send = 0;
int frame_expected = 0;
int too_far = WINDOW_SIZE;
int i;
std::string id;
bool sender = 0;
int nBuffered = 0;
bool ack_timer = false;
bool no_nak = true;
int data_length = 0;

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
    timer_buffer[num] = 0;
}
void start_ack_timer(Node *node) {
    ack_timer = true;
    cMessage *selfMessage = new cMessage("", 1);
    node->scheduleAt(simTime() + ACK_TIMEOUT_THRESHOLD, selfMessage);
}
void stop_ack_timer() {
    ack_timer = false;
}
bool between(int a, int b, int c) {
    return ((a <= b) && (b < c)) || ((c < a) && (a <= b))
            || ((b < c) && (c < a));
}
void send_frame(DATA_KIND type, int frame_number, int frame_expected,
        std::vector<std::string> buffer, Node *node) {
    MyMessage_Base *msg = new MyMessage_Base();
    msg->setType(type);
    if (type == DATA) // Data
            {
        std::string frame = buffer[frame_number % WINDOW_SIZE];
        auto crc = computeCRC(messageToBinary(frame));
        msg->setTrailer(binaryToMessage(crc)[0]);
        msg->setPayload(frame.c_str());
        write_before_transmission(simTime().str(), id,
                std::to_string(frame_number), frame, crc);
    }
    msg->setHeader(frame_number);
    msg->setAck_number(frame_expected);
    EV << msg->getType() << ' ' << msg->getAck_number() << ' '
              << msg->getHeader() << ' ' << msg->getPayload() << ' '
              << msg->getTrailer() << ' ' << endl;
    if (type == NACK) // NACK
            {
        no_nak = false;
        write_control_frame(simTime().str(), id, NACK,
                std::to_string(frame_number));
    }
    if (type == ACK) {
        write_control_frame(simTime().str(), id, ACK,
                std::to_string(frame_number));
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
    if (ack_timer) {
        send_frame(ACK, 0, frame_expected, out_buf, node);
    }

}

Define_Module(Node);

void Node::initialize() {
    // Timer initialization
    timer_buffer = std::vector<bool>(WINDOW_SIZE, false);
    isArrived = std::vector<bool>(WINDOW_SIZE, false);
    out_buf = std::vector<std::string>(WINDOW_SIZE);
    in_buf = std::vector<std::string>(WINDOW_SIZE);
    auto number = this->getName()[5] == '0' ? false : true;
    id = number ? "0" : "1";
    if (!number) {                        // TODO handle who is the receiver
        auto delay_time = 1; // must be given from the coordinator
        data = read_file(number);
        EV << data[4].second << endl;
        sender = true;
        scheduleAt(simTime() + delay_time, new cMessage("", 2));
        i = 0;
        data_length = data.size();
    }
}

void Node::handleMessage(cMessage *msg) {
    if (msg->isSelfMessage()) {

        auto kind = msg->getKind();
        // Either ack or message timeout
        if (kind == 0) {
            handle_message_timeout(msg, this);
        } else if (kind == 1) {
            handle_ack_timeout(this);
        } else {
            if (sender) {
                while (nBuffered < WINDOW_SIZE) {
                    nBuffered++;
                    // TODO: Handle delays here
                    out_buf[next_frame_to_send % WINDOW_SIZE] = framing(
                            data[i].second);

                    send_frame(DATA, next_frame_to_send, frame_expected,
                            out_buf, this);
                    inc(next_frame_to_send);
                    EV << "Next Frame to send" << next_frame_to_send << endl;
                    i++;
                }
            }
        }
        cancelAndDelete(msg);
        return;
    }
    MyMessage_Base *mmsg = check_and_cast<MyMessage_Base*>(msg);
    int kind = mmsg->getType();
    int ack_number = mmsg->getAck_number();
    int seqNum = mmsg->getHeader();
    std::string payload = mmsg->getPayload();
    EV << mmsg->getTrailer() << endl;
    char trailer = mmsg->getTrailer();
    EV << kind << ' ' << ack_number << ' ' << seqNum << ' ' << payload << ' '
              << trailer << ' ' << endl;
    cancelAndDelete(msg);
    if (kind == DATA) {

        if (seqNum != frame_expected && no_nak) {
            send_frame(NACK, 0, frame_expected, out_buf, this);
        } else {
            start_ack_timer(this);
        }
        // Message check
        EV << payload << " " << trailer << endl;
//        std::string temp = payload;
//        temp+=trailer;
//        EV << temp<<endl;
        if (!checkMessage(messageToBinary(payload), trailer)) {
//            EV << payload << " " << trailer << endl;
//            EV << payload + trailer;
            if (no_nak) {
                send_frame(NACK, 0, frame_expected, out_buf, this);
                return;
            }
        }
        if (between(frame_expected, seqNum, too_far)
                && !isArrived[seqNum % WINDOW_SIZE]) {
            std::string receivedFrame = binaryToMessage(
                    getMessageFromEncoded(messageToBinary(payload + trailer)));
            isArrived[seqNum % WINDOW_SIZE] = true;
            in_buf[seqNum % WINDOW_SIZE] = receivedFrame;
//            EV << receivedFrame << endl;
//            int x = frame_expected;
//            int y = too_far;
//            while (x != y) {
//                if (!isArrived[x % WINDOW_SIZE]) {
//                    if (between(seqNum, x, too_far)) {
//                        send_frame(ACK, frame_expected, x, out_buf, this);
//                    } else {
//                        send_frame(NACK, frame_expected, x, out_buf, this);
//                    }
//                    frame_expected = (x) % WINDOW_SIZE;
//                    too_far = (frame_expected + WINDOW_SIZE) % WINDOW_SIZE;
//                    break;
//                }
//                inc(x);
//            }
//            if (x == y && isArrived[y]) {
//                frame_expected = (y + 1) % WINDOW_SIZE;
//                too_far = (frame_expected + WINDOW_SIZE) % WINDOW_SIZE;
//            }
//            start_ack_timer(this);

            EV << "Before while" << endl;
            while (isArrived[frame_expected % WINDOW_SIZE]) {
                EV << "Inside while" << endl;
                std::string receivedMessage = deframing(
                        in_buf[frame_expected % WINDOW_SIZE]);
                // TODO: Do what you want with this message
                write_frame_received(receivedMessage, std::to_string(seqNum));

                no_nak = true;
                isArrived[frame_expected % WINDOW_SIZE] = false;
                inc(frame_expected);
                EV << "Expected = " << frame_expected << endl;
//                frame_expected = (frame_expected + 1) % (MAX_SEQ+1);
//                EV << "Expected = " << frame_expected << endl;
                inc(too_far);
                EV << "too_far = " << too_far << endl;
                start_ack_timer(this);
            }

            EV << "After while ";

            if (!between(frame_expected, seqNum, too_far)) {
                EV << "Expected = " << frame_expected << endl;
                send_frame(ACK, seqNum, frame_expected, out_buf, this);
            } else {
                send_frame(NACK, seqNum, frame_expected, out_buf, this);
            }
        }
    }

    if (kind == NACK
            && between(ack_expected, (ack_number + 1) % (MAX_SEQ + 1),
                    next_frame_to_send)) {
        send_frame(DATA, (ack_number - WINDOW_SIZE) % (WINDOW_SIZE),
                frame_expected, out_buf, this);
    }

    while (between(ack_expected, ack_number, next_frame_to_send)) {
        nBuffered--;
        stop_timer(ack_expected % WINDOW_SIZE);
        inc(ack_expected);
    }
    // Send message if any
    if (sender) {
        if (nBuffered < WINDOW_SIZE && i < data_length) {
            nBuffered++;
            // TODO: Handle delays here
            out_buf[next_frame_to_send % WINDOW_SIZE] = framing(data[i].second);
            send_frame(DATA, next_frame_to_send, frame_expected, out_buf, this);

            inc(next_frame_to_send);
            EV << "Next Frame to send" << next_frame_to_send << endl;
            i++;
        }
    }
}
