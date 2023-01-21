#include "usb_config.h"

void CDCTx(int idx);
void out_buffer_handler(char *buffer, int length);

int out_index, out_length;
char *out_buffer;
enum {IDLE, HANDLER, PENDING} out_state;

void CDCTx(char *buffer, int length) {
    out_buffer = buffer; out_length = length;
    switch (out_state) {
        case IDLE:
            out_state = HANDLER;
            out_buffer_handler(buffer, length);
            CDCTx(out_index ^= 1);
            break;
        case HANDLER:
            out_state = PENDING;
            break;
        default:;
    }
}

void out_handler_done(void) {
    switch (out_state) {
        case HANDLER:
            out_state = IDLE;
            break;
        case PENDING:
            out_state = HANDLER;
            out_buffer_handler(out_buffer, out_length);
            CDCTx(out_index ^= 1);
            break;
        default:;
    }
}

//////////////////////////////////////////////////////////////

char *get_CDCRx_buffer(int idx);
void CDCRx(int idx, int length);

enum {INVALID, PARTIAL, USER, FULL} in_state;
char *in_buffer;
int in_length, in_index;
bool inBusy;

char *get_in_buffer(int &length) {
    switch (in_state) {
        case INVALID:
            in_state = USER;
            in_length = 0;           
            length = USB_EP2_BUFF_SIZE;
            return in_buffer = get_CDCRx_buffer(in_index);
        case PARTIAL:
            in_state = USER;
            length = USB_EP2_BUFF_SIZE - in_length;
            return &in_buffer[in_length];
        case FULL:
            return 0;
        default:;
    }
}

bool CDCRx(int length) {
    if (inBusy) return false;
    in_state = INVALID;
    CDCRx(in_index, length);
    in_index ^= 1;
    return inBusy = true;
}

void CDCRx(void) {
    inBusy = false;
    switch (in_state) {
        case PARTIAL:
            if (!in_length) break;
        case FULL:
            CDCRx(in_length);
            break;
        default:;
    }
}

void return_in_buffer(int length) {
    if (!CDCRx(in_length += length))
        in_state = in_length == USB_EP2_BUFF_SIZE ? FULL : PARTIAL;
}