class cMessage
{
    const char *message;
    short int kind;

public:
    cMessage(const char *msg, short int k)
    {
        message = msg;
        kind = k;
    }
    short getKind(){
        return kind;
    }
    const char* getName(){
        return message;
    }
    bool isSelfMessage(){
        return true;
    }
};
unsigned long long simTime();
void scheduleAt(unsigned long long time, cMessage *msg);
void send(MyMessage_Base *msg, const char *port);