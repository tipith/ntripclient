/*
  Serial port access for NTRIP client for POSIX.
  $Id$
  Copyright (C) 2008 by Dirk Stöcker <soft@dstoecker.de>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  or read http://www.gnu.org/licenses/gpl.txt
*/

/* system includes */
#include <stdio.h>
#include <string.h>

#include "serial.h"


enum SerialParity SerialGetParity(const char *buf, int *ressize)
{
  int r = 0;
  enum SerialParity p = SPAPARITY_NONE;
  if(!strncasecmp(buf, "none", 4))
  { r = 4; p = SPAPARITY_NONE; }
  else if(!strncasecmp(buf, "no", 2))
  { r = 2; p = SPAPARITY_NONE; }
  else if(!strncasecmp(buf, "odd", 3))
  { r = 3; p = SPAPARITY_ODD; }
  else if(!strncasecmp(buf, "even", 4))
  { r = 4; p = SPAPARITY_EVEN; }
  else if(*buf == 'N' || *buf == 'n')
  { r = 1; p = SPAPARITY_NONE; }
  else if(*buf == 'O' || *buf == 'o')
  { r = 1; p = SPAPARITY_ODD; }
  else if(*buf == 'E' || *buf == 'e')
  { r = 1; p = SPAPARITY_EVEN; }
  if(ressize) *ressize = r;
  return p;
}

enum SerialProtocol SerialGetProtocol(const char *buf, int *ressize)
{
  int r = 0;
  enum SerialProtocol Protocol = SPAPROTOCOL_NONE;
  /* try some possible forms for input, be as gentle as possible */
  if(!strncasecmp("xonxoff",buf,7)){r = 7; Protocol=SPAPROTOCOL_XON_XOFF;}
  else if(!strncasecmp("xon_xoff",buf,8)){r = 8; Protocol=SPAPROTOCOL_XON_XOFF;}
  else if(!strncasecmp("xon-xoff",buf,8)){r = 8; Protocol=SPAPROTOCOL_XON_XOFF;}
  else if(!strncasecmp("xon xoff",buf,8)){r = 8; Protocol=SPAPROTOCOL_XON_XOFF;}
  else if(!strncasecmp("xoff",buf,4)){r = 4; Protocol=SPAPROTOCOL_XON_XOFF;}
  else if(!strncasecmp("xon",buf,3)){r = 3; Protocol=SPAPROTOCOL_XON_XOFF;}
  else if(*buf == 'x' || *buf == 'X'){r = 1; Protocol=SPAPROTOCOL_XON_XOFF;}
  else if(!strncasecmp("rtscts",buf,6)){r = 6; Protocol=SPAPROTOCOL_RTS_CTS;}
  else if(!strncasecmp("rts_cts",buf,7)){r = 7; Protocol=SPAPROTOCOL_RTS_CTS;}
  else if(!strncasecmp("rts-cts",buf,7)){r = 7; Protocol=SPAPROTOCOL_RTS_CTS;}
  else if(!strncasecmp("rts cts",buf,7)){r = 7; Protocol=SPAPROTOCOL_RTS_CTS;}
  else if(!strncasecmp("rts",buf,3)){r = 3; Protocol=SPAPROTOCOL_RTS_CTS;}
  else if(!strncasecmp("cts",buf,3)){r = 3; Protocol=SPAPROTOCOL_RTS_CTS;}
  else if(*buf == 'r' || *buf == 'R' || *buf == 'c' || *buf == 'C')
  {r = 1; Protocol=SPAPROTOCOL_RTS_CTS;}
  else if(!strncasecmp("none",buf,4)){r = 4; Protocol=SPAPROTOCOL_NONE;}
  else if(!strncasecmp("no",buf,2)){r = 2; Protocol=SPAPROTOCOL_NONE;}
  else if(*buf == 'n' || *buf == 'N'){r = 1; Protocol=SPAPROTOCOL_NONE;}
  if(ressize) *ressize = r;
  return Protocol;
}

enum SerialBaud SerialBaudrate(int baud)
{
  enum SerialBaud Baud = SPAPROTOCOL_NONE;

  switch (baud)
  {
    case 50: return SPABAUD_50;
    case 110: return SPABAUD_110;
    case 300: return SPABAUD_300;
    case 600: return SPABAUD_600;
    case 1200: return SPABAUD_1200;
    case 2400: return SPABAUD_2400;
    case 4800: return SPABAUD_4800;
    case 9600: return SPABAUD_9600;
    case 19200: return SPABAUD_19200;
    case 38400: return SPABAUD_38400;
    case 57600: return SPABAUD_57600;
    case 115200: return SPABAUD_115200;
    case 230400: return SPABAUD_230400;
    case 460800: return SPABAUD_460800;
    case 500000: return SPABAUD_500000;
    case 576000: return SPABAUD_576000;
    case 921600: return SPABAUD_921600;
    default: return SPABAUD_ERROR;
  }
}

