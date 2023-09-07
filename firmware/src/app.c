// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2022 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/
// DOM-IGNORE-END

/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It
    implements the logic of the application's state machine and it may call
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <string.h>
#include "app.h"
#include "definitions.h"
#include "app_ble.h"
#include "system/console/sys_console.h"
#include "nfc4_click/demo.h"
#include "nfc4_click/RFAL/st25r3916/st25r3916_irq.h"
#include "nfc4_click/RFAL/st25r3916/st25r3916.h"
#include "nfc4_click/RFAL/rfal_platform.h"
#include "nfc4_click/st_errno.h"
#include "nfc4_click/RFAL/rfal_rf.h"
#include"nfc4_click/RFAL/rfal_analogConfig.h"
#include "nfc4_click/NDEF/message/ndef_record.h"
#include "nfc4_click/NDEF/message/ndef_types.h"
#include "ble_dis/ble_dis.h"

SYS_CONSOLE_HANDLE uartConsoleHandle;
uint16_t conn_hdl = 0xFFFF;
uint8_t tag_uid[ 10 ] = { 0 };
uint8_t uid_len = 0;


uint8_t globalCommProtectCnt = 0;
volatile uint8_t IRQ_Flag = false;
//OSAL_MUTEX_HANDLE_TYPE irq_mutex;

static uint8_t ndefFile[80];    /* Buffer to encode NDEF file */
const uint8_t *demoNdefFile = ndefFile; /* Overwrite weak demoNdefFile pointer */
uint32_t demoNdefFileLen = sizeof(ndefFile);

uint8_t oob_confirm_data[16];
uint8_t oob_rand_data[16];
//extern uint8_t ScanName;

// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.

    Application strings and buffers are be defined outside this structure.
*/

APP_DATA appData;

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary callback functions.
*/

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


/* TODO:  Add any necessary local functions.
*/


// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Initialize ( void )
{
    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_INIT;


    appData.appQueue = xQueueCreate( 64, sizeof(APP_Msg_T) );
    appISRQueue = xQueueCreate( 64, sizeof(APP_Msg_T) );
    /* TODO: Initialize your application's state machine and other
     * parameters.
     */
}

void PrintBtAddress(uint8_t *addr)
{
    uint8_t i;
    SYS_CONSOLE_PRINT("Addr: ");
    for(i = 0; i < GAP_MAX_BD_ADDRESS_LEN; i++)
    {
        SYS_CONSOLE_PRINT("%02X ",addr[5-i]);
    }
    SYS_CONSOLE_PRINT("\r\n");
}

void PrintScOob(uint8_t *addr)
{
    uint8_t i;
    for(i = 0; i < 16; i++)
    {
        SYS_CONSOLE_PRINT("%02X ",addr[i]);
    }
    SYS_CONSOLE_PRINT("\r\n");
}

uint8_t asciiNum_to_hex(uint8_t ascii_val)
{
    uint8_t hex_val = 0xFF;
    if((ascii_val>='0') && (ascii_val<='9'))
    {
        hex_val = ascii_val - 48;
    }
    else if( (ascii_val>='A') && (ascii_val<='F')  )
    {
         hex_val = ascii_val - 55;
    }
    else if ( (ascii_val>='a') && (ascii_val<='f') )
    {
        hex_val = ascii_val - 87;
    }
    else
    {
        hex_val = 0xFF;
    }
    return hex_val;
}

void UART_to_OOB_Confirm(uint8_t * buffer,int len)
{
    int count=0;
    char temp_buffer[50];
    memset(oob_confirm_data, '\0', sizeof(oob_confirm_data));
    for(int i=0;i<len;i+=2)
    {       
        temp_buffer[i]=asciiNum_to_hex(buffer[i]);
        temp_buffer[i+1]=asciiNum_to_hex(buffer[i+1]);        
        oob_confirm_data[count]=(temp_buffer[i]<<4)|(temp_buffer[i+1]);        
        count++;
    }    
}

void UART_to_OOB_Rand(uint8_t * buffer,int len)
{
    int count=0;
    char temp_buffer[50];
    memset(oob_rand_data, '\0', sizeof(oob_rand_data));
    for(int i=0;i<len;i+=2)
    {       
        temp_buffer[i]=asciiNum_to_hex(buffer[i]);
        temp_buffer[i+1]=asciiNum_to_hex(buffer[i+1]);        
        oob_rand_data[count]=(temp_buffer[i]<<4)|(temp_buffer[i+1]);        
        count++;
    }    
}

