#ifndef _TWI_
#define _TWI_

#include <avr/io.h>
#include <stdint.h>

typedef enum {
    TWI_INIT = 0, 
    TWI_READY,
    TWI_ERROR
} TWI_Status;

typedef enum{
    TWI_WRITE = 0,
    TWI_READ = 1
} TWI_Direction;

uint8_t twictl_start(uint8_t address, TWI_Direction dir);
uint8_t twictl_tx(uint8_t data); 
uint8_t twictl_txn(uint8_t *pData, uint8_t len); 
uint8_t twictl_rx(uint8_t *data); 
uint8_t twictl_rxn(uint8_t *pData, uint8_t len); 
uint8_t twictl_txna(uint8_t address, uint8_t *pData, uint8_t len);
uint8_t twictl_rxna(uint8_t address, uint8_t *pData, uint8_t len); 
void    twictl_stop(void);

void twictl_init(long freq, uint8_t CTRLA){     		     
    TWI0.CTRLA = CTRLA;			 		     
    TWI0.MBAUD = ((((float)F_CPU/(float)freq)-10)/2);   
    TWI0.MCTRLA = TWI_ENABLE_bm;         		     
    TWI0.MADDR = 0x00;                   		     
    TWI0.MDATA = 0x00;                   		     
    TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc; 
}

static TWI_Status TWI_GetStatus(void){
    TWI_Status state = TWI_INIT;
    do{
        if (TWI0.MSTATUS & (TWI_WIF_bm | TWI_RIF_bm)){
            state = TWI_READY;
        }
        else if (TWI0.MSTATUS & (TWI_BUSERR_bm | TWI_ARBLOST_bm)){
            state = TWI_ERROR;
        }
    } while (!state);
    return state;
}

//returns 1 on ack
static uint8_t RX_acked(void){
    return (!(TWI0.MSTATUS & TWI_RXACK_bm));
}

//returns 1 on ack
uint8_t twictl_start(uint8_t address, TWI_Direction dir){
    TWI0.MADDR = (address << 1) | dir;
    return ((TWI_GetStatus() == TWI_READY) && RX_acked());
}

uint8_t twictl_tx(uint8_t data){
    TWI0.MDATA = data;
    return RX_acked();
}

//returns -1 on error
uint8_t twictl_txn(uint8_t *pData, uint8_t len){
    uint8_t retVal = 0;
    if ((len != 0) && (pData != 0)){
        while (len--){
            TWI0.MDATA = *pData;
            if ((TWI_GetStatus() == TWI_READY) && RX_acked()){
                retVal++;
                pData++;
                continue;
            }
            else{
                break;
            }
        }
    }
    return retVal;
}

//sends to addresss
uint8_t twictl_txna(uint8_t address, uint8_t *pData, uint8_t len){
    if (!twictl_start(address, TWI_WRITE))
        return ((uint8_t)-1);
    return twictl_txn(pData, len);
}

//returns how many bytes have been read 
uint8_t twictl_rx(uint8_t *pData){
    if (TWI_GetStatus() == TWI_READY){
        *pData = TWI0.MDATA;
        return 1;
    }
    else
    return 0;
}

//returns how many bytes have been read
uint8_t twictl_rxn(uint8_t *pData, uint8_t len){
    uint8_t retVal = 0;

    if ((len != 0) && (pData != 0)) {
        while (len--){
            if (TWI_GetStatus() == TWI_READY){
                *pData = TWI0.MDATA;
                TWI0.MCTRLB = (len == 0) ? TWI_ACKACT_bm | TWI_MCMD_STOP_gc : TWI_MCMD_RECVTRANS_gc;
                retVal++;
                pData++;
                continue;
            }
            else
                break;
        }
    }

    return retVal;
}

//returns how many bytes have been received, -1 means NACK at address 
uint8_t twictl_rxna(uint8_t address, uint8_t *pData, uint8_t len){
    uint8_t retVal = (uint8_t)-1;

    if (!twictl_start(address, TWI_READ))
        return retVal;

    retVal = 0;
    if ((len != 0) && (pData != 0)){
        while (len--) {
            if (TWI_GetStatus() == TWI_READY){
                *pData = TWI0.MDATA;
                TWI0.MCTRLB = (len == 0) ? TWI_ACKACT_bm | TWI_MCMD_STOP_gc : TWI_MCMD_RECVTRANS_gc;
                retVal++;
                pData++;
                continue;
            }
            else
                break;
        }
    }

    return retVal;
}

void twictl_stop(void){
    TWI0.MCTRLB = TWI_MCMD_STOP_gc;
}
#endif
