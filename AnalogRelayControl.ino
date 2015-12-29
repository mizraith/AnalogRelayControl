/**************************************************************************
 * AnalogRelayControl
 * BASIC SERIAL PORT ANALOG INTERFACE AND RELAY OUTPUT CONTROLLER
 *
 * FUNCTIONALITY BASICS:
 *   (1) Reads port A0
 *   (2) Outputs to purt D6
 *   (3) Output status indicator on D13 (or D9 depending on Arduino)
 *   (4) Serial controlled and responses
 *   (5) Unique ID number so that you can gang several of these up if need be.
 *
 *
 * 
 * 12/22/2015  Red Byer - Author - original date   http://github.com/mizraith
 * 
 * v1.0 12/28/2015   Initial release
 ************************************************************************* */

#include <Arduino.h>
#include <avr/pgmspace.h>
#include <EEPROM.h>
// included for the datetime class.  Wire.h makes the RTCLib happy.  
//#include <Wire.h>   
#include"mizraith_DateTime.h"   

const DateTime COMPILED_ON = DateTime(__DATE__, __TIME__);  
const int SID_MAX_LENGTH = 25;   //24 + null
const int SID_START_ADDRESS = 0;
char SID[SID_MAX_LENGTH] = "USE >SID:xxx TO SET";

#define ANALOG_INPUT   A0
#define LED_PIN        13
#define RELAY_PIN      6

uint16_t AnalogValue = 0x000;


//--------------------------------------------------------
//  SERIAL INPUT FUNCTION
// ACTS IMMEDIATELY ONE CHAR AT A TIME
//--------------------------------------------------------
char c;
const char NONE = -1;
const char MAX_INPUT_STRING_LENGTH = 32;  
bool accumulatestring = false;
char userstringbuff[MAX_INPUT_STRING_LENGTH];

void serialEvent() {
    c = Serial.read();  
    if (  (c == '\r') || 
          (c == '\n') )  {
       c = NONE;
       if(accumulatestring) {
           accumulatestring = false;
           handleInputString(userstringbuff);
           return;
       }
    }
   
    if( c == '>') {
        accumulatestring = true;
        strcpy(userstringbuff, "");  //blank it to accumulate
    }
    
    if(accumulatestring) {
        if ( c == '>') {
            return;  //don't accumulate start char
        }  else {
            if (strlen(userstringbuff) > (MAX_INPUT_STRING_LENGTH - 2) ) {
                return;               //too long, don't append anymore
            } else {
                char cbuff[2] = " ";  //char plus null0
                cbuff[0] = c;
                strcat(userstringbuff,  cbuff);
            }
        }  
//    } else if (c != NONE) {
//      //SEE IF USER ENTERED A MODE NUMBER
//      int x = c - 48;     //simple c to i conversion minus 1 for array indexing :  ord for 0 is 48. 
//      if ( (x >= mizraith_DigitalClock::INPUT_EVENT_OFFSET ) && (x < mizraith_DigitalClock::MAX_NUMBER_OF_INPUT_EVENTS ) ) {
//        input_event = x;
//        input_event_data = 0;
//        Serial.print(F("Input Event Via Serial: "));
//        Serial.println(x);
    } else {
        Serial.println(F("!Invalid input given. Commands start with >"));
    }
    c = NONE;       //reset
}

/* SERIALCOMMAND SET: 
   User commands start with ">" and end with "\r" or "\n"
   Responses start with "#" and end with "\r" or "\n"
   
   Command format:   >CMD:VALUE   
      CMD is  [get , D6 , SID]
          "get" returns all values, does not use :VALUE
          "D6" is used to set output.  VALUE must be 1 or 0
          "SID" is used to set station id:  VALUE is string, < 24 bytes
   Response format:  #TOKEN TOKEN TOKEN....
       The first TOKEN is the station identifer in the form:  #SID:a8ads9sadn
       Other TOKENS are in format  IDENTIFER:VALUE:UNITS   where UNITS is optional
           IDENTIFIER may be: [SID, A0, D6]
           VALUES may be integers or floats
           UNITS are a string, but may be [ARB, C, BIN, F, A, V]
   Examples   
       User sends:  >get\r
       Response:    #SID:01234 A0:1038:ARB D6:OFF:BIN
       User sends:  >D6:1
       Response:    #SID:01234 A0:1038:ARB D6:1:BIN
       User sends:  >D6:0
       Response:    #SID:01234 A0:1038:ARB D6:0:BIN
 */