#ifndef WINDOWSVERSION
void SerialFree(struct serial *sn)
{
  if(sn->Stream)
  {
    /* reset old settings */
    tcsetattr(sn->Stream, TCSANOW, &sn->Termios);
    close(sn->Stream);
    sn->Stream = 0;
  }
}

const char * SerialInit(struct serial *sn,
const char *Device, enum SerialBaud Baud, enum SerialStopbits StopBits,
enum SerialProtocol Protocol, enum SerialParity Parity,
enum SerialDatabits DataBits, int dowrite
#ifdef __GNUC__
__attribute__((__unused__))
#endif /* __GNUC__ */
)
{
  struct termios newtermios;

  if((sn->Stream = open(Device, O_RDWR | O_NOCTTY | O_NONBLOCK)) <= 0)
    return "could not open serial port";
  tcgetattr(sn->Stream, &sn->Termios);

  memset(&newtermios, 0, sizeof(struct termios));
  newtermios.c_cflag = Baud | StopBits | Parity | DataBits
  | CLOCAL | CREAD;
  if(Protocol == SPAPROTOCOL_RTS_CTS)
    newtermios.c_cflag |= CRTSCTS;
  else
    newtermios.c_cflag |= Protocol;
  newtermios.c_cc[VMIN] = 1;
  tcflush(sn->Stream, TCIOFLUSH);
  tcsetattr(sn->Stream, TCSANOW, &newtermios);
  tcflush(sn->Stream, TCIOFLUSH);
  fcntl(sn->Stream, F_SETFL, O_NONBLOCK);
  return 0;
}

int SerialRead(struct serial *sn, char *buffer, size_t size)
{
  int j = read(sn->Stream, buffer, size);
  if(j < 0)
  {
    if(errno == EAGAIN)
      return 0;
    else
      return j;
  }
  return j;
}

int SerialWrite(struct serial *sn, const char *buffer, size_t size)
{
  int j = write(sn->Stream, buffer, size);
  if(j < 0)
  {
    if(errno == EAGAIN)
      return 0;
    else
      return j;
  }
  return j;
}

#else /* WINDOWSVERSION */
void SerialFree(struct serial *sn)
{
  if(sn->Stream)
  {
    SetCommState(sn->Stream, &sn->Termios);
    /* reset old settings */
    CloseHandle(sn->Stream);
    sn->Stream = 0;
  }
}

const char * SerialInit(struct serial *sn, const char *Device,
enum SerialBaud Baud, enum SerialStopbits StopBits,
enum SerialProtocol Protocol, enum SerialParity Parity,
enum SerialDatabits DataBits, int dowrite)
{
  char mydevice[50];
  if(Device[0] != '\\')
    snprintf(mydevice, sizeof(mydevice), "\\\\.\\%s", Device);
  else
    mydevice[0] = 0;

  if((sn->Stream = CreateFile(mydevice[0] ? mydevice : Device,
  dowrite ? GENERIC_WRITE|GENERIC_READ : GENERIC_READ, 0, 0, OPEN_EXISTING,
  0, 0)) == INVALID_HANDLE_VALUE)
    return "could not create file";

  memset(&sn->Termios, 0, sizeof(sn->Termios));
  GetCommState(sn->Stream, &sn->Termios);

  DCB dcb;
  memset(&dcb, 0, sizeof(dcb));
  char str[100];
  snprintf(str,sizeof(str),
  "baud=%d parity=%c data=%d stop=%d xon=%s octs=%s rts=%s",
  Baud, Parity, DataBits, StopBits,
  Protocol == SPAPROTOCOL_XON_XOFF ? "on" : "off",
  Protocol == SPAPROTOCOL_RTS_CTS ? "on" : "off",
  Protocol == SPAPROTOCOL_RTS_CTS ? "on" : "off");
#ifdef DEBUG
  fprintf(stderr, "COM Settings: %s\n", str);
#endif

  COMMTIMEOUTS ct = {MAXDWORD, 0, 0, 0, 0};

  if(!BuildCommDCB(str, &dcb))
  {
    CloseHandle(sn->Stream);
    return "creating device parameters failed";
  }
  else if(!SetCommState(sn->Stream, &dcb))
  {
    CloseHandle(sn->Stream);
    return "setting device parameters failed";
  }
  else if(!SetCommTimeouts(sn->Stream, &ct))
  {
    CloseHandle(sn->Stream);
    return "setting timeouts failed";
  }

  return 0;
}

int SerialRead(struct serial *sn, char *buffer, size_t size)
{
  DWORD j = 0;
  if(!ReadFile(sn->Stream, buffer, size, &j, 0))
    return -1;
  return j;
}

int SerialWrite(struct serial *sn, const char *buffer, size_t size)
{
  DWORD j = 0;
  if(!WriteFile(sn->Stream, buffer, size, &j, 0))
    return -1;
  return j;
}
#endif /* WINDOWSVERSION */
