#ifndef _STRING_STREAM_H_INCLUDED_
#define _STRING_STREAM_H_INCLUDED_

#include "application.h"

class StringStream : public Stream
{
public:
    StringStream(const String &s) : value(s), position(0) { }

    // Stream methods
    virtual int available() { return value.length() - position; }
    virtual int read() { return position < value.length() ? value[position++] : -1; }
    virtual int peek() { return position < value.length() ? value[position] : -1; }
    virtual void flush() { };
    // Print methods
    virtual size_t write(uint8_t c) { value += (char)c; };

protected:
    String value;
    int position;
};

#endif // _STRING_STREAM_H_INCLUDED_
