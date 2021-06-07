
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <xc.h>
#include "soft_I2C.h"

//....................................................................
// This function generates an I2C Start Condition
//....................................................................
void i2c_start(void)
{
unsigned int i;

SDA_TRIS = 1;                   // ensure SDA & SCL are high
SCL = 1;
SDA_TRIS = 0;                   // SDA = output
SDA = 0;                        // pull SDA low
for (i=0;i<2;i++) NOP();
SCL = 0;                        // pull SCL low
}


//....................................................................
// This function generates an I2C Stop Condition
//....................................................................
void i2c_stop(void)
{
unsigned int i;

SCL = 0;                        // ensure SCL is low
SDA_TRIS = 0;                   // SDA = output
SDA = 0;                        // SDA low
for (i=0;i<3;i++) NOP();
SCL = 1;                        // pull SCL high
SDA_TRIS = 1;                   // allow SDA to be pulled high
for (i=0;i<3;i++) NOP();
SCL=0;                          // ensure SCL is low
}


//....................................................................
// Outputs a bit to the I2C bus
//....................................................................
void bit_out(unsigned char data)
{
unsigned int i;

SCL = 0;                        // ensure SCL is low
SDA_TRIS=0;                     // configure SDA as an output
SDA= (data>>7);                 // output the MSB
for (i=0;i<2;i++) NOP();
SCL = 1;                        // pull SCL high to clock bit
for (i=0;i<3;i++) NOP();
SCL = 0;                        // pull SCL low for next bit
}


//....................................................................
// Inputs a bit from the I2C bus
//....................................................................
void bit_in(unsigned char *data)
{
unsigned int i;

SCL = 0;                        // ensure SCL is low
SDA_TRIS = 1;                   // configure SDA as an input
SCL = 1;                        // bring SCL high to begin transfer
for (i=0;i<3;i++) NOP();
*data |= SDA;                   // input the received bit
SCL = 0;                        // bring SCL low again.
}


//....................................................................
// Writes a byte to the I2C bus
//....................................................................
unsigned char i2c_wr(unsigned char data)
{
unsigned char i;                // loop counter
unsigned char ack;              // ACK bit

ack = 0;
for (i = 0; i < 8; i++)         // loop through each bit
    {
    bit_out(data);              // output bit
    data = data << 1;           // shift left for next bit
    }

bit_in(&ack);                   // input ACK bit
return ack;
}


//....................................................................
// Reads a byte from the I2C bus
//....................................................................
unsigned char i2c_rd(unsigned char ack)
{
unsigned char i;                // loop counter
unsigned char ret=0;            // return value

for (i = 0; i < 8; i++)         // loop through each bit
    {
    ret = ret << 1;             // shift left for next bit
    bit_in(&ret);               // input bit
    }

bit_out(ack);                   // output ACK/NAK bit
return ret;
}


//.............................................................................
//          Polls the bus for ACK from device
//.............................................................................
void ack_poll (unsigned char control)
{
unsigned char result=1;

while(result)
	{
	i2c_start();            // generate Restart condition
	result=i2c_wr(control); // send control byte (WRITE command)
        }

i2c_stop();                     // generate Stop condition
}


