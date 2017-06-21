/*----------------------------------------------------------------------------------------------
************************************ PREPARING FOR RIA'S CODE ************************************
  Author	: Abu Zanki
  Date		: 8th June 2015
  Information	: - Using Timer/Counter 1 16-bit As Counter
		  - Using Timer/Counter 2 8-bit As Timer with 10 ms default range of interupt (can be changed)
		  - Using Serial Communication with State Machine As Communication Protocol
  ----------------------------------------------------------------------------------------------*/
#include "TextCommand.h"
#include <TLC5615.h>
#include <Wire.h>

TLC5615 LLD(10);
TLC5615 ULD(9);
TextCommand cmd(&Serial, 100);
TextCommand i2c(&Wire, 100);

const int signPin = 5;        //T1

volatile unsigned long signCount;
volatile unsigned int tick;

//data ditampung dalam char array yang berisi
//{nilai LLD, nilai ULD, nilai Timer, }
char* data[5];
String bfuCaddr, bfReg, bfval;
byte slave[255];
//set timer dalam ms dan set counter time dalam sec
unsigned int cntTime = 60;
float waktu, tim = 10;
char uCaddr, Reg;
boolean Rd = false;
boolean Wt = false;

void parsing() {

  int n = cmd.available();
  if (n > 0) {
    //    Serial.print(n);
    //    Serial.print(' ');
    //    Serial.println((char *)cmd.command());
    //    Serial.print(cmd.isWrite());
    //    Serial.print(' ');
    //    Serial.print(cmd.ucAddress(), HEX);
    //    Serial.print(' ');
    //    Serial.print(cmd.ioAddress(), HEX);
    //    Serial.print(" '");
    //    Serial.print((char *)cmd.data());
    //    Serial.println("':OK");
  } else {
    if (n != TextCommand::ErrCmdNone) {
      Serial.print(n);
      Serial.print(":ERROR,");
    }
    switch (n) {
      /*
        case TextCommand::ErrCmdNone:
        Serial.println("none");
        break;
      */
      case TextCommand::ErrInvalidCmd:
        Serial.println("invalid");
        break;
      case TextCommand::ErrUnknownEscape:
        Serial.println("unknown escape char");
        break;
      case TextCommand::ErrInternal:
        Serial.println("internal error");
        break;
      case TextCommand::ErrTimeout:
        Serial.println("timeout");
        break;
      case TextCommand::ErrNoData:
        Serial.println("nodata");
        break;
    }
  }

}

void scn_slave() {
  byte device;

  for (int x = 1; x < 256; x++) {
    Wire.beginTransmission(x);
    Wire.write("!R");
    if (x < 17) {
      Wire.write("0");
    }
    Wire.print(x, HEX);
    Wire.println("02");

    int a = i2c.available();
    if (a > 0) {
      device++;
      slave[x] = i2c.ucAddress();
    }
  }

}

void command() {

  if (cmd.isWrite() == 1) {
    if (cmd.ucAddress() == 0) {
      switch (cmd.ioAddress()) {
        case 16:
          data[0] = (char*) cmd.data();
          LLD.analogWrite(data[0]);
          Serial.print("!w0010 ");
          Serial.println(data[0]);
          break;
        case 17:
          data[1] = (char*)cmd.data();
          ULD.analogWrite(data[1]);
          Serial.print("!w0011 ");
          Serial.println(data[1]);
          break;
        case 18:
          data[2] = (char*)cmd.data();
          tim = atoi(data[2]);
          Serial.print("!w0012 ");
          Serial.println(tim);
          //          Serial.println(" ms");
          break;
        case 22:
          data[3] = (char*)cmd.data();
          cntTime = atoi(data[3]);
          Serial.print("!w0016 ");
          Serial.println(cntTime);
          //          Serial.println(" sec");
          break;
      }
    }
    else {
      //kirim data ke device lain
      Wire.beginTransmission(cmd.ucAddress());
      Wire.println((char*)cmd.command());
    }
  }
  else if (cmd.isWrite() == 0) {
    if (cmd.ucAddress() == 0) {
      switch (cmd.ioAddress()) {
        case 0:
          //kirim versi firmware FORMAT ==> aa = nomor revisi FW, bbbb = tahun pembuatan FW, cc = bulan pembuatan FW, RIA
          Serial.println("!R0000 00201706RIA");
          break;
        case 1:
          //kirim list fungsi dan keterangan yang dimiliki device 0

          break;
        case 2:
          //Scan Slave device
          scn_slave();
          break;
        case 19:
          Serial.print("!R0013 ");
          Serial.print(signCount);
          Serial.print(",");
          waktu = tick * tim / 1000;
          Serial.println(waktu);
          Serial.println(tick);
          break;
        case 20:
          start_count();
          Serial.println("!R0014");
          break;
        case 21:
          stop_count();
          Serial.println("!R0015");
          break;

      }
    }
    else {
      //kirim data ke device lain
      Wire.beginTransmission(cmd.ucAddress());
      Wire.println((char*)cmd.command());
    }
  }

}

void start_count() {

  //setup timer2
  //ctc mode
  TCCR2A = (1 << WGM21);
  //prescaler 1024
  TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20);
  //10 ms, 16 MHz, prescaler 1024
  OCR2A = tim * 0.001 / 0.000064;
  //compare match
  TIMSK2 = (1 << OCIE2A);
  TCNT2 = 0;

  //setup counter 1, count on rising edge
  TCCR1B = (1 << CS12) | (1 << CS11) | (1 << CS10);
  TCNT1 = 0;

  sei();

  tick = 0;
  signCount = 0;
}

void stop_count() {

  //non-aktifkan ISR_TIM2OV
  TIMSK2 = (0 << OCIE2A);

  //non-aktifkan counter
  TCCR1B = (0 << CS12) | (0 << CS11) | (0 << CS10);

}

void setup() {

  //init counter pin
  pinMode(signCount, INPUT);
  digitalWrite(signPin, LOW);

  for (int x = 0; x < 5; x++) {
    data[x] = 0;
  }

  sei();
  LLD.begin();
  ULD.begin();
  Serial.begin(115200);
  Wire.begin();

}

void loop() {

  //parsing serial data
  parsing();
  command();

}

ISR (TIMER2_COMPA_vect) // timer2 overflow interrupt
{
  signCount += TCNT1;
  TCNT1 = 0;
  tick += 1;
  if (tick == (cntTime * 1000) / tim) {
    stop_count();
  }
}

