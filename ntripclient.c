/*
  NTRIP client for POSIX.
  $Id: ntripclient.c,v 1.51 2009/09/11 09:49:19 stoecker Exp $
  Copyright (C) 2003-2008 by Dirk Stöcker <soft@dstoecker.de>

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

#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#ifdef WINDOWSVERSION

#else
  #include <signal.h>
#endif

#include "serial.h"
#include "receive.h"


#ifndef COMPILEDATE
#define COMPILEDATE " built " __DATE__
#endif

/* CVS revision and version */
static char revisionstr[] = "$Revision: 1.51 $";
static char datestr[]     = "$Date: 2009/09/11 09:49:19 $";

/* option parsing */
#ifdef NO_LONG_OPTS
#define LONG_OPT(a)
#else
#define LONG_OPT(a) a
static struct option opts[] = {
{ "bitrate",    no_argument,       0, 'b'},
{ "data",       required_argument, 0, 'd'}, /* compatibility */
{ "mountpoint", required_argument, 0, 'm'},
{ "initudp",    no_argument,       0, 'I'},
{ "udpport",    required_argument, 0, 'P'},
{ "server",     required_argument, 0, 's'},
{ "password",   required_argument, 0, 'p'},
{ "port",       required_argument, 0, 'r'},
{ "proxyport",  required_argument, 0, 'R'},
{ "proxyhost",  required_argument, 0, 'S'},
{ "user",       required_argument, 0, 'u'},
{ "nmea",       required_argument, 0, 'n'},
{ "mode",       required_argument, 0, 'M'},
{ "serdevice",  required_argument, 0, 'D'},
{ "baud",       required_argument, 0, 'B'},
{ "stopbits",   required_argument, 0, 'T'},
{ "protocol",   required_argument, 0, 'C'},
{ "parity",     required_argument, 0, 'Y'},
{ "databits",   required_argument, 0, 'A'},
{ "serlogfile", required_argument, 0, 'l'},
{ "help",       no_argument,       0, 'h'},
{0,0,0,0}};
#endif
#define ARGOPT "-d:m:bhp:r:s:u:n:S:R:M:IP:D:B:T:C:Y:A:l:"

#ifndef WINDOWSVERSION
int sigstop = 0;
#ifdef __GNUC__
static __attribute__ ((noreturn)) void sighandler_alarm(
int sig __attribute__((__unused__)))
#else /* __GNUC__ */
static void sighandler_alarm(int sig)
#endif /* __GNUC__ */
{
  if(!sigstop)
    fprintf(stderr, "ERROR: more than %d seconds no activity\n", ALARMTIME);
  else
    fprintf(stderr, "ERROR: user break\n");
  exit(1);
}

#ifdef __GNUC__
static void sighandler_int(int sig __attribute__((__unused__)))
#else /* __GNUC__ */
static void sighandler_alarm(int sig)
#endif /* __GNUC__ */
{
  sigstop = 1;
  alarm(2);
  receive_stop();
}
#endif /* WINDOWSVERSION */

static const char *encodeurl(const char *req)
{
  char *h = "0123456789abcdef";
  static char buf[128];
  char *urlenc = buf;
  char *bufend = buf + sizeof(buf) - 3;

  while(*req && urlenc < bufend)
  {
    if(isalnum(*req)
    || *req == '-' || *req == '_' || *req == '.')
      *urlenc++ = *req++;
    else
    {
      *urlenc++ = '%';
      *urlenc++ = h[*req >> 4];
      *urlenc++ = h[*req & 0x0f];
      req++;
    }
  }
  *urlenc = 0;
  return buf;
}

