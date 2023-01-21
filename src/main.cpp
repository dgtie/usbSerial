void init(void), toggle(void), USBDeviceInit(void);
bool wait(unsigned);

int main(void) {
    init();
    USBDeviceInit();
    while (wait(0));
}

void poll(unsigned t) {
    if (t & 511) return;
    toggle();
}
