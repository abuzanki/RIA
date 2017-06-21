#include "TextCommand.h"

TextCommand cmd(&Serial, 100);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

void loop() {
  int n = cmd.available();
  if (n > 0) {
    Serial.print(n);
    Serial.print(' ');
    Serial.println((char *)cmd.command());
    Serial.print(cmd.isWrite());
    Serial.print(' ');
    Serial.print(cmd.ucAddress(), HEX);
    Serial.print(' ');
    Serial.print(cmd.ioAddress(), HEX);
    Serial.print(" '");
    Serial.print((char *)cmd.data());
    Serial.println("':OK");
  } else {
    if (n != TextCommand::ErrCmdNone) {
      Serial.print(n);
      Serial.print(":ERROR,");
    }
    switch(n) {
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
