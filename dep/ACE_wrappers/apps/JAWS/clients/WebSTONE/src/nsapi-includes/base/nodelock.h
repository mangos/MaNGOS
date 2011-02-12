/*
 * $Id: nodelock.h 80826 2008-03-04 14:51:23Z wotte $
 *
 * Copyright (c) 1994, 1995.  Netscape Communications Corporation.  All
 * rights reserved.
 *
 * Use of this software is governed by the terms of the license agreement for
 * the Netscape Communications or Netscape Comemrce Server between the
 * parties.
 */


/* ------------------------------------------------------------------------ */


/*
 * nodelock.h:  licensing stuff
 */

#ifndef _NODELOCK_H
#define _NODELOCK_H

/*
 * Do the initial IP address check and expiration date check.  Reads a file
 * from admin/config, as #define'd.
 *
 * Returns 1 on error, 0 on AOK.
 */

int node_init(void);

/*
 * Check the expiration date against The Now.
 *
 * Returns 1 on error, 0 on AOK.
 */

int node_check(void);

/*
 * So how we doin, license
 *
 * Returns 1 on error, 0 on AOK
 */
int node_status(void);

#endif