void GetScOob()
{
    BLE_SMP_EvtGenScOobDataDone_T oobdata;
    ssize_t nUnreadBytes = 0;
    uint8_t oob_confirm[32];
    uint8_t oob_rand[32];
    
    SYS_CONSOLE_MESSAGE(" Enter confirm Data: ");
    do{
        nUnreadBytes = 0;
        nUnreadBytes = SYS_CONSOLE_ReadCountGet(uartConsoleHandle);
    }while(nUnreadBytes != 32 );
    
    SYS_CONSOLE_Read( uartConsoleHandle, oob_confirm, nUnreadBytes );
    UART_to_OOB_Confirm(oob_confirm,nUnreadBytes);    
    memcpy(&oobdata.confirm, &oob_confirm_data, sizeof(oob_confirm_data));
    
    SYS_CONSOLE_MESSAGE("\n\r Enter Rand Data: ");
    nUnreadBytes = 0;    
    do{      
        nUnreadBytes = 0;
        nUnreadBytes = SYS_CONSOLE_ReadCountGet(uartConsoleHandle);
    }while(nUnreadBytes != 32);
    SYS_CONSOLE_Read( uartConsoleHandle, oob_rand, nUnreadBytes );
    UART_to_OOB_Rand(oob_rand,nUnreadBytes);
    memcpy(&oobdata.rand, &oob_rand_data, sizeof(oob_rand_data));
        
    SYS_CONSOLE_MESSAGE("\n\r Confirm: ");
    PrintScOob(oobdata.confirm);
    SYS_CONSOLE_MESSAGE("\n\r Rand: ");
    PrintScOob(oobdata.rand);
    
   
}

/**
  * @brief Example to encode a Bluetooth record
  *
  * This function creates a Bluetooth OOB data record, thanks to the NDEF API
  * providing hardcoded device address and local name.
  * Note that the encoded buffer is prepared with the NDEF file length stored on
  * the first two bytes. This allows to use the same buffer for both the T3T
  * and T4T Card Emulation demos: The information is irrelevant and not used
  * for T3T but it is for T4T demo.
  *
  * @param[in,out] buffer to use to create the Bluetooth record
  * @param[in]     length of the buffer
  *
  * @retval ERR_NONE if successful or a standard error code
  */
ndefStatus demoEncodeBluetoothRecord(uint8_t *buffer, uint32_t length)
{
    ndefTypeBluetooth bluetooth;
    ndefRecord record;
    ndefType ndefBt;
    ndefStatus err;

    if( buffer == NULL )
    {
        return ERR_PARAM;
    }

    err = ndefBluetoothReset(&bluetooth);
    if( err != ERR_NONE )
    {
        return err;
    }
    
    uint8_t deviceAddress[6] = {0};
    BLE_GAP_Addr_T devAddr;
    if (IB_GetBdAddr((uint8_t *)&devAddr))
    { 
        for(int i=0;i<=5;i++)
        {
            deviceAddress[i] = devAddr.addr[i];
        }
        
    }
        
    /* Device address, in reverse order */
//    uint8_t deviceAddress[] = {0xB5,0x69,0x40,0x6E,0x95,0x9C}; //{ 0x06, 0x05, 0x04, 0x03, 0x02, 0x01 };
    ndefConstBuffer bufDeviceAddress = { (uint8_t*)deviceAddress, sizeof(deviceAddress) };

    bluetooth.bufDeviceAddress = bufDeviceAddress;

    uint8_t localName[] = { 'W', 'B', 'Z', '4', '5', '1', '_', 'N', 'F', 'C' };
    uint8_t eirLocalName[] = { sizeof(uint8_t) + sizeof(localName),
                               NDEF_BT_EIR_COMPLETE_LOCAL_NAME,
                                'W', 'B', 'Z', '4', '5', '1', '_', 'N', 'F', 'C' };

    err = ndefBluetoothSetEir(&bluetooth, (uint8_t*)&eirLocalName);
    if( err != ERR_NONE )
    {
        return err;
    }

    err = ndefBluetoothBrEdrInit(&ndefBt, &bluetooth);
    if( err != ERR_NONE )
    {
        return err;
    }

    err = ndefTypeToRecord(&ndefBt, &record);
    if( err != ERR_NONE )
    {
        return err;
    }

    /* Encode the record starting after the first two bytes that are saved
       to store the NDEF file length */
    ndefBuffer bufRecord = { &buffer[2], length - 2 };
    err = ndefRecordEncode(&record, &bufRecord);
    if( err != ERR_NONE )
    {
        return err;
    }

    /* Store the NDEF file length on the two first bytes */
    buffer[0] = bufRecord.length >> 8;
    buffer[1] = bufRecord.length & 0xFF;

    return err;
}

