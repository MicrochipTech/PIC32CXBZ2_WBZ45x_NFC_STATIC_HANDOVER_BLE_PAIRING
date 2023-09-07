/******************************************************************************
  * @attention
  *
  * COPYRIGHT 2019 STMicroelectronics, all rights reserved
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied,
  * AND SPECIFICALLY DISCLAIMING THE IMPLIED WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
******************************************************************************/


/*
 *      PROJECT:   NDEF firmware
 *      Revision:
 *      LANGUAGE:  ISO C99
 */

/*! \file
 *
 *  \author
 *
 *  \brief NDEF message dump utils
 *
 */

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */

#include <stdio.h>
#include "st_errno.h"
#include "utils.h"
#include "NDEF/message/ndef_message.h"
#include "NDEF/message/ndef_record.h"
#include "NDEF/message/ndef_types.h"
#include "ndef_dump.h"


/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

/*! Type to associate enums to pointer to function */
typedef struct
{
    ndefTypeId typeId;                        /*!< NDEF type Id             */
    ndefStatus (*dump)(const ndefType* type); /*!< Pointer to dump function */
} ndefTypeDumpTable;


#if NDEF_TYPE_VCARD_SUPPORT
/*! Type to associate a property and string description */
typedef struct
{
    ndefConstBuffer bufType;
    char* string;
} ndefVCardTypeTable;
#endif


#if NDEF_TYPE_BLUETOOTH_SUPPORT
/*! Type to associate EIRs and string description */
typedef struct
{
    uint8_t eirType;
    char* string;
} ndefEirTable;
#endif


/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */


/*
 ******************************************************************************
 * LOCAL FUNCTION PROTOTYPES
 ******************************************************************************
 */


/*
 ******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************
 */


/*****************************************************************************/
static bool isPrintableASCII(const uint8_t* str, uint32_t strLen)
{
    if ((str == NULL) || (strLen == 0U))
    {
        return false;
    }

    for (uint32_t i = 0; i < strLen; i++)
    {
        if ((str[i] < 0x20U) || (str[i] > 0x7EU))
        {
            return false;
        }
    }

    return true;
}


/*****************************************************************************/
ndefStatus ndefRecordDump(const ndefRecord* record, bool verbose)
{
    static uint32_t index;
    static const uint8_t *ndefTNFNames[] =
    {
        (uint8_t *)"Empty",
        (uint8_t *)"NFC Forum well-known type [NFC RTD]",
        (uint8_t *)"Media-type as defined in RFC 2046",
        (uint8_t *)"Absolute URI as defined in RFC 3986",
        (uint8_t *)"NFC Forum external type [NFC RTD]",
        (uint8_t *)"Unknown",
        (uint8_t *)"Unchanged",
        (uint8_t *)"Reserved"
    };
    uint8_t* headerSR = (uint8_t*)"";
    ndefStatus err;

    if (record == NULL)
    {
        SYS_CONSOLE_MESSAGE("No record\r\n");
        return ERR_NONE;
    }

    if (ndefHeaderIsSetMB(record))
    {
        index = 1U;
    }
    else
    {
        index++;
    }

    if (verbose == true)
    {
        headerSR = (uint8_t*)(ndefHeaderIsSetSR(record) ? " - Short Record" : " - Standard Record");
    }

    SYS_CONSOLE_PRINT("Record #%d%s\r\n", index, headerSR);

    /* Well-known type dump */
    err = ndefRecordDumpType(record); // TODO Move this below
    if (verbose == true)
    {
        /* Raw dump */
        //platformLog(" MB:%d ME:%d CF:%d SR:%d IL:%d TNF:%d\r\n", ndefHeaderMB(record), ndefHeaderME(record), ndefHeaderCF(record), ndefHeaderSR(record), ndefHeaderIL(record), ndefHeaderTNF(record));
        SYS_CONSOLE_MESSAGE(" MB ME CF SR IL TNF\r\n");
        SYS_CONSOLE_PRINT("  %d  %d  %d  %d  %d   %d\r\n", ndefHeaderMB(record), ndefHeaderME(record), ndefHeaderCF(record), ndefHeaderSR(record), ndefHeaderIL(record), ndefHeaderTNF(record));
    }
    if ( (err != ERR_NONE) || (verbose == true) )
    {
        SYS_CONSOLE_PRINT(" Type Name Format: %s\r\n", ndefTNFNames[ndefHeaderTNF(record)]);

        uint8_t tnf;
        ndefConstBuffer8 bufRecordType;
        ndefRecordGetType(record, &tnf, &bufRecordType);
        if ( (tnf == NDEF_TNF_EMPTY) && (bufRecordType.length == 0U) )
        {
            SYS_CONSOLE_MESSAGE(" Empty NDEF record\r\n");
        }
        else
        {
            ndefBuffer8Print(" Type: \"", &bufRecordType, "\"\r\n");
        }

        if (ndefHeaderIsSetIL(record))
        {
            /* ID Length bit set */
            ndefConstBuffer8 bufRecordId;
            ndefRecordGetId(record, &bufRecordId);
            ndefBuffer8Print(" ID: \"", &bufRecordId, "\"\r\n");
        }

        ndefConstBuffer bufRecordPayload;
        ndefRecordGetPayload(record, &bufRecordPayload);
        ndefBufferDump(" Payload:", &bufRecordPayload, verbose);
        if (ndefRecordGetPayloadLength(record) != bufRecordPayload.length)
        {
            SYS_CONSOLE_MESSAGE(" Payload stored as a well-known type\r\n");
        }
    }

    return ERR_NONE;
}


