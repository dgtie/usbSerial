#include <xc.h>
#include "uart.h"

void out_handler_done(void);
char *get_in_buffer(int &length);
void return_in_buffer(int length);

namespace {
    
    char *buffer;
    int idx, length;
    
}

bool stop_selection(int i) {
    if (i == NUM_STOP_BITS_1) { U1MODEbits.STSEL = 0; return true; }
    if (i == NUM_STOP_BITS_2) { U1MODEbits.STSEL = 1; return true; }
    return false;
}

bool parity_data_selection(int p, int d) {
    if ((p == PARITY_NONE) && (d == 9)) { U1MODEbits.PDSEL = 3; return true; }
    if (d == 8) {
        if (p == PARITY_NONE) { U1MODEbits.PDSEL = 0; return true; }
        if (p == PARITY_EVEN) { U1MODEbits.PDSEL = 1; return true; }
        if (p == PARITY_ODD) { U1MODEbits.PDSEL = 2; return true; }
    }
    return false;
}

bool baud_selection(unsigned b) {
    unsigned baud = (2500000 + (b>>1))/b - 1;
    if (baud && (baud < 65536)) return U1BRG = baud;
    return false;
}

extern "C"
void __attribute__((interrupt(ipl1soft), vector(_UART1_VECTOR), nomips16)) u1ISR(void) {
    if (IFS1bits.U1RXIF) {
        int length;
        char *buffer = get_in_buffer(length);
        char c = U1RXREG;
        if (buffer) {
            buffer[0] = c;
            return_in_buffer(1);
        }
        IFS1bits.U1RXIF = 0;
    }
    if (IEC1bits.U1TXIE) if (IFS1bits.U1TXIF) {
        if (idx == length) {
            IEC1bits.U1TXIE = 0;
            out_handler_done();
        } else U1TXREG = buffer[idx++];
        IFS1bits.U1TXIF = 0;
    }
}

void out_buffer_handler(char *buf, int len) {
    buffer = buf; length = len; idx = 0;
    if (len) IEC1bits.U1TXIE = 1;
    else out_handler_done();
}
