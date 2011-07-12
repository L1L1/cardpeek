/* 
 * This file is part of cardpeek (L1L1@gmx.com).
 *
 * It's content was mostly taken from the OPENSC project, with a few
 * tweaks and simplifications.
 *
 * OPENSC is licenced under the GNU LGPL 2.1. 
 *
 * See http://www.opensc-project.org/
 *
 */

#ifndef WIN32_WINSCARD_H
#define WIN32_WINSCARD_H

#include <windef.h>

#define SCardConnect SCardConnectA
#define SCardListReaders SCardListReadersA 
#define SCardStatus SCardStatusA

/* partial import from opensc.h */

#define MAX_ATR_SIZE                   33      /* Maximum ATR size */

#define SCARD_PROTOCOL_T0		0x0001	/* T=0 active protocol. */
#define SCARD_PROTOCOL_T1		0x0002	/* T=1 active protocol. */
#define SCARD_PROTOCOL_RAW		0x0004	/* Raw active protocol. */

#define SCARD_STATE_UNAWARE		0x0000	/* App wants status */
#define SCARD_STATE_IGNORE		0x0001	/* Ignore this reader */
#define SCARD_STATE_CHANGED		0x0002	/* State has changed */
#define SCARD_STATE_UNKNOWN		0x0004	/* Reader unknown */
#define SCARD_STATE_UNAVAILABLE		0x0008	/* Status unavailable */
#define SCARD_STATE_EMPTY		0x0010	/* Card removed */
#define SCARD_STATE_PRESENT		0x0020	/* Card inserted */
#define SCARD_STATE_EXCLUSIVE		0x0080	/* Exclusive Mode */
#define SCARD_STATE_INUSE		0x0100	/* Shared Mode */
#define SCARD_STATE_MUTE		0x0200	/* Unresponsive card */
#define SCARD_STATE_UNPOWERED		0x0400	/* Unpowered card */


#define SCARD_SHARE_EXCLUSIVE		0x0001	/* Exclusive mode only */
#define SCARD_SHARE_SHARED		0x0002	/* Shared mode only */
#define SCARD_SHARE_DIRECT		0x0003	/* Raw mode only */

#define SCARD_LEAVE_CARD		0x0000	/* Do nothing on close */
#define SCARD_RESET_CARD		0x0001	/* Reset on close */
#define SCARD_UNPOWER_CARD		0x0002	/* Power down on close */

#define SCARD_SCOPE_USER		0x0000	/* Scope in user space */
#define SCARD_SCOPE_SYSTEM		0x0000

#ifndef SCARD_S_SUCCESS	/* conflict in mingw-w64 */
#define SCARD_S_SUCCESS			0x00000000 /* No error was encountered. */
#define SCARD_E_CANCELLED		0x80100002 /* The action was cancelled by an SCardCancel request. */
#define SCARD_E_INVALID_HANDLE		0x80100003 /* The supplied handle was invalid. */
#define SCARD_E_TIMEOUT			0x8010000A /* The user-specified timeout value has expired. */
#define SCARD_E_SHARING_VIOLATION	0x8010000B /* The smart card cannot be accessed because of other connections outstanding. */
#define SCARD_E_NO_SMARTCARD		0x8010000C /* The operation requires a smart card, but no smart card is currently in the device. */
#define SCARD_E_PROTO_MISMATCH		0x8010000F /* The requested protocols are incompatible with the protocol currently in use with the smart card. */
#define SCARD_E_NOT_TRANSACTED		0x80100016 /* An attempt was made to end a non-existent transaction. */
#define SCARD_E_READER_UNAVAILABLE	0x80100017 /* The specified reader is not currently available for use. */
#define SCARD_E_NO_SERVICE		0x8010001D /* The Smart card resource manager is not running. */
#define SCARD_E_NO_READERS_AVAILABLE    0x8010002E /* Cannot find a smart card reader. */
#define SCARD_W_UNRESPONSIVE_CARD	0x80100066 /* The smart card is not responding to a reset. */
#define SCARD_W_UNPOWERED_CARD		0x80100067 /* Power has been removed from the smart card, so that further communication is not possible. */
#define SCARD_W_RESET_CARD		0x80100068 /* The smart card has been reset, so any shared state information is invalid. */
#define SCARD_W_REMOVED_CARD		0x80100069 /* The smart card has been removed, so further communication is not possible. */
#endif


