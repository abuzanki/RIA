#include <Stream.h>
#include <Arduino.h>
#include <limits.h>
#include "TextCommand.h"

//calculate interval between two values
static inline interval(unsigned long s, unsigned long e)
{
    if (s > e) {
        return (ULONG_MAX - s) + e;
    }
    return (e - s);
}
static inline uint8_t hex2Num(char ch) 
{
    /*
    switch(ch) {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'a':
    case 'A':
        return 10;
    case 'b':
    case 'B':
        return 11;
    case 'c':
    case 'C':
        return 12;
    case 'd':
    case 'D':
        return 13;
    case 'e':
    case 'E':
        return 14;
    case 'f':
    case 'F':
        return 15;
    default:
        return 0;
    }
    */
    if (ch >= '0' && ch <= '9')
        return (ch - '0');
    else if (ch >= 'a' && ch <= 'f')
        return ((ch - 'a') + 10);
    else if (ch >= 'A' && ch <= 'F')
        return ((ch - 'A') + 10);
    return 0x0F;
}
static inline uint8_t hex2Byte(uint8_t *p)
{
    uint8_t hi = hex2Num(p[0]);
    uint8_t lo = hex2Num(p[1]);
    return ((hi << 4) | lo);
}

TextCommand::TextCommand(Stream *s, size_t sz)
    : _stream(s), _cmdSize(sz), _cmdLen(0), _msTimeout(1000)
 {
    _ucAddr = 0xFF;
    _ioAddr = 0xFF;
    _cmd = (uint8_t *)malloc(sizeof(uint8_t)*sz);
    memset(_cmd, 0, sz);
 }
TextCommand::~TextCommand()
{
    free(_cmd);
}

void TextCommand::setTimeout(unsigned long ms)
{
    _msTimeout = ms;
}

bool TextCommand::isWrite() const {
    if (_cmdSize > 2)
        return (_cmd[1] == 'w') || (_cmd[1] == 'W');
    return false;
}

uint8_t TextCommand::ucAddress() const
{
    return _ucAddr;
}
uint8_t TextCommand::ioAddress() const
{
    return _ioAddr;
}
uint8_t *TextCommand::data()
{
    //!mmaa data
    if (_cmdSize > 7)
        return (uint8_t *)(_cmd + 7);
    return NULL;
}
uint8_t *TextCommand::command()
{
    return _cmd;
}

size_t TextCommand::commandLen() const 
{
    return _cmdLen;
}

/**
 * Command format
 * !rmmaa data\r
 */
int TextCommand::available()
{
    //error
    if (_cmdSize < 7 || _cmd == NULL) 
    {
        return ErrInternal;
    }


    //state
    const uint8_t WaitStartCmd  = 0;
    const uint8_t WaitRW        = 1;
    const uint8_t WaitUCAddr    = 2;
    const uint8_t WaitIOAddr    = 3;
    const uint8_t WaitSpace     = 4;
    const uint8_t WaitEscape    = 5;
    const uint8_t WaitEndCmd    = 6;

    //default state
    _ucAddr = 0xFF;
    _ioAddr = 0xFF;
    *_cmd = '\0';

    //
    uint8_t addr;
    //int cap = cmdSize;
    bool done = false;
    uint8_t *buf = _cmd;
    uint8_t *end = (uint8_t *)(_cmd + _cmdSize-1);
    uint8_t state = WaitStartCmd;
    unsigned long elapsed, start = millis();
    while (buf != end && !done) {
        //Timeout check
        elapsed = interval(start, millis());
        if (elapsed >= _msTimeout)
            return (buf == _cmd ? ErrCmdNone : ErrTimeout);  //timeout
        
        //check available stream
        int n = _stream->available();
        while(!done && n > 0 && buf != end) {
            n--;
            int ch = _stream->read();

            //check current parser state
            switch(state) {
            case WaitStartCmd:
                if (ch == '!') {
                    *buf = ch;
                    buf++;
                    state = WaitRW;
                } else {
                    //ignore any character before 
                    //start command marker '!'
                }
                break;
            case WaitRW:
                if (ch == 'r' || ch == 'w' || ch == 'R' || ch == 'W') {
                    *buf = ch;
                    buf++;
                    addr = 0;
                    state = WaitUCAddr;
                } else {
                    return ErrInvalidCmd;  //invalid command
                }
                break;
            case WaitUCAddr:
                if (isHexadecimalDigit(ch)) {
                    *buf = ch;
                    buf++;
                    addr++;
                    if (addr == 2) {
                        addr = 0;
                        state = WaitIOAddr;
                    }
                } else {
                    return ErrInvalidCmd;  //invalid command
                }
                break;
            case WaitIOAddr:
                if (isHexadecimalDigit(ch)) {
                    *buf = ch;
                    buf++;
                    addr++;
                    if (addr == 2) {
                        addr = 0;
                        state = WaitSpace;
                    }
                } else {
                    return ErrInvalidCmd;  //invalid command
                }
                break;
            case WaitSpace:
                if (!isWrite() && ch == '\r') {
                    done = true;
                    _cmdLen = (int)(buf - _cmd);
                    *buf = '\0';
                } else if (isSpace(ch)) {
                    *buf = ' ';
                    buf++;
                    state = WaitEndCmd;
                } else {
                    return ErrInvalidCmd;  //invalid command
                }
                break;
            case WaitEscape:
                if (strchr("rnt!\\", ch) != NULL) {
                    switch(ch){
                    case 'r': *buf = '\r'; break;
                    case 'n': *buf = '\n'; break;
                    case 't': *buf = '\t'; break;
                    default: *buf = ch; break;
                    }

                    buf++;
                    state = WaitEndCmd;
                } else {
                    return ErrUnknownEscape;
                }
                break;
            case WaitEndCmd:
                if (ch == '\\') {
                    state = WaitEscape;
                } else if (ch == '\r') {
                    done = true;
                    _cmdLen = (int)(buf - _cmd);
                    *buf = '\0';
                } else {
                    *buf = ch;
                    buf++;
                }
                break;
            }
        }       
    }

    //parse command
    if (_cmdLen < 6) 
        return ErrInvalidCmd;
    else if (isWrite() && _cmdLen <= 7)
        return ErrNoData;

    //micro address
    _ucAddr = hex2Byte((uint8_t *)(_cmd + 2));
    _ioAddr = hex2Byte((uint8_t *)(_cmd + 4));

    return _cmdLen;    
}
