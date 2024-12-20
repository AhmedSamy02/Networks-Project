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

enum DATA_KIND {
    NACK, ACK, DATA
};
int TIMEOUT_THRESHOLD = 0;
int MAX_SEQ = 0;
int WINDOW_SIZE = 0;
double ERROR_DELAY = 0;
double PROCESSING_TIME = 0;
double DUPLICATION_DELAY = 0;

void inc(int &k) {
    if (k < MAX_SEQ)
        k = k + 1;
    else
        k = 0;
}
void start_timer(int num, Node *node) {
    char c = char(num);
    if (node->timer_buffer[num % WINDOW_SIZE]) {
        node->cancelEvent(node->timer_buffer[num % WINDOW_SIZE]);
        delete node->timer_buffer[num % WINDOW_SIZE];
    }
    cMessage *selfMessage = new cMessage(&c, 0);
    node->timer_buffer[num] = selfMessage;
    node->scheduleAt(simTime() + TIMEOUT_THRESHOLD, selfMessage);
}
void stop_timer(int num, Node *node) {
    num = num % WINDOW_SIZE;
    cMessage *timer = node->timer_buffer[num];
    if (node->timer_buffer[num]) {
        node->cancelEvent(timer);
        delete timer;
        node->timer_buffer[num] = nullptr;
    }
}
void start_ack_timer(Node *node) {

    cMessage *selfMessage = new cMessage("", 1);
    node->ack_timer = selfMessage;
    node->scheduleAt(simTime() + TIMEOUT_THRESHOLD, selfMessage);
}
void stop_ack_timer(Node *node) {
    node->cancelEvent(node->ack_timer);
}
bool between(int a, int b, int c) {
    return ((a <= b) && (b < c)) || ((c < a) && (a <= b))
            || ((b < c) && (c < a));
}
void send_frame(DATA_KIND type, int frame_number, int frame_expected,
        std::vector<std::string> buffer, Node *node, char delay = '0',
        char duplicate = '0') {
    MyMessage_Base *msg = new MyMessage_Base();
    auto id = node->getName()[5];
    msg->setType(type);
    if (type == DATA) // Data
            {
        std::string frame = buffer[frame_number % WINDOW_SIZE];
        auto crc = computeCRC(messageToBinary(frame));
        msg->setTrailer(binaryToMessage(crc)[0]);
        msg->setPayload(frame.c_str());
    }
    msg->setHeader(frame_number);
    msg->setAck_number(frame_expected);
    EV << msg->getType() << ' ' << msg->getAck_number() << ' '
              << msg->getHeader() << ' ' << msg->getPayload() << ' '
              << msg->getTrailer() << ' ' << endl;
    if (type == NACK) // NACK
            {
        node->no_nak = false;
        write_control_frame((simTime()).str(), id, NACK,
                std::to_string(frame_expected));
        node->send(msg, "out");
    } else if (type == ACK) {
//        EV << "ACKKKKKKK   " << (simTime() + PROCESSING_TIME).str() << endl;
        write_control_frame(simTime().str(), id, ACK,
                std::to_string(frame_expected));
        node->send(msg, "out");
    } else {
        if (delay == '1') {
            node->delayed[frame_number % WINDOW_SIZE] = true;
        }
        if (duplicate == '1') {
            node->duplicated[frame_number % WINDOW_SIZE] = true;
            EV << "Is duplicated = "
                      << node->duplicated[frame_number % WINDOW_SIZE] << endl;
        }
        EV << msg->getPayload() << endl;
        node->scheduleAt(simTime() + PROCESSING_TIME, msg);

    }
//    stop_ack_timer(node);
}
void handle_message_timeout(cMessage *msg, Node *node) {
    int num = int(msg->getName()[0]);
    EV << "Number in timer = " << num;
    if (node->timer_buffer[num]) {
        send_frame(DATA, num, node->frame_expected, node->out_buf, node);
    }
}
void handle_ack_timeout(Node *node) {
//    if (node->ack_timer) {
    send_frame(ACK, 0, node->frame_expected, node->out_buf, node);
//    }

}

Define_Module(Node);

