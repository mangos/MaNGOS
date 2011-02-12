/* $Id: timefunc.h 80826 2008-03-04 14:51:23Z wotte $ */
/**************************************************************************
 *               Copyright (C) 1995 Silicon Graphics, Inc.                *
 *                                                                        *
 *  These coded instructions, statements, and computer programs were      *
 *  developed by SGI for public use.  If any changes are made to this code*
 *  please try to get the changes back to the author.  Feel free to make  *
 *  modifications and changes to the code and release it.                 *
 *                                                                        *
 **************************************************************************/

#ifndef __TIMEFUNC_H__
#define __TIMEFUNC_H__

extern double   timevaldouble(struct timeval *);
extern void     doubletimeval(const double, struct timeval *);

extern void     addtime(struct timeval *, struct timeval *);
extern void     compdifftime(struct timeval *, struct timeval *, struct timeval *);
extern void     mintime(struct timeval *, struct timeval *);
extern void     maxtime(struct timeval *, struct timeval *);
extern void     avgtime(struct timeval *, int, struct timeval *);
extern void     variancetime(struct timeval *, double, int, struct timeval *);
extern void     stddevtime(struct timeval *, double, int, struct timeval *);

extern void     sqtime(struct timeval *, struct timeval *);

extern double   thruputpersec(const double, struct timeval *);

#endif /* !__TIMEFUNC_H__ */
