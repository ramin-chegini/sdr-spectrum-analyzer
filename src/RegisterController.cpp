#include "RegisterController.h"
#include "RegisterMap.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>

RegisterController::RegisterController()
{
    fd = -1;
    regs = nullptr;
}

RegisterController::~RegisterController()
{
    close();
}

bool RegisterController::open()
{
    fd = ::open("/dev/mem", O_RDWR | O_SYNC);

    if (fd < 0)
    {
        perror("open");
        return false;
    }

    regs = (volatile uint32_t *)mmap(
        NULL,
        FPGA_REG_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        FPGA_REG_BASE);

    if (regs == MAP_FAILED)
    {
        perror("mmap");

        ::close(fd);

        fd = -1;
        regs = nullptr;

        return false;
    }

    return true;
}

void RegisterController::close()
{
    if (regs)
    {
        munmap((void *)regs, FPGA_REG_SIZE);
        regs = nullptr;
    }

    if (fd >= 0)
    {
        ::close(fd);
        fd = -1;
    }
}

void RegisterController::writeRegister(uint32_t offset,
                                       uint32_t value)
{
    regs[offset / 4] = value;
}

uint32_t RegisterController::readRegister(uint32_t offset)
{
    return regs[offset / 4];
}


/* ============================================================
 * CONTROL REGISTER
 * ============================================================ */

void RegisterController::setSoftReset(bool enable)
{
    if (!enable)
        return;

    uint32_t reg = readRegister(REG_CONTROL);

    // Generate reset pulse
    reg |= CTRL_SOFT_RESET;
    writeRegister(REG_CONTROL, reg);

    // Clear reset bit
    reg &= ~CTRL_SOFT_RESET;
    writeRegister(REG_CONTROL, reg);
}

void RegisterController::setSoftStart(bool enable)
{
    uint32_t reg = readRegister(REG_CONTROL);

    if (enable)
        reg |= CTRL_SOFT_START;
    else
        reg &= ~CTRL_SOFT_START;

    writeRegister(REG_CONTROL, reg);
}

void RegisterController::setSendPacket(bool enable)
{
    uint32_t reg = readRegister(REG_CONTROL);

    if (enable)
        reg |= CTRL_SEND_PACKET;
    else
        reg &= ~CTRL_SEND_PACKET;

    writeRegister(REG_CONTROL, reg);
}

void RegisterController::setModEnable(bool enable)
{
    uint32_t reg = readRegister(REG_CONTROL);

    if (enable)
        reg |= CTRL_MOD_ENABLE;
    else
        reg &= ~CTRL_MOD_ENABLE;

    writeRegister(REG_CONTROL, reg);
}

void RegisterController::setSquelchEnable(bool enable)
{
    uint32_t reg = readRegister(REG_CONTROL);

    if (enable)
        reg |= CTRL_SQUELCH_ENABLE;
    else
        reg &= ~CTRL_SQUELCH_ENABLE;

    writeRegister(REG_CONTROL, reg);
}

void RegisterController::setMGC2On(bool enable)
{
    uint32_t reg = readRegister(REG_CONTROL);

    if (enable)
        reg |= CTRL_MGC2_ON;
    else
        reg &= ~CTRL_MGC2_ON;

    writeRegister(REG_CONTROL, reg);
}

void RegisterController::setAGC1On(bool enable)
{
    uint32_t reg = readRegister(REG_CONTROL);

    if (enable)
        reg |= CTRL_AGC1_ON;
    else
        reg &= ~CTRL_AGC1_ON;

    writeRegister(REG_CONTROL, reg);
}

void RegisterController::setAGC2On(bool enable)
{
    uint32_t reg = readRegister(REG_CONTROL);

    if (enable)
        reg |= CTRL_AGC2_ON;
    else
        reg &= ~CTRL_AGC2_ON;

    writeRegister(REG_CONTROL, reg);
}

void RegisterController::setAmpOK(bool enable)
{
    uint32_t reg = readRegister(REG_CONTROL);

    if (enable)
        reg |= CTRL_AMP_OK;
    else
        reg &= ~CTRL_AMP_OK;

    writeRegister(REG_CONTROL, reg);
}


/* ============================================================
 * CONFIGURATION REGISTERS
 * ============================================================ */

void RegisterController::setBandwidth(uint32_t bandwidth)
{
    uint32_t reg = readRegister(REG_BANDWIDTH);

    reg &= ~BANDWIDTH_MASK;
    reg |= (bandwidth & BANDWIDTH_MASK);

    writeRegister(REG_BANDWIDTH, reg);
}

void RegisterController::setModeSelect(uint32_t mode)
{
    uint32_t reg = readRegister(REG_MODE);

    reg &= ~MODE_SELECT_MASK;
    reg |= (mode & MODE_SELECT_MASK);

    writeRegister(REG_MODE, reg);
}

void RegisterController::setLedTimer(uint32_t timer)
{
    writeRegister(REG_LED_TIMER, timer);
}

void RegisterController::setMsgValue(uint32_t value)
{
    writeRegister(REG_MESSAGE, value);
}

void RegisterController::setACoeff1(uint32_t value)
{
    uint32_t reg = readRegister(REG_COEFFICIENTS);

    reg &= ~A_COEFF1_MASK;
    reg |= (value & 0xFFu);

    writeRegister(REG_COEFFICIENTS, reg);
}

void RegisterController::setACoeff2(uint32_t value)
{
    uint32_t reg = readRegister(REG_COEFFICIENTS);

    reg &= ~A_COEFF2_MASK;
    reg |= ((value & 0xFFu) << 8);

    writeRegister(REG_COEFFICIENTS, reg);
}

void RegisterController::setRef1(uint32_t value)
{
    writeRegister(REG_REF1, value & REF1_MASK);
}

void RegisterController::setRef2(uint32_t value)
{
    writeRegister(REG_REF2, value & REF2_MASK);
}


/* ============================================================
 * READBACK
 * ============================================================ */

uint32_t RegisterController::getControl()
{
    return readRegister(REG_CONTROL);
}

uint32_t RegisterController::getBandwidth()
{
    return readRegister(REG_BANDWIDTH);
}

uint32_t RegisterController::getModeSelect()
{
    return readRegister(REG_MODE);
}

uint32_t RegisterController::getLedTimer()
{
    return readRegister(REG_LED_TIMER);
}

uint32_t RegisterController::getMsgValue()
{
    return readRegister(REG_MESSAGE);
}

uint32_t RegisterController::getCoefficients()
{
    return readRegister(REG_COEFFICIENTS);
}

uint32_t RegisterController::getRef1()
{
    return readRegister(REG_REF1);
}

uint32_t RegisterController::getRef2()
{
    return readRegister(REG_REF2);
}