static const char *geturl(const char *url, struct Args *args)
{
  static char buf[1000];
  static char *Buffer = buf;
  static char *Bufend = buf+sizeof(buf);
  char *h = "0123456789abcdef";

  if(strncmp("ntrip:", url, 6))
    return "URL must start with 'ntrip:'.";
  url += 6; /* skip ntrip: */

  if(*url != '@' && *url != '/')
  {
    /* scan for mountpoint */
    args->data = Buffer;
    if(*url != '?')
    {
       while(*url && *url != '@' &&  *url != ';' && *url != '/' && Buffer != Bufend)
         *(Buffer++) = *(url++);
    }
    else
    {
       while(*url && *url != '@' &&  *url != '/' && Buffer != Bufend)
       {
          if(isalnum(*url) || *url == '-' || *url == '_' || *url == '.')
            *Buffer++ = *url++;
          else
          {
            *Buffer++ = '%';
            *Buffer++ = h[*url >> 4];
            *Buffer++ = h[*url & 0x0f];
            url++;
          }
       }
    }
    if(Buffer == args->data)
      return "Mountpoint required.";
    else if(Buffer >= Bufend-1)
      return "Parsing buffer too short.";
    *(Buffer++) = 0;
  }

  if(*url == '/') /* username and password */
  {
    ++url;
    args->user = Buffer;
    while(*url && *url != '@' && *url != ';' && *url != ':' && Buffer != Bufend)
      *(Buffer++) = *(url++);
    if(Buffer == args->user)
      return "Username cannot be empty.";
    else if(Buffer >= Bufend-1)
      return "Parsing buffer too short.";
    *(Buffer++) = 0;

    if(*url == ':') ++url;

    args->password = Buffer;
    while(*url && *url != '@' && *url != ';' && Buffer != Bufend)
      *(Buffer++) = *(url++);
    if(Buffer == args->password)
      return "Password cannot be empty.";
    else if(Buffer >= Bufend-1)
      return "Parsing buffer too short.";
    *(Buffer++) = 0;
  }

  if(*url == '@') /* server */
  {
    ++url;
    if(*url != '@' && *url != ':')
    {
      args->server = Buffer;
      while(*url && *url != '@' && *url != ':' && *url != ';' && Buffer != Bufend)
        *(Buffer++) = *(url++);
      if(Buffer == args->server)
        return "Servername cannot be empty.";
      else if(Buffer >= Bufend-1)
        return "Parsing buffer too short.";
      *(Buffer++) = 0;
    }

    if(*url == ':')
    {
      ++url;
      args->port = Buffer;
      while(*url && *url != '@' && *url != ';' && Buffer != Bufend)
        *(Buffer++) = *(url++);
      if(Buffer == args->port)
        return "Port cannot be empty.";
      else if(Buffer >= Bufend-1)
        return "Parsing buffer too short.";
      *(Buffer++) = 0;
    }

    if(*url == '@') /* proxy */
    {
      ++url;
      args->proxyhost = Buffer;
      while(*url && *url != ':' && *url != ';' && Buffer != Bufend)
        *(Buffer++) = *(url++);
      if(Buffer == args->proxyhost)
        return "Proxy servername cannot be empty.";
      else if(Buffer >= Bufend-1)
        return "Parsing buffer too short.";
      *(Buffer++) = 0;

      if(*url == ':')
      {
        ++url;
        args->proxyport = Buffer;
        while(*url && *url != ';' && Buffer != Bufend)
          *(Buffer++) = *(url++);
        if(Buffer == args->proxyport)
          return "Proxy port cannot be empty.";
        else if(Buffer >= Bufend-1)
          return "Parsing buffer too short.";
        *(Buffer++) = 0;
      }
    }
  }
  if(*url == ';') /* NMEA */
  {
    args->nmea = ++url;
    while(*url)
      ++url;
  }

  return *url ? "Garbage at end of server string." : 0;
}

