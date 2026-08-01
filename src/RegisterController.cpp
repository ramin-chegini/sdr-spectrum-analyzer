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
void RegisterController::enableDSP(bool enable)
{

    uint32_t reg = readRegister(REG_CONTROL);

    if(enable)
        reg |= CTRL_ENABLE;
    else
        reg &= ~CTRL_ENABLE;

    writeRegister(REG_CONTROL,
                  reg);

}
void RegisterController::resetDSP()
{
    uint32_t reg;

    reg = readRegister(REG_CONTROL);

    reg |= CTRL_RESET;

    writeRegister(REG_CONTROL,
                  reg);

    reg &= ~CTRL_RESET;

    writeRegister(REG_CONTROL,
                  reg);
}

void RegisterController::setMode(uint32_t mode)
{
    writeRegister(REG_MODE,
                  mode);
}

void RegisterController::setFilter(uint32_t filter)
{
    writeRegister(REG_FILTER,
                  filter);
}

void RegisterController::setParam0(uint32_t value)
{
    writeRegister(REG_PARAM0,
                  value);
}
void RegisterController::setParam1(uint32_t value)
{
    writeRegister(REG_PARAM1,
                  value);
}

uint32_t RegisterController::getMode()
{
    return readRegister(REG_MODE);
}

uint32_t RegisterController::getFilter()
{
    return readRegister(REG_FILTER);
}

uint32_t RegisterController::getParam0()
{
    return readRegister(REG_PARAM0);
}

uint32_t RegisterController::getParam1()
{
    return readRegister(REG_PARAM1);
}

bool RegisterController::isEnabled()
{
    return (readRegister(REG_CONTROL) & CTRL_ENABLE) != 0;
}