/*****************************************************************************/
ndefStatus ndefMessageDump(const ndefMessage* message, bool verbose)
{
    ndefStatus  err;
    ndefRecord* record;

    if (message == NULL)
    {
        SYS_CONSOLE_MESSAGE("Empty NDEF message\r\n");
        return ERR_NONE;
    }
    else
    {
        SYS_CONSOLE_MESSAGE("Decoding NDEF message\r\n");
    }

    record = ndefMessageGetFirstRecord(message);

    while (record != NULL)
    {
        err = ndefRecordDump(record, verbose);
        if (err != ERR_NONE)
        {
            return err;
        }

        record = ndefMessageGetNextRecord(record);
    }

    return ERR_NONE;
}


#if NDEF_TYPE_FLAT_SUPPORT
/*****************************************************************************/
ndefStatus ndefFlatPayloadTypeDump(const ndefType* type)
{
    if ( (type == NULL) || (type->id != NDEF_TYPE_ID_FLAT) )
    {
        return ERR_PARAM;
    }

    ndefBufferDump("Flat payload", (ndefConstBuffer*)&type->data.bufPayload, "");

    return ERR_NONE;
}
#endif


#if NDEF_TYPE_EMPTY_SUPPORT
/*****************************************************************************/
ndefStatus ndefEmptyTypeDump(const ndefType* type)
{
    if ( (type == NULL) || (type->id != NDEF_TYPE_ID_EMPTY) )
    {
        return ERR_PARAM;
    }

    SYS_CONSOLE_MESSAGE(" Empty record\r\n");

    return ERR_NONE;
}
#endif


#if NDEF_TYPE_RTD_DEVICE_INFO_SUPPORT
/*****************************************************************************/
ndefStatus ndefRtdDeviceInfoDump(const ndefType* type)
{
    ndefTypeRtdDeviceInfo devInfoData;

    static const uint8_t* ndefDeviceInfoName[] =
    {
        (uint8_t*)"Manufacturer",
        (uint8_t*)"Model",
        (uint8_t*)"Device",
        (uint8_t*)"UUID",
        (uint8_t*)"Firmware version",
    };

    ndefStatus err = ndefGetRtdDeviceInfo(type, &devInfoData);
    if (err != ERR_NONE)
    {
        return err;
    }

    SYS_CONSOLE_MESSAGE(" Device Information:\r\n");

    for (uint32_t info = 0; info < NDEF_DEVICE_INFO_TYPE_COUNT; info++)
    {
        if (devInfoData.devInfo[info].buffer != NULL)
        {
            SYS_CONSOLE_PRINT(" - %s: ", ndefDeviceInfoName[devInfoData.devInfo[info].type]);

            if (info != NDEF_DEVICE_INFO_UUID)
            {
                for (uint32_t i = 0; i < devInfoData.devInfo[info].length; i++)
                {
                    SYS_CONSOLE_PRINT("%c", devInfoData.devInfo[info].buffer[i]); /* character */
                }
            }
            else
            {
                for (uint32_t i = 0; i < devInfoData.devInfo[info].length; i++)
                {
                    SYS_CONSOLE_PRINT("%.2X", devInfoData.devInfo[info].buffer[i]); /* hex number */
                }
            }
            SYS_CONSOLE_MESSAGE("\r\n");
        }
    }

    return ERR_NONE;
}
#endif


#if NDEF_TYPE_RTD_TEXT_SUPPORT
/*****************************************************************************/
ndefStatus ndefRtdTextDump(const ndefType* type)
{
    uint8_t utfEncoding;
    ndefConstBuffer8 bufLanguageCode;
    ndefConstBuffer  bufSentence;

    ndefStatus err = ndefGetRtdText(type, &utfEncoding, &bufLanguageCode, &bufSentence);
    if (err != ERR_NONE)
    {
        return err;
    }

    ndefBufferPrint(" Text: \"", &bufSentence, "");

    SYS_CONSOLE_PRINT("\" (%s,", (utfEncoding == TEXT_ENCODING_UTF8) ? "UTF8" : "UTF16");

    ndefBuffer8Print(" language code \"", &bufLanguageCode, "\")\r\n");

    return ERR_NONE;
}
#endif


#if NDEF_TYPE_RTD_URI_SUPPORT
/*****************************************************************************/
ndefStatus ndefRtdUriDump(const ndefType* type)
{
    ndefConstBuffer bufProtocol;
    ndefConstBuffer bufUriString;

    ndefStatus err = ndefGetRtdUri(type, &bufProtocol, &bufUriString);
    if (err != ERR_NONE)
    {
        return err;
    }

    ndefBufferPrint(" URI: (", &bufProtocol, ")");
    ndefBufferPrint("", &bufUriString, "\r\n");

    return ERR_NONE;
}
#endif


#if NDEF_TYPE_RTD_AAR_SUPPORT
/*****************************************************************************/
ndefStatus ndefRtdAarDump(const ndefType* type)
{
    ndefConstBuffer bufAarString;

    ndefStatus err = ndefGetRtdAar(type, &bufAarString);
    if (err != ERR_NONE)
    {
        return err;
    }

    ndefBufferPrint(" AAR Package: ", &bufAarString, "\r\n");

    return ERR_NONE;
}
#endif


