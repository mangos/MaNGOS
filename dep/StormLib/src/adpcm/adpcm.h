/*****************************************************************************/
/* adpcm.h                                Copyright (c) Ladislav Zezula 2003 */
/*---------------------------------------------------------------------------*/
/* Header file for adpcm decompress functions                                */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* 31.03.03  1.00  Lad  The first version of adpcm.h                         */
/*****************************************************************************/

#ifndef __ADPCM_H__
#define __ADPCM_H__

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// Functions

#include "../StormPort.h"

int  CompressADPCM  (unsigned char * pbOutBuffer, int dwOutLength, short * pwInBuffer, int dwInLength, int nCmpType, int nChannels);
int  DecompressADPCM(unsigned char * pbOutBuffer, int dwOutLength, unsigned char * pbInBuffer, int dwInLength, int nChannels);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif // __ADPCM_H__