// 2 character commands
const String str_A0   = "A0";
const String str_D6   = "D6";
// 3 character commands
const String str_SID  = "SID";
const String str_get  = "get";
// other strings
const String str_ON   = "1";
const String str_OFF  = "0";
const String str_ARB  = "ARB";
const String str_BIN  = "BIN";
const String str_RESP = "#";
const String str_COLON  = ":";

void handleInputString( char * userstringbuff ) {
    boolean success = false;
    
    Serial.print(F("RECEIVED INPUT->"));
    Serial.println(userstringbuff);
    //Note that the ">" has already been removed by this point.
    
    // 2 Character Commands
    char cmd2[3];
    strncpy(cmd2, userstringbuff, 2);
    cmd2[2] = '\0';
    // 3 Character Commands
    //cmdlen = sizeof(str_SID);                //cmdlen = 4
    char cmd3[4];                        
    strncpy(cmd3, userstringbuff, 3);          //  copy first 3 chars 
    cmd3[3] = '\0';
    
    if ( str_D6.equalsIgnoreCase(cmd2) ) {
        success = handleCommand_D6(userstringbuff);
    } 
    else if ( str_SID.equalsIgnoreCase(cmd3) ) {
        success = handleCommand_SID(userstringbuff);
    }
    else if ( str_get.equalsIgnoreCase(cmd3) ) {
        // Format get    nothing else matters
        success = true;  // we'll return a response at the end
    }
    
    if (success) {
        printSerialResponseString();
        return;
    } else {
        //if we got here, there was no command to parse
        Serial.print(F("!BAD_COMMAND:"));
        Serial.println(userstringbuff);
        return;
    }
}


boolean handleCommand_D6( char * userstringbuff ) {
    // Format   D6:1  or D6:0   
    // check for : at [2]
    boolean success = false;
    char c = userstringbuff[2];
    if ( c == str_COLON[0] ) {
        // check for a 0 or a 1
        c = userstringbuff[3];
        
        if ( c == str_ON[0] ) {
            digitalWrite( 6, HIGH );    
            digitalWrite(LED_PIN, HIGH);   
            success = true;
        }
        else if ( c == str_OFF[0] ) {
            digitalWrite( 6, LOW );
            digitalWrite(LED_PIN, LOW);
            success = true;
        } 
        else {
            success = false;
        }
    }      
    return success;    
}

boolean handleCommand_SID( char * userstringbuff ) {
    // Format SID:29138384
    //check for  at [3]
    boolean success = false;
    char c = userstringbuff[3];
    uint8_t offset = 4;        // position of first SID character in command string
    
    if ( c == str_COLON[0] ) {
        //get the rest of the string
        char SIDbuff[SID_MAX_LENGTH];
        for (int i=0; i< SID_MAX_LENGTH - 1 ; i++) {
            SIDbuff[i] = userstringbuff[ offset + i ];
            if ( SIDbuff[i] == '\0' ) {
                break;
            }
        }
        strncpy(SID, SIDbuff, SID_MAX_LENGTH);   // copy SIDbuff into SID
        writeSIDToEEPROM();
        success = true;    
    } 
    return success;
}


void printSerialResponseString( ) {
    // Format:   #SID:xxxxx A0:val:units D6:val:units 
    int relay = digitalRead( RELAY_PIN );
    Serial.print(F("#SID:"));Serial.print(SID);
    Serial.print(F(" "));
    Serial.print(F("A0:"));Serial.print(AnalogValue, DEC);Serial.print(F(":ARB "));
    Serial.print(F(" "));
    Serial.print(F("D6:"));Serial.print(relay, DEC);Serial.print(F(":BIN"));
    Serial.println();
    
}




/***************************************************
 ***************************************************
 *   SETUP 
 ***************************************************
 ***************************************************/

void setup() {  
    readSIDFromEEPROM();
    
    Serial.begin(57600);
    serialPrintHeaderString();
    checkRAMandExitIfLow(0);
    
    pinMode(ANALOG_INPUT, INPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);
    
    digitalWrite(RELAY_PIN, LOW);
    
    printSerialInputInstructions();
    printSerialDataStart();
}


/***************************************************
 ***************************************************
 *   MAIN LOOP 
 ***************************************************
 ***************************************************/
void loop() {
    // put your main code here, to run repeatedly:
    AnalogValue = analogRead(ANALOG_INPUT);
    //AnalogValue = AnalogValue + 501;
    delay(25);
}





