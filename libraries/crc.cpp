#include <iostream>
#include <bitset>
#include <string.h>
using namespace std;
const string GENERATOR = "10101010";
const int GENERATOR_LENGTH = 8;
string messageToBinary(string const message)
{
    string binaryMessage = "";
    for (auto letter : message)
    {
        binaryMessage += bitset<8>(letter).to_string();
    }
    return binaryMessage;
}
string binaryToMessage(string const binary)
{
    string message = "";
    int binaryLength = binary.length();
    for (int i = 0; i < binaryLength; i += 8)
    {
        message += char(bitset<8>(binary.substr(i, 8)).to_ulong());
    }
    return message;
}
string computeCRC(string const message, bool sender = true)
{
    cout << "Message in compute = " << binaryToMessage(message) << endl;
    int m = message.length();
    string encodedMessage = message;
    if (sender)
    {
        for (int i = 0; i < GENERATOR_LENGTH - 1; i++)
        {
            encodedMessage += '0';
        }
    }


    int newMessageLength = encodedMessage.length();
    for (int i = 0; i <= newMessageLength - GENERATOR_LENGTH;)
    {
        for (int j = 0; j < GENERATOR_LENGTH; j++)
            // 0 xor 0 = 0 and also 1 xor 1 = 0 otherwise equals one
            encodedMessage[i + j] = encodedMessage[i + j] == GENERATOR[j] ? '0' : '1';
        for (; i < newMessageLength && encodedMessage[i] != '1'; i++)
            ;
    }
    // cout << encodedMessage << endl;
    string remainder = encodedMessage.substr(newMessageLength - GENERATOR_LENGTH + 1);
    cout << remainder << endl;
    return remainder;
}
// For Sender
string encodeMessage(string const message)
{

    return message + computeCRC(message);
}
// For Receiver
bool checkMessage(string const message, char trailer)
{
    cout << "Message = " << message << endl;
    string computedRemainder = "0" + computeCRC(message, true);
    string temp = bitset<8>(trailer).to_string();
    cout << "Computer Remainder  = " << computedRemainder << endl;
    cout << "Temp = " << temp << endl;

    if (strcmp(computedRemainder.c_str(), temp.c_str())==0)
    {
        return true;
    }
    // for (auto letter : computedRemainder)
    // {
    //     if (letter != '0')
    //     {
    //         return false;
    //     }
    // }
    return false;
}
string getMessageFromEncoded(string const encodedMessage)
{
    return encodedMessage.substr(0, encodedMessage.length() - GENERATOR_LENGTH + 1);
}

int main()
{
    string s="$A flower, sometimes $";
    // cin >> s;
    s = messageToBinary(s);
    cout << s << endl;
    // Sender
    string crc = computeCRC(s);
    cout << binaryToMessage(crc) << endl;
    // Adding random error
    // int randomLocation = rand() % encodedMessage.length();
    // encodedMessage[randomLocation] = encodedMessage[randomLocation] == '0' ? '1' : '0';
    // Receiver
    if (!checkMessage(s,binaryToMessage( crc)[0]))
    {
        cout << "An error ocurred in message";
    }
    else
    {
        cout << "The message is received correctly" << endl;
        // cout << binaryToMessage(getMessageFromEncoded(encodedMessage));
    }
    return 0;
}
