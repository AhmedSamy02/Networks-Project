#include <vector>
#include <string>
#include <fstream>
using namespace std;
ofstream out("output_files/output.txt");
void write_reading_file_line(string processing_time, char node_id,
                             string error_code)
{
    out << "At time[" << processing_time << "], Node[" << node_id
        << "] , Introducing channel error with code= [" << error_code
        << "]." << endl;
}

void write_before_transmission(string sending_time, char node_id,
                               string seqNum, string payload, string trailer_in_bits, char delay = '0',
                               char duplicate = '0', string modified = "-1", string lost = "No",
                               string sent = "sent")
{
    out << "At time [" << sending_time << "], Node[" << node_id << "] [" << sent
        << "] frame with seq_num=[" << seqNum << "] and payload=["
        << payload << "] and trailer=[" << trailer_in_bits
        << "] , Modified [" << modified << "] , Lost[" << lost
        << "], Duplicate [" << duplicate << "], Delay [" << delay << "]."
        << endl;
}
void write_timeout_event(string timeout_time, char node_id, string seqNum)
{
    out << "Time out event at time [" << timeout_time << "], at Node["
        << node_id << "] for frame with seq_num=[" << seqNum << "]" << endl;
}
void write_control_frame(string sending_time, char node_id, bool ack_or_nack,
                         string ack_number, string loss = "No")
{
    string check = ack_or_nack ? "ACK" : "NACK";
    out << "At time[" << sending_time << "], Node[" << node_id << "] Sending ["
        << check << "] with number [" << ack_number << "] , loss [" << loss
        << "]" << endl;
}

void write_frame_received(string payload, string seqNum)
{
    out << "Uploading payload=[" << payload << "] and seq_num =[" << seqNum
        << "] to the network layer" << endl;
}
