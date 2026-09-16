#include "CommandDispatcher.h"
#include "RegisterController.h"
#include "l2_protocol.h"
#include "audio_handler.h"

#include <iostream>
#include <iomanip>

using namespace std;


/* ============================================================
 * Helper Functions
 * ============================================================ */

static uint32_t decodeValue32(const uint8_t *command)
{
    return
        (static_cast<uint32_t>(command[5]) << 24) |
        (static_cast<uint32_t>(command[6]) << 16) |
        (static_cast<uint32_t>(command[7]) << 8)  |
        (static_cast<uint32_t>(command[8]));
}


static uint32_t decodeRef24(const uint8_t *command)
{
    return
        (static_cast<uint32_t>(command[6]) << 16) |
        (static_cast<uint32_t>(command[7]) << 8)  |
        (static_cast<uint32_t>(command[8]));
}


static void printCommandBytes(
    const uint8_t *command,
    size_t length)
{
    cout << "Raw Command     : ";

    for (size_t i = 0; i < length; ++i)
    {
        cout << hex
             << uppercase
             << setw(2)
             << setfill('0')
             << static_cast<unsigned int>(command[i]);

        if (i + 1 < length)
            cout << " ";
    }

    cout << dec
         << setfill(' ')
         << endl;
}


static const char *getDirectCommandName(uint8_t command_addr)
{
    switch (command_addr)
    {
        case CMD_SOFT_RESET:
            return "Soft Reset";

        // case CMD_SOFT_START:
        //     return "Soft Start";

        case CMD_MOD_ENABLE:
            return "Mod Enable";            

        case CMD_SEND_PACKET:
            return "Send Packet";

        case CMD_LED_TIMER:
            return "LED Timer";

        default:
            return "UNKNOWN";
    }
}


static const char *getDemodulatorRegisterName(uint8_t reg_addr)
{
    switch (reg_addr)
    {

        case DEMOD_REG_AGC_RESET:
            return "AGC Reset"; 

        case DEMOD_REG_MODE:
            return "Mode Select";

        case DEMOD_REG_BANDWIDTH:
            return "Bandwidth";

        case DEMOD_REG_REF1_A1:
            return "Ref1 / A Coeff1";

        case DEMOD_REG_REF2_A2:
            return "Ref2 / A Coeff2";

        case DEMOD_REG_CONTROL:
            return "Demodulator Control";

        case DEMOD_REG_MESSAGE:
            return "Message Value";

        default:
            return "UNKNOWN";
    }
}


/*
 * Validate common command framing.
 */
static bool validateCommand(
    const uint8_t *command,
    size_t command_len)
{
    if (command == nullptr)
    {
        cerr << "Command Dispatcher: null command pointer."
             << endl;

        return false;
    }


    if (command_len != L2_COMMAND_SIZE)
    {
        cerr << "Command Dispatcher: invalid command length = "
             << command_len
             << " (expected "
             << L2_COMMAND_SIZE
             << ")"
             << endl;

        return false;
    }


    /*
     * Current protocol reserves bytes 1..3 and byte 9.
     */
    if (command[1] != 0x00 ||
        command[2] != 0x00 ||
        command[3] != 0x00 ||
        command[9] != 0x00)
    {
        cerr << "Command Dispatcher: invalid reserved bytes."
             << endl;

        return false;
    }


    if (command[10] != L2_COMMAND_END)
    {
        cerr << "Command Dispatcher: invalid command terminator 0x"
             << hex
             << uppercase
             << setw(2)
             << setfill('0')
             << static_cast<unsigned int>(command[10])
             << dec
             << setfill(' ')
             << endl;

        return false;
    }


    return true;
}


/* ============================================================
 * MAIN DISPATCHER
 * ============================================================ */

