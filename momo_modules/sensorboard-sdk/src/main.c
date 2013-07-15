#include <pic12f1822.h>
#include "bus_master.h"
#include "flash_memory.h"

/* Use internal oscillator as the frequency oscillator. */
#pragma config FOSC=INTOSC
/* Disable the watchdog timer. */
#pragma config WDTE=OFF
/* Disbable 4x phase lock loop. */
#pragma config PLLEN=OFF
/* Flash memory write protection off. */
#pragma config WRT=OFF

#define LED RA5
#define BUTT RA4

void heartbeat();
void initialize ();

//*****************************************************************************
//  This project must be compiled with :
//  Optimization settings : SPEED must be set
//  ROM Ranges = 0 - 1FF
//  Additional command line :  -L-pstrings=CODE
//  *All values here are in hex.
//*****************************************************************************

// The bootloader code does not use any interrupts.
// However, the downloaded code may use interrupts.
// The interrupt vector on a PIC12LF1822 is located at
// address 0x0004. The following function will be located
// at the interrupt vector and will contain a jump to
// 0x0204
void interrupt service_isr() {
    // Handle i2c interrupts (MSSP) in the bootloader.
    if (SSP1IF == 1) {
        while (SSP1IF == 1)
        {
            SSP1IF = 0; //Do this because with our slow clock we might miss an interrupt
            if (i2c_slave_active()) {
                i2c_slave_interrupt();
            } else {
                i2c_master_interrupt();
            }
        }
    } else {
        /*#asm
            GOTO 0x204;
        #endasm*/
    }
}

void main() {
    initialize();
    bus_init();

    // If button is pressed, then force bootloader mode
    if (BUTT) {
        goto Bootloader;
    }

    flash_erase_application();

    bus_master_compose_params(plist_define1(kMIBInt16Type));
    set_intparam(0,5);
    bus_master_rpc_sync(0x08, 0x01, 0x00);

    // 0x7ff is highest word in flash memory for pic12lf1822
    // if application is loaded, highest byte will be 0x55
    //if (flash_memory_read (0x7FF) == 0x3455) {
        /*#asm
            GOTO 0x200;
        #endasm*/
    //}

Bootloader:
    while (1) {
        ;
    }
}

void initialize ()
{
    /* Software HS internal oscilator at 4 Mhz */
    OSCCON = (0b1101 << 3);

    //wait for the pll to stabilize.
    while (!HFIOFS)
        ;

    /* Set all PORTA pins to be input. */
    TRISA = 0xff;
    /* Set PORTA pin 5 to be output. */
    TRISA5 = 0;
    RA5 = 0;

    /* Set all PORTA pins to be digital I/O (instead of analog input). */
    ANSELA = 0;
    /* Turn Timer1 on with 1:8 prescale using FOSC/4 as source. */
    T1CON = 0x31;
    
    /* Enable interrupts globally. */
    GIE = 1;
    /* Enable peripheral interrupts. */
    PEIE = 1;
}
