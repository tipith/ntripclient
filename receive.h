#ifndef _RECEIVE_RUN_H_
#define _RECEIVE_RUN_H_

#ifdef __cplusplus  
extern "C" { 
#endif 

#include "serial.h"

#define ALARMTIME   (30)

enum MODE { HTTP = 1, RTSP = 2, NTRIP1 = 3, AUTO = 4, UDP = 5, END };

struct Args
{
  const char *server;
  const char *port;
  const char *user;
  const char *proxyhost;
  const char *proxyport;
  const char *password;
  const char *nmea;
  const char *data;
  int         bitrate;
  enum MODE   mode;
  int         udpport;
  int         initudp;
  enum SerialBaud baud;
  enum SerialDatabits databits;
  enum SerialStopbits stopbits;
  enum SerialParity parity;
  enum SerialProtocol protocol;
  const char *serdevice;
  const char *serlogfile;
  const char *revisionstr;
};

int receive_run(const struct Args* args);
void receive_stop(void);

#ifdef __cplusplus 
} 
#endif 

#endif /* _RECEIVE_RUN_H_ */
