
/*
 *  Realtime GPS+GPRS Tracking of Vehicles Using Arduino
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see http://www.gnu.org/licenses/.
 *
 *  Version:           1.0
 *  Author:            J. Barthelemy
 *  Inspired from the work of David Gascón and Marcos Martinez
 */

#include <SoftwareSerial.h>

// Serial port to communicate with the shield
SoftwareSerial serial_shield(2, 3); // RX, TX

// Global variables
int8_t answer;

// various phone numbers
char number[20];
char realnumber[10];
char phone_number[] = "0467230059";

int a = 0;
int b = 0;
int c = 0;
int gps_on = 0;

// auxiliary strings
char aux_str[200];
char aux;
int x = 0;

// gps data
char frame[200];
char* latitude;
char* longitude;
char* altitude;
char* date;
char* satellites;
char* speed;
char* course;

// Init the Arduino and the Shield
void setup() {

  Serial.begin(9600);
  serial_shield.begin(9600);

  // check that the Shield is powered on
  while (sendATcommand("AT", "OK", 2000) == 0) {   // Send AT every two seconds and wait for the answer
    delay(2000);
  }

  // register to the network
  while ( (sendATcommand("AT+CREG?", "+CREG: 0,1", 1000) || sendATcommand("AT+CREG?", "+CREG: 0,5", 1000)) == 0 );
  sendATcommand("AT+CLIP=1", "OK", 1000);
  while (sendATcommand("AT+CREG?", "+CREG: 0,1", 2000) == 0);

  // sets GPRS APN , user name and password
  sendATcommand("AT+SAPBR=3,1,\"Contype\",\"GPRS\"", "OK", 2000);
  sendATcommand("AT+SAPBR=3,1,\"APN\",\"mdata.net.au\"", "OK", 2000);
  sendATcommand("AT+SAPBR=3,1,\"USER\",\"\"", "OK", 2000);
  sendATcommand("AT+SAPBR=3,1,\"PWD\",\"\"", "OK", 2000);

  // gets the GPRS bearer
  while (sendATcommand("AT+SAPBR=1,1", "OK", 20000) == 0) {
    delay(5000);
  }

  // emptying the serial buffer
  delay(1000);
  while(serial_shield.available() != 0) {
    serial_shield.read();
  }

}

// Main loop
void loop() {

  //Detect incomming call
  answer = sendATcommand("", "+CLIP", 1000);
  if (answer == 1) {

    // read the incoming byte:
    for (int i = 0; i < 19; i++) {   
      // ... wait until the serial communication is ready
      while (serial_shield.available() == 0) {
        delay(50);
      }
      // ... stores phone number
      number[i] = serial_shield.read();
    }
    
    serial_shield.flush();
  
    //Stores phone calling number
    for (int i = 0; i <= 14; i++) {
      if (number[i] == '"') {
        i++; realnumber[0] = number[i];
        i++; realnumber[1] = number[i];
        i++; realnumber[2] = number[i];
        i++; realnumber[3] = number[i];
        i++; realnumber[4] = number[i];
        i++; realnumber[5] = number[i];
        i++; realnumber[6] = number[i];
        i++; realnumber[7] = number[i];
        i++; realnumber[8] = number[i];
        i++; realnumber[9] = number[i];
        break;
      }
    }
    
    // Check phone number
    //Serial.print(F("Calling number: "));
    //Serial.print(realnumber);
    //Serial.println(F("..."));
    for (int i = 0; i < 10; i++) {
      if (realnumber[i] == phone_number[i]) {
        a++;
        if (a == 10) {
          //Serial.println(F("Correct number!"));
          sendATcommand("ATH", "OK", 1000);
          if (b == 1) {
            b = 0;
          } 
          else {
            b = 1;
            c = 1;
          }
          // powering on the gps
          while (gps_on == 0) {
            gps_on = start_GPS();
          }
          break;
        }
      } 
      else {
        //Serial.println(F("Wrong number!"));
        break;
      }
    }
    
    a = 0;
    answer = 0;
  
  }

  //Send SMS once and position to HTTP
  if (b == 1) {
    get_GPS();
    send_HTTP();
    if (c == 1) {
      delay(500);
      sendSMS();
      delay(100);
      c = 0;
    }
    // wait 10 sec; so when added to the 2 sec delay of the main loop,
    // the gps data will be send 5 times per minute
    delay(10000);
  }

  // wait 2 secs before next loop
  delay(2000);

}