#if NDEF_TYPE_RTD_WLC_SUPPORT
/*****************************************************************************/
ndefStatus ndefRtdWlcCapabilityDump(const ndefType* type)
{
    ndefTypeRtdWlcCapability wlcCap;

    ndefStatus err = ndefGetRtdWlcCapability(type, &wlcCap);
    if (err != ERR_NONE)
    {
        return err;
    }

    SYS_CONSOLE_MESSAGE(" WLC Capabilities: \r\n");
    SYS_CONSOLE_PRINT(" WLC Protocol Version: 0x%.2X\r\n",         wlcCap.wlcProtocolVersion);
    SYS_CONSOLE_PRINT(" WLC Config - Mode Req: 0x%.2X\r\n",        wlcCap.wlcConfigModeReq);
    SYS_CONSOLE_PRINT(" WLC Config - Wait Time Retry: 0x%.2X\r\n", wlcCap.wlcConfigWaitTimeRetry);
    SYS_CONSOLE_PRINT(" WLC Config - Nego Wait: 0x%.2X\r\n",       wlcCap.wlcConfigNegoWait);
    SYS_CONSOLE_PRINT(" WLC Config - Rd Conf: 0x%.2X\r\n",         wlcCap.wlcConfigRdConf);
    SYS_CONSOLE_PRINT(" WLC CapWtIntRfu: 0x%.2X\r\n",              wlcCap.capWtIntRfu);
    SYS_CONSOLE_PRINT(" WLC CapWtInt: 0x%.2X\r\n",                 wlcCap.capWtInt);
    SYS_CONSOLE_PRINT(" WLC NDEF Rd Wt: 0x%.2X\r\n",               wlcCap.ndefRdWt);
    SYS_CONSOLE_PRINT(" WLC NDEF Write To Int: 0x%.2X\r\n",        wlcCap.ndefWriteToInt);
    SYS_CONSOLE_PRINT(" WLC NDEF Write Wt Int: 0x%.2X\r\n",        wlcCap.ndefWriteWtInt);

    return ERR_NONE;
}


/*****************************************************************************/
ndefStatus ndefRtdWlcStatusInfoDump(const ndefType* type)
{
    ndefTypeRtdWlcStatusInfo wlcStatusInfo;

    ndefStatus err = ndefGetRtdWlcStatusInfo(type, &wlcStatusInfo);
    if (err != ERR_NONE)
    {
        return err;
    }

    SYS_CONSOLE_MESSAGE(" WLC Status Info: \r\n");
    SYS_CONSOLE_PRINT(" Control byte 1: 0x%.2X ",         wlcStatusInfo.controlByte1);

    bool bat = wlcStatusInfo.controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_BATTERY_LEVEL_MASK;
    bool pow = wlcStatusInfo.controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_RECEIVE_POWER_MASK;
    bool vol = wlcStatusInfo.controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_RECEIVE_VOLTAGE_MASK;
    bool cur = wlcStatusInfo.controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_RECEIVE_CURRENT_MASK;
    bool tpb = wlcStatusInfo.controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_TEMPERATURE_BATTERY_MASK;
    bool tpw = wlcStatusInfo.controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_TEMPERATURE_WLCL_MASK;
    bool rfu = wlcStatusInfo.controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_RFU_MASK;
    bool cb2 = wlcStatusInfo.controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_CONTROL_BYTE_2_MASK;
    SYS_CONSOLE_PRINT("(0b%d%d%d%d%d%d%d%d)\r\n", cb2, rfu, tpw, tpb, cur, vol, pow, bat);

    SYS_CONSOLE_PRINT(" Battery level : 0x%.2X\r\n",      wlcStatusInfo.batteryLevel);
    SYS_CONSOLE_PRINT(" Receive power: 0x%.2X\r\n",       wlcStatusInfo.receivePower);
    SYS_CONSOLE_PRINT(" Receive voltage: 0x%.2X\r\n",     wlcStatusInfo.receiveVoltage);
    SYS_CONSOLE_PRINT(" Receive current: 0x%.2X\r\n",     wlcStatusInfo.receiveCurrent);
    SYS_CONSOLE_PRINT(" Temperature battery: 0x%.2X\r\n", wlcStatusInfo.temperatureBattery);
    SYS_CONSOLE_PRINT(" Temperature WLCL: 0x%.2X\r\n",    wlcStatusInfo.temperatureWlcl);
    SYS_CONSOLE_PRINT(" RFU: 0x%.2X\r\n",                 wlcStatusInfo.rfu);
    SYS_CONSOLE_PRINT(" Control byte 2: 0x%.2X\r\n",      wlcStatusInfo.controlByte2);

    return ERR_NONE;
}


/*****************************************************************************/
ndefStatus ndefRtdWlcInfoDump(const ndefType* type)
{
    ndefTypeRtdWlcPollInfo wlcPollInfo;

    ndefStatus err = ndefGetRtdWlcPollInfo(type, &wlcPollInfo);
    if (err != ERR_NONE)
    {
        return err;
    }

    SYS_CONSOLE_MESSAGE(" WLC Poll Info: \r\n");
    SYS_CONSOLE_PRINT(" WLC P Tx: 0x%.2X\r\n",              wlcPollInfo.pTx);
    SYS_CONSOLE_PRINT(" WLC WLC-P Cap: 0x%.2X\r\n",         wlcPollInfo.wlcPCap);
    SYS_CONSOLE_PRINT(" WLC Power Class: 0x%.2X\r\n",       wlcPollInfo.powerClass);
    SYS_CONSOLE_PRINT(" WLC tot Power State: 0x%.2X\r\n",   wlcPollInfo.totPowerSteps);
    SYS_CONSOLE_PRINT(" WLC cur Power State: 0x%.2X\r\n",   wlcPollInfo.curPowerStep);
    SYS_CONSOLE_PRINT(" WLC next Min Step Inc: 0x%.2X\r\n", wlcPollInfo.nextMinStepInc);
    SYS_CONSOLE_PRINT(" WLC next Min Step Dec: 0x%.2X\r\n", wlcPollInfo.nextMinStepDec);

    return ERR_NONE;
}


