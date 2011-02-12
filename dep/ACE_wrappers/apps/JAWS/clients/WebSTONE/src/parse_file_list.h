/* $Id: parse_file_list.h 80826 2008-03-04 14:51:23Z wotte $ */
/**************************************************************************
 *                                                                        *
 *               Copyright (C) 1995 Silicon Graphics, Inc.                *
 *                                                                        *
 *  These coded instructions, statements, and computer programs  where    *
 *  deveolped by SGI for public use.  If anychanges are made to this code *
 *  please try to get the changes back to the author.  Feel free to make  *
 *  modfications and changes to the code and release it.                  *
 *                                                                        *
 **************************************************************************/
#ifndef __PARSE_FILE_LIST_H__
#define __PARSE_FILE_LIST_H__

extern int count_file_list(const char *);

extern void parse_file_list (char *, page_list_t *,
                                 long int *, long int *);

#endif /* !__PARSE_FILE_LIST_H__ */
