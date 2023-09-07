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


/*! \file
 *
 *  \author 
 *
 *  \brief Demo application
 *
 *  This demo shows how to poll for several types of NFC cards/devices and how 
 *  to exchange data with these devices, using the RFAL library.
 *
 *  This demo does not fully implement the activities according to the standards,
 *  it performs the required to communicate with a card/device and retrieve 
 *  its UID. Also blocking methods are used for data exchange which may lead to
 *  long periods of blocking CPU/MCU.
 *  For standard compliant example please refer to the Examples provided
 *  with the RFAL library.
 * 
 */
 
/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include <stdio.h>
#include "demo.h"
#include "utils.h"
//#include "logger.h"
#include "RFAL/rfal_nfc.h"

/*
******************************************************************************
* GLOBAL DEFINES
******************************************************************************
*/

/* Definition of possible states the demo state machine could have */
#define DEMO_ST_NOTINIT               0     /*!< Demo State:  Not initialized        */
#define DEMO_ST_START_DISCOVERY       1     /*!< Demo State:  Start Discovery        */
#define DEMO_ST_DISCOVERY             2     /*!< Demo State:  Discovery              */

#define DEMO_NFCV_BLOCK_LEN           4     /*!< NFCV Block len                      */

#define DEMO_NFCV_USE_SELECT_MODE     false /*!< NFCV demonstrate select mode        */
#define DEMO_NFCV_WRITE_TAG           false /*!< NFCV demonstrate Write Single Block */

#define MAX_HEX_STR         4
#define MAX_HEX_STR_LENGTH  128
/*
 ******************************************************************************
 * GLOBAL MACROS
 ******************************************************************************
 */

/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */

/* P2P communication data */
static uint8_t NFCID3[] = {0x01, 0xFE, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
static uint8_t GB[] = {0x46, 0x66, 0x6d, 0x01, 0x01, 0x11, 0x02, 0x02, 0x07, 0x80, 0x03, 0x02, 0x00, 0x03, 0x04, 0x01, 0x32, 0x07, 0x01, 0x03};
    
/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */
static rfalNfcDiscoverParam discParam;
static uint8_t              state = DEMO_ST_NOTINIT;

/*
******************************************************************************
* LOCAL FUNCTION PROTOTYPES
******************************************************************************
*/
extern char* hex2Str(unsigned char * data, size_t dataLen);
char hexStr[MAX_HEX_STR][MAX_HEX_STR_LENGTH];
uint8_t hexStrIdx = 0;
static void demoP2P( rfalNfcDevice *nfcDev );
static void demoNotif( rfalNfcState st );
ReturnCode  demoTransceiveBlocking( uint8_t *txBuf, uint16_t txBufSize, uint8_t **rxBuf, uint16_t **rcvLen, uint32_t fwt );

//#if (RFAL_VERSION) <= 0x020102
/* Dummy implementation for older RFAL version */
bool rfalNfcaListenerIsSleepReq( const uint8_t *buf, uint16_t bufLen ){ return false; }
//#endif


char* hex2Str(unsigned char * data, size_t dataLen)
{
//#if (USE_LOGGER == LOGGER_ON)
    const char * hex = "0123456789ABCDEF";
    
    unsigned char * pin  = data;
    char *          pout = hexStr[hexStrIdx];

    uint8_t idx = hexStrIdx;

    if( dataLen > (MAX_HEX_STR_LENGTH/2) )
    {
        dataLen = (MAX_HEX_STR_LENGTH/2) - 1;
    }

    for(uint32_t i = 0; i < dataLen; i++)
    {
        *pout++ = hex[(*pin>>4) & 0x0F];
        *pout++ = hex[(*pin++)  & 0x0F];
    }
    *pout = 0;

    hexStrIdx++;
    hexStrIdx %= MAX_HEX_STR;
    
    return hexStr[idx];
//#else
//    return NULL;
//#endif /* #if USE_LOGGER == LOGGER_ON */
}


/*!
 *****************************************************************************
 * \brief Demo Notification
 *
 *  This function receives the event notifications from RFAL
 *****************************************************************************
 */
static void demoNotif( rfalNfcState st )
{
    uint8_t       devCnt;
    rfalNfcDevice *dev;
    
    
    if( st == RFAL_NFC_STATE_WAKEUP_MODE )
    {
        SYS_CONSOLE_MESSAGE("Wake Up mode started \r\n");
    }
    else if( st == RFAL_NFC_STATE_POLL_TECHDETECT )
    {
        if( discParam.wakeupEnabled )
        {
            SYS_CONSOLE_MESSAGE("Wake Up mode terminated. Polling for devices \r\n");
        }
    }
    else if( st == RFAL_NFC_STATE_POLL_SELECT )
    {
        /* Multiple devices were found, activate first of them */
        rfalNfcGetDevicesFound( &dev, &devCnt );
        rfalNfcSelect( 0 );
        
        SYS_CONSOLE_PRINT("Multiple Tags detected: %d \r\n", devCnt);
    }
}

/*!
 *****************************************************************************
 * \brief Demo Ini
 *
 *  This function Initializes the required layers for the demo
 *
 * \return true  : Initialization ok
 * \return false : Initialization failed
 *****************************************************************************
 */
bool demoIni( void )
{
    ReturnCode err;
    
    err = rfalNfcInitialize();
    if( err == ERR_NONE )
    {
        rfalNfcDefaultDiscParams( &discParam );
        discParam.devLimit      = 1U;
        
        ST_MEMCPY( &discParam.nfcid3, NFCID3, sizeof(NFCID3) );
        ST_MEMCPY( &discParam.GB, GB, sizeof(GB) );
        discParam.GBLen         = sizeof(GB);
        discParam.p2pNfcaPrio   = true;

        discParam.notifyCb             = demoNotif;
        discParam.totalDuration        = 1000U;
        discParam.wakeupConfigDefault  = true;
        
        discParam.techs2Find   = (RFAL_NFC_POLL_TECH_AP2P | RFAL_NFC_LISTEN_TECH_AP2P);

        err = rfalNfcDiscover( &discParam );
        if( err != ERR_NONE )
        {
            return false;
        }
        
        state = DEMO_ST_START_DISCOVERY;
        return true;
    }
    return false;
}

void wakeup_toggle()
{

//    if( platformGpioIsLow(PLATFORM_USER_BUTTON_PORT, PLATFORM_USER_BUTTON_PIN))
//    {
        discParam.wakeupEnabled = !discParam.wakeupEnabled;    /* enable/disable wakeup */
        state = DEMO_ST_START_DISCOVERY;                       /* restart loop          */
        SYS_CONSOLE_PRINT("Toggling Wake Up mode %s\r\n", discParam.wakeupEnabled ? "ON": "OFF");
//
//        /* Debounce button */
//        while( platformGpioIsLow(PLATFORM_USER_BUTTON_PORT, PLATFORM_USER_BUTTON_PIN) );
//    }
    
    
}

/*!
 *****************************************************************************
 * \brief Demo Cycle
 *
 *  This function executes the demo state machine. 
 *  It must be called periodically
 *****************************************************************************
 */
void demoCycle( void )
{
    static rfalNfcDevice *nfcDevice;
    
    rfalNfcWorker();                                    /* Run RFAL worker periodically */

    /*******************************************************************************/
    /* Check if USER button is pressed */
//    if( platformGpioIsLow(PLATFORM_USER_BUTTON_PORT, PLATFORM_USER_BUTTON_PIN))
//    {
//        discParam.wakeupEnabled = !discParam.wakeupEnabled;    /* enable/disable wakeup */
//        state = DEMO_ST_START_DISCOVERY;                       /* restart loop          */
//        SYS_CONSOLE_PRINT("Toggling Wake Up mode %s\r\n", discParam.wakeupEnabled ? "ON": "OFF");
//
//        /* Debounce button */
//        while( platformGpioIsLow(PLATFORM_USER_BUTTON_PORT, PLATFORM_USER_BUTTON_PIN) );
//    }
  
    
    switch( state )
    {
        /*******************************************************************************/
        case DEMO_ST_START_DISCOVERY:

//          platformLedOff(PLATFORM_LED_A_PORT, PLATFORM_LED_A_PIN);
//          platformLedOff(PLATFORM_LED_B_PORT, PLATFORM_LED_B_PIN);
//          platformLedOff(PLATFORM_LED_F_PORT, PLATFORM_LED_F_PIN);
//          platformLedOff(PLATFORM_LED_V_PORT, PLATFORM_LED_V_PIN);
//          platformLedOff(PLATFORM_LED_AP2P_PORT, PLATFORM_LED_AP2P_PIN);
//          platformLedOff(PLATFORM_LED_FIELD_PORT, PLATFORM_LED_FIELD_PIN);
          
          state = DEMO_ST_DISCOVERY;
          break;

        /*******************************************************************************/
        case DEMO_ST_DISCOVERY:
        
            if( rfalNfcIsDevActivated( rfalNfcGetState() ) )
            {
                rfalNfcGetActiveDevice( &nfcDevice );
                
                switch( nfcDevice->type )
                {
                    /*******************************************************************************/
                    case RFAL_NFC_LISTEN_TYPE_AP2P:
                    case RFAL_NFC_POLL_TYPE_AP2P:
                        
//                        platformLedOn(PLATFORM_LED_AP2P_PORT, PLATFORM_LED_AP2P_PIN);
                        SYS_CONSOLE_PRINT("NFC Active P2P device found. NFCID3: %s\r\n", hex2Str(nfcDevice->nfcid, nfcDevice->nfcidLen));
                    
                        demoP2P( nfcDevice );
                        break;
                    
                    default:
                        break;
                }
                
                rfalNfcDeactivate( RFAL_NFC_DEACTIVATE_DISCOVERY );
                state = DEMO_ST_START_DISCOVERY;
            }
            break;

        /*******************************************************************************/
        case DEMO_ST_NOTINIT:
        default:
            break;
    }
}


/*!
 *****************************************************************************
* \brief Demo Stop
 *
 *  This function stops the demo operation
 *
 *****************************************************************************
 */
void demoStop( void )
{
    rfalNfcDeactivate( RFAL_NFC_DEACTIVATE_IDLE );
    state = DEMO_ST_NOTINIT;
}


const char message_string[]="%s says: \"The horse does not eat cucumber salad! %d\"\r\nd";// 'd' being a dummy character
uint8_t message[256];

/*!
 *****************************************************************************
 * \brief Demo P2P Exchange
 *
 * Sends a NDEF URI record 'http://www.ST.com' via NFC-DEP (P2P) protocol.
 * 
 * This method sends a set of static predefined frames which tries to establish
 * a LLCP connection, followed by the NDEF record, and then keeps sending 
 * LLCP SYMM packets to maintain the connection.
 * 
 * 
 *****************************************************************************
 */
void demoP2P( rfalNfcDevice *nfcDev )
{
    static int counter;
    
#if RFAL_FEATURE_NFC_DEP
    
    uint16_t   *rxLen;
    uint8_t    *rxData;
    ReturnCode err = ERR_NONE;
    const char * role;
    
    /* In Listen mode retrieve the first request from Initiator */
    if( nfcDev->type == RFAL_NFC_POLL_TYPE_AP2P )
    {
        demoTransceiveBlocking( NULL, 0, &rxData, &rxLen, 0);
        SYS_CONSOLE_MESSAGE("Received: ");
        rxData[(*rxLen > 0)?(*rxLen-1):(0)] = 0; // terminate string for printing - by overwriting last received character 'd'
        SYS_CONSOLE_PRINT("%d",(char*)rxData);

        role = "Target";
    }
    else
        role = "Initiator";

    SYS_CONSOLE_MESSAGE(" Initialize device .. ");
    while(err == ERR_NONE)
    {
        counter ++;
        if (counter > 999) counter = 0;
        snprintf((char*)message,sizeof(message),message_string,role,counter);

        err = demoTransceiveBlocking( message, strlen((char*)message), &rxData, &rxLen, RFAL_FWT_NONE);
        if( err != ERR_NONE )
        {
            break;
        }

        SYS_CONSOLE_MESSAGE("Received: ");
        rxData[(*rxLen > 0)?(*rxLen-1):(0)] = 0; // terminate string for printing - by overwriting last received character 'd'
        SYS_CONSOLE_PRINT("%d",(char*)rxData);
    }

    SYS_CONSOLE_MESSAGE("\r\n Device removed.\r\n");
    
#endif /* RFAL_FEATURE_NFC_DEP */
}

/*!
 *****************************************************************************
 * \brief Demo Blocking Transceive 
 *
 * Helper function to send data in a blocking manner via the rfalNfc module 
 *  
 * \warning A protocol transceive handles long timeouts (several seconds), 
 * transmission errors and retransmissions which may lead to a long period of 
 * time where the MCU/CPU is blocked in this method.
 * This is a demo implementation, for a non-blocking usage example please 
 * refer to the Examples available with RFAL
 *
 * \param[in]  txBuf      : data to be transmitted
 * \param[in]  txBufSize  : size of the data to be transmited
 * \param[out] rxData     : location where the received data has been placed
 * \param[out] rcvLen     : number of data bytes received
 * \param[in]  fwt        : FWT to be used (only for RF frame interface, 
 *                                          otherwise use RFAL_FWT_NONE)
 *
 * 
 *  \return ERR_PARAM     : Invalid parameters
 *  \return ERR_TIMEOUT   : Timeout error
 *  \return ERR_FRAMING   : Framing error detected
 *  \return ERR_PROTO     : Protocol error detected
 *  \return ERR_NONE      : No error, activation successful
 * 
 *****************************************************************************
 */
ReturnCode demoTransceiveBlocking( uint8_t *txBuf, uint16_t txBufSize, uint8_t **rxData, uint16_t **rcvLen, uint32_t fwt )
{
    ReturnCode err;
    
    err = rfalNfcDataExchangeStart( txBuf, txBufSize, rxData, rcvLen, fwt );
    if( err == ERR_NONE )
    {
        do{
            rfalNfcWorker();
            err = rfalNfcDataExchangeGetStatus();
        }
        while( err == ERR_BUSY );
    }
    return err;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