// sending AT commands to the Sim808 module
int8_t sendATcommand(char* ATcommand, char* expected_answer, unsigned int timeout) {

  uint8_t x = 0, answer = 0;
  char response[200];
  unsigned long previous;

  memset(response, '\0', 200);    // Initialice the string

  delay(100);

  // Clean the input buffer
  while (serial_shield.available() > 0 ) serial_shield.read();

  if (ATcommand[0] != '\0') {
    // Send the AT command
    serial_shield.println(ATcommand);
    //Serial.print(F("AT Command: "));
    //Serial.println(ATcommand);
  }

  // Waiting for the answer
  x = 0;
  previous = millis();

  do {

    // if there are data in the UART input buffer, reads it and checks for the asnwer
    if (serial_shield.available() > 0) {

      response[x] = serial_shield.read();
      x++;
      // check if the desired answer (OK) is in the response of the module
      if (strstr(response, expected_answer) != NULL) {
        answer = 1;
      }
    }

  } while ((answer == 0) && ((millis() - previous) < timeout));   // Waits for the asnwer with time out

  //Serial.println(F("START RESPONSE:"));
  //Serial.println(response);
  //Serial.println(F("END RESPONSE"));
  //if (answer == 0) {
  //  Serial.println("Could not get the expected answer!");
  //}
  //else {
  //  Serial.println("Found expected answer!");
  //}
  
  return answer;
  
}

int8_t start_GPS() {

  unsigned long previous = millis();

  // starts the GPS
  sendATcommand("AT+CGNSPWR=1", "OK", 2000);

  // waits for fix GPS with a time out
  while ((sendATcommand("AT+CGNSINF", "+CGNSINF: 1,1", 5000) == 0) && (millis() - previous < 180000));

  if ((millis() - previous) < 90000) {
    return 1;
  }  else {
    return 0;
  }
  
}

// retrieiving the location data
int8_t get_GPS() {

  int8_t counter, answer;
  long previous;

  // First get the NMEA string
  // Clean the input buffer
  while (serial_shield.available() > 0) serial_shield.read();
  
  // request Basic string when have a gps fix
  //sendATcommand("AT+CGNSINF", "+CGNSINF:", 2000);
  while (sendATcommand("AT+CGNSINF", "+CGNSINF: 1,1", 2000) == 0);

  // this loop waits for the NMEA string (with a time out)
  counter = 0;
  answer = 0;
  memset(frame, '\0', 200);    // Initialize the string
  previous = millis();
  do {

    if (serial_shield.available() != 0) {
      frame[counter] = serial_shield.read();
      counter++;
      // check if the desired answer is in the response of the module
      if (strstr(frame, "OK") != NULL) {
        answer = 1;
      }
    }
  } while ((answer == 0) && ((millis() - previous) < 2000));

  frame[counter - 3] = '\0';

  // Parses the string
  //Serial.println(F("FRAME"));
  //Serial.println(frame);
  date = strtok(frame, ",");
  latitude = strtok(NULL, ",");  // Gets latitude
  longitude = strtok(NULL, ","); // Gets longitude
  altitude = strtok(NULL, ",");  // Gets altitude
  speed = strtok(NULL, ",");     // Gets speed over ground (unit is knots)
  course = strtok(NULL, ",");    // Gets course
  strtok(NULL, ",");
  strtok(NULL, ",");
  strtok(NULL, ",");
  strtok(NULL, ",");
  satellites = strtok(NULL, ","); // Gets number of satellites used

  //Serial.println(F("DATA"));
  //Serial.println(date);
  //Serial.println(longitude);
  //Serial.println(latitude);
  //Serial.println(altitude);
  //Serial.println(speed);
  //Serial.println(course);
  //Serial.println(satellites);

  return answer;

}