/*****************************************************************************/
ndefStatus ndefRtdWlcControlDump(const ndefType* type)
{
    ndefTypeRtdWlcListenCtl wlcListenCtl;

    ndefStatus err = ndefGetRtdWlcListenCtl(type, &wlcListenCtl);
    if (err != ERR_NONE)
    {
        return err;
    }

    SYS_CONSOLE_MESSAGE(" WLC Listen Control: \r\n");
    SYS_CONSOLE_PRINT(" WLC Status Info - Error Flag: 0x%.2X\r\n",     wlcListenCtl.statusInfoErrorFlag);
    SYS_CONSOLE_PRINT(" WLC Status Info - Baterry Status: 0x%.2X\r\n", wlcListenCtl.statusInfoBatteryStatus);
    SYS_CONSOLE_PRINT(" WLC Status Info - Cnt: 0x%.2X\r\n",            wlcListenCtl.statusInfoCnt);
    SYS_CONSOLE_PRINT(" WLC WPT Config - WPT Req: 0x%.2X\r\n",         wlcListenCtl.wptConfigWptReq);
    SYS_CONSOLE_PRINT(" WLC WPT Config - WPT Duration: 0x%.2X\r\n",    wlcListenCtl.wptConfigWptDuration);
    SYS_CONSOLE_PRINT(" WLC WPT Config - Info Req: 0x%.2X\r\n",        wlcListenCtl.wptConfigInfoReq);
    SYS_CONSOLE_PRINT(" WLC Power Adj Req: 0x%.2X\r\n",                wlcListenCtl.powerAdjReq);
    SYS_CONSOLE_PRINT(" WLC Battery Level: 0x%.2X\r\n",                wlcListenCtl.batteryLevel);
    SYS_CONSOLE_PRINT(" WLC Drv Info - Flag: 0x%.2X\r\n",              wlcListenCtl.drvInfoFlag);
    SYS_CONSOLE_PRINT(" WLC Drv Info - Int: 0x%.2X\r\n",               wlcListenCtl.drvInfoInt);
    SYS_CONSOLE_PRINT(" WLC Hold Off Wt Int: 0x%.2X\r\n",              wlcListenCtl.holdOffWtInt);
    SYS_CONSOLE_PRINT(" WLC Error Info - Error: 0x%.2X\r\n",           wlcListenCtl.errorInfoError);
    SYS_CONSOLE_PRINT(" WLC Error Info - Temperature: 0x%.2X\r\n",     wlcListenCtl.errorInfoTemperature);

    return ERR_NONE;
}
#endif


#if NDEF_TYPE_RTD_WPCWLC_SUPPORT
/*****************************************************************************/
ndefStatus ndefRtdWpcWlcDump(const ndefType* type)
{
    ndefConstBuffer wpcWlc;

    static const uint8_t* ndefKiData[NDEF_KI_V10_PAYLOAD_LENGTH] =
    {
        (uint8_t*)"Application Profile",
        (uint8_t*)"Version",
        (uint8_t*)"Alive FDT",
        (uint8_t*)"Read Data Start Address",
        (uint8_t*)"Write Data Start Address",
        (uint8_t*)"Read Data Buffer Size",
        (uint8_t*)"Write Data Buffer Size",
        (uint8_t*)"Read Command Code",
        (uint8_t*)"Write Commande Code",
        (uint8_t*)"Maximum T_SLOT FOD",
        (uint8_t*)"Minimum T_POWER",
        (uint8_t*)"T_SUSPEND",
        (uint8_t*)"T_COMM_LAG,MAX",
        (uint8_t*)"Write Sequence Length",
        (uint8_t*)"Minimum Power",
        (uint8_t*)"Maximum Power",
    };

    ndefStatus err = ndefGetRtdWpcWlc(type, &wpcWlc);
    if (err != ERR_NONE)
    {
        return err;
    }

    SYS_CONSOLE_MESSAGE(" WPC WLC: \r\n");
    uint8_t appliProfile = wpcWlc.buffer[NDEF_KI_APPLICATION_PROFILE_OFFSET];
    if ( (appliProfile  == NDEF_KI_APPLICATION_PROFILE) &&
         (wpcWlc.length != NDEF_KI_V10_PAYLOAD_LENGTH) )
    {
        SYS_CONSOLE_MESSAGE(" Unexpected buffer length, decoding known fields...\r\n");
    }

    uint32_t len = (wpcWlc.length < NDEF_KI_V10_PAYLOAD_LENGTH) ? wpcWlc.length : NDEF_KI_V10_PAYLOAD_LENGTH;

    for (uint32_t i = 0; i < len; i++)
    {
        SYS_CONSOLE_PRINT(" Ki %s: 0x%.2X\r\n", ndefKiData[i], wpcWlc.buffer[i]);
    }
    for (uint32_t i = len; i < wpcWlc.length; i++)
    {
        SYS_CONSOLE_PRINT(" Ki payload[%d]: 0x%.2X\r\n", i, wpcWlc.buffer[i]);
    }

    return ERR_NONE;
}
#endif


