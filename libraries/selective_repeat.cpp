#include <iostream>
#include <vector>
#include "message_type.h"
#include "crc.cpp"
#include "framing.cpp"
#include "testing_lib.h"
#define MAX_SEQ 7
#define WINDOW_SIZE 4
#define TIMEOUT_THRESHOLD 3     // seconds
#define ACK_TIMEOUT_THRESHOLD 5 // seconds

enum DATA_KIND
{
    NACK,
    ACK,
    DATA
};
int ack_expected = 0;
int next_frame_to_send = 0;
int frame_expected = 0;
int too_far = WINDOW_SIZE;
// int i = 0;
// bool sender = 0;
int nBuffered = 0;
bool ack_timer = false;
bool no_nak = true;

std::vector<bool> timer_buffer;
std::vector<bool> arrived;
std::vector<std::string> out_buf;
std::vector<std::string> in_buf;

void start_timer(int num)
{
    timer_buffer[num] = true;
    char c = char(num);
    cMessage *selfMessage = new cMessage(&c, 0);
    scheduleAt(simTime() + TIMEOUT_THRESHOLD, selfMessage);
}
void stop_timer(int num)
{
    timer_buffer[num] = -1;
}
void start_ack_timer()
{
    ack_timer = true;
    cMessage *selfMessage = new cMessage("", 1);
    scheduleAt(simTime() + ACK_TIMEOUT_THRESHOLD, selfMessage);
}
void stop_ack_timer()
{
    ack_timer = false;
}
static bool between(int a, int b, int c)
{
    return ((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a));
}
static void send_frame(DATA_KIND type, int frame_number, int frame_expected, std::vector<std::string> buffer)
{
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
    send(msg, "out");
    if (type == DATA)
    {
        start_timer(frame_number % WINDOW_SIZE);
    }
    stop_ack_timer();
}
void handle_message_timeout(cMessage *msg)
{
    int num = int(msg->getName());
    if (timer_buffer[num])
    {
        send_frame(DATA, num, frame_expected, out_buf);
    }
}
void handle_ack_timeout()
{
    send_frame(ACK, 0, frame_expected, out_buf);
}
void initialize()
{
    // Timer initialization
    timer_buffer = std::vector<bool>(WINDOW_SIZE, false);
    arrived = std::vector<bool>(WINDOW_SIZE, false);
    // if (std::strcmp(this->getName(),"Node 0")){
    //     sender = true;
    // }
}
// frame arrival
void handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        // Either ack or message timeout
        if (msg->getKind() == 0)
            handle_message_timeout(msg);
        else
            handle_ack_timeout();
        return;
    }
    // MyMessage_Base *mmsg = check_and_cast<MyMessage_Base *>(msg);
    MyMessage_Base *mmsg = new MyMessage_Base();
    int kind = mmsg->getType();
    if (kind == DATA)
    {
        int seqNum = mmsg->getHeader();
        if (seqNum != frame_expected && no_nak)
        {
            send_frame(NACK, 0, frame_expected, out_buf);
        }
        // Message check
        std::string payload = mmsg->getPayload();
        char trailer = mmsg->getTrailer();
        if (!checkMessage(payload + trailer))
        {
            if (no_nak)
            {
                send_frame(NACK, 0, frame_expected, out_buf);
                return;
            }
        }
        if (between(frame_expected, seqNum, too_far) && !arrived[seqNum % WINDOW_SIZE])
        {
            arrived[seqNum % WINDOW_SIZE] = true;
            std::string receivedFrame = binaryToMessage(
                getMessageFromEncoded(messageToBinary(payload + trailer)));
            in_buf[seqNum % WINDOW_SIZE] = receivedFrame;
            while (arrived[frame_expected % WINDOW_SIZE])
            {
                std::string receivedMessage = deframing(receivedFrame);
                // TODO: Do what you want with this message
                no_nak = true;
                arrived[frame_expected % WINDOW_SIZE] = false;
                frame_expected++;
                too_far++;
                start_ack_timer();
            }
        }
    }
    int ack_number = mmsg->getAck_number();
    if (kind == NACK && between(ack_expected, (ack_number + 1) % (MAX_SEQ + 1), next_frame_to_send))
        send_frame(DATA, (ack_number + 1) % (MAX_SEQ + 1), frame_expected, out_buf);
    while (between(ack_expected, ack_number, next_frame_to_send))
    {
        nBuffered--;
        stop_timer(ack_expected % WINDOW_SIZE);
        ack_expected++;
    }
}
int main(int argc, char const *argv[])
{

    return 0;
}
