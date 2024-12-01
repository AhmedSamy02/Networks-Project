#include <string>
#include "testing_lib.h"
class MyMessage_Base
{

    int header;
    std::string payload;
    char trailer;
    short type;
    int ack_number;

public:
    MyMessage_Base() {

    };
    int getHeader() const;
    void setHeader(int header);

    const char *getPayload() const;
    void setPayload(const char *payload);

    char getTrailer() const;
    void setTrailer(char trailer);

    short getType() const;
    void setType(short type);

    int getAck_number() const;
    void setAck_number(int ack_number);
    bool isSelfMessage()
    {
        return true;
    }
};