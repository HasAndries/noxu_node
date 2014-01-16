class CommandMessage{
private:
    //---------- properties ----------
    bool freed;
protected:
public:
    //---------- properties ----------
    byte control;
    bool fromCommander;
    byte instruction;
    byte dataLength;
    byte *data;
    byte hopCount;
    byte *hops;
    byte lastHop;

    CommandMessage();
    CommandMessage(byte *buffer);
    ~CommandMessage();

    bool validate();
    byte* buildBuffer();
    void addHop(byte id);
    byte removeLastHop();

    void print(char *heading);
};