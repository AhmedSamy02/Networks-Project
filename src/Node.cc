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
#include <omnetpp.h>

// Helper function to print vector contents
int addRandomError(string &message)
{
    int randomLocation = rand() % message.length();
    message[randomLocation] = message[randomLocation] == '0' ? '1' : '0';
    return randomLocation;
}
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
    cout << "Message hi 125" << endl;

    std::ostringstream oss;
    for (const auto *msg : vec)
    {
        cout << "Message hi 111" << endl;
        try
        {
            if (msg)
            {
                oss << msg->getName() << " "; // Assuming cMessage has a getName() method
            }
            else
            {
                oss << "null ";
                cout << "NULL" << endl;
            }
        }
        catch (std::exception &e)
        {
            std::cout << e.what() << std::endl;
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
void start_timer(int num, Node *node, int iterator)
{
    cout << "Message hi 10" << endl;

    char c = char(num);
    cout << "c number" << num << endl;
    // cout << cMessageVectorToString(node->timer_buffer) << endl;
    auto m = node->timer_buffer[num % WINDOW_SIZE];
    cout << m << endl;
    if (node->timer_buffer[num % WINDOW_SIZE])
    {
        cout << "in if" << endl;
        EV << "the timer to stop " << num % WINDOW_SIZE << endl;
        EV << "cancel 3" << endl;

        node->cancelEvent(node->timer_buffer[num % WINDOW_SIZE]);
        cout << "in in if" << endl;
        delete node->timer_buffer[num % WINDOW_SIZE];
    }
    cout << "out if" << endl;
    EV << "the timer to start " << num << endl;
    cMessage *selfMessage = new cMessage(&c, 0);
    node->timer_buffer[num % WINDOW_SIZE] = selfMessage;
    EV << "timers " << cMessageVectorToString(node->timer_buffer) << endl;
    node->scheduleAt(simTime() + TIMEOUT_THRESHOLD + iterator * PROCESSING_TIME, selfMessage);
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
    node->manipulation[num] = false;
}

void send_frame(DATA_KIND type, int frame_number, int frame_expected,
                std::vector<std::string> buffer, Node *node, char delay = '0',
                char duplicate = '0', char loss = '0', char manipulation = '0', int iterator = 1)
{
    int modified = -1;
    reset_buffers_for_seq(frame_number, node);
    MyMessage_Base *msg = new MyMessage_Base();
    auto id = node->getName()[5];
    msg->setType(type);
    if (type == DATA) // Data
    {
        std::string frame = buffer[frame_number % WINDOW_SIZE];
        EV << "get from out buff" << endl;
        auto crc = computeCRC(messageToBinary(frame));
        msg->setTrailer(binaryToMessage(crc)[0]);
        if (manipulation == '1')
        {
            EV << "Manipulation" << endl;
            modified = addRandomError(frame);
        }
        msg->setPayload(frame.c_str());
    }
    msg->setHeader(frame_number);
    msg->setAck_number(frame_expected);
    // EV << msg->getType() << ' ' << msg->getAck_number() << ' '
    //    << msg->getHeader() << ' ' << msg->getPayload() << ' '
    //    << msg->getTrailer() << ' ' << endl;

    // double probability = ((double) rand() / (RAND_MAX));
    double probability = 1;

    if (type == NACK) // NACK
    {
        node->no_nak = false;
        // if (probability > 0.5) {

        write_control_frame((simTime() + PROCESSING_TIME).str(), id, NACK,
                            std::to_string(frame_expected));

        node->sendDelayed(msg, PROCESSING_TIME, "out");
        // } else {
        //     write_control_frame((simTime()).str(), id, NACK,
        //             std::to_string(frame_expected), "Yes");
        // }

        //        EV << "nack" << frame_expected << ' ' << endl;
    }
    else if (type == ACK)
    {

        // if (probability > 0.5) {

        write_control_frame((simTime() + PROCESSING_TIME).str(), id, ACK,
                            std::to_string(frame_expected));

        node->sendDelayed(msg, PROCESSING_TIME, "out");
        // } else {
        //     write_control_frame((simTime()).str(), id, ACK,
        //             std::to_string(frame_expected), "Yes");
        // }

        //        EV << "ack" << frame_expected << endl;
        node->ack_time = simTime().raw() / 1000.0;
    }
    // if (type == NACK) // NACK
    // {
    //     node->no_nak = false;
    //     write_control_frame((simTime() + PROCESSING_TIME).str(), id, NACK,
    //                         std::to_string(frame_expected));
    //     if (probability > 0.5)
    //         node->sendDelayed(msg, PROCESSING_TIME, "out");

    //     EV << "nack" << frame_expected << ' ' << endl;
    // }
    // else if (type == ACK)
    // {
    //     auto ayhaga = 0.0;
    //     EV << "ack_time" << node->ack_time << endl;
    //     EV << "SIM TIM: " << simTime().raw() / 1000.0 << endl;
    //     EV << "before" << simTime() + PROCESSING_TIME + ayhaga << endl;

    //     if ((simTime().raw() / 1000.0) > node->ack_time && (simTime().raw() / 1000.0) < node->ack_time + 0.5)
    //     {
    //         // EV << "before" << simTime() + PROCESSING_TIME + ayhaga << endl;
    //         EV << "ayhaga" << endl;
    //         ayhaga += 0.4;
    //     }
    //     EV << "after" << simTime() + PROCESSING_TIME + ayhaga << endl;

    //     write_control_frame((simTime() + PROCESSING_TIME + ayhaga).str(), id, ACK,
    //                         std::to_string(frame_expected));
    //     if (probability > 0.5)
    //         node->sendDelayed(msg, PROCESSING_TIME + ayhaga, "out");
    //     EV << "ack" << frame_expected << endl;
    //     node->ack_time = simTime().raw() / 1000.0;
    // }
    else
    {
        auto time = PROCESSING_TIME * iterator;
        std::string res = "";
        res += manipulation;
        res += loss;
        res += duplicate;
        res += delay;

        start_timer(frame_number, node, iterator - 1);

        if (loss == '1')
        {
            EV << "loss" << endl;

            write_before_transmission(
                (simTime() + time).str(),
                node->getName()[5], std::to_string(frame_number),
                msg->getPayload(), messageToBinary(std::string(1, msg->getTrailer())),
                delay ? '1' : '0', duplicate ? '1' : '0', std::to_string(modified), loss ? "Yes" : "No");

            return;
        }
        EV << "time" << time << endl;
        EV << "sim" << simTime() << endl;
        if (delay == '1')
        {
            EV << "delay" << endl;
            time += ERROR_DELAY;
        }
        if (duplicate == '1')
        {
            EV << "Dulpication" << endl;
            node->sendDelayed(msg->dup(), time + DUPLICATION_DELAY, "out");
        }
        if (node->nacked)
        {
            node->nacked = false;
            time += 0.001;
        }
        write_before_transmission(
            (simTime() + PROCESSING_TIME * iterator).str(),
            node->getName()[5], std::to_string(frame_number),
            msg->getPayload(), messageToBinary(std::string(1, msg->getTrailer())),
            delay, duplicate, std::to_string(modified), loss == '1' ? "Yes" : "No");

        if (duplicate == '1')
        {
            write_before_transmission(
                (simTime() + PROCESSING_TIME * iterator + DUPLICATION_DELAY).str(),
                node->getName()[5], std::to_string(frame_number),
                msg->getPayload(), messageToBinary(std::string(1, msg->getTrailer())),
                delay ? '1' : '0', duplicate ? '1' : '0', std::to_string(modified), loss ? "Yes" : "No");
        }
        node->sendDelayed(msg, time, "out");
        EV << msg->getType() << ' ' << msg->getAck_number() << ' '
           << msg->getHeader() << ' ' << msg->getPayload() << ' '
           << msg->getTrailer() << ' ' << endl;
    }
}
void handle_message_timeout(int num, Node *node)
{
    write_timeout_event(simTime().str(), node->getName()[5], std::to_string(num));
    EV << "Number in timer = " << num;
    if (node->timer_buffer[num % WINDOW_SIZE])
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
    manipulation = std::vector<bool>(WINDOW_SIZE, false);
    out_buf = std::vector<std::string>(WINDOW_SIZE);
    in_buf = std::vector<std::string>(WINDOW_SIZE);
    sender = false;
    ack_time = -1;
}

void Node::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        auto kind = msg->getKind();
        if (kind == 0)
        {
            handle_message_timeout(int(msg->getName()[0]), this);
            // stop_timer(int(msg->getName()[0]), this);
        }
        else if (kind == 1)
        {
            handle_ack_timeout(this);
            cancelAndDelete(msg);
        }
        return;
        // }/
    }
    MyMessage_Base *mmsg = check_and_cast<MyMessage_Base *>(msg);
    int kind = mmsg->getType();
    int ack_number = mmsg->getAck_number();
    int seqNum = mmsg->getHeader();
    std::string payload = mmsg->getPayload();
    char trailer = mmsg->getTrailer();
    EV << "cancel 2" << endl;
    cancelAndDelete(msg);
    cout << "cancel 3" << endl;

    if (payload == "Start sending")
    {
        data = read_file(this->getName()[5] == '0' ? false : true);
        i = 0;
        this->sender = true;
        data_length = data.size();
        EV << nBuffered << endl;
        while (nBuffered < WINDOW_SIZE)
        {
            auto mss = framing(data[i].second);
            out_buf[next_frame_to_send % WINDOW_SIZE] = mss;
            inc(next_frame_to_send);

            MyMessage_Base *newmsg = new MyMessage_Base();
            auto seqNum = i % (MAX_SEQ + 1);
            cout << "Message hi3" << endl;
            newmsg->setType(DATA);
            out_buf[seqNum % WINDOW_SIZE] = mss;
            auto crc = computeCRC(messageToBinary(mss));
            newmsg->setTrailer(binaryToMessage(crc)[0]);
            newmsg->setHeader(seqNum);
            newmsg->setAck_number(frame_expected);
            nBuffered++;
            auto time = PROCESSING_TIME * (i + 1);
            EV << "TIME: " << time << endl;
            cMessage *selfMessage = new cMessage(std::to_string(seqNum).c_str(), 0);
            timer_buffer[seqNum % WINDOW_SIZE] = selfMessage;
            scheduleAt(simTime() + time + TIMEOUT_THRESHOLD, selfMessage);

            int modified = -1;
            bool lost = false;
            bool delayed = false;
            bool duplicated = false;

            if (data[i].first[1] != '1')
            {
                EV << "not loss" << endl;
                if (data[i].first[0] == '1')
                {
                    EV << "Manipulation" << endl;

                    modified = addRandomError(mss);
                }
                newmsg->setPayload(mss.c_str());

                if (data[i].first[3] == '1')
                {
                    EV << "delay" << endl;
                    time += ERROR_DELAY;
                    delayed = true;
                }
                if (data[i].first[2] == '1')
                {
                    EV << "Dulpication" << endl;
                    duplicated = true;
                    sendDelayed(newmsg->dup(), time + DUPLICATION_DELAY, "out");
                }

                sendDelayed(newmsg, time, "out");
                write_reading_file_line((simTime() + PROCESSING_TIME * i).str(), this->getName()[5], data[i].first);
                write_before_transmission(
                    (simTime() + PROCESSING_TIME * (i + 1)).str(),
                    this->getName()[5], std::to_string(seqNum),
                    newmsg->getPayload(), messageToBinary(std::string(1, newmsg->getTrailer())),
                    delayed ? '1' : '0', duplicated ? '1' : '0', std::to_string(modified), lost ? "Yes" : "No");

                EV << "sending" << endl;
                EV << nBuffered << endl;
                if (data[i].first[2] == '1')
                {
                    write_before_transmission(
                        (simTime() + DUPLICATION_DELAY + PROCESSING_TIME * (i + 1)).str(),
                        this->getName()[5], std::to_string(seqNum),
                        newmsg->getPayload(), messageToBinary(std::string(1, newmsg->getTrailer())),
                        delayed ? '1' : '0', '1', std::to_string(modified), lost ? "Yes" : "No");
                }
            }
            else
            {
                write_reading_file_line((simTime() + PROCESSING_TIME * i).str(), this->getName()[5],
                                        data[i].first);
                write_before_transmission(
                    (simTime() + time).str(),
                    this->getName()[5], std::to_string(seqNum),
                    newmsg->getPayload(), messageToBinary(std::string(1, newmsg->getTrailer())),
                    delayed ? '1' : '0', duplicated ? '1' : '0', std::to_string(modified), lost ? "Yes" : "No");
            }
            i++;
        }
        return;
    }
    // Receiver
    if (kind == DATA)
    {
        EV << "data received" << payload << endl;
        EV << "frame expexcted " << frame_expected << endl;
        EV << "seq num " << seqNum << endl;
        EV << "too_far " << too_far << endl;
        if (!checkMessage(messageToBinary(payload), trailer))
        {
            // return;

            // if (no_nak)
            // {
            //     send_frame(NACK, 0, frame_expected, out_buf, this);
            //     return;
            // }
        }
        // if (between(frame_expected, seqNum, too_far) && isArrived[seqNum % WINDOW_SIZE])
        // {
        //     EV << "received" << endl;
        //     send_frame(ACK, 0, frame_expected, out_buf, this);
        // }
        EV << " the vheck " << checkMessage(messageToBinary(payload), trailer) << endl;
        EV << "message " << payload << endl;
        EV << ack_number << ' '
           << seqNum << ' ' << payload << ' '
           << trailer << ' ' << endl;
        if (between(frame_expected, seqNum, too_far) && !isArrived[seqNum % WINDOW_SIZE] && checkMessage(messageToBinary(payload), trailer))
        {
            EV << "in between " << endl;
            std::string receivedFrame = binaryToMessage(
                getMessageFromEncoded(messageToBinary(payload + trailer)));
            isArrived[seqNum % WINDOW_SIZE] = true;
            in_buf[seqNum % WINDOW_SIZE] = receivedFrame;
            while (isArrived[frame_expected % WINDOW_SIZE])
            {
                std::string receivedMessage = deframing(
                    in_buf[frame_expected % WINDOW_SIZE]);
                // TODO: Do what you want with this message
                write_frame_received(receivedMessage, std::to_string(frame_expected));
                no_nak = true;
                isArrived[frame_expected % WINDOW_SIZE] = false;
                inc(frame_expected);

                inc(too_far);
            }

            if (!between(frame_expected, seqNum, too_far))
            {
                EV << "send not between " << frame_expected << " and " << endl;
                send_frame(ACK, seqNum, frame_expected, out_buf, this);
            }
            else
            {
                EV << "send between " << frame_expected << " and " << endl;
                if (no_nak)
                {
                    no_nak = false;
                    send_frame(NACK, seqNum, frame_expected, out_buf, this);
                }
                else
                {
                    send_frame(ACK, seqNum, frame_expected, out_buf, this);
                }
            }
        }
    }

    if (kind == NACK && between(ack_expected, ack_number,
                                next_frame_to_send))
    {
        EV << "not ack send frame" << endl;
        nacked = true;
        send_frame(DATA, ack_number, frame_expected,
                   out_buf, this);
    }

    // EV << "timer arr = " << cMessageVectorToString(timer_buffer) << endl;
    cout << "becore" << endl;
    while (between(ack_expected, (ack_number - 1) % (MAX_SEQ + 1), next_frame_to_send)) // sender
    {
        EV << "kind " << kind << endl;
        EV << "in while " << ack_expected << " " << ack_number << next_frame_to_send << endl;
        nBuffered--;
        stop_timer(ack_expected % WINDOW_SIZE, this);
        inc(ack_expected);
        EV << "Inside nBuffered = " << nBuffered << endl;
    }

    cout << "after" << endl;
    if (kind == ACK)
    {
        EV << "kind " << kind << endl;
        EV << "ACK recieved " << ack_number << endl;
        int temp = 1;
        if (i == data_length && ack_number != next_frame_to_send)
        {
            send_frame(DATA, ack_number, frame_expected, out_buf, this);
        }
        while (nBuffered < WINDOW_SIZE && i < data_length && simTime().raw() >= last_time)
        {
            last_time = simTime().raw();
            EV << "3 Mategy nab3t  " << data[i].second << endl;
            nBuffered++;
            EV << "nBuffered = " << nBuffered << endl;
            EV << "buffered out = " << vectorToString(out_buf) << endl;
            EV << "buffered in = " << vectorToString(in_buf) << endl;
            EV << "timer arr = " << cMessageVectorToString(timer_buffer) << endl;
            EV << "isArrived = " << vectorToString(isArrived) << endl;
            // TODO: Handle delays here
            out_buf[next_frame_to_send % WINDOW_SIZE] = framing(data[i].second);
            write_reading_file_line((simTime() + PROCESSING_TIME * (temp - 1)).str(), this->getName()[5], data[i].first);
            send_frame(DATA, next_frame_to_send, frame_expected, out_buf, this,
                       data[i].first[3], data[i].first[2], data[i].first[1], data[i].first[0], temp);
            inc(next_frame_to_send);
            i++;
            temp++;
            EV << "i= " << i << endl;
        }
    }
}
