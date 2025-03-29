#pragma once

#include "serial.hpp"
#include <iostream>
#include <memory>
#include <unistd.h>

class PipeBase {
  private:
    std::optional<int> pipeFd{std::nullopt};
    void closePipeBaseIfOwned() {
        if (pipeFd.has_value()) {
            std::cout << "Closing pipe with fd number " << pipeFd.value()
                      << std::endl;
            close(pipeFd.value());
            pipeFd = {};
        }
    }

  public:
    [[deprecated]] PipeBase(int nr) : pipeFd(nr) {};
    PipeBase(std::optional<int> nr) : pipeFd{nr} {};
    ~PipeBase() { closePipeBaseIfOwned(); }

    // Delete copy-constructor
    PipeBase(const PipeBase &) = delete;
    // Delete copy assignment operator
    PipeBase &operator=(const PipeBase &) = delete;

    PipeBase(PipeBase &&other) noexcept {
        if (other.pipeFd.has_value()) {
            pipeFd = other.pipeFd.value();
            other.pipeFd = {};
        }
    }

    PipeBase &operator=(PipeBase &&other) noexcept {
        if (this != &other) {
            closePipeBaseIfOwned();

            if (other.pipeFd.has_value()) {
                pipeFd = other.pipeFd.value();
                other.pipeFd = {};
            }
        }
        return *this;
    }

    [[deprecated("Use getFd instead")]] int getPipeBaseFd() {
        return pipeFd.value_or(-1);
    }
    std::optional<int> getFd() { return pipeFd; }

    void closePipeBase() { closePipeBaseIfOwned(); }
};

class RxPipe : public SerialReader, PipeBase {
  private:
  public:
    RxPipe(std::optional<int> pipeNr) : PipeBase{pipeNr} {};
    RxPipe() : PipeBase{std::nullopt} {};

    ~RxPipe() {
        std::cout << "Closing RxPipe" << std::endl;
        if (PipeBase::getFd().has_value()) {
            std::cout << " RxPipe's pipebase has nr "
                      << PipeBase::getFd().value() << std::endl;
        }
    }

    RxPipe(const RxPipe &) = delete;
    RxPipe &operator=(const RxPipe &) = delete;

    RxPipe(RxPipe &&other) : PipeBase{std::move(other)} {}

    RxPipe &operator=(RxPipe &&other) noexcept {
        PipeBase::operator=(std::move(other));
        return *this;
    }

    [[deprecated("Use getFd instead")]] int getPipeFd() {
        return PipeBase::getPipeBaseFd();
    }
    std::optional<int> getFd() { return PipeBase::getFd(); }

    std::optional<std::vector<char>> get(int maxlen) override {

        std::optional<int> fd = PipeBase::getFd();
        if (!fd.has_value()) {
            throw std::logic_error{"Trying to get from a non-existant Fd..."};
        }

        std::vector<char> output_bytes;

        ssize_t bytesRead;
        char character;

        while ((bytesRead = read(fd.value(), &character, 1))) {
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

class TxPipe : public SerialWriter, PipeBase {
  private:
  public:
    [[deprecated]] TxPipe(int pipeNr) : PipeBase{pipeNr} {};
    TxPipe(std::optional<int> pipeNr) : PipeBase{pipeNr} {};

    TxPipe(const TxPipe &) = delete;
    TxPipe &operator=(const TxPipe &) = delete;

    ~TxPipe() { std::cout << "Closing TxPipe" << std::endl; }

    TxPipe(TxPipe &&other) : PipeBase{std::move(other)} {}

    TxPipe &operator=(TxPipe &&other) noexcept {
        PipeBase::operator=(std::move(other));
        return *this;
    }

    [[deprecated]] int getPipeFd() { return PipeBase::getPipeBaseFd(); }
    std::optional<int> getFd() { return PipeBase::getFd(); }

    int send(char c) override {
        std::optional<int> fd = PipeBase::getFd();
        if (!fd.has_value()) {
            throw std::logic_error{"Trying to read from a non-existant fd..."};
        }
        return write(fd.value(), &c, 1);
    }
    void terminate() override { PipeBase::closePipeBase(); }
};

class Piper {
  public:
    RxPipe rxPipe{std::nullopt};
    TxPipe txPipe{std::nullopt};

    Piper() {
        int pipe_creation_retval = pipe(pipes);
        if (pipe_creation_retval != 0 || pipes[0] == 0 || pipes[1] == 0) {
            throw std::logic_error{"Could not create pipes :("};
        }
        std::cout << "Made some pipes! " << pipes[0] << " " << pipes[1]
                  << std::endl;
        rxPipe = RxPipe{std::optional<int>{pipes[0]}};
        txPipe = TxPipe{std::optional<int>{pipes[1]}};
        std::cout << "End of piper constructor" << std::endl;
    }
    RxPipe getRx() { return std::move(rxPipe); }
    TxPipe getTx() { return std::move(txPipe); }

    int pipes[2]{};
};
