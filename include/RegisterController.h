#ifndef REGISTER_CONTROLLER_H
#define REGISTER_CONTROLLER_H

#include <stdint.h>

class RegisterController
{

public:

    RegisterController();

    ~RegisterController();

    bool open();

    void close();

    void writeRegister(uint32_t offset,
                       uint32_t value);

    uint32_t readRegister(uint32_t offset);

    void enableDSP(bool enable);

    void resetDSP();

    void setMode(uint32_t mode);

    void setFilter(uint32_t filter);

    void setParam0(uint32_t value);

    void setParam1(uint32_t value);

    uint32_t getMode();

    uint32_t getFilter();

    uint32_t getParam0();

    uint32_t getParam1();

    bool isEnabled();    

private:

    int fd;

    volatile uint32_t *regs;

};

#endif
