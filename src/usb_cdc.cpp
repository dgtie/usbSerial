#include <xc.h>
#include "usb.h"
#include "uart.h"
#include "usb_config.h"

#define SERIAL_STATE 0x20

void bd_fill(int index, char *buf, int size, int stat);

bool stop_selection(int);
bool parity_data_selection(int p, int d);
bool baud_selection(unsigned);
void CDCTx(char *buffer, int length);
void CDCRx(void);

typedef struct
{
    unsigned   dwDTERate;
    char    bCharFormat;
    char    bParityType;
    char    bDataBits;
} LINE_CODING;

namespace {
    
    struct {
        unsigned char bmRequestType;
        unsigned char bNotification;
        unsigned short wValue;
        unsigned short wIndex;
        unsigned short wLength;
        struct {
            unsigned DCD:1;
            unsigned DSR:1;
            unsigned break_in:1;
            unsigned RI:1;
            unsigned frame_err:1;
            unsigned parity_err:1;
            unsigned overrun_err:1;
            unsigned reserved:1;
            unsigned char nothing;
        };
    } cdc_notice_packet = { 0xa1, SERIAL_STATE, 0, 0, 2, 0 };
    
    LINE_CODING line_coding = { 115200, NUM_STOP_BITS_1, PARITY_NONE, 8 };
    char temp[8];
    char in_buffer[2][USB_EP2_BUFF_SIZE], *rx_buffer;
    char out_buffer[2][USB_EP2_BUFF_SIZE];
    int rx_index;
    
}

char *get_CDCRx_buffer(int idx) { return in_buffer[idx]; }

void CDCRx(int idx, int length) {
    bd_fill(idx | 10, in_buffer[idx], length, idx ? 0xc0 : 0x80);
}

void CDCTx(int idx) {
    bd_fill(idx | 8, out_buffer[idx], USB_EP2_BUFF_SIZE, idx ? 0xc0 : 0x80);
}

void CDCInitEndpoint(int config) {
    if (config) {
        U1EP1 = 5;      // EPTXEN, EPHSHK
        U1EP2 = 0x1d;   // EPCONDIS, EPRXEN, EPTXEN, EPHSHK
        rx_buffer = in_buffer[0];
        rx_index = 0;
        bd_fill(6, (char*)&cdc_notice_packet, USB_EP1_BUFF_SIZE, 0x80);
        bd_fill(8, out_buffer[0], USB_EP2_BUFF_SIZE, 0x80);
    }
}

void CDC_TRN_Handler(int length) {
    int bd = U1STAT >> 2;
    switch (bd) {
        case 6:
        case 7:
            bd_fill(bd ^ 1, (char*)&cdc_notice_packet, USB_EP1_BUFF_SIZE, bd & 1 ? 0xc0 : 0x80);
            break;
        case 8:
        case 9:
            CDCTx(out_buffer[bd & 1], length);
            break;
        case 10:
        case 11:
            CDCRx();
            break;
        default:;
    }
}

char *CDCTrfSetupHandler(setup_packet *SetupPkt) {
    switch (SetupPkt->request) {
        case SET_CONTROL_LINE_STATE:
        case SET_LINE_CODING: return temp;
        case GET_LINE_CODING: return (char*)&line_coding;
        default: return 0;
    }
}

void CDCCtrlTrfSetupComplete(setup_packet *SetupPkt) {
    LINE_CODING *lc = (LINE_CODING*)temp;
    switch (SetupPkt->request) {
        case SET_LINE_CODING:
            if (stop_selection(lc->bCharFormat))
                line_coding.bCharFormat = lc->bCharFormat;
            if (parity_data_selection(lc->bParityType, lc->bDataBits)) {
                line_coding.bDataBits = lc->bDataBits;
                line_coding.bParityType = lc->bParityType;
            }
            if (baud_selection(lc->dwDTERate))
                line_coding.dwDTERate = lc->dwDTERate;
            break;
        default:;
    }
}