#if NDEF_TYPE_RTD_TNEP_SUPPORT
/*****************************************************************************/
ndefStatus ndefRtdTnepServiceParamDump(const ndefType* type)
{
    uint8_t  tnepVersion;
    ndefConstBuffer bufServiceUri;
    uint8_t  comMode;
    uint8_t  minWaitingTime;
    uint8_t  maxExtensions;
    uint16_t maxMessageSize;

    ndefStatus err = ndefGetRtdTnepServiceParameter(type, &tnepVersion, &bufServiceUri, &comMode, &minWaitingTime, &maxExtensions, &maxMessageSize);
    if (err != ERR_NONE)
    {
        return err;
    }

    SYS_CONSOLE_MESSAGE(" TNEP Service Parameter: \r\n");
    SYS_CONSOLE_PRINT(" TNEP version: 0x%.2X\r\n", tnepVersion);
    ndefBufferPrint(" TNEP Service URI: ", &bufServiceUri, "\r\n");
    SYS_CONSOLE_PRINT(" TNEP communication mode: 0x%.2X\r\n", comMode);
    SYS_CONSOLE_PRINT(" TNEP min Waiting Time: 0x%.2X\r\n", minWaitingTime);
    SYS_CONSOLE_PRINT(" TNEP max Extensions:  0x%.2X\r\n", maxExtensions);
    SYS_CONSOLE_PRINT(" TNEP max Message Size: 0x%.4X\r\n", maxMessageSize);

    return ERR_NONE;
}


/*****************************************************************************/
ndefStatus ndefRtdTnepServiceSelectDump(const ndefType* type)
{
    ndefConstBuffer bufServiceUri;

    ndefStatus err = ndefGetRtdTnepServiceSelect(type, &bufServiceUri);
    if (err != ERR_NONE)
    {
        return err;
    }

    SYS_CONSOLE_MESSAGE(" TNEP Service Select: \r\n");
    ndefBufferPrint(" TNEP Service URI: ", &bufServiceUri, "\r\n");

    return ERR_NONE;
}


/*****************************************************************************/
ndefStatus ndefRtdTnepStatusDump(const ndefType* type)
{
    uint8_t statusType;

    ndefStatus err = ndefGetRtdTnepStatus(type, &statusType);
    if (err != ERR_NONE)
    {
        return err;
    }

    SYS_CONSOLE_MESSAGE(" TNEP Status: \r\n");
    SYS_CONSOLE_PRINT(" TNEP Status type: 0x%.2X\r\n", statusType);

    return ERR_NONE;
}
#endif


#if NDEF_TYPE_MEDIA_SUPPORT
/*****************************************************************************/
ndefStatus ndefMediaTypeDump(const ndefType* type)
{
    ndefConstBuffer8 bufType;
    ndefConstBuffer  bufPayload;

    ndefStatus err = ndefGetMedia(type, &bufType, &bufPayload);
    if (err != ERR_NONE)
    {
        return err;
    }

    ndefBuffer8Print(" Media Type: ", &bufType, "\r\n");
    ndefBufferPrint(" Payload: ", &bufPayload, "\r\n");

    return ERR_NONE;
}
#endif


#if NDEF_TYPE_BLUETOOTH_SUPPORT
/*****************************************************************************/
ndefStatus ndefBluetoothEirDump(const char* string, const uint8_t* eir, bool verbose)
{
    char message[128];
    if (eir == NULL)
    {
        SYS_CONSOLE_PRINT(" EIR%s None\r\n", string ? string : "");
        return ERR_PARAM;
    }

    uint8_t eir_length  = ndefBluetoothEirLength(eir);
    uint8_t data_length = ndefBluetoothEirDataLength(eir);
    uint8_t type        = ndefBluetoothEirType(eir);
    const uint8_t *data = ndefBluetoothEirData((uint8_t*)eir);

    ndefConstBuffer bufEir = { data, data_length };

    snprintf(message, sizeof(message), " EIR%s (EIR length: 0x%.2X, EIR type: 0x%.2X)", string, eir_length, type);

    return ndefBufferDump(message, &bufEir, verbose);
}


