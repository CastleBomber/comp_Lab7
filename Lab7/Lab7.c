/**  SPI C Seven-Segment Display Example, Written by Derek Molloy
*    COMP 462
*    Editor: Richie Romero CastleBomber
*/
#include<stdlib.h>
#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/ioctl.h>
#include<stdint.h>
#include<linux/spi/spidev.h>
#define SPI_PATH "/dev/spidev0.0"
#include<linux/i2c.h>
#include<linux/i2c-dev.h>
#define BUFFER_SIZE 19

// binary data
const unsigned char symbols[16] = {                   //(msb) HGFEDCBA (lsb)
     0b00111111, 0b00000110, 0b01011011, 0b01001111,  // 0123
     0b01100110, 0b01101101, 0b01111101, 0b00000111,  // 4567
     0b01111111, 0b01100111, 0b01110111, 0b01111100,  // 89Ab
     0b00111001, 0b01011110, 0b01111001, 0b01110001   // CdEF
};

int transfer(int fd, unsigned char send[], unsigned char rec[], int len){
   struct spi_ioc_transfer transfer;        //the transfer structure
   transfer.tx_buf = (unsigned long) send;  //the buffer for sending data
   transfer.rx_buf = (unsigned long) rec;   //the buffer for receiving data
   transfer.len = len;                      //the length of buffer
   transfer.speed_hz = 1000000;             //the speed in Hz
   transfer.bits_per_word = 8;              //bits per word
   transfer.delay_usecs = 0;                //delay in us
   transfer.cs_change = 0;       // affects chip select after transfer
   transfer.tx_nbits = 0;        // no. bits for writing (default 0)
   transfer.rx_nbits = 0;        // no. bits for reading (default 0)
   transfer.pad = 0;             // interbyte delay - check version

   // send the SPI message (all of the above fields, inc. buffers)
   int status = ioctl(fd, SPI_IOC_MESSAGE(1), &transfer);
   if (status < 0) {
      perror("SPI: SPI_IOC_MESSAGE Failed");
      return -1;
   }
   return status;
}

// the time is in the registers in decimal form
int bcdToDec(char b) { return (b/16)*10 + (b%16); }

int main(){
   int fd, i;                              // file handle and loop counter
   unsigned char null=0x00;                // sending only a single char
   uint8_t mode = 3;                       // SPI mode 3
   int file;

   //[8.1]
   printf("Starting the DS3231 test application\n");
   if((file=open("/dev/i2c-1", O_RDWR)) < 0){
      perror("failed to open the bus\n");
      return 1;
   }
   if(ioctl(file, I2C_SLAVE, 0x68) < 0){
      perror("Failed to connect to the sensor\n");
      return 1;
   }
   char writeBuffer[1] = {0x00};
   if(write(file, writeBuffer, 1)!=1){
      perror("Failed to reset the read address\n");
      return 1;
   }
   char buf[BUFFER_SIZE];
   if(read(file, buf, BUFFER_SIZE)!=BUFFER_SIZE){
      perror("Failed to read in the buffer\n");
      return 1;
   }

   //[8.4]
   // Call SPI bus properties
   if ((fd = open(SPI_PATH, O_RDWR))<0){
      perror("SPI Error: Can't open device.");
      return -1;
   }
   if (ioctl(fd, SPI_IOC_WR_MODE, &mode)==-1){
      perror("SPI: Can't set SPI mode.");
      return -1;
   }
   if (ioctl(fd, SPI_IOC_RD_MODE, &mode)==-1){
      perror("SPI: Can't get SPI mode.");
      return -1;
   }
   printf("SPI Mode is: %d\n", mode);
   printf("Counting in hexadecimal from 0 to F now:\n");

   // [8.4] SPI
   for (i=0; i<=15; i++)
   {
      // send, recieve data
      if (transfer(fd, (unsigned char*) &symbols[i], &null, 1)==-1){
         perror("Failed to update the display");
         return -1;
      }

      printf("%4d\r", i);
      fflush(stdout);       // flush output
      usleep(500000);
   }

   printf("RTC time is %02d:%02d:%02d\n", bcdToDec(buf[2]),
                                          bcdToDec(buf[1]),
                                          bcdToDec(buf[0]));

   char secondsAsString[];
   sprintf(secondsAsString, "%d", bcdToDec(buf[0]));

   if ((bcdToDec(buf[0])) >= 10){
     printf("Big\n");
     // printf(#)
   } else{
     printf("Small");
   }

   close(file);
   close(fd);               // close the file
   return 0;
}
