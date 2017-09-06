#include "variant.h"
#include <due_can.h>
#include <SD.h>
// #include <avr/pgmspace.h>

#define VERBOSE 0

#define ATCMD     "AT"
#define ATECMDTRUE  "ATE"
#define ATECMDFALSE "ATE0"
#define ATRCVCAN    "ATRCV"
#define ATSNDCAN    "ATSND"
#define ATGETSENSORDATA    "ATGSDT"
#define ATSNIFFCAN  "ATSNIFF"
#define ATDUMP    "ATDUMP"
#define OKSTR     "OK"
#define ERRORSTR  "ERROR"
#define ATSD      "ATSD"

#define RELAYPIN 44
#define TIMEOUT 5000
#define BAUDRATE 9600

#define CAN_ARRAY_BUFFER_SIZE 100

// SD-related / CONFIG-related
int g_gids[40][2];
int g_num_of_nodes = 40;
char g_mastername[6] = "XXXXX";
char g_timestamp[19];
int g_chip_select = SS3;
int g_turn_on_delay = 10; // in centi seconds ( ie. 100 centiseconds = 1 sec) 
int g_sensor_version = 3;
int g_datalogger_version = 2;


// CAN-related
char g_temp_dump[1000];
String g_string;
String g_string_proc;
int g_sampling_max_retry = 3;
CAN_FRAME g_can_buffer[CAN_ARRAY_BUFFER_SIZE];

bool ate=true;

void setup() {
  Serial.begin(BAUDRATE);
  Serial.println("Receiving AT Command. . .");
  init_can();
  init_strings();
  init_gids();
  init_sd(); 
  open_config();
  print_stored_config();
  pinMode(RELAYPIN, OUTPUT);
}

void loop(){
  getATCommand();
}

void getATCommand(){
  String serial_line, command;
  int i_equals = 0;
  
  do{
    serial_line = Serial.readStringUntil('\r\n');
  } while(serial_line == "");
  serial_line.toUpperCase();
  serial_line.replace("\r","");

  // echo command if ate is set, default true
  if (ate) Serial.println(serial_line);
    i_equals = serial_line.indexOf('=');
  if (i_equals == -1) command = serial_line;
  else command = serial_line.substring(0,i_equals);
  
  if (command == ATCMD)
    Serial.println(OKSTR);
  else if (command == ATECMDTRUE){
    ate = true;
    Serial.println(OKSTR);
  }
  else if (command == ATECMDFALSE){
    ate = false;
    Serial.println(OKSTR);
  }
  else if (command == ATRCVCAN){
    get_all_frames(TIMEOUT);
    Serial.println(OKSTR);
  }
  else if (command == ATSNDCAN){
    send_frame();
    Serial.println(OKSTR);
  }
  else if (command == ATGETSENSORDATA){
    get_data(11,1);
    get_data(12,1);
    Serial.println(OKSTR);
  }
  else if (command == ATSNIFFCAN){
    while (true){
      get_all_frames(TIMEOUT);
      Serial.println(OKSTR);
    }
  }
  else if (command == ATDUMP){
      Serial.println(g_string);
      process_g_string();
      Serial.println(g_string_proc);
      Serial.println(OKSTR);
  }
  else if (command == ATSD){
      String conf;
      init_sd();
      open_config();
      Serial.println(F(OKSTR));
  }
  else{
    Serial.println(ERRORSTR);
  }
}

void getArguments(String at_cmd, String *arguments){
  int i_from = 0, i_to = 0, i_arg = 0;
  bool f_exit = true;
  String sub;

  i_from = at_cmd.indexOf('=');

  do{
    i_to = at_cmd.indexOf(',',i_from+1);
    if (i_to < 0){
      sub = at_cmd.substring(i_from+1);
      f_exit = false;
    }
    else sub = at_cmd.substring(i_from+1,i_to);

    arguments[i_arg] = sub;
    i_from = i_to;
    i_arg += 1;

  } while(f_exit);
}