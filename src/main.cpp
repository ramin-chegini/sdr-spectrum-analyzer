#include <iostream>
#include <string>
#include <cstdlib>

#include "RegisterController.h"

using namespace std;

static void printHelp()
{
    cout << "Usage:" << endl;
    cout << "  sdr_app enable" << endl;
    cout << "  sdr_app disable" << endl;
    cout << "  sdr_app reset" << endl;
    cout << "  sdr_app mode <value>" << endl;
    cout << "  sdr_app filter <value>" << endl;
    cout << "  sdr_app param0 <value>" << endl;
    cout << "  sdr_app param1 <value>" << endl;
    cout << "  sdr_app status" << endl;
}

int main(int argc, char *argv[])
{
    RegisterController fpga;

    if (!fpga.open())
    {
        cout << "FPGA Open Failed" << endl;
        return -1;
    }

    if (argc < 2)
    {
        printHelp();
        return 0;
    }

    string cmd(argv[1]);

    if (cmd == "enable")
    {
        fpga.enableDSP(true);
        cout << "DSP Enabled" << endl;
    }
    else if (cmd == "disable")
    {
        fpga.enableDSP(false);
        cout << "DSP Disabled" << endl;
    }
    else if (cmd == "reset")
    {
        fpga.resetDSP();
        cout << "DSP Reset Done" << endl;
    }
    else if (cmd == "mode")
    {
        if (argc < 3)
        {
            cout << "Missing mode value" << endl;
            return -1;
        }

        uint32_t mode = strtoul(argv[2], nullptr, 0);

        fpga.setMode(mode);

        cout << "Mode = " << mode << endl;
    }
    else if (cmd == "filter")
    {
        if (argc < 3)
        {
            cout << "Missing filter value" << endl;
            return -1;
        }

        uint32_t filter = strtoul(argv[2], nullptr, 0);

        fpga.setFilter(filter);

        cout << "Filter = " << filter << endl;
    }
    else if (cmd == "param0")
    {
        if (argc < 3)
        {
            cout << "Missing Param0 value" << endl;
            return -1;
        }

        uint32_t value = strtoul(argv[2], nullptr, 0);

        fpga.setParam0(value);

        cout << "Param0 = 0x"
             << hex << value << dec << endl;
    }
    else if (cmd == "param1")
    {
        if (argc < 3)
        {
            cout << "Missing Param1 value" << endl;
            return -1;
        }

        uint32_t value = strtoul(argv[2], nullptr, 0);

        fpga.setParam1(value);

        cout << "Param1 = 0x"
             << hex << value << dec << endl;
    }
    else if (cmd == "status")
    {
        cout << endl;
        cout << "====================================" << endl;
        cout << " Register Controller Status" << endl;
        cout << "====================================" << endl;

        cout << "DSP Enable : "
             << (fpga.isEnabled() ? "ON" : "OFF") << endl;

        cout << "Mode       : "
             << fpga.getMode() << endl;

        cout << "Filter     : "
             << fpga.getFilter() << endl;

        cout << "Param0     : 0x"
             << hex << fpga.getParam0() << dec << endl;

        cout << "Param1     : 0x"
             << hex << fpga.getParam1() << dec << endl;
    }
    else
    {
        cout << "Unknown command: " << cmd << endl << endl;
        printHelp();
        return -1;
    }

    return 0;
}