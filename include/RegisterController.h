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

    void setSoftStart(bool enable);

    void setSendPacket(bool enable);

    void setModEnable(bool enable);

    void setSquelchEnable(bool enable);

    void setMGC2On(bool enable);

    void setAGC1On(bool enable);

    void setAGC2On(bool enable);

    void setAmpOK(bool enable);

    /* Configuration */

    void setBandwidth(uint32_t bandwidth);

    void setModeSelect(uint32_t mode);

    void setLedTimer(uint32_t timer);

    void setMsgValue(uint32_t value);

    void setACoeff1(uint32_t value);

    void setACoeff2(uint32_t value);

    void setRef1(uint32_t value);

    void setRef2(uint32_t value);

    /* Readback */

    uint32_t getControl();

    uint32_t getBandwidth();

    uint32_t getModeSelect();

    uint32_t getLedTimer();

    uint32_t getMsgValue();

    uint32_t getCoefficients();

    uint32_t getRef1();

    uint32_t getRef2();

private:

    int fd;

    volatile uint32_t *regs;

};

#endif
