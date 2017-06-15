#ifndef BUFFER_H
#define BUFFER_H

#include <string>

typedef int int32;
typedef unsigned long long int  uint64;
typedef unsigned int   uint32;
typedef unsigned short uint16;
typedef unsigned char  uint8;

class Buffer {
public:
    Buffer();
    Buffer(const Buffer& buf);
    ~Buffer();

    void clear();
    bool empty() const;
    int size() const;
    char* begin() const;
    char* end() const;
    const char* data() const;
    void move();
    void append(char* buf, int len);
    void alloc(int len);
    void addBegin(int len);
    std::string retrieveBuf(int len);

    int32 peekInt32() {
        int32 be32 = 0;
        memcpy(&be32, begin(), sizeof be32);
        return be32;
    }

    uint8 peekUint8() {
        uint8 be8 = 0;
        memcpy(&be8, begin(), sizeof be8);
        return be8;
    }

    uint16 peekUint16() {
        uint16 be16 = 0;
        memcpy(&be16, begin(), sizeof be16);
        return be16;
    }
    uint32 peekUint32() {
        uint32 be32 = 0;
        memcpy(&be32, begin(), sizeof be32);
        return be32;
    }
    uint64 peekUint64() {
        uint64 be64 = 0;
        memcpy(&be64, begin(), sizeof be64);
        return be64;
    }

    //read
    int32 readInt32() {
        uint32 be32 = 0;
        memcpy(&be32, begin(), sizeof be32);
        addBegin(4);
        return be32;
    }

    uint8 readUint8() {
        uint8 be8 = 0;
        memcpy(&be8, begin(), sizeof be8);
        addBegin(1);
        return be8;
    }

    uint16 readUint16() {
        uint16 be16 = 0;
        memcpy(&be16, begin(), sizeof be16);
        addBegin(2);
        return be16;
    }

    uint32 readUint32() {
        uint32 be32 = 0;
        memcpy(&be32, begin(), sizeof be32);
        addBegin(4);
        return be32;
    }

    uint64 readUint64() {
        uint64 be64 = 0;
        memcpy(&be64, begin(), sizeof be64);
        addBegin(8);
        return be64;
    }

    std::string show();

private:
    char* buf_;
    int size_, begin_, end_, cap_;
};

#endif // BUFFER_H