void Node::initialize() {
    // Read parameters
    WINDOW_SIZE = getParentModule()->par("WS").intValue();
    MAX_SEQ = getParentModule()->par("SN").intValue();
    TIMEOUT_THRESHOLD = getParentModule()->par("TO").intValue();
    PROCESSING_TIME = getParentModule()->par("PT").doubleValue();
    ERROR_DELAY = getParentModule()->par("ED").doubleValue();
    DUPLICATION_DELAY = getParentModule()->par("DD").doubleValue();
    // Variables initialization
    too_far = WINDOW_SIZE;
    // Timer initialization
    timer_buffer = std::vector<cMessage*>(WINDOW_SIZE, nullptr);
    isArrived = std::vector<bool>(WINDOW_SIZE, false);
    delayed = std::vector<bool>(WINDOW_SIZE, false);
    duplicated = std::vector<bool>(WINDOW_SIZE, false);
    out_buf = std::vector<std::string>(WINDOW_SIZE);
    in_buf = std::vector<std::string>(WINDOW_SIZE);

    if (this->getName()[5] == '0') {          // TODO handle who is the receiver
//        EV << this->getName() << " I\'m Sender" << endl;
        auto delay_time = 0; // must be given from the coordinator for sender to start sending ya yara
        data = read_file(0);
        scheduleAt(simTime() + delay_time, new cMessage("", 2));
        i = 0;
        this->sender = true;
        data_length = data.size();
    } else {
//        EV << this->getName() << "I\'m Reciever";
        sender = false;
    }
}

