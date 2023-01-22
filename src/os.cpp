#include <xc.h>

void poll(unsigned);

/*** DEVCFG0 ***/
//#pragma config DEBUG =      OFF
//chipkit compiler pic32-g++ has a bug here: DEBUG=OFF is not OFF
//Anyway, default is DEBUG OFF
#pragma config JTAGEN =     OFF
#pragma config ICESEL = ICS_PGx1   // ICE/ICD Comm Channel Select
#pragma config PWP =        OFF
#pragma config BWP =        OFF
#pragma config CP =         OFF

/*** DEVCFG1 ***/
#pragma config FNOSC =      PRIPLL
#pragma config FPBDIV =     DIV_1
#pragma config FSOSCEN =    OFF
#pragma config IESO =       ON
#pragma config POSCMOD =    XT
#pragma config OSCIOFNC =   OFF
#pragma config FCKSM =      CSDCMD
#pragma config WDTPS =      PS1048576
#pragma config FWDTEN =     OFF
#pragma config WINDIS =     OFF
#pragma config FWDTWINSZ =  WINSZ_25

/*** DEVCFG2 ***/
#pragma config FPLLIDIV =   DIV_2    // 8 MHz -> 4 MHz
#pragma config FPLLMUL =    MUL_20   // 80 MHz
#pragma config FPLLODIV =   DIV_2    // 40MHz System Clock
#pragma config UPLLEN =     ON
#pragma config UPLLIDIV =   DIV_2

/*** DEVCFG3 ***/
#pragma config FVBUSONIO =  ON
#pragma config USERID =     0xFFFF
// another chipkit compiler bug here in USERID
#pragma config PMDL1WAY =   OFF
#pragma config IOL1WAY =    OFF
#pragma config FUSBIDIO =   OFF

#define MS 40000

void init(void) {
  __builtin_disable_interrupts();
  _CP0_SET_COMPARE(MS);
  _CP0_SET_COMPARE(MS);
  SYSKEY = 0;                       // ensure OSCCON is locked
  SYSKEY = 0xAA996655;              // unlock sequence
  SYSKEY = 0x556699AA;
  CFGCONbits.IOLOCK = 0;            // allow write
  U1RXR = 0b0100;                   // PB2    (uart1)
  RPB3R = 0b0001;                   // U1TX
//  U2RXR = 0b0010;                   // PB1    (uart2 for debug)
//  RPB0R = 0b0010;                   // U2TX
  CFGCONbits.IOLOCK = 1;            // forbidden write
  SYSKEY = 0;                       // relock
  ANSELBCLR = 0xf;
  TRISBCLR = 0x200;                 // RB9
  U1BRG = 21;               // UART1 - baud rate = 115200
//  U2STAbits.UTXEN = 1;
  U1STASET = 0x1400;                // UART1 - URXEN & UTXEN
  IPC0bits.CTIP = 1;
  IEC0bits.CTIE = 1;
  IPC8bits.U1IP = 1; IPC8bits.U1IS = 1;
  IEC1bits.U1RXIE = 1;
  U1MODEbits.ON = 1;
//  U2MODEbits.ON = 1;
  INTCONSET = _INTCON_MVEC_MASK;
  __builtin_enable_interrupts();
}

namespace {
    
volatile unsigned tick;

void loop(unsigned t) {
  static unsigned tick;
  if (tick == t) return;
  poll(tick = t);
}
    
}//anonymous

bool wait(unsigned i) {
    unsigned u = tick;
    for (loop(u); i--; u = tick) while (u == tick) loop(u);
    return true;
}

extern "C"
void __attribute__((interrupt(ipl1soft), vector(_CORE_TIMER_VECTOR), nomips16)) rtcISR(void) {
  _CP0_SET_COMPARE(_CP0_GET_COMPARE() + MS);
  tick++;
  IFS0bits.CTIF = 0;
}

void toggle(void) { LATBINV = 0x200; }

