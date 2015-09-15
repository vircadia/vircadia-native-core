//==============================================================================

#ifndef _H_connexionclientapi
#define _H_connexionclientapi

#include <stdbool.h>
#include "ConnexionClient.h"

//==============================================================================
#ifdef __cplusplus
extern "C" {
#endif
//==============================================================================
// Callback procedure types

typedef void	(*ConnexionAddedHandlerProc)		(unsigned int productID);
typedef void	(*ConnexionRemovedHandlerProc)		(unsigned int productID);
typedef void	(*ConnexionMessageHandlerProc)		(unsigned int productID, unsigned int messageType, void *messageArgument);

// NOTE for ConnexionMessageHandlerProc:
// when messageType == kConnexionMsgDeviceState, messageArgument points to ConnexionDeviceState with size kConnexionDeviceStateSize
// when messageType == kConnexionMsgPrefsChanged, messageArgument points to the target application signature with size sizeof(uint32_t)

//==============================================================================
// Public APIs to be called once when the application starts up or shuts down

int16_t			SetConnexionHandlers				(ConnexionMessageHandlerProc messageHandler, ConnexionAddedHandlerProc addedHandler, ConnexionRemovedHandlerProc removedHandler, bool useSeparateThread);
void			CleanupConnexionHandlers			(void);

// Obsolete API replaced by SetConnexionHandlers, will be removed in the future

int16_t			InstallConnexionHandlers			(ConnexionMessageHandlerProc messageHandler, ConnexionAddedHandlerProc addedHandler, ConnexionRemovedHandlerProc removedHandler);

//==============================================================================
// Public APIs to be called whenever the app wants to start/stop receiving data
// the mask parameter (client capabilities mask) specifies which controls must be forwarded to the client
// buttonMask (previously part of the client capabilities mask) specifies which buttons must be forwarded to the client

uint16_t		RegisterConnexionClient				(uint32_t signature, uint8_t *name, uint16_t mode, uint32_t mask);
void			SetConnexionClientMask				(uint16_t clientID, uint32_t mask);
void			SetConnexionClientButtonMask		(uint16_t clientID, uint32_t buttonMask);
void			UnregisterConnexionClient			(uint16_t clientID);

//==============================================================================
// Public API to send control commands to the driver and retrieve a result value
// Note: the new ConnexionClientControl variant is strictly required for
// kConnexionCtlSetSwitches and kConnexionCtlClearSwitches but also works for
// all other Control calls. The old variant remains for backwards compatibility.

int16_t			ConnexionControl					(uint32_t message, int32_t param, int32_t *result);
int16_t			ConnexionClientControl				(uint16_t clientID, uint32_t message, int32_t param, int32_t *result);

//==============================================================================
// Public API to fetch the current device preferences for either the first connected device or a specific device type (kDevID_Xxx)

int16_t			ConnexionGetCurrentDevicePrefs		(uint32_t deviceID, ConnexionDevicePrefs *prefs);

//==============================================================================
// Public API to set all button labels in the iOS/Android "virtual device" apps

int16_t			ConnexionSetButtonLabels			(uint8_t *labels, uint16_t size);

// Labels data is a series of 32 variable-length null-terminated UTF8-encoded strings.
// The sequence of strings follows the SpacePilot Pro button numbering.
// Empty strings revert the button label to its default value.
// As an example, this data would set the label for button Top to "Top" and
// revert all other button labels to their default values:
// 
//	0x00,						// empty string for Menu
//	0x00,						// empty string for Fit
//	0x54, 0x6F, 0x70, 0x00,		// utf-8 encoded "Top" string for Top
//	0x00,						// empty string for Left
//	0x00, 0x00, 0x00, 0x00,		// empty strings for Right, Front, etc...
//	0x00, 0x00, 0x00, 0x00,
//	0x00, 0x00, 0x00, 0x00,
//	0x00, 0x00, 0x00, 0x00,
//	0x00, 0x00, 0x00, 0x00,
//	0x00, 0x00, 0x00, 0x00,
//	0x00, 0x00, 0x00, 0x00
//==============================================================================
#ifdef __cplusplus
}
#endif
//==============================================================================

#endif	// _H_connexionclientapi

//==============================================================================