/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Tasks ( void )
{
    APP_Msg_T    appMsg[1];
    APP_Msg_T   *p_appMsg;
    p_appMsg=appMsg;

    /* Check the application's current state. */
    switch ( appData.state )
    {
        /* Application's initial state. */
        case APP_STATE_INIT:
        {
            bool appInitialized = true;
            //appData.appQueue = xQueueCreate( 10, sizeof(APP_Msg_T) );
            APP_BleStackInit();
            RTC_Timer32Start();
            uartConsoleHandle=SYS_CONSOLE_HandleGet(SYS_CONSOLE_INDEX_0);
            SYS_CONSOLE_MESSAGE("\n\r WBZ451_NFC_BLE \n\r");
            
//            GetScOob();
            SYS_CONSOLE_MESSAGE("Started Advertising\n\r");
            BLE_GAP_SetAdvEnable(true, 0);
            BLE_DIS_Add();
            
            if (appInitialized)
            {

                appData.state = APP_STATE_NFC_INIT;
            }
            break;
        }
        
        case APP_STATE_NFC_INIT:
        {
            EIC_InterruptEnable(EIC_PIN_1);
            EIC_CallbackRegister(EIC_PIN_1, wakeup_toggle, (uintptr_t) NULL);
            
            appData.handle = DRV_SPI_Open(DRV_SPI_INDEX_0, DRV_IO_INTENT_EXCLUSIVE);
            DRV_SPI_TRANSFER_SETUP setup;
            setup.baudRateInHz = 100000;
            setup.clockPhase = DRV_SPI_CLOCK_PHASE_VALID_TRAILING_EDGE;
            setup.clockPolarity = DRV_SPI_CLOCK_POLARITY_IDLE_LOW;
            setup.dataBits = DRV_SPI_DATA_BITS_8;
//            setup.chipSelect = SYS_PORT_PIN_RA9;
            setup.csPolarity = DRV_SPI_CS_POLARITY_ACTIVE_LOW;

            DRV_SPI_TransferSetup(appData.handle, &setup);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            
//            CLICK_NFC_IRQ_InputEnable();
//            OSAL_MUTEX_Create(&irq_mutex);
 
            appData.state = APP_STATE_SERVICE_TASKS;             
            appMsg->msgId = APP_MSG_BLE_NFC_INIT_EVT;
            OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
            break;
        }

        case APP_STATE_SERVICE_TASKS:
        {
            if (OSAL_QUEUE_Receive(&appData.appQueue, &appMsg, OSAL_WAIT_FOREVER))
            {
                if(p_appMsg->msgId==APP_MSG_BLE_STACK_EVT)
                {
                    // Pass BLE Stack Event Message to User Application for handling
                    APP_BleStackEvtHandler((STACK_Event_T *)p_appMsg->msgData);
                }
                else if(p_appMsg->msgId==APP_MSG_BLE_STACK_LOG)
                {
                    // Pass BLE LOG Event Message to User Application for handling
                    APP_BleStackLogHandler((BT_SYS_LogEvent_T *)p_appMsg->msgData);
                }
                else if( p_appMsg->msgId == APP_MSG_BLE_NFC_INIT_EVT)
                {
                    APP_Msg_T appMsg;
                    if( demoEncodeBluetoothRecord(ndefFile, sizeof(ndefFile)) != ERR_NONE )
                        {
                            SYS_CONSOLE_MESSAGE("Encoding Bluetooth record failed.\r\n");
                            while(1);
                        }                    
                    demoIni();               
                    appMsg.msgId = APP_MSG_BLE_NFC_DEMO_EVT;
                    OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
                }
                else if( p_appMsg->msgId == APP_MSG_BLE_NFC_DEMO_EVT)
                {     
                    APP_Msg_T appMsg;
                    demoCycle();
                    vTaskDelay(30 / portTICK_PERIOD_MS);
                    appMsg.msgId = APP_MSG_BLE_NFC_DEMO_EVT;
                    OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
                }
            }
            break;
        }

        /* TODO: implement your application state machine.*/


        /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
}

void ISR_Tasks ( void )
{
    APP_Msg_T    appMsg[1];
    APP_Msg_T   *p_appMsg;
    p_appMsg=appMsg;
    
    if (OSAL_QUEUE_Receive(&appISRQueue, &appMsg, OSAL_WAIT_FOREVER))
    {
        if(p_appMsg->msgId==APP_MSG_NFC_IRQ_EVT)
        {
            st25r3916CheckForReceivedInterrupts();
            IRQ_Flag = true;
        }
    }
    
}

/*******************************************************************************
 End of File
 */
