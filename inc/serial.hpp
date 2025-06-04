#pragma once

#include <optional>
#include <vector>

class SerialWriter {
  public:
    virtual int send(char c) = 0;
    virtual void terminate() = 0;
};

class SerialReader {
  public:
    // Should try and read the number of bytes in maxlen
    // If the reader is empty, and will no longer get any data (like a closed
    // rxPipe) we return {}
    virtual std::optional<std::vector<char>> get(int maxlen) = 0;
};