bool applyL2Command(
    uint16_t fpga_id,
    const uint8_t *command,
    size_t command_len)
{
    if (!validateCommand(command, command_len))
        return false;


    /*
     * command[0] is Ethernet Frame Byte 17.
     */
    const uint8_t module_or_command = command[0];


    /*
     * Common 32-bit data field.
     *
     * For:
     *
     *   Soft Reset
     *   Soft Start
     *   Send Packet
     *   LED Timer
     *
     * the value occupies the low 32 bits of the 8-byte field.
     *
     * For Demodulator registers, the same 4-byte wire field
     * is used according to the register-specific format.
     */
    const uint32_t value = decodeValue32(command);


    cout << endl;
    cout << "====================================" << endl;
    cout << "          L2 Command Dispatcher" << endl;
    cout << "====================================" << endl;

    cout << "FPGA ID         : 0x"
         << hex
         << uppercase
         << setw(4)
         << setfill('0')
         << fpga_id
         << dec
         << setfill(' ')
         << endl;

    cout << "Command Byte    : 0x"
         << hex
         << uppercase
         << setw(2)
         << setfill('0')
         << static_cast<unsigned int>(module_or_command)
         << dec
         << setfill(' ')
         << endl;

    printCommandBytes(command, command_len);


    /*
     * ----------------------------------------------------------
     * DEMODULATOR MODULE
     * ----------------------------------------------------------
     *
     * Byte 17 = 0x03
     *
     * IMPORTANT:
     * The register address is valid ONLY after the module
     * address has been confirmed as DEMODULATOR_MODULE_ADDR.
     */
    if (module_or_command == DEMODULATOR_MODULE_ADDR)
    {
        const uint8_t reg_addr = command[4];

        cout << "Module Address  : 0x03 (Demodulator)"
             << endl;

        cout << "Register Address: 0x"
             << hex
             << uppercase
             << setw(2)
             << setfill('0')
             << static_cast<unsigned int>(reg_addr)
             << dec
             << setfill(' ')
             << endl;

        cout << "Command         : "
             << getDemodulatorRegisterName(reg_addr)
             << endl;


        RegisterController controller;

        if (!controller.open())
        {
            cerr << "Command Dispatcher: failed to open Register Controller."
                 << endl;

            return false;
        }


        bool success = true;


        switch (reg_addr)
        {
            /*
             * --------------------------------------------------
             * AGC Reset
             *
             * 0x08 -> TRUE
             * 0x00 -> FALSE
             *
             * Currently receive/decode only.
             * No FPGA action is assigned yet.
             * --------------------------------------------------
             */
            case DEMOD_REG_AGC_RESET:
            {
                const uint8_t agc_reset_value = command[8];

                if (agc_reset_value != DEMOD_AGC_RESET_OFF &&
                    agc_reset_value != DEMOD_AGC_RESET_ON)
                {
                    cerr << "Invalid AGC Reset value: 0x"
                         << hex
                         << uppercase
                         << setw(2)
                         << setfill('0')
                         << static_cast<unsigned int>(agc_reset_value)
                         << dec
                         << setfill(' ')
                         << endl;

                    success = false;
                    break;
                }

                cout << "AGC Reset Value : 0x"
                     << hex
                     << uppercase
                     << setw(2)
                     << setfill('0')
                     << static_cast<unsigned int>(agc_reset_value)
                     << dec
                     << setfill(' ')
                     << endl;

                cout << "AGC Reset       : "
                     << ((agc_reset_value == DEMOD_AGC_RESET_ON)
                         ? "TRUE"
                         : "FALSE")
                     << endl;

                /*
                 * No FPGA/RegisterController action yet.
                 */
                break;
            }

            /*
             * --------------------------------------------------
             * Mode Select / Sample Select
             *
             * Both commands use register address 0x85.
             *
             * Mode Select:
             *
             *   03 00 00 00 85 00 00 XX 00 00 16
             *                         ^^
             *                       command[7]
             *
             * Sample Select:
             *
             *   03 00 00 00 85 00 00 00 XX 00 16
             *                            ^^
             *                          command[8]
             *
             * Sample Select is currently receive/decode only.
             * No FPGA action is performed.
             * --------------------------------------------------
             */
            case DEMOD_REG_MODE:
            {
                const uint8_t mode_value   = command[7];
                const uint8_t sample_value = command[8];

                /*
                 * If command[7] is non-zero, this is Mode Select.
                 */
                if (mode_value != 0x00)
                {
                    if (mode_value > DEMOD_MODE_PM)
                    {
                        cerr << "Invalid Mode value: 0x"
                             << hex
                             << uppercase
                             << setw(2)
                             << setfill('0')
                             << static_cast<unsigned int>(mode_value)
                             << dec
                             << setfill(' ')
                             << endl;

                        success = false;
                        break;
                    }

                    cout << "Command         : Mode Select"
                         << endl;

                    cout << "Mode Value      : 0x"
                         << hex
                         << uppercase
                         << setw(2)
                         << setfill('0')
                         << static_cast<unsigned int>(mode_value)
                         << dec
                         << setfill(' ')
                         << endl;

                    switch (mode_value)
                    {
                        case DEMOD_MODE_AM:
                            cout << "Mode            : AM"
                                 << endl;
                            break;

                        case DEMOD_MODE_FM:
                            cout << "Mode            : FM"
                                 << endl;
                            break;

                        case DEMOD_MODE_PM:
                            cout << "Mode            : PM"
                                 << endl;
                            break;
                    }

                    controller.setModeSelect(
                        static_cast<uint32_t>(mode_value));

                    break;
                }


                /*
                 * If command[7] is zero and command[8] is non-zero,
                 * this is Sample Select.
                 *
                 * Currently receive/decode only.
                 */
                if (sample_value != 0x00)
                {
                    cout << "Command         : Sample Select"
                         << endl;

                    cout << "Sample Select   : 0x"
                         << hex
                         << uppercase
                         << setw(2)
                         << setfill('0')
                         << static_cast<unsigned int>(sample_value)
                         << dec
                         << setfill(' ')
                         << endl;

                    cout << "Sample Select   : Receive/Decode only"
                         << endl;

                    /*
                     * No FPGA/RegisterController action.
                     */

                    break;
                }


                /*
                 * Both command[7] and command[8] are zero.
                 *
                 * This is interpreted as Mode = AM.
                 */
                cout << "Command         : Mode Select"
                     << endl;

                cout << "Mode Value      : 0x00"
                     << endl;

                cout << "Mode            : AM"
                     << endl;

                controller.setModeSelect(0);

                break;
            }


            case DEMOD_REG_BANDWIDTH:
            {
                // For Bandwidth command, only command[8] is valid.
                // command[5], command[6], command[7] are ignored.
                const uint8_t bandwidth_value = command[8];

                if (bandwidth_value > 0x0B)
                {
                    cerr << "Invalid Bandwidth value: 0x"
                        << hex
                        << uppercase
                        << static_cast<unsigned int>(bandwidth_value)
                        << dec
                        << endl;

                    success = false;
                    break;
                }

                cout << "Bandwidth Value: 0x"
                    << hex
                    << uppercase
                    << static_cast<unsigned int>(bandwidth_value)
                    << dec
                    << endl;

                switch (bandwidth_value)
                {
                    case 0x00:
                        cout << "Bandwidth      : WB "
                                "(AD9361 Filter / FPGA FIR Bypass, Dec=2)"
                            << endl;
                        break;

                    case 0x01:
                        cout << "Bandwidth      : 200 kHz"
                            << endl;
                        break;

                    case 0x02:
                        cout << "Bandwidth      : 150 kHz"
                            << endl;
                        break;

                    case 0x03:
                        cout << "Bandwidth      : 60 kHz"
                            << endl;
                        break;

                    case 0x04:
                        cout << "Bandwidth      : 30 kHz"
                            << endl;
                        break;

                    case 0x05:
                        cout << "Bandwidth      : 15 kHz"
                            << endl;
                        break;

                    case 0x06:
                        cout << "Bandwidth      : 12 kHz"
                            << endl;
                        break;

                    case 0x07:
                        cout << "Bandwidth      : 7.5 kHz"
                            << endl;
                        break;

                    case 0x08:
                        cout << "Bandwidth      : 5 kHz"
                            << endl;
                        break;

                    case 0x09:
                        cout << "Bandwidth      : 3 kHz"
                            << endl;
                        break;

                    case 0x0A:
                        cout << "Bandwidth      : 1.2 kHz"
                            << endl;
                        break;

                    case 0x0B:
                        cout << "Bandwidth      : 0.6 kHz"
                            << endl;
                        break;
                }

                controller.setBandwidth(
                    static_cast<uint32_t>(bandwidth_value)
                );

                break;
            }


            /*
             * --------------------------------------------------
             * Ref1 / A Coefficient 1
             *
             * Ref1:
             *
             *   03 00 00 00 93 00 XX XX XX 00 16
             *
             * A Coeff1:
             *
             *   03 00 00 00 93 XX 00 00 00 00 16
             *
             * The byte placement distinguishes the operation.
             */
            case DEMOD_REG_REF1_A1:
            {
                // command[5]    = A_Coeff1
                // command[6..8] = Ref1[23:0]

                const uint32_t ref1 = decodeRef24(command);
                const uint32_t a_coeff1 =
                    static_cast<uint32_t>(command[5]);

                controller.setRef1(ref1);
                controller.setACoeff1(a_coeff1);

                cout << "A Coeff1        : 0x"
                    << hex << uppercase
                    << setw(2) << setfill('0')
                    << a_coeff1
                    << dec << setfill(' ')
                    << endl;

                cout << "Ref1            : 0x"
                    << hex << uppercase
                    << setw(6) << setfill('0')
                    << ref1
                    << dec << setfill(' ')
                    << endl;

                break;
            }


            /*
             * --------------------------------------------------
             * Ref2 / A Coefficient 2
             */
            case DEMOD_REG_REF2_A2:
            {
                // command[5]    = A_Coeff2
                // command[6..8] = Ref2[23:0]

                const uint32_t ref2 = decodeRef24(command);

                const uint32_t a_coeff2 =
                    static_cast<uint32_t>(command[5]);

                controller.setRef2(ref2);
                controller.setACoeff2(a_coeff2);

                cout << "A Coeff2        : 0x"
                    << hex << uppercase
                    << setw(2) << setfill('0')
                    << a_coeff2
                    << dec << setfill(' ')
                    << endl;

                cout << "Ref2            : 0x"
                    << hex << uppercase
                    << setw(6) << setfill('0')
                    << ref2
                    << dec << setfill(' ')
                    << endl;

                break;
            }

            /*
            * --------------------------------------------------
            * Demodulator Control
            *
            * 0x00 -> Mod Disable
            * 0x01 -> AGC1 On
            * 0x04 -> AGC2 On
            * 0x08 -> MGC2 On
            * 0x20 -> Mod Enable
            * 0x24 -> Squelch Disable + AGC2 + Mod Enable
            * 0x34 -> Squelch Enable + AGC2 + Mod Enable
            */
            // case DEMOD_REG_CONTROL:
            // {
            //     switch (value)
            //     {
            //     case 0x00:
            //         // Mod Disable + MGC2 OFF
            //         controller.setModEnable(false);
            //         controller.setMGC2On(false);
            //         controller.setAGC1On(false);
            //         controller.setAGC2On(false);
            //         break;

            //     case DEMOD_MGC2_ON:
            //         controller.setMGC2On(true);
            //         break;

            //     case DEMOD_AGC1_ON:
            //         controller.setAGC1On(true);
            //         break;

            //     case DEMOD_AGC2_ON:
            //         controller.setAGC2On(true);
            //         break;

            //     case DEMOD_MOD_ENABLE:
            //         controller.setModEnable(true);
            //         break;

            //     case DEMOD_SQUELCH_ENABLE:
            //         controller.setSquelchEnable(true);
            //         controller.setAGC2On(true);
            //         controller.setModEnable(true);
            //         controller.setAmpOK(true);
            //         break;

            //     case DEMOD_SQUELCH_DISABLE:
            //         controller.setSquelchEnable(false);
            //         controller.setAmpOK(false);
            //         controller.setAGC2On(true);
            //         controller.setModEnable(true);
            //         break;

            //     default:
            //         cerr << "Unknown Demodulator Control value: 0x"
            //              << hex
            //              << uppercase
            //              << value
            //              << dec
            //              << endl;

            //         success = false;
            //         break;
            //     }

            //     break;
            // }

            /*
             * --------------------------------------------------
             * Demodulator Control - Register 0x95
             *
             * Bit 0 -> AGC1
             * Bit 1 -> MGC1       (PS receive/decode only)
             * Bit 2 -> AGC2
             * Bit 3 -> MGC2
             * Bit 4 -> Squelch
             * Bit 5 -> Mod Enable
             *
             * Command format:
             *
             *   03 00 00 00 95 00 00 00 XX 00 16
             *
             * XX = command[8]
             * --------------------------------------------------
             */
            case DEMOD_REG_CONTROL:
            {
                const uint8_t control_value = command[8];

                /*
                 * Only bits 0..5 are currently defined.
                 */
                if (control_value & 0xC0)
                {
                    cerr << "Invalid Demodulator Control value: 0x"
                         << hex
                         << uppercase
                         << setw(2)
                         << setfill('0')
                         << static_cast<unsigned int>(control_value)
                         << dec
                         << setfill(' ')
                         << endl;

                    success = false;
                    break;
                }


                /*
                 * ----------------------------------------------
                 * Bit 0 -> AGC1
                 * ----------------------------------------------
                 */
                const bool agc1_on =
                    (control_value & DEMOD_CTRL_AGC1) != 0;

                controller.setAGC1On(agc1_on);


                /*
                 * ----------------------------------------------
                 * Bit 1 -> MGC1
                 *
                 * Receive/decode only.
                 * No FPGA action.
                 * ----------------------------------------------
                 */
                const bool mgc1_on =
                    (control_value & DEMOD_CTRL_MGC1) != 0;

                cout << "MGC1            : "
                     << (mgc1_on ? "ON" : "OFF")
                     << " (PS only)"
                     << endl;


                /*
                 * ----------------------------------------------
                 * Bit 2 -> AGC2
                 * ----------------------------------------------
                 */
                const bool agc2_on =
                    (control_value & DEMOD_CTRL_AGC2) != 0;

                controller.setAGC2On(agc2_on);


                /*
                 * ----------------------------------------------
                 * Bit 3 -> MGC2
                 * ----------------------------------------------
                 */
                const bool mgc2_on =
                    (control_value & DEMOD_CTRL_MGC2) != 0;

                controller.setMGC2On(mgc2_on);


                /*
                 * ----------------------------------------------
                 * Bit 4 -> Squelch
                 * ----------------------------------------------
                 */
                const bool squelch_enable =
                    (control_value & DEMOD_CTRL_SQUELCH) != 0;

                controller.setSquelchEnable(squelch_enable);
                controller.setAmpOK(squelch_enable);


                /*
                 * ----------------------------------------------
                 * Bit 5 -> Mod Enable
                 * ----------------------------------------------
                 */
                const bool mod_enable =
                    (control_value & DEMOD_CTRL_MOD) != 0;

                //controller.setModEnable(mod_enable);


                /*
                 * ----------------------------------------------
                 * Display complete decoded state
                 * ----------------------------------------------
                 */
                cout << "Control Value   : 0x"
                     << hex
                     << uppercase
                     << setw(2)
                     << setfill('0')
                     << static_cast<unsigned int>(control_value)
                     << dec
                     << setfill(' ')
                     << endl;

                cout << "AGC1            : "
                     << (agc1_on ? "ON" : "OFF")
                     << endl;

                cout << "AGC2            : "
                     << (agc2_on ? "ON" : "OFF")
                     << endl;

                cout << "MGC2            : "
                     << (mgc2_on ? "ON" : "OFF")
                     << endl;

                cout << "Squelch         : "
                     << (squelch_enable ? "ON" : "OFF")
                     << endl;

                cout << "Mod Enable      : "
                     << (mod_enable ? "ON" : "OFF")
                     << endl;

                break;
            }            

            /*
             * --------------------------------------------------
             * Message Value
             */
            case DEMOD_REG_MESSAGE:
            {
                controller.setMsgValue(value);
                break;
            }


            default:
                cerr << "Unknown Demodulator Register Address: 0x"
                     << hex
                     << uppercase
                     << setw(2)
                     << setfill('0')
                     << static_cast<unsigned int>(reg_addr)
                     << dec
                     << setfill(' ')
                     << endl;

                success = false;
                break;
        }


        controller.close();


        if (!success)
            return false;


        cout << "Demodulator command applied successfully."
             << endl;

        return true;
    }


    /*
     * ----------------------------------------------------------
     * DIRECT / SYSTEM COMMANDS
     * ----------------------------------------------------------
     */
    RegisterController controller;

    if (!controller.open())
    {
        cerr << "Command Dispatcher: failed to open Register Controller."
             << endl;

        return false;
    }


    cout << "Direct Command    : "
         << getDirectCommandName(module_or_command)
         << endl;


    bool success = true;


    switch (module_or_command)
    {
        case CMD_SOFT_RESET:
        {
            if (value != 0x00000000 &&
                value != 0x00000001)
            {
                cerr << "Invalid Soft Reset value."
                     << endl;

                success = false;
                break;
            }

            controller.setSoftReset(value != 0);

            break;
        }


        // case CMD_SOFT_START:
        // {
        //     if (value != 0x00000000 &&
        //         value != 0x00000001)
        //     {
        //         cerr << "Invalid Soft Start value."
        //              << endl;

        //         success = false;
        //         break;
        //     }

        //     controller.setSoftStart(value != 0);

        //     break;
        // }

        case CMD_MOD_ENABLE:
        {
            if (value != 0 && value != 1)
            {
                cout << "Invalid Mod Enable value: "
                    << value
                    << endl;

                controller.close();
                return false;
            }

            const bool mod_enable = (value != 0);

            cout << "Direct Command    : Mod Enable" << endl;
            cout << "Mod Enable        : "
                << (mod_enable ? "ON" : "OFF")
                << endl;

            /*
            * ----------------------------------------------------------
            * Update FPGA Mod Enable register
            * ----------------------------------------------------------
            */
            controller.setModEnable(mod_enable);

            /*
            * ----------------------------------------------------------
            * Start / Stop continuous Audio streaming
            *
            * Mod Enable ON  -> Start background Audio DMA
            * Mod Enable OFF -> Stop background Audio DMA
            * ----------------------------------------------------------
            */
            if (mod_enable)
            {
                cout << endl;
                cout << "[AUDIO] Mod Enable ON -> "
                    << "starting continuous Audio streaming..."
                    << endl;

                if (!audio_stream_start(nullptr))
                {
                    cout << "[AUDIO] Failed to start Audio streaming."
                        << endl;

                    controller.close();
                    return false;
                }
            }
            else
            {
                cout << endl;
                cout << "[AUDIO] Mod Enable OFF -> "
                    << "stopping Audio streaming..."
                    << endl;

                if (!audio_stream_stop())
                {
                    cout << "[AUDIO] Failed to stop Audio streaming."
                        << endl;

                    controller.close();
                    return false;
                }
            }

            break;
        } 

        case CMD_SEND_PACKET:
        {
            if (value != 0x00000000 &&
                value != 0x00000001)
            {
                cerr << "Invalid Send Packet value."
                     << endl;

                success = false;
                break;
            }

            controller.setSendPacket(value != 0);

            break;
        }


        case CMD_LED_TIMER:
        {
            controller.setLedTimer(value);
            break;
        }


        default:
        {
            cerr << "Unknown L2 Command Address: 0x"
                 << hex
                 << uppercase
                 << setw(2)
                 << setfill('0')
                 << static_cast<unsigned int>(module_or_command)
                 << dec
                 << setfill(' ')
                 << endl;

            success = false;
            break;
        }
    }


    controller.close();


    if (!success)
        return false;


    cout << "L2 command applied successfully."
         << endl;

    return true;
}