void send_HTTP() {

  uint8_t answer = 0;
  char *data = (char*)malloc(strlen(latitude) + strlen(longitude) + strlen(altitude) + strlen(date)
                             + strlen(satellites) + strlen(speed) + strlen(course) + 1);
  
  // Initializes HTTP service
  answer = sendATcommand("AT+HTTPINIT", "OK", 10000);
  if (answer == 1) {
    // Sets CID parameter
    answer = sendATcommand("AT+HTTPPARA=\"CID\",1", "OK", 5000);
    if (answer == 1) {
      // Sets url
      //sprintf(aux_str, "AT+HTTPPARA=\"URL\",\"http://jbar.duckdns.org/gps?\"");
      sprintf(aux_str, "AT+HTTPPARA=\"URL\",\"http://jbar.duckdns.org/gps?");
      //Serial.print("aux_str: ");
      //Serial.println(aux_str);
      serial_shield.print(aux_str);
      sprintf(data, "latitude=%s&longitude=%s&altitude=%s&date=%s&satellites=%s&speed=%s&course=%s",
              latitude, longitude, altitude, date, satellites, speed, course);
      serial_shield.print(data);
      Serial.println(data);
      answer = sendATcommand("\"", "OK", 5000);
      if (answer == 1) {
        // Starts GET action
        //while (sendATcommand("AT+HTTPACTION=0", "+HTTPACTION: 0,200", 30000) == 0);
        //answer = 1; 
        answer = sendATcommand("AT+HTTPACTION=0", "+HTTPACTION: 0,200", 30000);
        if (answer == 1) {
          Serial.println(F("Sent to server!"));
        }
        else {
          Serial.println(F("Error getting url!"));
        }

      }
      else {
        Serial.println(F("Error setting the url!"));
      }
    }
    else {
      Serial.println(F("Error setting the CID"));
    }
  }
  else {
    Serial.println(F("Error initializating"));
  }

  sendATcommand("AT+HTTPTERM", "OK", 5000);
  free(data);

}

void sendSMS() {

  delay(3000);

  //Serial.println(F("Connecting to the network..."));
  while ( (sendATcommand("AT+CREG?", "+CREG: 0,1", 500) ||
           sendATcommand("AT+CREG?", "+CREG: 0,5", 500)) == 0 );

  //Serial.print(F("Setting SMS mode..."));
  sendATcommand("AT+CMGF=1", "OK", 1000);    // sets the SMS mode to text

  //Serial.println(F("Sending SMS"));
  sprintf(aux_str, "AT+CMGS=\"%s\"", phone_number);
  answer = sendATcommand(aux_str, ">", 2000);    // send the SMS number
  if (answer == 1) {
    serial_shield.print("Hello! My location is: ");
    serial_shield.print("Latitude: ");
    int i = 0;
    while (latitude[i] != 0) {
      serial_shield.print(latitude[i]);
      i++;
    }
    serial_shield.print(" / Longitude: ");
    i = 0;
    while (longitude[i] != 0) {
      serial_shield.print(longitude[i]);
      i++;
    }
    serial_shield.write(0x1A);
    answer = sendATcommand("", "OK", 20000);
    //if (answer == 1) {
    //  Serial.println("Sent!");
    //}
    //else {
    //  Serial.println(F("Error while sending the SMS!"));
    //}
  }
  
  //else {
  //  Serial.print(F("Error while"));
  //  Serial.println(answer, DEC);
  //}

}

