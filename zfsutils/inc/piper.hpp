#pragma once

#include "serial.hpp"
#include <iostream>
#include <memory>
#include <unistd.h>

class PipeBase {
  private:
    int pipenr;
    void closePipeBaseIfOwned() {
        if (pipenr != -1) {
            std::cout << "Closing pipe" << std::endl;
            close(pipenr);
            pipenr = -1;
        }
    }

  public:
    PipeBase(int nr) : pipenr(nr) {};
    ~PipeBase() { closePipeBaseIfOwned(); }

    // Delete copy-constructor
    PipeBase(const PipeBase &) = delete;
    // Delete copy assignment operator
    PipeBase &operator=(const PipeBase &) = delete;

    PipeBase(PipeBase &&other) noexcept {
        pipenr = other.pipenr;
        other.pipenr = -1;
    }

    PipeBase &operator=(PipeBase &&other) noexcept {
        if (this != &other) {
            closePipeBaseIfOwned();

            pipenr = other.pipenr;
            other.pipenr = -1;
        }
        return *this;
    }

    int getPipeBaseFd() { return pipenr; }

    void closePipeBase() { closePipeBaseIfOwned(); }
};

class RxPipe : public SerialReader {
  private:
    PipeBase pipeBase;

  public:
    RxPipe(int pipeNr) : pipeBase{pipeNr} {};
    RxPipe(const RxPipe &) = delete;
    RxPipe &operator=(const RxPipe &) = delete;

    RxPipe(RxPipe &&other) : pipeBase{-1} {
        pipeBase = std::move(other.pipeBase);
    }

    RxPipe &operator=(RxPipe &&other) noexcept {
        pipeBase = std::move(other.pipeBase);
        return *this;
    }

    std::optional<std::vector<char>> get(int maxlen) override {
        std::vector<char> output_bytes;

        ssize_t bytesRead;
        char character;

        while ((bytesRead = read(pipeBase.getPipeBaseFd(), &character, 1))) {
            if (bytesRead == -1) {
                throw std::logic_error{"Pipe read fail!"};
                break;
            }
            if (bytesRead == 0) {
                std::cout << "Pipe retuned EOF" << std::endl;
                break;
            }
            output_bytes.push_back(character);

            if (output_bytes.size() >= maxlen) {
                break;
            }
        }

        if (output_bytes.size() != 0) {
            return output_bytes;
        }
        return {};
    };
};

class TxPipe : public SerialWriter {
  private:
    PipeBase pipeBase;

  public:
    TxPipe(int pipeNr) : pipeBase{pipeNr} {};
    TxPipe(const TxPipe &) = delete;
    TxPipe &operator=(const TxPipe &) = delete;

    TxPipe(TxPipe &&other) : pipeBase{-1} {
        pipeBase = std::move(other.pipeBase);
    }

    TxPipe &operator=(TxPipe &&other) noexcept {
        pipeBase = std::move(other.pipeBase);
        return *this;
    }

    int send(char c) override { return write(pipeBase.getPipeBaseFd(), &c, 1); }
    void terminate() override { pipeBase.closePipeBase(); }
};

class Piper {
  public:
    RxPipe rxPipe{-1};
    TxPipe txPipe{-1};

    Piper() {
        int pipe_creation_retval = pipe(pipes);
        if (pipe_creation_retval != 0) {
            std::logic_error{"Could not create pipes :("};
        }
        std::cout << "Made some pipes! " << pipes[0] << " " << pipes[1]
                  << std::endl;
        rxPipe = RxPipe{pipes[0]};
        txPipe = TxPipe{pipes[1]};
        std::cout << "End of piper constructor" << std::endl;
    }
    RxPipe getRx() { return std::move(rxPipe); }
    TxPipe getTx() { return std::move(txPipe); }
    int pipes[2]{};
};
