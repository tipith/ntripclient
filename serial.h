#ifndef _SERIAL_H_
#define _SERIAL_H_

#ifndef WINDOWSVERSION
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#define SERIALDEFAULTDEVICE "/dev/ttyS0"
enum SerialBaud {
  SPABAUD_50 = B50, SPABAUD_110 = B110, SPABAUD_300 = B300, SPABAUD_600 = B600,
  SPABAUD_1200 = B1200, SPABAUD_2400 = B2400, SPABAUD_4800 = B4800,
  SPABAUD_9600 = B9600, SPABAUD_19200 = B19200,
  SPABAUD_38400 = B38400, SPABAUD_57600 = B57600, SPABAUD_115200 = B115200 };
enum SerialDatabits {
  SPADATABITS_5 = CS5, SPADATABITS_6 = CS6, SPADATABITS_7 = CS7, SPADATABITS_8 = CS8 };
enum SerialStopbits {
  SPASTOPBITS_1 = 0, SPASTOPBITS_2 = CSTOPB };
enum SerialParity {
  SPAPARITY_NONE = 0, SPAPARITY_ODD = PARODD | PARENB, SPAPARITY_EVEN = PARENB };
enum SerialProtocol {
  SPAPROTOCOL_NONE = 0, SPAPROTOCOL_RTS_CTS = 9999,
  SPAPROTOCOL_XON_XOFF = IXOFF | IXON };

struct serial
{
  struct termios Termios;
  int            Stream;
};
#else /* WINDOWSVERSION */
#include <windows.h>
#define SERIALDEFAULTDEVICE "COM1"
enum SerialBaud {
  SPABAUD_50 = 50, SPABAUD_110 = 110, SPABAUD_300 = 300, SPABAUD_600 = 600,
  SPABAUD_1200 = 1200, SPABAUD_2400 = 2400, SPABAUD_4800 = 4800,
  SPABAUD_9600 = 9600, SPABAUD_19200 = 19200,
  SPABAUD_38400 = 38400, SPABAUD_57600 = 57600, SPABAUD_115200 = 115200 };
enum SerialDatabits {
  SPADATABITS_5 = 5, SPADATABITS_6 = 6, SPADATABITS_7 = 7, SPADATABITS_8 = 8 };
enum SerialStopbits {
  SPASTOPBITS_1 = 1, SPASTOPBITS_2 = 2 };
enum SerialParity {
  SPAPARITY_NONE = 'N', SPAPARITY_ODD = 'O', SPAPARITY_EVEN = 'E'};
enum SerialProtocol {
  SPAPROTOCOL_NONE, SPAPROTOCOL_RTS_CTS, SPAPROTOCOL_XON_XOFF};

struct serial
{
  DCB    Termios;
  HANDLE Stream;
};
#if !defined(__GNUC__)
int strncasecmp(const char *a, const char *b, int len)
{
  while(len-- && *a && tolower(*a) == tolower(*b))
  {
    ++a; ++b;
  }
  return len ? (tolower(*a) - tolower(*b)) : 0;
}
#endif /* !__GNUC__ */

#endif /* WINDOWSVERSION */

const char * SerialInit(
    struct serial *sn, 
    const char *Device,
    enum SerialBaud Baud, 
    enum SerialStopbits StopBits,
    enum SerialProtocol Protocol, 
    enum SerialParity Parity,
    enum SerialDatabits DataBits, 
    int dowrite);
int SerialRead(struct serial *sn, char *buffer, size_t size);
int SerialWrite(struct serial *sn, const char *buffer, size_t size);
void SerialFree(struct serial *sn);

enum SerialProtocol SerialGetProtocol(const char *buf, int *ressize);
enum SerialParity SerialGetParity(const char *buf, int *ressize);

#endif /* _SERIAL_H_ */