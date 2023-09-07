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
 *  \brief NDEF message dump utils header file
 *
 */

#ifndef NDEF_DUMP_H
#define NDEF_DUMP_H


/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */

#include "NDEF/message/ndef_record.h"
#include "NDEF/message/ndef_message.h"
#include "NDEF/message/ndef_types.h"

/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */


/*
 ******************************************************************************
 * GLOBAL TYPES
 ******************************************************************************
 */


/*
 ******************************************************************************
 * GLOBAL FUNCTION PROTOTYPES
 ******************************************************************************
 */


/*!
 *****************************************************************************
 * Dump an NDEF record
 *
 * This function dumps an NDEF record in a formatted readable style
 * The verbose selector enables more or less dump output.
 *
 * \param[in] record
 * \param[in] verbose
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ndefStatus ndefRecordDump(const ndefRecord* record, bool verbose);


/*!
 *****************************************************************************
 * Dump an NDEF message
 *
 * This function dumps an NDEF message in a formatted readable style
 * The verbose selector enables more or less dump output.
 *
 * \param[in] message
 * \param[in] verbose
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ndefStatus ndefMessageDump(const ndefMessage* message, bool verbose);


#if NDEF_TYPE_FLAT_SUPPORT
/*!
 *****************************************************************************
 * Dump a Flat type
 *
 * \param[in] flat: Type to dump
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ndefStatus ndefFlatPayloadTypeDump(const ndefType* flat);
#endif


#if NDEF_TYPE_EMPTY_SUPPORT
/*!
 *****************************************************************************
 * Dump an Empty type
 *
 * \param[in] empty: Type to dump
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ndefStatus ndefEmptyTypeDump(const ndefType* empty);
#endif


#if NDEF_TYPE_RTD_DEVICE_INFO_SUPPORT
/*!
 *****************************************************************************
 * Dump a Device Information RTD well-known type
 *
 * \param[in] devInfo: Well-known type to dump
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ndefStatus ndefRtdDeviceInfoDump(const ndefType* devInfo);
#endif


#if NDEF_TYPE_RTD_TEXT_SUPPORT
/*!
 *****************************************************************************
 * Dump a Text RTD well-known type
 *
 * \param[in] text: Well-known type to dump
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ndefStatus ndefRtdTextDump(const ndefType* text);
#endif


#if NDEF_TYPE_RTD_URI_SUPPORT
/*!
 *****************************************************************************
 * Dump a URI RTD well-known type
 *
 * \param[in] uri: Well-known type to dump
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ndefStatus ndefRtdUriDump(const ndefType* uri);
#endif


#if NDEF_TYPE_RTD_AAR_SUPPORT
/*!
 *****************************************************************************
 * Dump an External RTD type
 *
 * \param[in] ext: Well-known type to dump
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ndefStatus ndefRtdAarDump(const ndefType* ext);
#endif


#if NDEF_TYPE_RTD_WLC_SUPPORT
/*!
 *****************************************************************************
 * Dump a RTD WLC Capability type
 *
 * \param[in] type: Well-known type to dump
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ndefStatus ndefRtdWlcCapabilityDump(const ndefType* type);


/*!
 *****************************************************************************
 * Dump a RTD WLC Status and Information type
 *
 * \param[in] type: Well-known type to dump
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ndefStatus ndefRtdWlcStatusInfoDump(const ndefType* type);


/*!
 *****************************************************************************
 * Dump a RTD WLC Poll Info type
 *
 * \param[in] type: Well-known type to dump
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ndefStatus ndefRtdWlcInfoDump(const ndefType* type);


/*!
 *****************************************************************************
 * Dump a RTD WLC Listen Control type
 *
 * \param[in] type: Well-known type to dump
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ndefStatus ndefRtdWlcControlDump(const ndefType* type);
#endif


#if NDEF_TYPE_MEDIA_SUPPORT
/*!
 *****************************************************************************
 * Dump a Media type
 *
 * \param[in] media: Media type to dump
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ndefStatus ndefMediaTypeDump(const ndefType* media);
#endif


#if NDEF_TYPE_BLUETOOTH_SUPPORT
/*!
 *****************************************************************************
 * Dump a Bluetooth EIR type
 *
 * \param[in] string:  Prefix
 * \param[in] eir:     Bluetooth EIR to dump
 * \param[in] verbose: Increase the length displayed
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ndefStatus ndefBluetoothEirDump(const char* string, const uint8_t* eir, bool verbose);


/*!
 *****************************************************************************
 * Dump a Bluetooth type
 *
 * \param[in] type: Bluetooth type to dump
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ndefStatus ndefBluetoothDump(const ndefType* type);
#endif


#if NDEF_TYPE_VCARD_SUPPORT
/*!
 *****************************************************************************
 * Dump a vCard Media type
 *
 * \param[in] vCard: vCard to dump
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ndefStatus ndefMediaVCardDump(const ndefType* vCard);
#endif


#if NDEF_TYPE_WIFI_SUPPORT
/*!
 *****************************************************************************
 * Dump a Wifi Media type
 *
 * \param[in] wifi: Wifi parameters to dump
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ndefStatus ndefMediaWifiDump(const ndefType* wifi);
#endif


/*!
 *****************************************************************************
 * Dump a supported type
 *
 * \param[in] type: type to dump
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ndefStatus ndefTypeDump(const ndefType* type);


/*!
 *****************************************************************************
 * Dump a well-known type
 *
 * \param[in] record: Record to dump
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ndefStatus ndefRecordDumpType(const ndefRecord* record);


/*!
 *****************************************************************************
 * Dump a raw buffer stored in a ndefBuffer
 *
 * This function dumps a raw buffer in a formatted style
 *
 * \param[in] string:     Message displayed before the buffer
 * \param[in] bufPayload: Payload buffer to display
 * \param[in] verbose     Increase the length displayed
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ndefStatus ndefBufferDump(const char* string, const ndefConstBuffer* bufPayload, bool verbose);


/*!
 *****************************************************************************
 * Dump a raw buffer stored in a ndefBuffer
 *
 * This function dumps a raw buffer in a formatted style
 *
 * \param[in] prefix:     String displayed before the buffer
 * \param[in] bufPayload: Payload buffer to display
 * \param[in] suffix:     String displayed after the buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ndefStatus ndefBufferPrint(const char* prefix, const ndefConstBuffer* bufPayload, const char* suffix);


/*!
 *****************************************************************************
 * Dump a raw buffer stored in a ndefBuffer8
 *
 * This function dumps a raw buffer in a formatted style
 *
 * \param[in] prefix:     String displayed before the buffer
 * \param[in] bufPayload: Payload buffer to display
 * \param[in] suffix:     String displayed after the buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ndefStatus ndefBuffer8Print(const char* prefix, const ndefConstBuffer8* bufPayload, const char* suffix);


#endif /* NDEF_DUMP_H */