/*****************************************************************************/
ndefStatus ndefBluetoothDump(const ndefType* type)
{
    ndefTypeBluetooth bluetooth;
    bool verbose = true;

    const ndefEirTable EIR_list[] =
    {
        { NDEF_BT_EIR_FLAGS                            , " Flags:" },
        { NDEF_BT_EIR_SERVICE_CLASS_UUID_PARTIAL_16    , " ClassUUID16_partial:" },
        { NDEF_BT_EIR_SERVICE_CLASS_UUID_COMPLETE_16   , " ClassUUID16:" },
        { NDEF_BT_EIR_SERVICE_CLASS_UUID_PARTIAL_32    , " ClassUUID32_partial:" },
        { NDEF_BT_EIR_SERVICE_CLASS_UUID_COMPLETE_32   , " ClassUUID32:" },
        { NDEF_BT_EIR_SERVICE_CLASS_UUID_PARTIAL_128   , " ClassUUID128_partial:" },
        { NDEF_BT_EIR_SERVICE_CLASS_UUID_COMPLETE_128  , " ClassUUID128:" },
        { NDEF_BT_EIR_SHORT_LOCAL_NAME                 , " Short Local Name:" },
        { NDEF_BT_EIR_COMPLETE_LOCAL_NAME              , " Local Name:" },
        { NDEF_BT_EIR_TX_POWER_LEVEL                   , " TxPowerLevel:" },
        { NDEF_BT_EIR_DEVICE_CLASS                     , " Device Class:" },
        { NDEF_BT_EIR_SIMPLE_PAIRING_HASH              , " SimplePairingHash:" },
        { NDEF_BT_EIR_SIMPLE_PAIRING_RANDOMIZER        , " SimplePairingRandomizer:" },
        { NDEF_BT_EIR_SECURITY_MANAGER_TK_VALUE        , " SecurityManagerTK:" },
        { NDEF_BT_EIR_SECURITY_MANAGER_FLAGS           , " Security Manager Flags:" },
        { NDEF_BT_EIR_SLAVE_CONNECTION_INTERVAL_RANGE  , " SlaveConnIntervalRange:" },
        { NDEF_BT_EIR_SERVICE_SOLICITATION_16          , " ServiceSolicitation16:" },
        { NDEF_BT_EIR_SERVICE_SOLICITATION_128         , " ServiceSolicitation128:" },
        { NDEF_BT_EIR_SERVICE_DATA                     , " ServiceData:" },
        { NDEF_BT_EIR_APPEARANCE                       , " Appearance:" },
        { NDEF_BT_EIR_LE_DEVICE_ADDRESS                , " LE Device Address:" },
        { NDEF_BT_EIR_LE_ROLE                          , " Role:" },
        { NDEF_BT_EIR_LE_SECURE_CONN_CONFIRMATION_VALUE, " Secure Connection Confirmation Value:" },
        { NDEF_BT_EIR_LE_SECURE_CONN_RANDOM_VALUE      , " Secure Connection Random Value:" },
        { NDEF_BT_EIR_MANUFACTURER_DATA                , " Manufacturer Data:" },
    };

    if (type == NULL)
    {
        return ERR_PARAM;
    }

    ndefStatus err = ndefGetBluetooth(type, &bluetooth);
    if (err != ERR_NONE)
    {
        return err;
    }

    SYS_CONSOLE_MESSAGE(" Bluetooth:\r\n");
    SYS_CONSOLE_PRINT(" Type: 0x%.2X\r\n", type->id);

    /* bufDeviceAddress is reversed, use ndefBluetoothGetEirDataReversed() to overcome this */
    ndefBufferDump(" Device Address:", &bluetooth.bufDeviceAddress, false);

    const uint8_t* eir;
    for (uint32_t i = 0; i < SIZEOF_ARRAY(EIR_list); i++)
    {
        eir = ndefBluetoothGetEir(&bluetooth, EIR_list[i].eirType);
        if ( (eir != NULL) || verbose )
        {
            ndefBluetoothEirDump(EIR_list[i].string, eir, false);
        }
    }

    eir = ndefBluetoothGetEir(&bluetooth, NDEF_BT_EIR_SERVICE_CLASS_UUID_COMPLETE_16);
    if ( (eir != NULL) || verbose )
    {
        SYS_CONSOLE_PRINT(" nbUUID16: %d\r\n", ndefBluetoothEirDataLength(eir) / 2U);
    }
    eir = ndefBluetoothGetEir(&bluetooth, NDEF_BT_EIR_SERVICE_CLASS_UUID_COMPLETE_32);
    if ( (eir != NULL) || verbose )
    {
        SYS_CONSOLE_PRINT(" nbUUID32: %d\r\n", ndefBluetoothEirDataLength(eir) / 4U);
    }
    eir = ndefBluetoothGetEir(&bluetooth, NDEF_BT_EIR_SERVICE_CLASS_UUID_COMPLETE_128);
    if ( (eir != NULL) || verbose )
    {
        SYS_CONSOLE_PRINT(" nbUUID128:%d\r\n", ndefBluetoothEirDataLength(eir) / 16U);
    }

    eir = ndefBluetoothGetEir(&bluetooth, NDEF_BT_EIR_SERVICE_SOLICITATION_16);
    if ( (eir != NULL) || verbose )
    {
        SYS_CONSOLE_PRINT(" nbServiceSolicitation16: 0x%.2X\r\n", ndefBluetoothEirDataLength(eir) / 2U);
    }
    eir = ndefBluetoothGetEir(&bluetooth, NDEF_BT_EIR_SERVICE_SOLICITATION_128);
    if ( (eir != NULL) || verbose )
    {
        SYS_CONSOLE_PRINT(" nbServiceSolicitation128:0x%.2X\r\n", ndefBluetoothEirDataLength(eir) / 16U);
    }

    return ERR_NONE;
}
#endif


