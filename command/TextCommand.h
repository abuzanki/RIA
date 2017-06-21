#ifndef TextCommand_h
#define TextCommand_h

/**
 * Command format
 * !rmmaa data\r
 */
class Stream;
class TextCommand 
{
public:
    //error definitions
    enum {
        OK                = 0,
        ErrCmdNone        = -1,
        ErrInvalidCmd     = -2,
        ErrUnknownEscape  = -3,
        ErrInternal       = -4,
        ErrNoData         = -5,
        ErrTimeout        = -6
    };
    //end error definition

    TextCommand(Stream *s, size_t sz);
    ~TextCommand();

    int available();
    void setTimeout(unsigned long ms);
    bool isWrite() const;
    uint8_t ucAddress() const;
    uint8_t ioAddress() const;
    uint8_t *data();
    uint8_t *command();
    size_t commandLen() const;

private:
    uint8_t *_cmd;
    Stream *_stream;
    size_t _cmdLen;
    size_t _cmdSize;
    uint8_t _dir, _ucAddr, _ioAddr;
    unsigned long _msTimeout;
};

#endif