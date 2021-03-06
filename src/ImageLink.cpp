// Copyright (C) 2014 mru@sisyphus.teil.cc
//
// 
//

#include <fstream>
#include <string>

#include "ImageLink.hpp"


ImageLink::ImageLink(const std::string& filename) {
    f.exceptions(std::ifstream::failbit);
    f.open(filename, std::ios_base::binary|std::ios_base::in);
}

void ImageLink::readMemory(unsigned addr, unsigned count, unsigned char* it) {
    f.seekg(addr);
    f.read((char*)(it), count);
}

std::string ImageLink::readVersion() {
    return "memory image device";
}