#if NDEF_TYPE_VCARD_SUPPORT
/*****************************************************************************/
ndefStatus ndefMediaVCardDump(const ndefType* type)
{
    ndefTypeVCard vCard;

    const uint8_t NDEF_VCARD_N[]     = "N";
    const uint8_t NDEF_VCARD_FN[]    = "FN";
    const uint8_t NDEF_VCARD_ADR[]   = "ADR";
    const uint8_t NDEF_VCARD_TEL[]   = "TEL";
    const uint8_t NDEF_VCARD_EMAIL[] = "EMAIL";
    const uint8_t NDEF_VCARD_TITLE[] = "TITLE";
    const uint8_t NDEF_VCARD_ORG[]   = "ORG";
    const uint8_t NDEF_VCARD_URL[]   = "URL";
    const uint8_t NDEF_VCARD_PHOTO[] = "PHOTO";

    const ndefConstBuffer bufVCard_N     = { NDEF_VCARD_N,     sizeof(NDEF_VCARD_N)     - 1U };
    const ndefConstBuffer bufVCard_FN    = { NDEF_VCARD_FN,    sizeof(NDEF_VCARD_FN)    - 1U };
    const ndefConstBuffer bufVCard_ADR   = { NDEF_VCARD_ADR,   sizeof(NDEF_VCARD_ADR)   - 1U };
    const ndefConstBuffer bufVCard_TEL   = { NDEF_VCARD_TEL,   sizeof(NDEF_VCARD_TEL)   - 1U };
    const ndefConstBuffer bufVCard_EMAIL = { NDEF_VCARD_EMAIL, sizeof(NDEF_VCARD_EMAIL) - 1U };
    const ndefConstBuffer bufVCard_TITLE = { NDEF_VCARD_TITLE, sizeof(NDEF_VCARD_TITLE) - 1U };
    const ndefConstBuffer bufVCard_ORG   = { NDEF_VCARD_ORG,   sizeof(NDEF_VCARD_ORG)   - 1U };
    const ndefConstBuffer bufVCard_URL   = { NDEF_VCARD_URL,   sizeof(NDEF_VCARD_URL)   - 1U };
    const ndefConstBuffer bufVCard_PHOTO = { NDEF_VCARD_PHOTO, sizeof(NDEF_VCARD_PHOTO) - 1U };

    const ndefVCardTypeTable vCardType[] =
    {
        { bufVCard_N            , "Name"           },
        { bufVCard_FN           , "Formatted Name" },
        { bufVCard_ADR          , "Address"        },
        { bufVCard_TEL          , "Phone"          },
        { bufVCard_EMAIL        , "Email"          },
        { bufVCard_TITLE        , "Title"          },
        { bufVCard_ORG          , "Org"            },
        { bufVCard_URL          , "URL"            },
        { bufVCard_PHOTO        , "Photo"          },
    };

    ndefStatus err = ndefGetVCard(type, &vCard);
    if (err != ERR_NONE)
    {
        return err;
    }

    SYS_CONSOLE_MESSAGE(" vCard decoded: \r\n");

    for (uint32_t i = 0; i < SIZEOF_ARRAY(vCardType); i++)
    {
        ndefConstBuffer bufProperty;
        err = ndefVCardGetProperty(&vCard, &vCardType[i].bufType, &bufProperty);
        if (err == ERR_NONE)
        {
            ndefConstBuffer bufType, bufSubtype, bufValue;
            err = ndefVCardParseProperty(&bufProperty, &bufType, &bufSubtype, &bufValue);
            if (err != ERR_NONE)
            {
                SYS_CONSOLE_PRINT("ndefVCardParseProperty error %d\r\n", err);
            }

            if (bufValue.buffer != NULL)
            {
                /* Type */
                SYS_CONSOLE_PRINT(" %s", vCardType[i].string);

                /* Subtype, if any */
                if (bufSubtype.buffer != NULL)
                {
                    ndefBufferPrint(" (", &bufSubtype, ")");
                }

                /* Value */
                if (ndefBufferMatch(&bufType, &bufVCard_PHOTO) == false)
                {
                    ndefBufferPrint(": ", &bufValue, "\r\n");
                }
                else
                {
                    SYS_CONSOLE_MESSAGE("Photo: <Not displayed>\r\n");
                }
            }
        }
    }

    return ERR_NONE;
}
#endif


#if NDEF_TYPE_WIFI_SUPPORT
/*****************************************************************************/
ndefStatus ndefMediaWifiDump(const ndefType* type)
{
    ndefTypeWifi wifiConfig;

    ndefStatus err = ndefGetWifi(type, &wifiConfig);
    if (err != ERR_NONE)
    {
        return err;
    }

    SYS_CONSOLE_MESSAGE(" Wifi config: \r\n");
    ndefBufferDump(" Network SSID:",       &wifiConfig.bufNetworkSSID, false);
    ndefBufferDump(" Network Key:",        &wifiConfig.bufNetworkKey, false);
    SYS_CONSOLE_PRINT(" Authentication: %d\r\n",  wifiConfig.authentication);
    SYS_CONSOLE_PRINT(" Encryption: %d\r\n",      wifiConfig.encryption);

    return ERR_NONE;
}
#endif


/*****************************************************************************/
ndefStatus ndefTypeDump(const ndefType* type)
{
    static const ndefTypeDumpTable typeDumpTable[] =
    {
        { NDEF_TYPE_ID_NONE,             NULL                     },
#if NDEF_TYPE_FLAT_SUPPORT
        { NDEF_TYPE_ID_FLAT,             ndefFlatPayloadTypeDump  },
#endif
#if NDEF_TYPE_EMPTY_SUPPORT
        { NDEF_TYPE_ID_EMPTY,            ndefEmptyTypeDump        },
#endif
#if NDEF_TYPE_RTD_DEVICE_INFO_SUPPORT
        { NDEF_TYPE_ID_RTD_DEVICE_INFO,  ndefRtdDeviceInfoDump    },
#endif
#if NDEF_TYPE_RTD_TEXT_SUPPORT
        { NDEF_TYPE_ID_RTD_TEXT,         ndefRtdTextDump          },
#endif
#if NDEF_TYPE_RTD_URI_SUPPORT
        { NDEF_TYPE_ID_RTD_URI,          ndefRtdUriDump           },
#endif
#if NDEF_TYPE_RTD_AAR_SUPPORT
        { NDEF_TYPE_ID_RTD_AAR,          ndefRtdAarDump           },
#endif
#if NDEF_TYPE_RTD_WLC_SUPPORT
        { NDEF_TYPE_ID_RTD_WLCCAP,       ndefRtdWlcCapabilityDump },
        { NDEF_TYPE_ID_RTD_WLCSTAI,      ndefRtdWlcStatusInfoDump },
        { NDEF_TYPE_ID_RTD_WLCINFO,      ndefRtdWlcInfoDump       },
        { NDEF_TYPE_ID_RTD_WLCCTL,       ndefRtdWlcControlDump    },
#endif
#if NDEF_TYPE_RTD_WPCWLC_SUPPORT
        { NDEF_TYPE_ID_RTD_WPCWLC,       ndefRtdWpcWlcDump        },
#endif
#if NDEF_TYPE_RTD_TNEP_SUPPORT
        { NDEF_TYPE_ID_RTD_TNEP_SERVICE_PARAMETER, ndefRtdTnepServiceParamDump  },
        { NDEF_TYPE_ID_RTD_TNEP_SERVICE_SELECT,    ndefRtdTnepServiceSelectDump },
        { NDEF_TYPE_ID_RTD_TNEP_STATUS,            ndefRtdTnepStatusDump        },
#endif
#if NDEF_TYPE_MEDIA_SUPPORT
        //ndefMediaTypeDump
#endif
#if NDEF_TYPE_BLUETOOTH_SUPPORT
        { NDEF_TYPE_ID_BLUETOOTH_BREDR,        ndefBluetoothDump  },
        { NDEF_TYPE_ID_BLUETOOTH_LE,           ndefBluetoothDump  },
        { NDEF_TYPE_ID_BLUETOOTH_SECURE_BREDR, ndefBluetoothDump  },
        { NDEF_TYPE_ID_BLUETOOTH_SECURE_LE,    ndefBluetoothDump  },
#endif
#if NDEF_TYPE_VCARD_SUPPORT
        { NDEF_TYPE_ID_MEDIA_VCARD,      ndefMediaVCardDump       },
#endif
#if NDEF_TYPE_WIFI_SUPPORT
        { NDEF_TYPE_ID_MEDIA_WIFI,       ndefMediaWifiDump        }
#endif
    };

    if (type == NULL)
    {
        return ERR_PARAM;
    }

    for (uint32_t i = 0; i < SIZEOF_ARRAY(typeDumpTable); i++)
    {
        if (type->id == typeDumpTable[i].typeId)
        {
            /* Call the appropriate function to the matching record type */
            if (typeDumpTable[i].dump != NULL)
            {
                return typeDumpTable[i].dump(type);
            }
        }
    }

    return ERR_NOT_IMPLEMENTED;
}


