// $Id: http.h 80826 2008-03-04 14:51:23Z wotte $

#include "global.h"

#define CONTENT_ENCODING_HEADER "Content-encoding: "
#define CONTENT_TYPE_HEADER "Content-type: "
#define INCOMING_FILE_NAME "/tmp/sumedh.web.inc"
#define TEMPORARY_FILE_NAME "/tmp/sumedh.web.tmp"

#define ENCODING_TAB "./encoding.tab"
#define CONTENT_TAB  "./content.tab"

int demime(void);
int decode(char *encoding);
int view(char *content);

