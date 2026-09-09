#include "CommandDispatcher.h"
#include "RegisterController.h"
#include "l2_protocol.h"

#include <iostream>
#include <iomanip>

using namespace std;


/*
 * Convert GUI Data[8] into a 32-bit value.
 *
 * Data format:
 *
 *   00 00 00 00 XX XX XX XX
 *   -----------------------
 *             32-bit value
 *
 * Example:
 *
 *   00 00 00 00 12 34 56 78
 *
 *   => 0x12345678
 *   => 305419896
 */
static uint32_t dataToUint32(const uint8_t *data)
{
    return
        (static_cast<uint32_t>(data[4]) << 24) |
        (static_cast<uint32_t>(data[5]) << 16) |
        (static_cast<uint32_t>(data[6]) << 8)  |
        (static_cast<uint32_t>(data[7]));
}

static const char *getCommandName(uint8_t reg_addr)
{
    switch (reg_addr)
    {
        case 1:  return "Soft Reset";
        case 2:  return "Soft Start";
        case 3:  return "Send Packet";
        case 4:  return "Mod Enable";
        case 5:  return "Squelch Enable";
        case 6:  return "MGC2 On";
        case 7:  return "AGC1 On";
        case 8:  return "AGC2 On";
        case 9:  return "Amp OK";
        case 10: return "Bandwidth";
        case 11: return "Mode Select";
        case 12: return "LED Timer";
        case 13: return "Message Value";
        case 14: return "A Coefficient 1";
        case 15: return "A Coefficient 2";
        case 16: return "Reference 1";
        case 17: return "Reference 2";
        default: return "UNKNOWN";
    }
}

/*
 * Apply one GUI register command to FPGA.
 *
 * Register Address:
 *
 *   1  -> Soft Reset
 *   2  -> Soft Start
 *   3  -> Send Packet
 *   4  -> Mod Enable
 *   5  -> Squelch Enable
 *   6  -> MGC2 On
 *   7  -> AGC1 On
 *   8  -> AGC2 On
 *   9  -> Amp OK
 *   10 -> Bandwidth
 *   11 -> Mode Select
 *   12 -> LED Timer
 *   13 -> Message Value
 *   14 -> A Coeff1
 *   15 -> A Coeff2
 *   16 -> Ref1
 *   17 -> Ref2
 */
bool applyRegisterCommand(
    uint16_t fpga_id,
    uint8_t reg_addr,
    const uint8_t *data)
{
    if (data == nullptr)
    {
        cerr << "Command Dispatcher: invalid data pointer."
             << endl;

        return false;
    }


    RegisterController controller;

    if (!controller.open())
    {
        cerr << "Command Dispatcher: failed to open Register Controller."
             << endl;

        return false;
    }


    uint32_t value = dataToUint32(data);


    // cout << endl;
    // cout << "====================================" << endl;
    // cout << "       GUI Register Command" << endl;
    // cout << "====================================" << endl;

    // cout << "FPGA ID         : 0x"
    //      << hex
    //      << uppercase
    //      << setw(4)
    //      << setfill('0')
    //      << fpga_id
    //      << dec
    //      << setfill(' ')
    //      << endl;

    // cout << "Register Addr   : "
    //      << static_cast<unsigned int>(reg_addr)
    //      << endl;

    // cout << "Data Value      : "
    //      << value
    //      << endl;

    cout << endl;
    cout << "====================================" << endl;
    cout << "       GUI Register Command" << endl;
    cout << "====================================" << endl;

    cout << endl;
    cout << "[PACKET]" << endl;

    cout << "Interface        : usb0" << endl;

    cout << "Packet Length    : 0x"
        << hex
        << uppercase
        << setw(4)
        << setfill('0')
        << L2_RX_PACKETLEN
        << dec
        << setfill(' ')
        << endl;

    cout << "Frame Size       : "
        << L2_RX_FRAME_SIZE
        << " bytes"
        << endl;

    cout << "FPGA ID          : 0x"
        << hex
        << uppercase
        << setw(4)
        << setfill('0')
        << fpga_id
        << dec
        << setfill(' ')
        << endl;


    cout << endl;
    cout << "[COMMAND]" << endl;

    cout << "Register Address : "
        << static_cast<unsigned int>(reg_addr)
        << " (0x"
        << hex
        << uppercase
        << setw(2)
        << setfill('0')
        << static_cast<unsigned int>(reg_addr)
        << dec
        << setfill(' ')
        << ")"
        << endl;

    cout << "Command          : "
        << getCommandName(reg_addr)
        << endl;


    cout << endl;
    cout << "[DATA]" << endl;

    cout << "Raw Data         : ";

    for (int i = 0; i < L2_RX_DATA_SIZE; i++)
    {
        cout << hex
            << uppercase
            << setw(2)
            << setfill('0')
            << static_cast<unsigned int>(data[i]);

        if (i < L2_RX_DATA_SIZE - 1)
            cout << " ";
    }

    cout << dec
        << setfill(' ')
        << endl;

    cout << "Decoded Value    : "
        << value
        << " (0x"
        << hex
        << uppercase
        << setw(8)
        << setfill('0')
        << value
        << dec
        << setfill(' ')
        << ")"
        << endl;


    cout << endl;
    cout << "[RESULT]" << endl;    


    switch (reg_addr)
    {
        case 1:
            controller.setSoftReset(value != 0);
            break;

        case 2:
            controller.setSoftStart(value != 0);
            break;

        case 3:
            controller.setSendPacket(value != 0);
            break;

        case 4:
            controller.setModEnable(value != 0);
            break;

        case 5:
            controller.setSquelchEnable(value != 0);
            break;

        case 6:
            controller.setMGC2On(value != 0);
            break;

        case 7:
            controller.setAGC1On(value != 0);
            break;

        case 8:
            controller.setAGC2On(value != 0);
            break;

        case 9:
            controller.setAmpOK(value != 0);
            break;

        case 10:
            controller.setBandwidth(value);
            break;

        case 11:
            controller.setModeSelect(value);
            break;

        case 12:
            controller.setLedTimer(value);
            break;

        case 13:
            controller.setMsgValue(value);
            break;

        case 14:
            controller.setACoeff1(value);
            break;

        case 15:
            controller.setACoeff2(value);
            break;

        case 16:
            controller.setRef1(value);
            break;

        case 17:
            controller.setRef2(value);
            break;

        default:
            cerr << "Command Dispatcher: unknown Register Address = "
                 << static_cast<unsigned int>(reg_addr)
                 << endl;

            controller.close();
            return false;
    }


    controller.close();

    cout << "Command applied successfully." << endl;

    return true;
}