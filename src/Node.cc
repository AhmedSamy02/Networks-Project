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
#include <iterator>
#include <string>
#include <sstream>

// Helper function to print vector contents
template <typename T>
std::string vectorToString(const std::vector<T> &vec)
{
    std::ostringstream oss;
    for (const auto &elem : vec)
    {
        oss << elem << " ";
    }
    return oss.str();
}

// Specialized helper for cMessage* vector
std::string cMessageVectorToString(const std::vector<cMessage *> &vec)
{
    std::ostringstream oss;
    for (const auto *msg : vec)
    {
        if (msg)
        {
            oss << msg->getName() << " "; // Assuming cMessage has a getName() method
        }
        else
        {
            oss << "null ";
        }
    }
    return oss.str();
}
enum DATA_KIND
{
    NACK,
    ACK,
    DATA
};
int TIMEOUT_THRESHOLD = 0;
int MAX_SEQ = 0;
int WINDOW_SIZE = 0;
double ERROR_DELAY = 0;
double PROCESSING_TIME = 0;
double DUPLICATION_DELAY = 0;

void inc(int &k)
{
    k = (k + 1) % (MAX_SEQ + 1);
}
void start_timer(int num, Node *node)
{
    char c = char(num);
    if (node->timer_buffer[num % WINDOW_SIZE])
    {
        EV << "the timer to stop " << num % WINDOW_SIZE << endl;
        node->cancelEvent(node->timer_buffer[num % WINDOW_SIZE]);
        delete node->timer_buffer[num % WINDOW_SIZE];
    }
    EV << "the timer to start " << num << endl;
    cMessage *selfMessage = new cMessage(&c, 0);
    node->timer_buffer[num] = selfMessage;
    EV << "timers " << cMessageVectorToString(node->timer_buffer) << endl;
    node->scheduleAt(simTime() + TIMEOUT_THRESHOLD, selfMessage);
}
void stop_timer(int num, Node *node)
{
    num = num % WINDOW_SIZE;
    cMessage *timer = node->timer_buffer[num];
    EV << "timer is to stop vale" << timer->getName() << endl;
    EV << "the timer stopped " << num << endl;
    if (node->timer_buffer[num])
    {
        node->cancelEvent(timer);
        delete timer;
        node->timer_buffer[num] = nullptr;
    }
}
void start_ack_timer(Node *node)
{

    cMessage *selfMessage = new cMessage("", 1);
    node->ack_timer = selfMessage;
    node->scheduleAt(simTime() + TIMEOUT_THRESHOLD, selfMessage);
}
void stop_ack_timer(Node *node)
{
    node->cancelEvent(node->ack_timer);
}
bool between(int a, int b, int c)
{
    return ((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a));
}
void reset_buffers_for_seq(int num, Node *node)
{
    num = num % WINDOW_SIZE;
    node->duplicated[num] = false;
    node->delayed[num] = false;
    node->lossed[num] = false;
}
void send_frame(DATA_KIND type, int frame_number, int frame_expected,
                std::vector<std::string> buffer, Node *node, char delay = '0',
                char duplicate = '0', char loss = '0')
{
    reset_buffers_for_seq(frame_number, node);
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
    // EV << msg->getType() << ' ' << msg->getAck_number() << ' '
    //    << msg->getHeader() << ' ' << msg->getPayload() << ' '
    //    << msg->getTrailer() << ' ' << endl;
    if (type == NACK) // NACK
    {
        node->no_nak = false;
        write_control_frame((simTime()).str(), id, NACK,
                            std::to_string(frame_expected));
        node->send(msg, "out");
        EV << "nack" << frame_expected << ' ' << endl;
    }
    else if (type == ACK)
    {
        write_control_frame(simTime().str(), id, ACK,
                            std::to_string(frame_expected));
        node->send(msg, "out");
        EV << "ack" << frame_expected << endl;
    }
    else
    {
        if (delay == '1')
        {
            node->delayed[frame_number % WINDOW_SIZE] = true;
        }
        if (duplicate == '1')
        {
            node->duplicated[frame_number % WINDOW_SIZE] = true;
            EV << "duplicate " << vectorToString(node->duplicated) << endl;
            EV << "Is duplicated = "
               << node->duplicated[frame_number % WINDOW_SIZE] << endl;
        }
        EV << msg->getPayload() << endl;
        EV << "loss " << loss << endl;
        if (loss == '1')
        {
            node->lossed[frame_number % WINDOW_SIZE] = true;
        }
        node->scheduleAt(simTime() + PROCESSING_TIME, msg);
    }
    //    stop_ack_timer(node);
}
void handle_message_timeout(cMessage *msg, Node *node)
{
    int num = int(msg->getName()[0]);
    EV << "Number in timer = " << num;
    if (node->timer_buffer[num])
    {
        send_frame(DATA, num, node->frame_expected, node->out_buf, node);
    }
}
void handle_ack_timeout(Node *node)
{
    send_frame(ACK, 0, node->frame_expected, node->out_buf, node);
}