typedef const BYTE *LPCBYTE;
typedef long SCARDCONTEXT; /* hContext returned by SCardEstablishContext() */
typedef SCARDCONTEXT *PSCARDCONTEXT;
typedef SCARDCONTEXT *LPSCARDCONTEXT;
typedef long SCARDHANDLE; /* hCard returned by SCardConnect() */
typedef SCARDHANDLE *PSCARDHANDLE;
typedef SCARDHANDLE *LPSCARDHANDLE;

typedef struct
{
	const char *szReader;
	void *pvUserData;
	unsigned long dwCurrentState;
	unsigned long dwEventState;
	unsigned long cbAtr;
	unsigned char rgbAtr[MAX_ATR_SIZE];
}
SCARD_READERSTATE, *LPSCARD_READERSTATE;

typedef struct _SCARD_IO_REQUEST
{
	unsigned long dwProtocol;	/* Protocol identifier */
	unsigned long cbPciLength;	/* Protocol Control Inf Length */
}
SCARD_IO_REQUEST, *PSCARD_IO_REQUEST, *LPSCARD_IO_REQUEST;

typedef const SCARD_IO_REQUEST *LPCSCARD_IO_REQUEST;


LONG WINAPI SCardEstablishContext(DWORD dwScope, LPCVOID pvReserved1,
		LPCVOID pvReserved2, LPSCARDCONTEXT phContext);

LONG WINAPI SCardReleaseContext(SCARDCONTEXT hContext);

LONG WINAPI SCardConnect(SCARDCONTEXT hContext, LPCSTR szReader, DWORD dwShareMode,
	DWORD dwPreferredProtocols, LPSCARDHANDLE phCard, LPDWORD pdwActiveProtocol);

LONG WINAPI SCardReconnect(SCARDHANDLE hCard, DWORD dwShareMode, DWORD dwPreferredProtocols,
	DWORD dwInitialization, LPDWORD pdwActiveProtocol);

LONG WINAPI SCardDisconnect(SCARDHANDLE hCard, DWORD dwDisposition);

LONG WINAPI SCardBeginTransaction(SCARDHANDLE hCard);

LONG WINAPI SCardEndTransaction(SCARDHANDLE hCard, DWORD dwDisposition);

LONG WINAPI SCardStatus(SCARDHANDLE hCard, LPSTR mszReaderNames, LPDWORD pcchReaderLen,
	LPDWORD pdwState, LPDWORD pdwProtocol, LPBYTE pbAtr, LPDWORD pcbAtrLen);

LONG WINAPI SCardGetStatusChange(SCARDCONTEXT hContext, DWORD dwTimeout,
	SCARD_READERSTATE *rgReaderStates, DWORD cReaders);

LONG WINAPI SCardCancel(SCARDCONTEXT hContext);

LONG WINAPI SCardControlOLD(SCARDHANDLE hCard, LPCVOID pbSendBuffer, DWORD cbSendLength,
	LPVOID pbRecvBuffer, LPDWORD lpBytesReturned);

LONG WINAPI SCardControl(SCARDHANDLE hCard, DWORD dwControlCode, LPCVOID pbSendBuffer,
	DWORD cbSendLength, LPVOID pbRecvBuffer, DWORD cbRecvLength,
	LPDWORD lpBytesReturned);

LONG WINAPI SCardTransmit(SCARDHANDLE hCard, LPCSCARD_IO_REQUEST pioSendPci,
	LPCBYTE pbSendBuffer, DWORD cbSendLength, LPSCARD_IO_REQUEST pioRecvPci,
	LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength);

LONG WINAPI SCardListReaders(SCARDCONTEXT hContext, LPCSTR mszGroups,
	LPSTR mszReaders, LPDWORD pcchReaders);

LONG WINAPI SCardGetAttrib(SCARDHANDLE hCard, DWORD dwAttrId,\
	LPBYTE pbAttr, LPDWORD pcbAttrLen);

/**************/

#define SCARD_ATTR_MAXINPUT 0x0007A007

extern SCARD_IO_REQUEST g_rgSCardT0Pci, g_rgSCardT1Pci,
       g_rgSCardRawPci;

#define SCARD_PCI_T0  (&g_rgSCardT0Pci)
#define SCARD_PCI_T1  (&g_rgSCardT1Pci)
#define SCARD_PCI_RAW (&g_rgSCardRawPci)

#define MAX_READERNAME 255

#endif
