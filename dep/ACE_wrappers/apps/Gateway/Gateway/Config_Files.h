/* -*- C++ -*- */
// $Id: Config_Files.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    gateway
//
// = FILENAME
//    Config_Files.h
//
// = AUTHOR
//    Doug Schmidt
//
// ============================================================================

#ifndef _CONFIG_FILES
#define _CONFIG_FILES

#include "File_Parser.h"
#include "Event.h"

// Forward declaration.
class Event_Channel;

class Connection_Config_Info
  // = TITLE
  //     Stores connection configuration information.
{
public:
  ACE_INT32 connection_id_;
  // Connection id for this Connection_Handler.

  char host_[BUFSIZ];
  // Host to connect with.

  u_short remote_port_;
  // Port to connect with.

  char connection_role_;
  // 'S' (supplier) or 'C' (consumer).

  ACE_INT32 max_retry_timeout_;
  // Maximum amount of time to wait for reconnecting.

  u_short local_port_;
  // Our local port number.

  ACE_INT32 priority_;
  // Priority by which different Consumers and Suppliers should be
  // serviced.

  Event_Channel *event_channel_;
  // We just need a place to store this until we can pass it along
  // when creating a Connection_Handler.
};

class Connection_Config_File_Parser : public File_Parser<Connection_Config_Info>
  // = TITLE
  //     Parser for the Connection_Handler Connection file.
{
public:
  virtual FPRT::Return_Type read_entry (Connection_Config_Info &entry,
                                        int &line_number);
  // Read in a <Connection_Config_Info> entry.

};

class Consumer_Config_Info
  // = TITLE
  //     Stores the information in a Consumer Map entry.
{
public:
  ACE_INT32 connection_id_;
  // Connection id.

  ACE_INT32 type_;
  // Message type.

  ACE_INT32 consumers_[MAX_CONSUMERS];
  // Connection ids for consumers that will be routed information
  // containing this <connection_id_>

  ACE_INT32 total_consumers_;
  // Total number of these consumers.
};

class Consumer_Config_File_Parser : public File_Parser<Consumer_Config_Info>
  // = TITLE
  //     Parser for the Consumer Map file.
{
public:
  virtual FPRT::Return_Type read_entry (Consumer_Config_Info &entry,
                                        int &line_number);
  // Read in a <Consumer_Config_Info> entry.
};

#endif /* _CONFIG_FILES */
