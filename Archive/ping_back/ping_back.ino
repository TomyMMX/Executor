/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

/**
 * Example for Getting Started with nRF24L01+ radios. 
 *
 * This is an example of how to use the RF24 class.  Write this sketch to two 
 * different nodes.  Put one of the nodes into 'transmit' mode by connecting 
 * with the serial monitor and sending a 'T'.  The ping node sends the current 
 * time to the pong node, which responds by sending the value back.  The ping 
 * node can then see how long the whole cycle took.
 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"


RF24 radio(4,10);

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xE8E8F0F0E1LL, 0xF0F0F0F0D2LL };

void setup(void)
{
  //
  // Print preamble
  //

  Serial.begin(57600);
  
  printf_begin();
  printf("\n\rRF24/back/\n\r");
  
  pinMode(17, OUTPUT);
  
  delay(5000);
  radio.begin();
  radio.setChannel(125);
  radio.setAutoAck(false);  
  radio.setRetries(15,15);
  //radio.setPayloadSize(16);
  
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1,pipes[0]);
  
  radio.printDetails();
  
  radio.startListening();
}

void loop(void)
{
    // if there is data ready
    if ( radio.available() )
    {
      // Dump the payloads until we've gotten everything
      unsigned long got_time[2];
      bool done = false;
      while (!done)
      {
        // Fetch the payload, and see if this was the last one.
        done = radio.read(got_time, sizeof(got_time) );    

        // Spew it
        printf("TeeTime %lu, ",got_time[0]);
        printf("SlideTime %lu.\n",got_time[1]);
        delay(10);
      }
      
      // First, stop listening so we can talk
      radio.stopListening();
      unsigned int bu = 1;
      radio.write(&bu, sizeof(unsigned int) );
      // Now, resume listening so we catch the next packets.
      radio.startListening();
    }
}