void Node::handleMessage(cMessage *msg) {
//    sender = this->get();

    if (msg->isSelfMessage()) {
//        if(i>=data_length){
//            return;
//        }
        try {
            MyMessage_Base *mmsg = check_and_cast<MyMessage_Base*>(msg);
            auto seqNum = mmsg->getHeader();
            auto kind = mmsg->getType();

            start_timer(seqNum % WINDOW_SIZE, this);
            if (delayed[seqNum % WINDOW_SIZE]) {
                delayed[seqNum % WINDOW_SIZE] = false;
                sendDelayed(mmsg, ERROR_DELAY + PROCESSING_TIME, "out");
                write_before_transmission(simTime().str(), this->getName()[5],
                        std::to_string(seqNum), mmsg->getPayload(),
                        mmsg->getTrailer(), '1',
                        duplicated[seqNum % WINDOW_SIZE] ? '1' : '0');
                if (duplicated[seqNum % WINDOW_SIZE]) {
                    sendDelayed(mmsg->dup(),
                            ERROR_DELAY + PROCESSING_TIME + DUPLICATION_DELAY,
                            "out");
                    write_before_transmission(
                            (simTime() + DUPLICATION_DELAY).str(),
                            this->getName()[5], std::to_string(seqNum),
                            mmsg->getPayload(), mmsg->getTrailer(), '1', '1');

                }
            } else {
                sendDelayed(mmsg, PROCESSING_TIME, "out");
                EV << "Sending" << endl;
                write_before_transmission(simTime().str(), this->getName()[5],
                        std::to_string(seqNum), mmsg->getPayload(),
                        mmsg->getTrailer(), '0',
                        duplicated[seqNum % WINDOW_SIZE] ? '1' : '0');
                if (duplicated[seqNum % WINDOW_SIZE]) {
                    sendDelayed(mmsg->dup(),
                            PROCESSING_TIME + DUPLICATION_DELAY, "out");
                    EV << "Sending Duplicate Without Delay" << endl;
                    write_before_transmission(
                            (simTime() + DUPLICATION_DELAY).str(),
                            this->getName()[5], std::to_string(seqNum),
                            mmsg->getPayload(), mmsg->getTrailer(), '0', '1');

                }
            }
            // Send message if any
            if (kind == DATA) {
                EV << nBuffered << endl;

                if (nBuffered < WINDOW_SIZE && i < data_length
                        && simTime().raw() > last_time) {
                    last_time = simTime().raw();
                    EV << "1 Mategy nab3t " << data[i].second << endl;
                    nBuffered++;
                    EV << "nBuffered = " << nBuffered << endl;
                    // TODO: Handle delays here
                    out_buf[next_frame_to_send % WINDOW_SIZE] = framing(
                            data[i].second);
                    write_reading_file_line(simTime().str(), this->getName()[5],
                            data[i].first);
                    send_frame(DATA, next_frame_to_send, frame_expected,
                            out_buf, this, data[i].first[3], data[i].first[1]);

                    inc(next_frame_to_send);
//                    EV << "Next Frame to send" << next_frame_to_send << endl;
                    i++;
                    EV << "i= " << i << endl;
                }
            }
            return;
        } catch (exception e) {
            auto kind = msg->getKind();
            // Either ack or message timeout
            if (kind == 0) {
                handle_message_timeout(msg, this);
            } else if (kind == 1) {
                handle_ack_timeout(this);
            } else {
                nBuffered++;
                EV << "nBuffered = " << nBuffered << endl;
                // TODO: Handle delays here
                out_buf[next_frame_to_send % WINDOW_SIZE] = framing(
                        data[i].second);
                write_reading_file_line(simTime().str(), this->getName()[5],
                        data[i].first);
                send_frame(DATA, next_frame_to_send, frame_expected, out_buf,
                        this, data[i].first[3]);
                inc(next_frame_to_send);
//                    EV << "Next Frame to send" << next_frame_to_send << endl;
                i++;
                EV << "i= " << i << endl;
            }
            cancelAndDelete(msg);
            return;
        }

    }
    MyMessage_Base *mmsg = check_and_cast<MyMessage_Base*>(msg);
    int kind = mmsg->getType();
    int ack_number = mmsg->getAck_number();
    int seqNum = mmsg->getHeader();
    std::string payload = mmsg->getPayload();
//    EV << mmsg->getTrailer() << endl;
    char trailer = mmsg->getTrailer();
//    EV << kind << ' ' << ack_number << ' ' << seqNum << ' ' << payload << ' '
//              << trailer << ' ' << endl;
    cancelAndDelete(msg);
    // Receiver
    if (kind == DATA) {

        if (seqNum != frame_expected && no_nak) {
            send_frame(NACK, 0, frame_expected, out_buf, this);
        } else {
//            start_ack_timer(this);
        }
        // Message check
//        EV << payload << " " << trailer << endl;
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
            while (isArrived[frame_expected % WINDOW_SIZE]) {
                std::string receivedMessage = deframing(
                        in_buf[frame_expected % WINDOW_SIZE]);
                // TODO: Do what you want with this message
                write_frame_received(receivedMessage, std::to_string(seqNum));
                no_nak = true;
                isArrived[frame_expected % WINDOW_SIZE] = false;
                inc(frame_expected);
//                EV << "Expected = " << frame_expected << endl;
//                frame_expected = (frame_expected + 1) % (MAX_SEQ+1);
//                EV << "Expected = " << frame_expected << endl;
                inc(too_far);
//                EV << "too_far = " << too_far << endl;
//                start_ack_timer(this);

            }

            if (!between(frame_expected, seqNum, too_far)) {

//                EV << "Expected = " << frame_expected << endl;
                send_frame(ACK, seqNum, frame_expected, out_buf, this);
            } else {
//                send_frame(NACK, seqNum, frame_expected, out_buf, this);
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
        stop_timer(ack_expected % WINDOW_SIZE, this);
        inc(ack_expected);
        EV << "Inside nBuffered = " << nBuffered << endl;
    }

    if (kind == ACK) {
        if (nBuffered < WINDOW_SIZE && i < data_length
                && simTime().raw() >= last_time) {
            last_time = simTime().raw();
            EV << "3 Mategy nab3t" << data[i].second << endl;
            nBuffered++;
            EV << "nBuffered = " << nBuffered << endl;
            // TODO: Handle delays here
            out_buf[next_frame_to_send % WINDOW_SIZE] = framing(data[i].second);
            write_reading_file_line(simTime().str(), this->getName()[5],
                    data[i].first);
            send_frame(DATA, next_frame_to_send, frame_expected, out_buf, this,
                    data[i].first[3]);
            inc(next_frame_to_send);
            i++;
            EV << "i= " << i << endl;
        }
    }

}
