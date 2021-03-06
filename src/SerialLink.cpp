// Copyright (C) 2014 mru@sisyphus.teil.cc
//
// simplistic interface to the gps watch
//

#include <vector>
#include <algorithm>
#include <iostream>

#include <cassert> 

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include "SerialLink.hpp"


SerialLink::SerialLink(const std::string& filename) : filename(filename) {


    fd = ::open (filename.c_str(), O_RDWR);
    if (fd == -1) {
        perror("failed to open serial device");
        throw std::runtime_error("failed to open serial port");
    }

    // http://trainingkits.gweb.io/serial-linux.html

    struct termios options;

    tcgetattr(fd, &options);
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);
    cfmakeraw(&options);
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 50;

    if(tcsetattr(fd, TCSANOW, &options)!= 0) {
        std::cerr << "error!" <<std::endl;
        throw std::runtime_error("failed to open serial port");
    }
}


void SerialLink::readMemory(unsigned addr, unsigned count, unsigned char* it) {
    unsigned char opcode;
    std::vector<unsigned char> tx(8);
    std::vector<unsigned char> rx;
    std::vector<unsigned char>::iterator tx_it = tx.begin();
    tx[0] = addr ;
    tx[1] = addr >> 8;
    tx[2] = addr >> 16;
    tx[3] = count;
    sendCommand(0x12, tx);
    receiveReply(opcode, rx);
    std::copy(rx.begin(), rx.end(), it);
}

std::string SerialLink::readVersion() {
    assert(fd != -1);
    std::vector<unsigned char> version;
    unsigned char opcode;
    sendCommand(0x10, std::vector<unsigned char>());
    receiveReply(opcode, version);

    std::string str(version.size(), '\0');
    std::copy(version.begin(), version.end(), str.begin());
    return str;
}


void SerialLink::sendCommand(unsigned char opcode, std::vector<unsigned char> payload) {
    assert (fd != -1);

    std::vector<unsigned char> frame(2+2+1+ payload.size() +2+2);
    std::vector<unsigned char>::iterator it = frame.begin();

    unsigned short l = payload.size() + 1;
    unsigned short cs = checksum(opcode, payload);
    *it++ = 0xa0;
    *it++ = 0xa2;
    *it++ = l >> 8 & 0xff;
    *it++ = l & 0xff;
    *it++ = opcode;
    std::copy(payload.begin(), payload.end(), it); 
    it += payload.size();
    *it++ = cs >> 8 & 0xff;
    *it++ = cs & 0xff;
    *it++ = 0xb0;
    *it++ = 0xb3;

    write(frame);
}

void SerialLink::receiveReply(unsigned char& opcode, std::vector<unsigned char>& target) {
    assert(fd!=-1);
    unsigned short l;
    unsigned short cs;
    expect(0xa0);
    expect(0xa2);
    l = read() << 8;
    l += read();
    opcode = read();
    target.resize(l-1);
    read(target);
    cs = checksum(opcode, target);
    expect(cs >> 8 &0xff);
    expect(cs & 0xff);
    expect(0xb0);
    expect(0xb3);
}

unsigned char SerialLink::expect(unsigned char val) {
    unsigned char rcv = read();
    assert (rcv == val);
    return rcv;
}

void SerialLink::write(std::vector<unsigned char>& buf) {
    ssize_t n;
    n = ::write(fd, &buf[0], buf.size());
    assert((size_t)n==buf.size());
}

unsigned char SerialLink::read() {
    assert(fd!=-1);
    unsigned char val;
    ssize_t n = ::read(fd, &val, 1);
    assert(n == 1);
    return val;
}

void SerialLink::read(std::vector<unsigned char>& buf) {

    assert(fd!=-1);
    ssize_t n = ::read(fd, &buf[0], buf.size());
    assert( (size_t)n == buf.size());
}

unsigned short SerialLink::checksum(unsigned char opcode, std::vector<unsigned char> payload) {

    unsigned short cs = opcode;
    for (auto v : payload) {
        cs += v;
    }
    return cs;
}