static int getargs(int argc, char **argv, struct Args *args)
{
  int res = 1;
  int getoptr;
  char *a;
  int i = 0, help = 0;

  args->server = "www.euref-ip.net";
  args->port = "2101";
  args->user = "";
  args->password = "";
  args->nmea = 0;
  args->data = 0;
  args->bitrate = 0;
  args->proxyhost = 0;
  args->proxyport = "2101";
  args->mode = AUTO;
  args->initudp = 0;
  args->udpport = 0;
  args->protocol = SPAPROTOCOL_NONE;
  args->parity = SPAPARITY_NONE;
  args->stopbits = SPASTOPBITS_1;
  args->databits = SPADATABITS_8;
  args->baud = SPABAUD_9600;
  args->serdevice = 0;
  args->serlogfile = 0;
  help = 0;

  do
  {
#ifdef NO_LONG_OPTS
    switch((getoptr = getopt(argc, argv, ARGOPT)))
#else
    switch((getoptr = getopt_long(argc, argv, ARGOPT, opts, 0)))
#endif
    {
    case 's': args->server = optarg; break;
    case 'u': args->user = optarg; break;
    case 'p': args->password = optarg; break;
    case 'd': /* legacy option, may get removed in future */
      fprintf(stderr, "Option -d or --data is deprecated. Use -m instead.\n");
      /* fall-through */
    case 'm':
      if(optarg && *optarg == '?')
        args->data = encodeurl(optarg);
      else
        args->data = optarg;
      break;
    case 'B':
      {
        int i = strtol(optarg, 0, 10);

        switch(i)
        {
        case 50: args->baud = SPABAUD_50; break;
        case 110: args->baud = SPABAUD_110; break;
        case 300: args->baud = SPABAUD_300; break;
        case 600: args->baud = SPABAUD_600; break;
        case 1200: args->baud = SPABAUD_1200; break;
        case 2400: args->baud = SPABAUD_2400; break;
        case 4800: args->baud = SPABAUD_4800; break;
        case 9600: args->baud = SPABAUD_9600; break;
        case 19200: args->baud = SPABAUD_19200; break;
        case 38400: args->baud = SPABAUD_38400; break;
        case 57600: args->baud = SPABAUD_57600; break;
        case 115200: args->baud = SPABAUD_115200; break;
        default:
          fprintf(stderr, "Baudrate '%s' unknown\n", optarg);
          res = 0;
          break;
        }
      }
      break;
    case 'T':
      if(!strcmp(optarg, "1")) args->stopbits = SPASTOPBITS_1;
      else if(!strcmp(optarg, "2")) args->stopbits = SPASTOPBITS_2;
      else
      {
        fprintf(stderr, "Stopbits '%s' unknown\n", optarg);
        res = 0;
      }
      break;
    case 'A':
      if(!strcmp(optarg, "5")) args->databits = SPADATABITS_5;
      else if(!strcmp(optarg, "6")) args->databits = SPADATABITS_6;
      else if(!strcmp(optarg, "7")) args->databits = SPADATABITS_7;
      else if(!strcmp(optarg, "8")) args->databits = SPADATABITS_8;
      else
      {
        fprintf(stderr, "Databits '%s' unknown\n", optarg);
        res = 0;
      }
      break;
    case 'C':
      {
        int i = 0;
        args->protocol = SerialGetProtocol(optarg, &i);
        if(!i)
        {
          fprintf(stderr, "Protocol '%s' unknown\n", optarg);
          res = 0;
        }
      }
      break;
    case 'Y':
      {
        int i = 0;
        args->parity = SerialGetParity(optarg, &i);
        if(!i)
        {
          fprintf(stderr, "Parity '%s' unknown\n", optarg);
          res = 0;
        }
      }
      break;
    case 'D': args->serdevice = optarg; break;
    case 'l': args->serlogfile = optarg; break;
    case 'I': args->initudp = 1; break;
    case 'P': args->udpport = strtol(optarg, 0, 10); break;
    case 'n': args->nmea = optarg; break;
    case 'b': args->bitrate = 1; break;
    case 'h': help=1; break;
    case 'r': args->port = optarg; break;
    case 'S': args->proxyhost = optarg; break;
    case 'R': args->proxyport = optarg; break;
    case 'M':
      args->mode = 0;
      if (!strcmp(optarg,"n") || !strcmp(optarg,"ntrip1"))
        args->mode = NTRIP1;
      else if(!strcmp(optarg,"h") || !strcmp(optarg,"http"))
        args->mode = HTTP;
      else if(!strcmp(optarg,"r") || !strcmp(optarg,"rtsp"))
        args->mode = RTSP;
      else if(!strcmp(optarg,"u") || !strcmp(optarg,"udp"))
        args->mode = UDP;
      else if(!strcmp(optarg,"a") || !strcmp(optarg,"auto"))
        args->mode = AUTO;
      else args->mode = atoi(optarg);
      if((args->mode == 0) || (args->mode >= END))
      {
        fprintf(stderr, "Mode %s unknown\n", optarg);
        res = 0;
      }
      break;
    case 1:
      {
        const char *err;
        if((err = geturl(optarg, args)))
        {
          fprintf(stderr, "%s\n\n", err);
          res = 0;
        }
      }
      break;
    case -1: break;
    }
  } while(getoptr != -1 && res);

  for(a = revisionstr+11; *a && *a != ' '; ++a)
    revisionstr[i++] = *a;
  revisionstr[i] = 0;
  datestr[0] = datestr[7];
  datestr[1] = datestr[8];
  datestr[2] = datestr[9];
  datestr[3] = datestr[10];
  datestr[5] = datestr[12];
  datestr[6] = datestr[13];
  datestr[8] = datestr[15];
  datestr[9] = datestr[16];
  datestr[4] = datestr[7] = '-';
  datestr[10] = 0;

  if(!res || help)
  {
    fprintf(stderr, "Version %s (%s) GPL" COMPILEDATE "\nUsage:\n%s -s server -u user ...\n"
    " -m " LONG_OPT("--mountpoint ") "the requested data set or sourcetable filtering criteria\n"
    " -s " LONG_OPT("--server     ") "the server name or address\n"
    " -p " LONG_OPT("--password   ") "the login password\n"
    " -r " LONG_OPT("--port       ") "the server port number (default 2101)\n"
    " -u " LONG_OPT("--user       ") "the user name\n"
    " -M " LONG_OPT("--mode       ") "mode for data request\n"
    "     Valid modes are:\n"
    "     1, h, http     NTRIP Version 2.0 Caster in TCP/IP mode\n"
    "     2, r, rtsp     NTRIP Version 2.0 Caster in RTSP/RTP mode\n"
    "     3, n, ntrip1   NTRIP Version 1.0 Caster\n"
    "     4, a, auto     automatic detection (default)\n"
    "     5, u, udp      NTRIP Version 2.0 Caster in UDP mode\n"
    "or using an URL:\n%s ntrip:mountpoint[/user[:password]][@[server][:port][@proxyhost[:proxyport]]][;nmea]\n"
    "\nExpert options:\n"
    " -n " LONG_OPT("--nmea       ") "NMEA string for sending to server\n"
    " -b " LONG_OPT("--bitrate    ") "output bitrate\n"
    " -I " LONG_OPT("--initudp    ") "send initial UDP packet for firewall handling\n"
    " -P " LONG_OPT("--udpport    ") "set the local UDP port\n"
    " -S " LONG_OPT("--proxyhost  ") "proxy name or address\n"
    " -R " LONG_OPT("--proxyport  ") "proxy port, optional (default 2101)\n"
    "\nSerial input/output:\n"
    " -D " LONG_OPT("--serdevice  ") "serial device for output\n"
    " -B " LONG_OPT("--baud       ") "baudrate for serial device\n"
    " -T " LONG_OPT("--stopbits   ") "stopbits for serial device\n"
    " -C " LONG_OPT("--protocol   ") "protocol for serial device\n"
    " -Y " LONG_OPT("--parity     ") "parity for serial device\n"
    " -A " LONG_OPT("--databits   ") "databits for serial device\n"
    " -l " LONG_OPT("--serlogfile ") "logfile for serial data\n"
    , revisionstr, datestr, argv[0], argv[0]);
    exit(1);
  }
  return res;
}

int main(int argc, char **argv)
{
  struct Args args;

  setbuf(stdout, 0);
  setbuf(stdin, 0);
  setbuf(stderr, 0);
#ifndef WINDOWSVERSION
  signal(SIGALRM,sighandler_alarm);
  signal(SIGINT,sighandler_int);
  alarm(ALARMTIME);
#else
  WSADATA wsaData;
  if(WSAStartup(MAKEWORD(1,1),&wsaData))
  {
    fprintf(stderr, "Could not init network access.\n");
    return 20;
  }
#endif

  if(getargs(argc, argv, &args))
  {
    args.revisionstr = revisionstr;
    return receive_run(&args);
  }

  return 0;
}