Define_Module(Node);

void Node::initialize()
{
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
    timer_buffer = std::vector<cMessage *>(WINDOW_SIZE, nullptr);
    isArrived = std::vector<bool>(WINDOW_SIZE, false);
    delayed = std::vector<bool>(WINDOW_SIZE, false);
    duplicated = std::vector<bool>(WINDOW_SIZE, false);
    lossed = std::vector<bool>(WINDOW_SIZE, false);
    out_buf = std::vector<std::string>(WINDOW_SIZE);
    in_buf = std::vector<std::string>(WINDOW_SIZE);

    if (this->getName()[5] == '0')
    {                        // TODO handle who is the receiver
        auto delay_time = 0; // must be given from the coordinator for sender to start sending ya yara
        data = read_file(0);
        scheduleAt(simTime() + delay_time, new cMessage("", 2));
        i = 0;
        this->sender = true;
        data_length = data.size();
    }
    else
    {
        sender = false;
    }
}

void Node::handleMessage(cMessage *msg)
{

    if (msg->isSelfMessage())
    {
        try
        {
            MyMessage_Base *mmsg = check_and_cast<MyMessage_Base *>(msg);
            auto seqNum = mmsg->getHeader();
            auto kind = mmsg->getType();

            start_timer(seqNum % WINDOW_SIZE, this);
            if (!lossed[seqNum % WINDOW_SIZE])
            { // loss packet after starting timer

                if (delayed[seqNum % WINDOW_SIZE])
                {
                    EV << "Sending delay " << mmsg->getPayload() << endl;
                    EV << "duliation " << vectorToString(duplicated) << endl;
                    if (duplicated[seqNum % WINDOW_SIZE])
                    {
                        EV << "sending delay duplicate " << mmsg->getPayload() << endl;
                        sendDelayed(mmsg->dup(),
                                    ERROR_DELAY + PROCESSING_TIME + DUPLICATION_DELAY,
                                    "out");
                        write_before_transmission(
                            (simTime() + DUPLICATION_DELAY).str(),
                            this->getName()[5], std::to_string(seqNum),
                            mmsg->getPayload(), mmsg->getTrailer(), '1', '1');
                    }
                    delayed[seqNum % WINDOW_SIZE] = false;
                    sendDelayed(mmsg, ERROR_DELAY + PROCESSING_TIME, "out");
                    write_before_transmission(simTime().str(), this->getName()[5],
                                              std::to_string(seqNum), mmsg->getPayload(),
                                              mmsg->getTrailer(), '1',
                                              duplicated[seqNum % WINDOW_SIZE] ? '1' : '0');
                }
                else
                {
                    sendDelayed(mmsg, PROCESSING_TIME, "out");
                    EV << "Sending no delay" << mmsg->getPayload() << endl;
                    write_before_transmission(simTime().str(), this->getName()[5],
                                              std::to_string(seqNum), mmsg->getPayload(),
                                              mmsg->getTrailer(), '0',
                                              duplicated[seqNum % WINDOW_SIZE] ? '1' : '0');
                    if (duplicated[seqNum % WINDOW_SIZE])
                    {
                        sendDelayed(mmsg->dup(),
                                    PROCESSING_TIME + DUPLICATION_DELAY, "out");
                        EV << "Sending Duplicate Without Delay" << endl;
                        write_before_transmission(
                            (simTime() + DUPLICATION_DELAY).str(),
                            this->getName()[5], std::to_string(seqNum),
                            mmsg->getPayload(), mmsg->getTrailer(), '0', '1');
                    }
                }
            }
            // Send message if any
            if (kind == DATA)
            {
                EV << nBuffered << endl;

                if (nBuffered < WINDOW_SIZE && i < data_length && simTime().raw() > last_time)
                {
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
                               out_buf, this, data[i].first[3], data[i].first[2], data[i].first[1]);
                    EV << "data " << data[i].first << endl;
                    EV << "data first " << data[i].first[1] << endl;

                    inc(next_frame_to_send);
                    i++;
                    EV << "i= " << i << endl;
                }
            }
            return;
        }
        catch (exception e)
        {
            auto kind = msg->getKind();
            // Either ack or message timeout
            if (kind == 0)
            {
                handle_message_timeout(msg, this);
            }
            else if (kind == 1)
            {
                handle_ack_timeout(this);
            }
            else
            {
                nBuffered++;
                EV << "nBuffered = " << nBuffered << endl;
                // TODO: Handle delays here
                out_buf[next_frame_to_send % WINDOW_SIZE] = framing(
                    data[i].second);
                write_reading_file_line(simTime().str(), this->getName()[5],
                                        data[i].first);
                send_frame(DATA, next_frame_to_send, frame_expected, out_buf,
                           this, data[i].first[3], data[i].first[2], data[i].first[1]);
                inc(next_frame_to_send);
                i++;
                EV << "i= " << i << endl;
            }
            cancelAndDelete(msg);
            return;
        }
    }
    MyMessage_Base *mmsg = check_and_cast<MyMessage_Base *>(msg);
    int kind = mmsg->getType();
    int ack_number = mmsg->getAck_number();
    int seqNum = mmsg->getHeader();
    std::string payload = mmsg->getPayload();
    char trailer = mmsg->getTrailer();
    cancelAndDelete(msg);
    // Receiver
    if (kind == DATA)
    {
        EV << "data received" << payload << endl;
        if (seqNum > frame_expected && no_nak)
        {
            EV << "frame expexcted " << frame_expected << "seq " << seqNum << "no ack " << no_nak << endl;
            send_frame(NACK, 0, frame_expected, out_buf, this);
        }
        else
        {
        }

        if (!checkMessage(messageToBinary(payload), trailer))
        {

            if (no_nak)
            {
                send_frame(NACK, 0, frame_expected, out_buf, this);
                return;
            }
        }
        if (between(frame_expected, seqNum, too_far) && !isArrived[seqNum % WINDOW_SIZE])
        {
            std::string receivedFrame = binaryToMessage(
                getMessageFromEncoded(messageToBinary(payload + trailer)));
            isArrived[seqNum % WINDOW_SIZE] = true;
            in_buf[seqNum % WINDOW_SIZE] = receivedFrame;
            while (isArrived[frame_expected % WINDOW_SIZE])
            {
                std::string receivedMessage = deframing(
                    in_buf[frame_expected % WINDOW_SIZE]);
                // TODO: Do what you want with this message
                write_frame_received(receivedMessage, std::to_string(seqNum));
                no_nak = true;
                isArrived[frame_expected % WINDOW_SIZE] = false;
                inc(frame_expected);

                inc(too_far);
            }

            if (!between(frame_expected, seqNum, too_far))
            {

                send_frame(ACK, seqNum, frame_expected, out_buf, this);
            }
            else
            {
                //                send_frame(NACK, seqNum, frame_expected, out_buf, this);
            }
        }
    }

    if (kind == NACK && between(ack_expected, ack_number,
                                next_frame_to_send))
    {
        EV << "not ack send frame" << endl;
        send_frame(DATA, ack_number, frame_expected,
                   out_buf, this);
    }
    EV << "timer arr = " << cMessageVectorToString(timer_buffer) << endl;
    while (between(ack_expected, ack_number - 1, next_frame_to_send)) // sender
    {
        EV << "in while " << ack_expected << " " << ack_number << next_frame_to_send << endl;
        nBuffered--;
        stop_timer(ack_expected % WINDOW_SIZE, this);
        inc(ack_expected);
        EV << "Inside nBuffered = " << nBuffered << endl;
    }
    if (kind == ACK)
    {
        EV << "ACK recieved " << ack_number << endl;
        if (nBuffered < WINDOW_SIZE && i < data_length && simTime().raw() >= last_time)
        {
            last_time = simTime().raw();
            EV << "3 Mategy nab3t" << data[i].second << endl;
            nBuffered++;
            EV << "nBuffered = " << nBuffered << endl;
            EV << "buffered out = " << vectorToString(out_buf) << endl;
            EV << "buffered in = " << vectorToString(in_buf) << endl;
            EV << "timer arr = " << cMessageVectorToString(timer_buffer) << endl;
            EV << "delayed = " << vectorToString(delayed) << endl;
            EV << "duplicated = " << vectorToString(duplicated) << endl;
            EV << "isArrived = " << vectorToString(isArrived) << endl;
            // TODO: Handle delays here
            out_buf[next_frame_to_send % WINDOW_SIZE] = framing(data[i].second);
            write_reading_file_line(simTime().str(), this->getName()[5],
                                    data[i].first);
            send_frame(DATA, next_frame_to_send, frame_expected, out_buf, this,
                       data[i].first[3], data[i].first[2], data[i].first[1]);
            inc(next_frame_to_send);
            i++;
            EV << "i= " << i << endl;
        }
    }
}
