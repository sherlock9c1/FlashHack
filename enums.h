#ifndef ENUMS_H
#define ENUMS_H

enum _program_region_state {
  REGION_ERASING,
  REGION_ERASED,
  REGION_PROGRAMMING,
  REGION_WRITTEN,
  REGION_READ,
  REGION_ERROR,
  REGION_UNKNOWN,
  REGION_EXEC
};

enum _loglevel {
  LOGLEVEL_ERROR = 0x00,
  LOGLEVEL_STATUS = 0x01,
  LOGLEVEL_DETAIL = 0x03,
  LOGLEVEL_DEBUG = 0x03,
  LOGLEVEL_COMM = 0x04
};

enum _comm_state {
  COMM_STATE_CLOSED,
  COMM_STATE_OPEN
};

enum _reply_errortype {
  ERRORTYPE_UNKNOWN,  // any other error
  ERRORTYPE_SIZE,     // packet size out of range
  ERRORTYPE_COMM,     // includes timeout or serial error
  ERRORTYPE_SYSTEM    // mostly programming errors
};

#endif // ENUMS_H
