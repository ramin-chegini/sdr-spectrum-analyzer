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

    if(fd < 0)
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

    if(regs == MAP_FAILED)
    {
        perror("mmap");

        ::close(fd);

        fd = -1;

        return false;
    }

    return true;
}

void RegisterController::close()
{

    if(regs)
    {
        munmap((void *)regs,
               FPGA_REG_SIZE);

        regs = nullptr;
    }

    if(fd >= 0)
    {
        ::close(fd);

        fd = -1;
    }

}

void RegisterController::writeRegister(uint32_t offset,
                                       uint32_t value)
{
    regs[offset/4] = value;
}

uint32_t RegisterController::readRegister(uint32_t offset)
{
    return regs[offset/4];
}

// void RegisterController::setSoftReset(bool enable)
// {
//     uint32_t reg = readRegister(REG_CONTROL);

//     if (enable)
//         reg |= CTRL_SOFT_RESET;
//     else
//         reg &= ~CTRL_SOFT_RESET;

//     writeRegister(REG_CONTROL, reg);
// }

void RegisterController::setSoftReset(bool enable)
{
    if (!enable)
        return;

    uint32_t reg = readRegister(REG_CONTROL);

    // Generate reset pulse
    reg |= CTRL_SOFT_RESET;
    writeRegister(REG_CONTROL, reg);

    // Clear reset bit immediately
    reg &= ~CTRL_SOFT_RESET;
    writeRegister(REG_CONTROL, reg);
}

void RegisterController::setPSDStart(bool enable)
{
    uint32_t reg = readRegister(REG_CONTROL);

    if (enable)
        reg |= CTRL_PSD_START;
    else
        reg &= ~CTRL_PSD_START;

    writeRegister(REG_CONTROL, reg);
}

void RegisterController::setPSDCaptureStart(bool enable)
{
    uint32_t reg = readRegister(REG_CONTROL);

    if (enable)
        reg |= CTRL_PSD_CAPTURE_START;
    else
        reg &= ~CTRL_PSD_CAPTURE_START;

    writeRegister(REG_CONTROL, reg);
}

void RegisterController::setMaxHoldEnable(bool enable)
{
    uint32_t reg = readRegister(REG_CONTROL);

    if (enable)
        reg |= CTRL_MAXHOLD_ENABLE;
    else
        reg &= ~CTRL_MAXHOLD_ENABLE;

    writeRegister(REG_CONTROL, reg);
}

void RegisterController::setRxChannel(uint32_t channel)
{
    uint32_t reg = readRegister(REG_CONTROL);

    if (channel)
        reg |= CTRL_RXCH_SELECT;
    else
        reg &= ~CTRL_RXCH_SELECT;

    writeRegister(REG_CONTROL, reg);
}

void RegisterController::setBandwidth(uint32_t bandwidth)
{
    uint32_t reg = readRegister(REG_BANDWIDTH);

    reg &= ~BANDWIDTH_MASK;
    reg |= (bandwidth & BANDWIDTH_MASK);

    writeRegister(REG_BANDWIDTH, reg);
}

void RegisterController::setPSDPerSec(uint32_t rate)
{
    uint32_t reg = readRegister(REG_PSD_RATE);

    reg &= ~PSD_RATE_MASK;
    reg |= (rate & PSD_RATE_MASK);

    writeRegister(REG_PSD_RATE, reg);
}

void RegisterController::setMaxHoldDelay(uint32_t delay)
{
    writeRegister(REG_MAXHOLD_DELAY, delay);
}

void RegisterController::setLedTimer(uint32_t timer)
{
    writeRegister(REG_LED_TIMER, timer);
}

uint32_t RegisterController::getControl()
{
    return readRegister(REG_CONTROL);
}

uint32_t RegisterController::getBandwidth()
{
    return readRegister(REG_BANDWIDTH);
}

uint32_t RegisterController::getPSDPerSec()
{
    return readRegister(REG_PSD_RATE);
}

uint32_t RegisterController::getMaxHoldDelay()
{
    return readRegister(REG_MAXHOLD_DELAY);
}

uint32_t RegisterController::getLedTimer()
{
    return readRegister(REG_LED_TIMER);
}







