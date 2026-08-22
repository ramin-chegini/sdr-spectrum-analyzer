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


    /* Control */

    void setSoftReset(bool enable);

    void setPSDStart(bool enable);

    void setPSDCaptureStart(bool enable);

    void setMaxHoldEnable(bool enable);

    void setRxChannel(uint32_t channel);


    /* Configuration */

    void setBandwidth(uint32_t bandwidth);

    void setPSDPerSec(uint32_t rate);

    void setMaxHoldDelay(uint32_t delay);

    void setLedTimer(uint32_t timer);


    /* Readback */

    uint32_t getControl();

    uint32_t getBandwidth();

    uint32_t getPSDPerSec();

    uint32_t getMaxHoldDelay();

    uint32_t getLedTimer();

private:

    int fd;

    volatile uint32_t *regs;

};

#endif
