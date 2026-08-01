#include <iostream>
#include "RegisterController.h"

int main()
{
    RegisterController fpga;

    if (!fpga.open())
    {
        std::cout << "FPGA Open Failed" << std::endl;
        return -1;
    }

    std::cout << "FPGA Open OK" << std::endl;

    fpga.enableDSP(true);

    std::cout << "DSP Enabled" << std::endl;

    fpga.setMode(3);

    std::cout << "Mode = 3" << std::endl;

    fpga.setFilter(2);

    std::cout << "Filter = 2" << std::endl;

    fpga.setParam0(0xFFFFFFFF);

    fpga.setParam1(0xABABABAB);

    std::cout << "Done" << std::endl;

    return 0;
}
