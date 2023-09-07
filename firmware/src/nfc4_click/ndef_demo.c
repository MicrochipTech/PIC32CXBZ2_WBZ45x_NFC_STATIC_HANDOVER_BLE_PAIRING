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
#include "demo.h"
#include "utils.h"
//#include "logger.h"

#include "RFAL/rfal_nfc.h"

#include "NDEF/poller/ndef_poller.h"
#include "NDEF/poller/ndef_t2t.h"
#include "NDEF/poller/ndef_t4t.h"
#include "NDEF/poller/ndef_t5t.h"
#include "NDEF/message/ndef_message.h"
#include "NDEF/message/ndef_types.h"
#include "ndef_dump.h"
#include "nfc_main.h"

/*
******************************************************************************
* GLOBAL DEFINES
******************************************************************************
*/

/* Definition of possible states the demo state machine could have */
#define DEMO_ST_NOTINIT               0    /*!< Demo State:  Not initialized */
#define DEMO_ST_START_DISCOVERY       1    /*!< Demo State:  Start Discovery */
#define DEMO_ST_DISCOVERY             2    /*!< Demo State:  Discovery       */

#define NDEF_DEMO_READ                0U   /*!< NDEF menu read               */
#define NDEF_DEMO_WRITE_MSG1          1U   /*!< NDEF menu write 1 record     */
#define NDEF_DEMO_WRITE_MSG2          2U   /*!< NDEF menu write 2 records    */
#define NDEF_DEMO_FORMAT_TAG          3U   /*!< NDEF menu format tag         */
#if NDEF_FEATURE_FULL_API             
#define NDEF_DEMO_MAX_FEATURES        4U   /*!< Number of menu items         */
#else                                 
#define NDEF_DEMO_MAX_FEATURES        1U   /*!< Number of menu items         */
#endif /* NDEF_FEATURE_FULL_API */    
#define NDEF_WRITE_FORMAT_TIMEOUT     10000U /*!< When write or format mode is selected, demo returns back to read mode after a timeout */
//#define NDEF_LED_BLINK_DURATION         250U /*!< Led blink duration         */ 
                                      
#define DEMO_RAW_MESSAGE_BUF_LEN        8192 /*!< Raw message buffer len     */
                                      
#define DEMO_ST_MANUFACTURER_ID        0x02U /*!< ST Manufacturer ID         */

//#define ndefDevice                rfalNfcDevice           /*!< NDEF Device         */
//#define NDEF_CC_BUF_LEN             17U                                                /*!< CC buffer len. Max length = 17 in case of T4T v3             */
//#define ndefStatus stError    /*!< NDEF error type */

/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */

/* P2P communication data */
static uint8_t NFCID3[] = {0x01, 0xFE, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
static uint8_t GB[] = {0x46, 0x66, 0x6d, 0x01, 0x01, 0x11, 0x02, 0x02, 0x07, 0x80, 0x03, 0x02, 0x00, 0x03, 0x04, 0x01, 0x32, 0x07, 0x01, 0x03};
    
///*! NDEF structure to handle const buffers */
//typedef struct
//{
//    const uint8_t* buffer; /*!< Pointer to const buffer */
//    uint32_t       length; /*!< buffer length           */
//} ndefConstBuffer;


#if RFAL_FEATURE_NFC_DEP
/* P2P communication data */    
static uint8_t ndefLLCPSYMM[] = {0x00, 0x00};
static uint8_t ndefInit[] = {0x05, 0x20, 0x06, 0x0F, 0x75, 0x72, 0x6E, 0x3A, 0x6E, 0x66, 0x63, 0x3A, 0x73, 0x6E, 0x3A, 0x73, 0x6E, 0x65, 0x70, 0x02, 0x02, 0x07, 0x80, 0x05, 0x01, 0x02};
static const uint8_t ndefSnepPrefix[] = { 0x13, 0x20, 0x00, 0x10, 0x02, 0x00, 0x00, 0x00 };
static const uint8_t URL[] = "st.com/st25-demo";
static ndefConstBuffer bufURL = { URL, sizeof(URL) - 1 };
static uint8_t ndefUriBuffer[255];
#endif /* RFAL_FEATURE_NFC_DEP */

#if RFAL_SUPPORT_CE && RFAL_FEATURE_LISTEN_MODE
#if RFAL_SUPPORT_MODE_LISTEN_NFCA
/* NFC-A CE config */
/* 4-byte UIDs with first byte 0x08 would need random number for the subsequent 3 bytes.
 * 4-byte UIDs with first byte 0x*F are Fixed number, not unique, use for this demo
 * 7-byte UIDs need a manufacturer ID and need to assure uniqueness of the rest.*/
static uint8_t ceNFCA_NFCID[]     = {0x5F, 'S', 'T', 'M'};    /* =_STM, 5F 53 54 4D NFCID1 / UID (4 bytes) */
static uint8_t ceNFCA_SENS_RES[]  = {0x02, 0x00};             /* SENS_RES / ATQA for 4-byte UID            */
static uint8_t ceNFCA_SEL_RES     = 0x20;                     /* SEL_RES / SAK                             */
#endif /*RFAL_SUPPORT_MODE_LISTEN_NFCA */

#if RFAL_SUPPORT_MODE_LISTEN_NFCF
/* NFC-F CE config */
static uint8_t ceNFCF_nfcid2[]     = {0x02, 0xFE, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
static uint8_t ceNFCF_SC[]         = {0x12, 0xFC};
static uint8_t ceNFCF_SENSF_RES[]  = {0x01,                                                       /* SENSF_RES                                */
                                      0x02, 0xFE, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,             /* NFCID2                                   */
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x7F, 0x00,             /* PAD0, PAD01, MRTIcheck, MRTIupdate, PAD2 */
                                      0x00, 0x00 };                                               /* RD                                       */
#endif /*RFAL_SUPPORT_MODE_LISTEN_NFCF */
#endif /*  RFAL_SUPPORT_CE && RFAL_FEATURE_LISTEN_MODE */

 

static uint8_t *ndefStates[] =
{
    (uint8_t *)"INVALID",
    (uint8_t *)"INITIALIZED",
    (uint8_t *)"READ/WRITE",
    (uint8_t *)"READ-ONLY"
};

static const uint8_t *ndefDemoFeatureDescription[NDEF_DEMO_MAX_FEATURES] =
{
    (uint8_t *)"1. Tap a tag to read its content",
#if NDEF_FEATURE_FULL_API
    (uint8_t *)"2. Present a tag to write a Text record",
    (uint8_t *)"3. Present a tag to write a URI record and an Android Application record",
    (uint8_t *)"4. Present an ST tag to format",
#endif /* NDEF_FEATURE_FULL_API */
};

#if NDEF_FEATURE_FULL_API
static uint8_t ndefURI[]          = "st.com";
static uint8_t ndefTEXT[]         = "Welcome to ST NDEF demo";
static uint8_t ndefTextLangCode[] = "en";

static uint8_t ndefAndroidPackName[] = "com.st.st25nfc";
#endif /* NDEF_FEATURE_FULL_API */

/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */

///*! NDEF device type */
//typedef enum {
//    NDEF_DEV_NONE          = 0x00U,                            /*!< Device type undef                                  */
//    NDEF_DEV_T1T           = 0x01U,                            /*!< Device type T1T                                    */
//    NDEF_DEV_T2T           = 0x02U,                            /*!< Device type T2T                                    */
//    NDEF_DEV_T3T           = 0x03U,                            /*!< Device type T3T                                    */
//    NDEF_DEV_T4T           = 0x04U,                            /*!< Device type T4AT/T4BT                              */
//    NDEF_DEV_T5T           = 0x05U,                            /*!< Device type T5T                                    */
//} ndefDeviceType;
//
///*! NDEF states  */
//typedef enum {
//    NDEF_STATE_INVALID     = 0x00U,                            /*!< Invalid state (e.g. no CC)                         */
//    NDEF_STATE_INITIALIZED = 0x01U,                            /*!< State Initialized (no NDEF message)                */
//    NDEF_STATE_READWRITE   = 0x02U,                            /*!< Valid NDEF found. Read/Write capability            */
//    NDEF_STATE_READONLY    = 0x03U,                            /*!< Valid NDEF found. Read only                        */
//} ndefState;
//
///*! Generic Capability Container  */
//typedef union {
//#if NDEF_FEATURE_T1T
//    ndefCapabilityContainerT1T   t1t;                          /*!< T1T Capability Container                           */
//#endif
//#if NDEF_FEATURE_T2T
//    ndefCapabilityContainerT2T   t2t;                          /*!< T2T Capability Container                           */
//#endif
//#if NDEF_FEATURE_T3T
//    ndefAttribInfoBlockT3T       t3t;                          /*!< T3T Attribute Information Block                    */
//#endif
//#if NDEF_FEATURE_T4T
//    ndefCapabilityContainerT4T   t4t;                          /*!< T4T Capability Container                           */
//#endif
//#if NDEF_FEATURE_T5T
//    ndefCapabilityContainerT5T   t5t;                          /*!< T5T Capability Container                           */
//#endif
//} ndefCapabilityContainer;
//
///*! NDEF Information */
//typedef struct {
//    uint8_t                  majorVersion;                     /*!< Major version                                      */
//    uint8_t                  minorVersion;                     /*!< Minor version                                      */
//    uint32_t                 areaLen;                          /*!< Area Length for NDEF storage                       */
//    uint32_t                 areaAvalableSpaceLen;             /*!< Remaining Space in case a propTLV is present       */
//    uint32_t                 messageLen;                       /*!< NDEF message Length                                */
//    ndefState                state;                            /*!< Tag state e.g. NDEF_STATE_INITIALIZED              */
//} ndefInfo;
//
///*! NDEF context structure */
//typedef struct {
//    ndefDeviceType               type;                         /*!< NDEF Device type                                   */
//    ndefDevice                   device;                       /*!< NDEF Device                                        */
//    ndefState                    state;                        /*!< Tag state e.g. NDEF_STATE_INITIALIZED              */
//    ndefCapabilityContainer      cc;                           /*!< Capability Container                               */
//    uint32_t                     messageLen;                   /*!< NDEF message length                                */
//    uint32_t                     messageOffset;                /*!< NDEF message offset                                */
//    uint32_t                     areaLen;                      /*!< Area Length for NDEF storage                       */
//    uint8_t                      ccBuf[NDEF_CC_BUF_LEN];       /*!< buffer for CC                                      */
//    const struct ndefPollerWrapperStruct*   ndefPollWrapper;              /*!< pointer to array of function for wrapper           */
//    union {
//#if NDEF_FEATURE_T1T
//        ndefT1TContext t1t;                                    /*!< T1T context                                        */
//#endif
//#if NDEF_FEATURE_T2T
//        ndefT2TContext t2t;                                    /*!< T2T context                                        */
//#endif
//#if NDEF_FEATURE_T3T
//        ndefT3TContext t3t;                                    /*!< T3T context                                        */
//#endif
//#if NDEF_FEATURE_T4T
//        ndefT4TContext t4t;                                    /*!< T4T context                                        */
//#endif
//#if NDEF_FEATURE_T5T
//        ndefT5TContext t5t;                                    /*!< T5T context                                        */
//#endif
//    } subCtx;                                                  /*!< Sub-context union                                  */
//} ndefContext;
//
///*! Wrapper structure to hold the function pointers on each tag type */
//typedef struct ndefPollerWrapperStruct
//{
//    ndefStatus (* pollerContextInitialization)(ndefContext *ctx, const ndefDevice *dev);                                                  /*!< ContextInitialization function pointer                 */
//    ndefStatus (* pollerNdefDetect)(ndefContext *ctx, ndefInfo *info);                                                                    /*!< NdefDetect function pointer                            */
//    ndefStatus (* pollerReadBytes)(ndefContext *ctx, uint32_t offset, uint32_t len, uint8_t *buf, uint32_t *rcvdLen);                     /*!< Read function pointer                                  */
//    ndefStatus (* pollerReadRawMessage)(ndefContext *ctx, uint8_t *buf, uint32_t bufLen, uint32_t *rcvdLen, bool single);                 /*!< ReadRawMessage function pointer                        */
//#if NDEF_FEATURE_FULL_API
//    ndefStatus (* pollerWriteBytes)(ndefContext *ctx, uint32_t offset, const uint8_t *buf, uint32_t len, bool pad, bool writeTerminator); /*!< Write function pointer                                 */
//    ndefStatus (* pollerWriteRawMessage)(ndefContext *ctx, const uint8_t *buf, uint32_t bufLen);                                          /*!< WriteRawMessage function pointer                       */
//    ndefStatus (* pollerTagFormat)(ndefContext *ctx, const ndefCapabilityContainer *cc, uint32_t options);                                /*!< TagFormat function pointer                             */
//    ndefStatus (* pollerWriteRawMessageLen)(ndefContext *ctx, uint32_t rawMessageLen, bool writeTerminator);                              /*!< WriteRawMessageLen function pointer                    */
//    ndefStatus (* pollerCheckPresence)(ndefContext *ctx);                                                                                 /*!< CheckPresence function pointer                         */
//    ndefStatus (* pollerCheckAvailableSpace)(const ndefContext *ctx, uint32_t messageLen);                                                /*!< CheckAvailableSpace function pointer                   */
//    ndefStatus (* pollerBeginWriteMessage)(ndefContext *ctx, uint32_t messageLen);                                                        /*!< BeginWriteMessage function pointer                     */
//    ndefStatus (* pollerEndWriteMessage)(ndefContext *ctx, uint32_t messageLen, bool writeTerminator);                                    /*!< EndWriteMessage function pointer                       */
//    ndefStatus (* pollerSetReadOnly)(ndefContext *ctx);                                                                                   /*!< SetReadOnly function pointer                           */
//#endif /* NDEF_FEATURE_FULL_API */
//} ndefPollerWrapper;



static rfalNfcDiscoverParam discParam;
static uint8_t              state = DEMO_ST_NOTINIT;

static ndefContext          ndefCtx;
static uint8_t              ndefDemoFeature     = NDEF_DEMO_READ;
static uint8_t              ndefDemoPrevFeature = 0xFF;
static bool                 verbose             = false;

static uint8_t              rawMessageBuf[DEMO_RAW_MESSAGE_BUF_LEN];

static uint32_t             timer;
static uint32_t             timerLed;
static bool                 ledOn;

extern char* hex2Str(unsigned char * data, size_t dataLen);
#define MAX_HEX_STR         4
#define MAX_HEX_STR_LENGTH  128
char hexStr[MAX_HEX_STR][MAX_HEX_STR_LENGTH];
uint8_t hexStrIdx = 0;

/*
******************************************************************************
* LOCAL FUNCTION PROTOTYPES
******************************************************************************
*/


static void demoNdef(rfalNfcDevice *nfcDevice);
static void ndefCCDump(ndefContext *ctx);
#if NDEF_FEATURE_T5T
static void ndefDumpSysInfo(ndefContext *ctx);
#endif /* RFAL_FEATURE_T5T */

#if NDEF_FEATURE_FULL_API
static bool ndefIsSTTag(ndefContext *ctx);
//static void LedNotificationWriteDone(void);
#endif /* NDEF_FEATURE_FULL_API */

static void demoP2P( void );
ReturnCode  demoTransceiveBlocking( uint8_t *txBuf, uint16_t txBufSize, uint8_t **rxBuf, uint16_t **rcvLen, uint32_t fwt );

//static void ledsOn(void);
//static void ledsOff(void);



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


///*!
// *****************************************************************************
// * \brief Check user button
// *
// *  This function check whethe the user button has been pressed
// *****************************************************************************
// */
//
//static void checkUserButton(void)
//{
//    /* Check if USER button is pressed */
//    if( platformGpioIsLow(PLATFORM_USER_BUTTON_PORT, PLATFORM_USER_BUTTON_PIN))
//    {
//        ndefDemoFeature++;
//        ndefDemoFeature %= NDEF_DEMO_MAX_FEATURES;
//
//        ledsOff();
//        ndefDemoPrevFeature = ndefDemoFeature;
//        platformLog("%s\r\n", ndefDemoFeatureDescription[ndefDemoFeature]);
//        /* Debounce button */
//        while( platformGpioIsLow(PLATFORM_USER_BUTTON_PORT, PLATFORM_USER_BUTTON_PIN) );
//        if( ndefDemoFeature != NDEF_DEMO_READ )
//        {
//            timer = platformTimerCreate(NDEF_WRITE_FORMAT_TIMEOUT);
//            timerLed = platformTimerCreate(NDEF_LED_BLINK_DURATION);
//        }
//    }
//}
//
///*!
// *****************************************************************************
// * \brief Show usage
// *
// *  This function displays usage information
// *****************************************************************************
// */
//static void ndefShowDemoUsage()
//{
//#if NDEF_FEATURE_FULL_API
//    uint32_t i;
//    
//    platformLog("Use the User button to cycle among the different modes:\r\n");
//    for (i = 0; i < SIZEOF_ARRAY(ndefDemoFeatureDescription); i++)
//    {
//        platformLog("%s\r\n", ndefDemoFeatureDescription[i]);
//    }
//    platformLog("In Write or Format mode (menu 2, 3 or 4), the demo returns to Read mode (menu 1) if no tag detected after %d seconds\r\n\n", NDEF_WRITE_FORMAT_TIMEOUT/1000);
//#endif /* NDEF_FEATURE_FULL_API */
//}

/*!
 *****************************************************************************
 * \brief Demo Ini
 *
 *  This function initializes the required layers for the demo
 *
 * \return true  : Initialization ok
 * \return false : Initialization failed
 *****************************************************************************
 */
bool demoIni( void )
{
    ReturnCode err;
    
//#if defined(STM32L476xx)
//    if( (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0)
//    {
//        verbose = true;
//    }
//#endif
//    ndefShowDemoUsage();
    
    err = rfalNfcInitialize();
    if( err == ERR_NONE )
    {
        rfalNfcDefaultDiscParams( &discParam );
        
        discParam.devLimit      = 1U;
        
        ST_MEMCPY( &discParam.nfcid3, NFCID3, sizeof(NFCID3) );
        ST_MEMCPY( &discParam.GB, GB, sizeof(GB) );
        discParam.GBLen         = sizeof(GB);
        discParam.p2pNfcaPrio   = true;

        discParam.notifyCb             = NULL;
        discParam.totalDuration        = 1000U;
        discParam.techs2Find           = RFAL_NFC_TECH_NONE;          /* For the demo, enable the NFC Technologies based on RFAL Feature switches */


#if RFAL_FEATURE_NFCA
        discParam.techs2Find          |= RFAL_NFC_POLL_TECH_A;
#endif /* RFAL_FEATURE_NFCA */

#if RFAL_FEATURE_NFCB
        discParam.techs2Find          |= RFAL_NFC_POLL_TECH_B;
#endif /* RFAL_FEATURE_NFCB */

#if RFAL_FEATURE_NFCF
        discParam.techs2Find          |= RFAL_NFC_POLL_TECH_F;
#endif /* RFAL_FEATURE_NFCF */

#if RFAL_FEATURE_NFCV
        discParam.techs2Find          |= RFAL_NFC_POLL_TECH_V;
#endif /* RFAL_FEATURE_NFCV */

#if RFAL_FEATURE_ST25TB
        discParam.techs2Find          |= RFAL_NFC_POLL_TECH_ST25TB;
#endif /* RFAL_FEATURE_ST25TB */

#if ST25R95
        discParam.isoDepFS           = RFAL_ISODEP_FSXI_128;          /* ST25R95 cannot support 256 bytes of data block */
#endif /* ST25R95 */

#if RFAL_SUPPORT_MODE_POLL_ACTIVE_P2P && RFAL_FEATURE_NFC_DEP
        discParam.techs2Find |= RFAL_NFC_POLL_TECH_AP2P;
#endif /* RFAL_SUPPORT_MODE_POLL_ACTIVE_P2P && RFAL_FEATURE_NFC_DEP */

#if RFAL_SUPPORT_MODE_LISTEN_ACTIVE_P2P && RFAL_FEATURE_NFC_DEP && RFAL_FEATURE_LISTEN_MODE
        discParam.techs2Find |= RFAL_NFC_LISTEN_TECH_AP2P;
#endif /* RFAL_SUPPORT_MODE_LISTEN_ACTIVE_P2P && RFAL_FEATURE_NFC_DEP && RFAL_FEATURE_LISTEN_MODE */


#if DEMO_CARD_EMULATION_ONLY
        discParam.totalDuration        = 60000U;              /* 60 seconds */
        discParam.techs2Find           = RFAL_NFC_TECH_NONE;  /* Overwrite any previous poller modes */
#endif /* DEMO_CARD_EMULATION_ONLY */

#if RFAL_SUPPORT_CE && RFAL_FEATURE_LISTEN_MODE
    
#if RFAL_SUPPORT_MODE_LISTEN_NFCA
        /* Set configuration for NFC-A CE */
        ST_MEMCPY( discParam.lmConfigPA.SENS_RES, ceNFCA_SENS_RES, RFAL_LM_SENS_RES_LEN );     /* Set SENS_RES / ATQA */
        ST_MEMCPY( discParam.lmConfigPA.nfcid, ceNFCA_NFCID, RFAL_LM_NFCID_LEN_04 );           /* Set NFCID / UID */
        discParam.lmConfigPA.nfcidLen = RFAL_LM_NFCID_LEN_04;                                  /* Set NFCID length to 7 bytes */
        discParam.lmConfigPA.SEL_RES  = ceNFCA_SEL_RES;                                        /* Set SEL_RES / SAK */

        discParam.techs2Find |= RFAL_NFC_LISTEN_TECH_A;
#endif /* RFAL_SUPPORT_MODE_LISTEN_NFCA */

#if RFAL_SUPPORT_MODE_LISTEN_NFCF
        /* Set configuration for NFC-F CE */
        ST_MEMCPY( discParam.lmConfigPF.SC, ceNFCF_SC, RFAL_LM_SENSF_SC_LEN );                 /* Set System Code */
        ST_MEMCPY( &ceNFCF_SENSF_RES[RFAL_NFCF_CMD_LEN], ceNFCF_nfcid2, RFAL_NFCID2_LEN );     /* Load NFCID2 on SENSF_RES */
        ST_MEMCPY( discParam.lmConfigPF.SENSF_RES, ceNFCF_SENSF_RES, RFAL_LM_SENSF_RES_LEN );  /* Set SENSF_RES / Poll Response */

        discParam.techs2Find |= RFAL_NFC_LISTEN_TECH_F;
#endif /* RFAL_SUPPORT_MODE_LISTEN_NFCF */
#endif /* RFAL_SUPPORT_CE && RFAL_FEATURE_LISTEN_MODE */

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

#if RFAL_FEATURE_NFCA
    rfalNfcaSensRes       sensRes;
    rfalNfcaSelRes        selRes;
#endif /* RFAL_FEATURE_NFCA */

#if RFAL_FEATURE_NFCB
    rfalNfcbSensbRes      sensbRes;
    uint8_t               sensbResLen;
#endif /* RFAL_FEATURE_NFCB */

#if RFAL_FEATURE_NFCF
    uint8_t               devCnt = 0;
    rfalFeliCaPollRes     cardList[1];
    uint8_t               collisions = 0U;
    rfalNfcfSensfRes*     sensfRes;
#endif /* RFAL_FEATURE_NFCF */

#if RFAL_FEATURE_NFCV
    rfalNfcvInventoryRes  invRes;
    uint16_t              rcvdLen;
#endif /* RFAL_FEATURE_NFCV */
    
    rfalNfcWorker();                                    /* Run RFAL worker periodically */

//    if( (ndefDemoFeature != NDEF_DEMO_READ) ) //if( (ndefDemoFeature != NDEF_DEMO_READ) && (platformTimerIsExpired(timer)) )
//    {
//        SYS_CONSOLE_MESSAGE("Timer expired, back to Read mode...\r\n");
//        ndefDemoFeature = NDEF_DEMO_READ;
////        ledsOff();
//    }
    
//    if( ndefDemoFeature != ndefDemoPrevFeature )
//    {
//        ndefDemoPrevFeature = ndefDemoFeature;
//        SYS_CONSOLE_PRINT("%s\r\n", ndefDemoFeatureDescription[ndefDemoFeature]);
//    }
//    
//    if( ndefDemoFeature != NDEF_DEMO_READ )
//    {
//        if( platformTimerIsExpired(timerLed) )
//        {
//            timerLed = platformTimerCreate(NDEF_LED_BLINK_DURATION);
//            ledOn = !ledOn;
//        }
//        if( ledOn )
//        {
//            ledsOn();
//        }
//        else
//        {
//            ledsOff();
//        }
//    }
    
//    checkUserButton();
    
    switch( state )
    {
        /*******************************************************************************/
        case DEMO_ST_START_DISCOVERY:
//            ledsOff();

            state = DEMO_ST_DISCOVERY;
            break;

        /*******************************************************************************/
        case DEMO_ST_DISCOVERY:
            if( rfalNfcIsDevActivated( rfalNfcGetState() ) )
            {
                rfalNfcGetActiveDevice( &nfcDevice );
                
//                ledsOff();
                platformDelay(50);
                ndefDemoPrevFeature = 0xFF; /* Force the display of the prompt */
                switch( nfcDevice->type )
                {
                    /*******************************************************************************/
                    #if RFAL_FEATURE_NFCA
                    case RFAL_NFC_LISTEN_TYPE_NFCA:
                    
//                        platformLedOn(PLATFORM_LED_A_PORT, PLATFORM_LED_A_PIN);
                        switch( nfcDevice->dev.nfca.type )
                        {
                            case RFAL_NFCA_T1T:
                                SYS_CONSOLE_PRINT("ISO14443A/Topaz (NFC-A T1T) TAG found. UID: %s\r\n", hex2Str( nfcDevice->nfcid, nfcDevice->nfcidLen ) );
                                rfalNfcaPollerSleep();
                                break;
                            
                            case RFAL_NFCA_T4T:
                                SYS_CONSOLE_PRINT("NFCA Passive ISO-DEP device found. UID: %s\r\n", hex2Str( nfcDevice->nfcid, nfcDevice->nfcidLen ) );
//                                demoNdef(nfcDevice);
//                                rfalIsoDepDeselect(); 
                                break;
                            
                            case RFAL_NFCA_T4T_NFCDEP:
                            case RFAL_NFCA_NFCDEP:
                                SYS_CONSOLE_PRINT("NFCA Passive P2P device found. NFCID: %s\r\n", hex2Str( nfcDevice->nfcid, nfcDevice->nfcidLen ) );
//                                demoP2P();
                                break;
                                
                            default:
                                SYS_CONSOLE_PRINT("ISO14443A/NFC-A card found. UID: %s\r\n", hex2Str( nfcDevice->nfcid, nfcDevice->nfcidLen ) );
                                demoNdef(nfcDevice);
                                rfalNfcaPollerSleep();
                                break;
                        }
                        /* Loop until tag is removed from the field */
                        SYS_CONSOLE_MESSAGE("Operation completed\r\nTag can be removed from the field\r\n");
                        rfalNfcaPollerInitialize();
                        while( rfalNfcaPollerCheckPresence(RFAL_14443A_SHORTFRAME_CMD_WUPA, &sensRes) == ERR_NONE )
                        {
                            if( ((nfcDevice->dev.nfca.type == RFAL_NFCA_T1T) && (!rfalNfcaIsSensResT1T(&sensRes ))) ||
                                ((nfcDevice->dev.nfca.type != RFAL_NFCA_T1T) && (rfalNfcaPollerSelect(nfcDevice->dev.nfca.nfcId1, nfcDevice->dev.nfca.nfcId1Len, &selRes) != ERR_NONE)) )
                            {
                                break;
                            }
                            rfalNfcaPollerSleep();
                            platformDelay(130);
                        }
                        break;
                    #endif /* RFAL_FEATURE_NFCA */
                    
                    /*******************************************************************************/
                    #if RFAL_FEATURE_NFCB
                    case RFAL_NFC_LISTEN_TYPE_NFCB:
                        
                        SYS_CONSOLE_PRINT("ISO14443B/NFC-B card found. UID: %s\r\n", hex2Str( nfcDevice->nfcid, nfcDevice->nfcidLen ) );
//                        platformLedOn(PLATFORM_LED_B_PORT, PLATFORM_LED_B_PIN);
                    
                        if( rfalNfcbIsIsoDepSupported( &nfcDevice->dev.nfcb ) )
                        {
//                            demoNdef(nfcDevice);
                            rfalIsoDepDeselect();
                        }
                        else
                        {
                            rfalNfcbPollerSleep(nfcDevice->dev.nfcb.sensbRes.nfcid0);
                        }
                        /* Loop until tag is removed from the field */
                        SYS_CONSOLE_MESSAGE("Operation completed\r\nTag can be removed from the field\r\n");
                        rfalNfcbPollerInitialize();
                        while( rfalNfcbPollerCheckPresence(RFAL_NFCB_SENS_CMD_ALLB_REQ, RFAL_NFCB_SLOT_NUM_1, &sensbRes, &sensbResLen) == ERR_NONE )
                        {
                            if( ST_BYTECMP(sensbRes.nfcid0, nfcDevice->dev.nfcb.sensbRes.nfcid0, RFAL_NFCB_NFCID0_LEN) != 0 )
                            {
                                break;
                            }
                            rfalNfcbPollerSleep(nfcDevice->dev.nfcb.sensbRes.nfcid0);
                            platformDelay(130);
                        }
                        break;
                    #endif /* RFAL_FEATURE_NFCB */
                        
                    /*******************************************************************************/
                    #if RFAL_FEATURE_NFCF
                    case RFAL_NFC_LISTEN_TYPE_NFCF:
                        
                        if( rfalNfcfIsNfcDepSupported( &nfcDevice->dev.nfcf ) )
                        {
                            SYS_CONSOLE_PRINT("NFCF Passive P2P device found. NFCID: %s\r\n", hex2Str( nfcDevice->nfcid, nfcDevice->nfcidLen ) );
//                            demoP2P();
                        }
                        else
                        {
                            SYS_CONSOLE_PRINT("Felica/NFC-F card found. UID: %s\r\n", hex2Str( nfcDevice->nfcid, nfcDevice->nfcidLen ));
//                            demoNdef(nfcDevice);
                        }
                        
//                        platformLedOn(PLATFORM_LED_F_PORT, PLATFORM_LED_F_PIN);
                        /* Loop until tag is removed from the field */
                        SYS_CONSOLE_MESSAGE("Operation completed\r\nTag can be removed from the field\r\n");
                        devCnt = 1;
                        rfalNfcfPollerInitialize( RFAL_BR_212 );
                        while (rfalNfcfPollerPoll( RFAL_FELICA_1_SLOT, RFAL_NFCF_SYSTEMCODE, RFAL_FELICA_POLL_RC_NO_REQUEST, cardList, &devCnt, &collisions ) == ERR_NONE)
                        {
                            /* Skip the length field byte */
                            sensfRes = (rfalNfcfSensfRes*)&((uint8_t *)cardList)[1];
                            if( ST_BYTECMP(sensfRes->NFCID2, nfcDevice->dev.nfcf.sensfRes.NFCID2, RFAL_NFCF_NFCID2_LEN) != 0 )
                            {
                                break;
                            }
                            platformDelay(130);
                        }
                        break;
                    #endif /* RFAL_FEATURE_NFCF */
                    
                    /*******************************************************************************/
                    #if RFAL_FEATURE_NFCV
                    case RFAL_NFC_LISTEN_TYPE_NFCV:
                        {
                            uint8_t devUID[RFAL_NFCV_UID_LEN];
                            
                            ST_MEMCPY( devUID, nfcDevice->nfcid, nfcDevice->nfcidLen );   /* Copy the UID into local var */
                            REVERSE_BYTES( devUID, RFAL_NFCV_UID_LEN );                 /* Reverse the UID for display purposes */
                            SYS_CONSOLE_PRINT("ISO15693/NFC-V card found. UID: %s\r\n", hex2Str(devUID, RFAL_NFCV_UID_LEN));
//                            platformLedOn(PLATFORM_LED_V_PORT, PLATFORM_LED_V_PIN);
                            
//                            demoNdef(nfcDevice);

                            /* Loop until tag is removed from the field */
                            SYS_CONSOLE_PRINT("Operation completed\r\nTag can be removed from the field\r\n");
                            rfalNfcvPollerInitialize();
                            while (rfalNfcvPollerInventory( RFAL_NFCV_NUM_SLOTS_1, RFAL_NFCV_UID_LEN * 8U, nfcDevice->dev.nfcv.InvRes.UID, &invRes, &rcvdLen) == ERR_NONE)
                            {
                                platformDelay(130);
                            }
                        }
                        break;
                    #endif /* RFAL_FEATURE_NFCV */
                        
                    /*******************************************************************************/
                    #if RFAL_FEATURE_ST25TB
                    case RFAL_NFC_LISTEN_TYPE_ST25TB:
                        
                        SYS_CONSOLE_PRINT("ST25TB card found. UID: %s\r\n", hex2Str( nfcDevice->nfcid, nfcDevice->nfcidLen ));
//                        platformLedOn(PLATFORM_LED_B_PORT, PLATFORM_LED_B_PIN);
                        break;
                    #endif /* RFAL_FEATURE_ST25TB */
                    
                    /*******************************************************************************/
                    case RFAL_NFC_LISTEN_TYPE_AP2P:
                        
                        SYS_CONSOLE_PRINT("NFC Active P2P device found. NFCID3: %s\r\n", hex2Str(nfcDevice->nfcid, nfcDevice->nfcidLen));
//                        platformLedOn(PLATFORM_LED_AP2P_PORT, PLATFORM_LED_AP2P_PIN);
                    
//                        demoP2P();
                        break;
                    
                    /*******************************************************************************/
                    default:
                        break;
                }
                
                rfalNfcDeactivate( RFAL_NFC_DEACTIVATE_DISCOVERY );
                state = DEMO_ST_START_DISCOVERY;
            }
            break;

        /*******************************************************************************/
        case DEMO_ST_NOTINIT:
            SYS_CONSOLE_MESSAGE("NFC NOT INITIALIZED\r\n");
            break;
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
void demoP2P( void )
{
#if RFAL_FEATURE_NFC_DEP

    uint16_t   *rxLen;
    uint8_t    *rxData;
    ReturnCode err;

    ndefBuffer  bufPayload;
    ndefMessage message;
    ndefRecord  record;
    ndefType    uri;

    platformLog(" Initialize device .. ");
    err = demoTransceiveBlocking( ndefInit, sizeof(ndefInit), &rxData, &rxLen, RFAL_FWT_NONE);
    if( err != ERR_NONE )
    {
        platformLog("failed.\r\n");
        return;
    }
    platformLog("succeeded.\r\n");
    
    err  = ndefRtdUriInit(&uri, NDEF_URI_PREFIX_HTTP_WWW, &bufURL);
    err |= ndefRtdUriToRecord(&uri, &record);

    err |= ndefMessageInit(&message);
    err |= ndefMessageAppend(&message, &record);  /* To get MB and ME bits set */

    /* Build the SNEP buffer made of the prefix, the length byte and the record */
    ST_MEMCPY(ndefUriBuffer, ndefSnepPrefix, sizeof(ndefSnepPrefix));

    /* Skip 1 byte for length byte */
    bufPayload.buffer = ndefUriBuffer + sizeof(ndefSnepPrefix) + 1;
    bufPayload.length = sizeof(ndefUriBuffer) - sizeof(ndefSnepPrefix);
    err |= ndefMessageEncode(&message, &bufPayload);

    ndefUriBuffer[sizeof(ndefSnepPrefix)] = bufPayload.length;

    bufPayload.buffer = ndefUriBuffer;
    bufPayload.length = sizeof(ndefSnepPrefix) + 1 + bufPayload.length;

    if( err != ERR_NONE )
    {
        platformLog("NDEF message creation failed (%d)\r\n", err);
        return;
    }

    ndefBufferDump("URL converted to SNEP:\r\n", (ndefConstBuffer*)&bufPayload, true);

    platformLog(" Push NDEF Uri: www.ST.com .. ");
    err = demoTransceiveBlocking(bufPayload.buffer, bufPayload.length, &rxData, &rxLen, RFAL_FWT_NONE);
    if( err != ERR_NONE )
    {
        platformLog("failed.\r\n");
        return;
    }
    platformLog("succeeded.\r\n");


    platformLog(" Device present, maintaining connection ");
    while(err == ERR_NONE) 
    {
        err = demoTransceiveBlocking( ndefLLCPSYMM, sizeof(ndefLLCPSYMM), &rxData, &rxLen, RFAL_FWT_NONE);
        platformLog(".");
        platformDelay(50);
    }
    platformLog("\r\n Device removed.\r\n");
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

static void demoNdef(rfalNfcDevice *pNfcDevice)
{
    ReturnCode       err;
    ndefMessage      message;
    uint32_t         rawMessageLen;
    ndefInfo         info;
    ndefBuffer       bufRawMessage;
    ndefConstBuffer  bufConstRawMessage;
 
#if NDEF_FEATURE_FULL_API
    ndefRecord       record1;
    ndefRecord       record2;

    ndefType         text;
    ndefType         uri;
    ndefType         aar;

    ndefConstBuffer8 bufTextLangCode;
    ndefConstBuffer bufTextLangText;
    ndefConstBuffer bufUri;
    ndefConstBuffer bufAndroidPackName;
#endif /* NDEF_FEATURE_FULL_API */

    /*
     * Perform NDEF Context Initialization
     */
    err = ndefPollerContextInitialization(&ndefCtx, pNfcDevice);
    if( err != ERR_NONE )
    {
        SYS_CONSOLE_PRINT("NDEF NOT DETECTED (ndefPollerContextInitialization returns %d)\r\n", err);
        return;
    }
    
    #if NDEF_FEATURE_T5T
    if( verbose && (pNfcDevice->type == RFAL_NFC_LISTEN_TYPE_NFCV) )
    {
        ndefDumpSysInfo(&ndefCtx);
    }
    #endif /* RFAL_FEATURE_T5T */
    
    /*
     * Perform NDEF Detect procedure
     */
    err = ndefPollerNdefDetect(&ndefCtx, &info);
    if( err != ERR_NONE )
    {
        SYS_CONSOLE_PRINT("NDEF NOT DETECTED (ndefPollerNdefDetect returns %d)\r\n", err);
        if( ndefDemoFeature != NDEF_DEMO_FORMAT_TAG)
        {
            return;
        }
    }
    else
    {
        SYS_CONSOLE_PRINT("%s NDEF detected.\r\n", ndefStates[info.state]);
        ndefCCDump(&ndefCtx);

        if( verbose )
        {
            SYS_CONSOLE_PRINT("NDEF Len: %d, Offset=%d\r\n", ndefCtx.messageLen, ndefCtx.messageOffset);
        }
    }

    switch( ndefDemoFeature )
    {
        /*
         * Demonstrate how to read the NDEF message from the Tag
         */
        case NDEF_DEMO_READ:
            if( info.state == NDEF_STATE_INITIALIZED )
            {
                /* Nothing to read... */
                return;
            }
            err = ndefPollerReadRawMessage(&ndefCtx, rawMessageBuf, sizeof(rawMessageBuf), &rawMessageLen, true);
            if( err != ERR_NONE )
            {
                SYS_CONSOLE_PRINT("NDEF message cannot be read (ndefPollerReadRawMessage returns %d)\r\n", err);
                return;
            }
            if( verbose )
            {
                bufRawMessage.buffer = rawMessageBuf;
                bufRawMessage.length = rawMessageLen;
                ndefBufferDump(" NDEF Content", (ndefConstBuffer*)&bufRawMessage, verbose);
            }
            bufConstRawMessage.buffer = rawMessageBuf;
            bufConstRawMessage.length = rawMessageLen;
            err = ndefMessageDecode(&bufConstRawMessage, &message);
            if( err != ERR_NONE )
            {
                SYS_CONSOLE_PRINT("NDEF message cannot be decoded (ndefMessageDecode returns %d)\r\n", err);
                return;
            }
            err = ndefMessageDump(&message, verbose);
            if( err != ERR_NONE )
            {
                SYS_CONSOLE_PRINT("NDEF message cannot be displayed (ndefMessageDump returns %d)\r\n", err);
                return;
            }
            break;

#if NDEF_FEATURE_FULL_API
        /*
         * Demonstrate how to encode a text record and write the message to the tag
         */
        case NDEF_DEMO_WRITE_MSG1:
            ndefDemoFeature = NDEF_DEMO_READ; /* returns to READ mode after write */
            err  = ndefMessageInit(&message); /* Initialize message structure */
            bufTextLangCode.buffer = ndefTextLangCode;
            bufTextLangCode.length = strlen((char *)ndefTextLangCode);

            bufTextLangText.buffer = ndefTEXT;
            bufTextLangText.length = strlen((char *)ndefTEXT);

            err |= ndefRtdTextInit(&text, TEXT_ENCODING_UTF8, &bufTextLangCode, &bufTextLangText); /* Initialize Text type structure */
            err |= ndefRtdTextToRecord(&text, &record1); /* Encode Text Record */
            err |= ndefMessageAppend(&message, &record1); /* Append Text record to message */
            if( err != ERR_NONE )
            {
                SYS_CONSOLE_PRINT("Message creation failed\r\n", err);
                return;
            }
            err = ndefPollerWriteMessage(&ndefCtx, &message); /* Write message */
            if( err != ERR_NONE )
            {
                SYS_CONSOLE_PRINT("Message cannot be written (ndefPollerWriteMessage return %d)\r\n", err);
                return;
            }
            SYS_CONSOLE_MESSAGE("Wrote 1 record to the Tag\r\n");
            if( verbose )
            {
                /* Dump raw message */
                bufRawMessage.buffer = rawMessageBuf;
                bufRawMessage.length = sizeof(rawMessageBuf);
                err = ndefMessageEncode(&message, &bufRawMessage);
                if( err == ERR_NONE )
                {
                    ndefBufferDump("Raw message", (ndefConstBuffer*)&bufRawMessage, verbose);
                }
            }
//            LedNotificationWriteDone();
            break;

        /*
         * Demonstrate how to encode a URI record and a AAR record, how to encode the message to a raw buffer and then how to write the raw buffer
         */
        case NDEF_DEMO_WRITE_MSG2:
            ndefDemoFeature = NDEF_DEMO_READ;  /* returns to READ mode after write */
            err  = ndefMessageInit(&message);  /* Initialize message structure */
            bufUri.buffer = ndefURI;
            bufUri.length = strlen((char *)ndefURI);
            err |= ndefRtdUriInit(&uri, NDEF_URI_PREFIX_HTTP_WWW, &bufUri); /* Initialize URI type structure */
            err |= ndefRtdUriToRecord(&uri, &record1); /* Encode URI Record */

            bufAndroidPackName.buffer = ndefAndroidPackName;
            bufAndroidPackName.length = sizeof(ndefAndroidPackName) - 1U;
            err |= ndefRtdAarInit(&aar, &bufAndroidPackName); /* Initialize AAR type structure */
            err |= ndefRtdAarToRecord(&aar, &record2); /* Encode AAR record */

            err |= ndefMessageAppend(&message, &record1); /* Append URI to message */
            err |= ndefMessageAppend(&message, &record2); /* Append AAR to message (record #2 is an example of preformatted record) */

            bufRawMessage.buffer = rawMessageBuf;
            bufRawMessage.length = sizeof(rawMessageBuf);
            err |= ndefMessageEncode(&message, &bufRawMessage); /* Encode the message to the raw buffer */
            if( err != ERR_NONE )
            {
                SYS_CONSOLE_PRINT("Raw message creation failed (%d)\r\n", err);
                return;
            }
            err = ndefPollerWriteRawMessage(&ndefCtx, bufRawMessage.buffer, bufRawMessage.length);
            if( err != ERR_NONE )
            {
                SYS_CONSOLE_PRINT("Message cannot be written (ndefPollerWriteRawMessage return %d)\r\n", err);
                return;
            }
            SYS_CONSOLE_MESSAGE("Wrote 2 records to the Tag\r\n");
            if( verbose )
            {
                /* Dump raw message */
                ndefBufferDump("Raw message", (ndefConstBuffer*)&bufRawMessage, verbose);
            }
//            LedNotificationWriteDone();
            break;

        /*
         * Demonstrate how to format a Tag
         */
        case NDEF_DEMO_FORMAT_TAG:
            ndefDemoFeature = NDEF_DEMO_READ;
            if( !ndefIsSTTag(&ndefCtx) )
            {
                SYS_CONSOLE_MESSAGE("Manufacturer ID not found or not an ST tag. Format aborted \r\n");
                return;
            }
            SYS_CONSOLE_MESSAGE("Formatting Tag...\r\n");
            /* Format Tag */
            err = ndefPollerTagFormat(&ndefCtx, NULL, 0);
            if( err != ERR_NONE )
            {
                SYS_CONSOLE_PRINT("Tag cannot be formatted (ndefPollerTagFormat returns %d)\r\n", err);
                return;
            }
            SYS_CONSOLE_MESSAGE("Tag formatted\r\n");
//            LedNotificationWriteDone();
            break;
#endif /* NDEF_FEATURE_FULL_API */

        default:
            ndefDemoFeature = NDEF_DEMO_READ;
            break;
    }
    return;
}

#if NDEF_FEATURE_T2T
static void ndefT2TCCDump(ndefContext *ctx)
{
    ndefConstBuffer bufCcBuf;

    platformLog(" * Magic: %2.2Xh Version: %d.%d Size: %d (%d bytes) \r\n * readAccess: %2.2xh writeAccess: %2.2xh \r\n", ctx->cc.t2t.magicNumber, ctx->cc.t2t.majorVersion, ctx->cc.t2t.minorVersion, ctx->cc.t2t.size, ctx->cc.t2t.size * 8U, ctx->cc.t2t.readAccess, ctx->cc.t2t.writeAccess);
    bufCcBuf.buffer = ctx->ccBuf;
    bufCcBuf.length = 4;
    ndefBufferDump(" CC Raw Data", &bufCcBuf, verbose);
  
}
#endif /* NDEF_FEATURE_T2T */

#if NDEF_FEATURE_T3T
static void ndefT3TAIBDump(ndefContext *ctx)
{
    ndefConstBuffer bufCcBuf;

    platformLog(" * Version: %d.%d Size: %d (%d bytes) NbR: %d NbW: %d\r\n * WriteFlag: %2.2xh RWFlag: %2.2xh \r\n", ctx->cc.t3t.majorVersion, ctx->cc.t3t.minorVersion, ctx->cc.t3t.nMaxB, ctx->cc.t3t.nMaxB * 16U, ctx->cc.t3t.nbR, ctx->cc.t3t.nbW, ctx->cc.t3t.writeFlag, ctx->cc.t3t.rwFlag);
    bufCcBuf.buffer = ctx->ccBuf;
    bufCcBuf.length = 16;
    ndefBufferDump(" CC Raw Data", &bufCcBuf, verbose);
}
#endif /* NDEF_FEATURE_T3T */

#if NDEF_FEATURE_T4T
static void ndefT4TCCDump(ndefContext *ctx)
{
    ndefConstBuffer bufCcBuf;
    
    platformLog(" * CCLEN: %d T4T_VNo: %xh MLe: %d MLc: %d FileId: %2.2x%2.2xh FileSize: %d\r\n * readAccess: %2.2xh writeAccess: %2.2xh\r\n", ctx->cc.t4t.ccLen, ctx->cc.t4t.vNo, ctx->cc.t4t.mLe, ctx->cc.t4t.mLc, ctx->cc.t4t.fileId[0], ctx->cc.t4t.fileId[1],ctx->cc.t4t.fileSize, ctx->cc.t4t.readAccess, ctx->cc.t4t.writeAccess);
    bufCcBuf.buffer = ctx->ccBuf;
    bufCcBuf.length = ctx->cc.t4t.ccLen;
    ndefBufferDump(" CC File Raw Data", &bufCcBuf, verbose);
}
#endif /* NDEF_FEATURE_T4T */

#if NDEF_FEATURE_T5T
static void ndefT5TCCDump(ndefContext *ctx)
{
    ndefConstBuffer bufCcBuf;
    
    platformLog(" * Block Length: %d\r\n", ctx->subCtx.t5t.blockLen);
    platformLog(" * %d bytes CC\r\n * Magic: %2.2Xh Version: %d.%d MLEN: %d (%d bytes) \r\n * readAccess: %2.2xh writeAccess: %2.2xh \r\n", ctx->cc.t5t.ccLen, ctx->cc.t5t.magicNumber, ctx->cc.t5t.majorVersion, ctx->cc.t5t.minorVersion, ctx->cc.t5t.memoryLen, ctx->cc.t5t.memoryLen * 8U, ctx->cc.t5t.readAccess, ctx->cc.t5t.writeAccess);
    platformLog(" * [%c] Special Frame\r\n",       ctx->cc.t5t.specialFrame ?      'X' : ' ');
    platformLog(" * [%c] Multiple block Read\r\n", ctx->cc.t5t.multipleBlockRead ? 'X' : ' ');
    platformLog(" * [%c] Lock Block\r\n",          ctx->cc.t5t.lockBlock ?         'X' : ' ');
    bufCcBuf.buffer = ctx->ccBuf;
    bufCcBuf.length = ctx->cc.t5t.ccLen;
    ndefBufferDump(" CC Raw Data", &bufCcBuf, verbose);
}
#endif /* NDEF_FEATURE_T5T */

static void ndefCCDump(ndefContext *ctx)
{
    if( (ctx == NULL) || !verbose)
    {
        return;
    }
    SYS_CONSOLE_PRINT("%s", (ctx->type == NDEF_DEV_T3T) ? "NDEF Attribute Information Block\r\n" : "NDEF Capability Container\r\n");
    switch( ctx->type )
    {
        #if NDEF_FEATURE_T2T
        case NDEF_DEV_T2T:
            ndefT2TCCDump(ctx);
            break;
        #endif /* NDEF_FEATURE_T2T */
        #if NDEF_FEATURE_T3T
        case NDEF_DEV_T3T:
            ndefT3TAIBDump(ctx);
            break;
        #endif /* NDEF_FEATURE_T3T */
        #if NDEF_FEATURE_T4T
        case NDEF_DEV_T4T:
            ndefT4TCCDump(ctx);
            break;
        #endif /* NDEF_FEATURE_T4T */
        #if NDEF_FEATURE_T5T
        case NDEF_DEV_T5T:
            ndefT5TCCDump(ctx);
            break;
        #endif /* NDEF_FEATURE_T5T */
        default:
            break;
    }
}

#if NDEF_FEATURE_T5T
static void ndefDumpSysInfo(ndefContext *ctx)
{
    ndefSystemInformation *sysInfo;

    if( (ctx == NULL) || !verbose)
    {
        return;
    }
    
    if( !ctx->subCtx.t5t.sysInfoSupported )
    {
        return;
    }
    
    sysInfo = &ctx->subCtx.t5t.sysInfo;
    platformLog("System Information\r\n");
    platformLog(" * %d byte(s) memory addressing\r\n", ndefT5TSysInfoMOIValue(sysInfo->infoFlags) + 1);
    if( ndefT5TSysInfoDFSIDPresent(sysInfo->infoFlags) )
    {
        platformLog(" * DFSID=%2.2Xh\r\n", sysInfo->DFSID);
    }
    if( ndefT5TSysInfoAFIPresent(sysInfo->infoFlags) )
    {
        platformLog(" * AFI=%2.2Xh\r\n", sysInfo->AFI);
    }
    if( ndefT5TSysInfoMemSizePresent(sysInfo->infoFlags) )
    {
        platformLog(" * %d blocks, %d bytes per block\r\n", sysInfo->numberOfBlock, sysInfo->blockSize);
    }
    if( ndefT5TSysInfoICRefPresent(sysInfo->infoFlags) )
    {
        platformLog(" * ICRef=%2.2xh\r\n", sysInfo->ICRef);
    }
    if( ndefT5TSysInfoCmdListPresent(sysInfo->infoFlags) )
    {
        platformLog(" * [%c] ReadSingleBlock                \r\n", ndefT5TSysInfoReadSingleBlockSupported(sysInfo->supportedCmd)                 ? 'X' : ' ');               
        platformLog(" * [%c] WriteSingleBlock               \r\n", ndefT5TSysInfoWriteSingleBlockSupported(sysInfo->supportedCmd)                ? 'X' : ' ');
        platformLog(" * [%c] LockSingleBlock                \r\n", ndefT5TSysInfoLockSingleBlockSupported(sysInfo->supportedCmd)                 ? 'X' : ' ');
        platformLog(" * [%c] ReadMultipleBlocks             \r\n", ndefT5TSysInfoReadMultipleBlocksSupported(sysInfo->supportedCmd)              ? 'X' : ' ');
        platformLog(" * [%c] WriteMultipleBlocks            \r\n", ndefT5TSysInfoWriteMultipleBlocksSupported(sysInfo->supportedCmd)             ? 'X' : ' ');
        platformLog(" * [%c] Select                         \r\n", ndefT5TSysInfoSelectSupported(sysInfo->supportedCmd)                          ? 'X' : ' ');
        platformLog(" * [%c] ResetToReady                   \r\n", ndefT5TSysInfoResetToReadySupported(sysInfo->supportedCmd)                    ? 'X' : ' ');
        platformLog(" * [%c] GetMultipleBlockSecStatus      \r\n", ndefT5TSysInfoGetMultipleBlockSecStatusSupported(sysInfo->supportedCmd)       ? 'X' : ' ');
        platformLog(" * [%c] WriteAFI                       \r\n", ndefT5TSysInfoWriteAFISupported(sysInfo->supportedCmd)                        ? 'X' : ' ');
        platformLog(" * [%c] LockAFI                        \r\n", ndefT5TSysInfoLockAFISupported(sysInfo->supportedCmd)                         ? 'X' : ' ');
        platformLog(" * [%c] WriteDSFID                     \r\n", ndefT5TSysInfoWriteDSFIDSupported(sysInfo->supportedCmd)                      ? 'X' : ' ');
        platformLog(" * [%c] LockDSFID                      \r\n", ndefT5TSysInfoLockDSFIDSupported(sysInfo->supportedCmd)                       ? 'X' : ' ');
        platformLog(" * [%c] GetSystemInformation           \r\n", ndefT5TSysInfoGetSystemInformationSupported(sysInfo->supportedCmd)            ? 'X' : ' ');
        platformLog(" * [%c] CustomCmds                     \r\n", ndefT5TSysInfoCustomCmdsSupported(sysInfo->supportedCmd)                      ? 'X' : ' ');
        platformLog(" * [%c] FastReadMultipleBlocks         \r\n", ndefT5TSysInfoFastReadMultipleBlocksSupported(sysInfo->supportedCmd)          ? 'X' : ' ');
        platformLog(" * [%c] ExtReadSingleBlock             \r\n", ndefT5TSysInfoExtReadSingleBlockSupported(sysInfo->supportedCmd)              ? 'X' : ' '); 
        platformLog(" * [%c] ExtWriteSingleBlock            \r\n", ndefT5TSysInfoExtWriteSingleBlockSupported(sysInfo->supportedCmd)             ? 'X' : ' ');
        platformLog(" * [%c] ExtLockSingleBlock             \r\n", ndefT5TSysInfoExtLockSingleBlockSupported(sysInfo->supportedCmd)              ? 'X' : ' ');
        platformLog(" * [%c] ExtReadMultipleBlocks          \r\n", ndefT5TSysInfoExtReadMultipleBlocksSupported(sysInfo->supportedCmd)           ? 'X' : ' ');
        platformLog(" * [%c] ExtWriteMultipleBlocks         \r\n", ndefT5TSysInfoExtWriteMultipleBlocksSupported(sysInfo->supportedCmd)          ? 'X' : ' ');
        platformLog(" * [%c] ExtGetMultipleBlockSecStatus   \r\n", ndefT5TSysInfoExtGetMultipleBlockSecStatusSupported(sysInfo->supportedCmd)    ? 'X' : ' ');
        platformLog(" * [%c] FastExtendedReadMultipleBlocks \r\n", ndefT5TSysInfoFastExtendedReadMultipleBlocksSupported(sysInfo->supportedCmd)  ? 'X' : ' ');
    }
    return;
}
#endif /* NDEF_FEATURE_T5T */

#if NDEF_FEATURE_FULL_API
static bool ndefIsSTTag(ndefContext *ctx)
{
    bool ret = false;

#if defined(STM32L476xx) /* Enable to format any manufacturer tag while debugging */
    if( (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0)
    {
        ret = true;
    }
#endif
    if( ctx == NULL )
    {   
        return ret;
    }
    switch (ctx->device.type)
    {
        case RFAL_NFC_LISTEN_TYPE_NFCA:
            if( (ctx->device.dev.nfca.nfcId1Len != 4) && (ctx->device.dev.nfca.nfcId1[0] == 0x02 ) )
            {  
                ret = true;
            }
            break;
        case RFAL_NFC_LISTEN_TYPE_NFCF:
            break;
        case RFAL_NFC_LISTEN_TYPE_NFCB:
            break;
        case RFAL_NFC_LISTEN_TYPE_NFCV:
            if( ctx->device.dev.nfcv.InvRes.UID[6] == 0x02 )
            {  
                ret = true;
            }
            break;
        default:
            break;
    }
    return (ret);
}
#endif /* NDEF_FEATURE_FULL_API */

//#if NDEF_FEATURE_FULL_API
//static void LedNotificationWriteDone(void)
//{
//    uint32_t i;
//
//    for (i = 0; i < 3; i++)
//    {
////        ledsOn();
//        platformDelay(100);
//
////        ledsOff();
//        platformDelay(100);
//    }
//}
//#endif /* NDEF_FEATURE_FULL_API */

//static void ledsOn(void)
//{
//    platformLedOn(PLATFORM_LED_A_PORT, PLATFORM_LED_A_PIN);
//    platformLedOn(PLATFORM_LED_B_PORT, PLATFORM_LED_B_PIN);
//    platformLedOn(PLATFORM_LED_F_PORT, PLATFORM_LED_F_PIN);
//    platformLedOn(PLATFORM_LED_V_PORT, PLATFORM_LED_V_PIN);
//    platformLedOn(PLATFORM_LED_AP2P_PORT, PLATFORM_LED_AP2P_PIN);
//    platformLedOn(PLATFORM_LED_FIELD_PORT, PLATFORM_LED_FIELD_PIN);
//}
//
//static void ledsOff(void)
//{
//    platformLedOff(PLATFORM_LED_A_PORT, PLATFORM_LED_A_PIN);
//    platformLedOff(PLATFORM_LED_B_PORT, PLATFORM_LED_B_PIN);
//    platformLedOff(PLATFORM_LED_F_PORT, PLATFORM_LED_F_PIN);
//    platformLedOff(PLATFORM_LED_V_PORT, PLATFORM_LED_V_PIN);
//    platformLedOff(PLATFORM_LED_AP2P_PORT, PLATFORM_LED_AP2P_PIN);
//    platformLedOff(PLATFORM_LED_FIELD_PORT, PLATFORM_LED_FIELD_PIN);
//}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