/*****************************************************************************/
ndefStatus ndefRecordDumpType(const ndefRecord* record)
{
    ndefStatus err;
    ndefType   type;

    err = ndefRecordToType(record, &type);
    if (err != ERR_NONE)
    {
        return err;
    }

    return ndefTypeDump(&type);
}


/*****************************************************************************/
static ndefStatus ndefBufferDumpLine(const uint8_t* buffer, const uint32_t offset, uint32_t lineLength, uint32_t remaining)
{
    uint32_t j;

    if (buffer == NULL)
    {
        return ERR_PARAM;
    }

    SYS_CONSOLE_PRINT(" [%.4X] ", offset);

    /* Dump hex data */
    for (j = 0; j < remaining; j++)
    {
        SYS_CONSOLE_PRINT("%.2X ", buffer[offset + j]);
    }
    /* Fill hex section if needed */
    for (j = 0; j < (lineLength - remaining); j++)
    {
        SYS_CONSOLE_MESSAGE("   ");
    }

    /* Dump characters */
    SYS_CONSOLE_MESSAGE("|");
    for (j = 0; j < remaining; j++)
    {
        /* Dump only ASCII characters, otherwise replace with a '.' */
        SYS_CONSOLE_PRINT("%2c", isPrintableASCII(&buffer[offset + j], 1) ? buffer[offset + j] : (uint8_t)'.');
    }
    /* Fill ASCII section if needed */
    for (j = 0; j < (lineLength - remaining); j++)
    {
        SYS_CONSOLE_MESSAGE("  ");
    }
    SYS_CONSOLE_MESSAGE(" |\r\n");

    return ERR_NONE;
}


/*****************************************************************************/
ndefStatus ndefBufferDump(const char* string, const ndefConstBuffer* bufPayload, bool verbose)
{
    uint32_t bufferLengthMax = 32;
    const uint32_t lineLength = 8;
    uint32_t displayed;
    uint32_t remaining;
    uint32_t offset;

    if ( (string == NULL) || (bufPayload == NULL) )
    {
        return ERR_PARAM;
    }

    displayed = bufPayload->length;
    remaining = bufPayload->length;

    SYS_CONSOLE_PRINT("%s (length %d)\r\n", string, bufPayload->length);

    if (verbose == true)
    {
        bufferLengthMax = 256;
    }
    if (bufPayload->length > bufferLengthMax)
    {
        /* Truncate output */
        displayed = bufferLengthMax;
    }

    for (offset = 0; offset < displayed; offset += lineLength)
    {
        ndefBufferDumpLine(bufPayload->buffer, offset, lineLength, (remaining > lineLength) ? lineLength : remaining);
        remaining -= lineLength;
    }

    if (displayed < bufPayload->length)
    {
        SYS_CONSOLE_MESSAGE(" ... (truncated)\r\n");
    }

    return ERR_NONE;
}


/*****************************************************************************/
ndefStatus ndefBufferPrint(const char* prefix, const ndefConstBuffer* bufString, const char* suffix)
{
    if ( (prefix == NULL) || (bufString == NULL) || (suffix  == NULL) )
    {
        return ERR_PARAM;
    }

    SYS_CONSOLE_PRINT("%s", prefix);
    if (bufString->buffer == NULL)
    {
        SYS_CONSOLE_MESSAGE("<No buffer>");
    }
    else
    {
        for (uint32_t i = 0; i < bufString->length; i++)
        {
            SYS_CONSOLE_PRINT("%c", bufString->buffer[i]);
        }
    }
    SYS_CONSOLE_PRINT("%s", suffix);

    return ERR_NONE;
}


/*****************************************************************************/
ndefStatus ndefBuffer8Print(const char* prefix, const ndefConstBuffer8* bufString, const char* suffix)
{
    ndefConstBuffer buf;

    if (bufString == NULL)
    {
        return ERR_PARAM;
    }

    buf.buffer = bufString->buffer;
    buf.length = bufString->length;

    return ndefBufferPrint(prefix, &buf, suffix);
}
