/* $Id: debug.h 80826 2008-03-04 14:51:23Z wotte $ */
/**************************************************************************
 *                                                                        *
 *               Copyright (C) 1995 Silicon Graphics, Inc.                *
 *                                                                        *
 *  These coded instructions, statements, and computer programs were      *
 *  developed by SGI for public use.  If any changes are made to this code*
 *  please try to get the changes back to the author.  Feel free to make  *
 *  modifications and changes to the code and release it.                 *
 *                                                                        *
 **************************************************************************/
#ifndef __DEBUG_H__

#define D_PRINTF  debug && fprintf(debugfile,
#define D_FLUSH   );fflush(debugfile)

#define __DEBUG_H__
#endif