/***************************************************
 *   SETUP HELPERS
 ***************************************************/

void serialPrintHeaderString() {
  Serial.println();
  Serial.println(F("#####HEADER#####"));
  Serial.println(F("#--------------------------------------------------"));  
  Serial.println(F("# Analog Monitor Relay Controller"));
  Serial.println(F("#--------------------------------------------------"));  
  Serial.println(F("# Red Byer    redbyer@ipgphotonics.com"));
  Serial.println(F("# VERSION DATE: 12/22/2015"));
  Serial.print(F("# COMPILED ON: "));
  Serial.print(COMPILED_ON.month());
  Serial.print(F("/"));
  Serial.print(COMPILED_ON.day());
  Serial.print(F("/"));
  Serial.print(COMPILED_ON.year());
  Serial.print(F(" @ "));
  Serial.print(COMPILED_ON.hour(), DEC);
  Serial.println(COMPILED_ON.minute(), DEC); 
  Serial.println(F("#--------------------------------------------------"));  
  Serial.println(F("#"));
}

void printSerialInputInstructions( void ) {
    Serial.println(F("#"));
    Serial.println(F("#--------------------------------------------------")); 
    Serial.println(F("# Serial Control Instructions"    ));
    Serial.println(F("#   Format:  >CMD:VALUE<CR><LF>"    ));
    Serial.println(F("#    > is start.  CMDs are:  [get , D6  , SID]"    ));
    Serial.println(F("#   VALUEs:  [1, 0]"    ));
    Serial.println(F("#   EXAMPLES:   >get     >D6:ON   >D6:OFF  >SID:2929"  ));
    Serial.println(F("#   RESPONSE    #SID:1234 A0:128:ARB D6:OFF:BIN"  ));
    Serial.println(F("#   Token is   IDENTIFIER:VALUE:UNITS  where UNITS is optional "  ));
    Serial.println(F("#--------------------------------------------------"));  
}

void printSerialDataStart() {
    Serial.println(F("#"));
    Serial.print(F("# Current Station ID: "));
    Serial.println(SID);
    Serial.println(F("#"));
    Serial.println("#####DATA#####");
}


/***************************************************
 *   EEPROM HELPERS
 ***************************************************/
void readSIDFromEEPROM( ) {
    for (int i=0; i < SID_MAX_LENGTH-1; i++) {
        SID[i] = EEPROM.read(i + SID_START_ADDRESS);
        if ( SID[i] == '\0' )  {
            return;          // reached string end
        }
        if (i == SID_MAX_LENGTH - 2) {     //coming up on end
            SID[SID_MAX_LENGTH-1] = '\0';  //make sure end is terminated
            return;
        }
        
    }
}

void writeSIDToEEPROM( ) {
    for (int i=0; i < SID_MAX_LENGTH-1; i++) {
        EEPROM.write(i + SID_START_ADDRESS, SID[i]);
        if ( SID[i] == '\0' ) {
            return;
        }
        if (i == SID_MAX_LENGTH - 2) {          // coming up on end
            //make sure to finish with a null
            EEPROM.write( (SID_MAX_LENGTH-1) + SID_START_ADDRESS, '\0' );  // last char must be null
            return;
        }      
    }
}











/***************************************************
 *   LOW LEVEL HELPERS
 ***************************************************/
void checkRAMandExitIfLow( uint8_t checkpointnum ) {
  int x = freeRam();
  if (x < 128) {
    Serial.print(F("!!! WARNING LOW SRAM !! FREE RAM at checkpoint "));
    Serial.print(checkpointnum, DEC);  
    Serial.print(F(" is: "));
    Serial.println(x, DEC);
    Serial.println();
    gotoEndLoop();
  } else {
    Serial.print(F("# FREE RAM, at checkpoint "));
    Serial.print(checkpointnum, DEC);  
    Serial.print(F(" is: "));
    Serial.println(x, DEC);
  }
}


//Endpoint for program (really for debugging)
void gotoEndLoop( void ) {
  Serial.println(F("--->END<---"));
  while (1) {
    delay(100);   
  }
}


/**
 * Extremely useful method for checking AVR RAM status.  I've used it extensively and trust it!
 * see: http://playground.arduino.cc/Code/AvailableMemory
 * and source:  http://www.controllerprojects.com/2011/05/23/determining-sram-usage-on-arduino/
 */
int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

