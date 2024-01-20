#include <WiFi.h>
#include <NTPClient.h>
#include <FirebaseESP32.h>
#include <stdio.h>
#include <string.h>

/* Update time 7:16 Pm - 07/06/2022 */

// Replace with your network credentials
//const char* ssid     = "BKer_1_5Ghz";
//const char* password = "huynhbs25251325";
//const char* ssid     = "BKer_1";
//const char* password = "huynhbs25251325";
//const char* ssid = "6BeVanDan_2.4G";
//const char* password = "99999999";
// const char* ssid     = "Viva Star Wifi Free";
// const char* password = "68686868";5
//const char* ssid     = "HUNG_VNPT";
//const char* password = "huynhhan";

// const char* ssid     = "BKer_1";
const char* ssid     = "BKer_1_EXT";
const char* password = "HuynhTop176C142129";


/* Definition */
#define FIREBASE_HOST "https://wifi-controller-94a3b.firebaseio.com/"
#define FIREBASE_AUTH "ttVcbl0zUQn8Hsz4clnrf0C15kIfP3KUZG2M6rTm"
#define PUMP_TURN_OFF 0x00
#define PUMP_TURN_ON 0x01
#define PUMP_INIT 0xFE
#define DISABLE 0x00
#define ENABLE 0x01
#define DEVELOPMENT_PHASE ENABLE

// /* Firebase global variables - OLD */
// FirebaseData firebaseData1;
// FirebaseData firebaseData2;
// FirebaseData firebase_Calendar;
// FirebaseData firebase_Pump_Status;
// FirebaseData firebase_Cmd_Pump_Status;
// String PumpSys_Path = "/Pump_Status";
// String PumpStatus_Path = "/Status";
// String Cmd_PumpSys_Path = "/Pump_Cmd_Sts//Cmd_PumpSys";
// String Cmd_PumpSys = "/Cmd_PumpSys";
// String path = "/Nodes";
// String path_Calendar = "/Calendar";
// String path_Days = "Days";
// String path_Times = "Times";
// String note_Day = "Day";
// String note_Month = "Month";
// String note_Year = "Year";
// String note_Hour = "Hour";
// String note_Min = "Min";
// String note_Second = "Second"; 
// String nodeID = "Node2";
// String otherNodeID = "Node1"; 

/* Firebase global variables - NEW */
/* Pump_System Root Path */
FirebaseData firebase_Automation_Pump_System;
String Path_Pump_System = "/Auto_Pump_System";
/* Calendar sub Branch */
String Path_Calendar = "Calendar";
String Path_Days = "Days";
String Path_Times = "Times";
String note_Day = "Day";
String note_Hour = "Hour";
/* PumpStatus sub Branch */
String Path_PumpStatus = "Pump_Status";
String Note_PumpStatus = "Status";
/* Cmd_PumpSys sub Branch */
String Path_Cmd_PumpSys = "Cmd_PumpSys";
String Note_Cmd_PumpSys_Sts = "Cmd_PumpSys_Sts";
/* Another Note */
String nodeID = "Node2";
String otherNodeID = "Node1";

/* Global Variable for Pump */
uint8_t Gbul_WaterPump_Sts;
String WaterPump_Sts = "OFF";
String Cmd_WaterPump_Sts = "INITSTS";
String RawCmd_WaterPump_Sts = "";

enum Water_Pump_Status  {
  OFF,
  ON
};

const int ledPin =  19; //GPIO19 for LED
const int swPin =  18; //GPIO18 for Switch
boolean swState = true;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

/* Timer Variable */
volatile uint32_t interruptCounter_1ms = 0x00;  //for counting interrupt
volatile uint32_t interruptCounter500ms_01 = 0x00;  //for counting interrupt
volatile uint32_t interruptCounter2000ms_01 = 0x012C;  //for counting interrupt
volatile uint32_t interruptCounter_5000ms_01 = 0x00;  //for counting interrupt
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

/* Timer Funtion */
void IRAM_ATTR onTimer() {      //Defining Inerrupt function with IRAM_ATTR for faster access
 portENTER_CRITICAL_ISR(&timerMux);
 interruptCounter_1ms++;
 interruptCounter500ms_01++;
 interruptCounter2000ms_01++;
 interruptCounter_5000ms_01++;
 portEXIT_CRITICAL_ISR(&timerMux);
}

//===================================
void streamCallback(StreamData data)
{

  if (data.dataType() == "boolean") {
    if (data.boolData()){
    #if DEVELOPMENT_PHASE == ENABLE
      Serial.println("*streamCallback* Set " + nodeID + " to High");
      #endif
    }
    else {
    #if DEVELOPMENT_PHASE == ENABLE
      Serial.println("*streamCallback* Set " + nodeID + " to Low");
      #endif
    }

    digitalWrite(ledPin, data.boolData());
  }
}
//===================================
void streamTimeoutCallback(bool timeout)
{
  if (timeout)
  {
    #if DEVELOPMENT_PHASE == ENABLE
    Serial.println();
    Serial.println("Stream timeout, resume streaming...");
    Serial.println();
    #endif
  }

  #if DEVELOPMENT_PHASE == ENABLE
  if (timeout)
    Serial.println("stream timed out, resuming...\n");
  /* Check firebase connection of firebase_Calendar */
  if (!firebase_Automation_Pump_System.httpConnected())
    Serial.printf("firebase_Automation_Pump_System error code: %d, reason: %s\n\n", firebase_Automation_Pump_System.httpCode(), firebase_Automation_Pump_System.errorReason().c_str());  

  // /* Check firebase connection of firebase_Pump_Status */
  // if (!firebase_Pump_Status.httpConnected())
  //   Serial.printf("firebase_Pump_Status error code: %d, reason: %s\n\n", firebase_Pump_Status.httpCode(), firebase_Pump_Status.errorReason().c_str());    

  // /* Check firebase connection of firebase_Cmd_Pump_Status */
  // if (!firebase_Cmd_Pump_Status.httpConnected())
  //   Serial.printf("firebase_Cmd_Pump_Status error code: %d, reason: %s\n\n", firebase_Cmd_Pump_Status.httpCode(), firebase_Cmd_Pump_Status.errorReason().c_str());    
  #endif   
    
}
//===================================
void setup() {

  /* Initialize Serial Monitor */
  Serial.begin(115200);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

/* Init global pump variable */
Gbul_WaterPump_Sts = PUMP_INIT;

/* Initial Port */
 pinMode(13, OUTPUT);  
 digitalWrite(13, HIGH); //turn OFF
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  /* Print local IP address and start web server */
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); //streamCallback

  /* Init firebase_Calendar */
  Firebase.begin(FIREBASE_HOST,FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  if ((!Firebase.beginStream(firebase_Automation_Pump_System, Path_Pump_System + "/" + Path_Calendar + "/" + Path_Days + "/" + note_Day))
    & (!Firebase.beginStream(firebase_Automation_Pump_System, Path_Pump_System + "/" + Path_Calendar + "/" + Path_Times + "/" + note_Hour))
    & (!Firebase.beginStream(firebase_Automation_Pump_System, Path_Pump_System + "/" + Path_PumpStatus + "/" + Note_PumpStatus))
    & (!Firebase.beginStream(firebase_Automation_Pump_System, Path_Pump_System + "/" + Path_Cmd_PumpSys + "/" + Note_Cmd_PumpSys_Sts)))
  {
    #if DEVELOPMENT_PHASE == ENABLE
    Serial.println("Could not begin stream");
    Serial.println("REASON: " + firebase_Automation_Pump_System.errorReason());
    Serial.println();
    #endif
  }
  else
  {
    #if DEVELOPMENT_PHASE == ENABLE
    Serial.println("start connect to firebase_Automation_Pump_System");
    #endif
  }
  Firebase.setStreamCallback(firebase_Automation_Pump_System, streamCallback, streamTimeoutCallback);

//   /* Init firebase_Pump_Status */
//   if (!Firebase.beginStream(firebase_Pump_Status, PumpSys_Path + "/" + PumpStatus_Path ))
//   {
//     #if DEVELOPMENT_PHASE == ENABLE
//     Serial.println("Could not begin stream");
//     Serial.println("REASON: " + firebase_Pump_Status.errorReason());
//     Serial.println();
//     #endif
//   }
//   else
//   {
//     #if DEVELOPMENT_PHASE == ENABLE
//     Serial.println("start connect to firebase_Pump_Status");
//     #endif
//     }
//   Firebase.setStreamCallback(firebase_Pump_Status, streamCallback, streamTimeoutCallback);  

// /* Init firebase_Cmd_Pump_Status */
//   if (!Firebase.beginStream(firebase_Cmd_Pump_Status, Cmd_PumpSys_Path + "/" + Cmd_PumpSys ))
//   {
//     #if DEVELOPMENT_PHASE == ENABLE    
//     Serial.println("Could not begin stream");
//     Serial.println("REASON: " + firebase_Cmd_Pump_Status.errorReason());
//     Serial.println();
//     #endif
//   }
//   else
//   {
//     #if DEVELOPMENT_PHASE == ENABLE
//     Serial.println("start connect to firebase_Cmd_Pump_Status");
//     #endif
//   }
//   Firebase.setStreamCallback(firebase_Cmd_Pump_Status, streamCallback, streamTimeoutCallback);    

  /* Initialize a NTPClient to get time */
  timeClient.begin();
  timeClient.setTimeOffset(+7*60*60);

/* Init Timer for create TASK */
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000, true);  		// Match value= 1000000 for 1 sec. delay.
  timerAlarmEnable(timer);           			// Enable Timer with interrupt (Alarm Enable)
}
void loop() {
  uint8_t led_Cnt = 0x00;
  uint16_t led_Cnt2 = 0x00;

/* Trigger TASK_500ms_01 */
if (0x01 <= (interruptCounter500ms_01 /500))
{
  TASK_500ms_01();  
  interruptCounter500ms_01 = 0x00;
}

/* Trigger TASK_2000ms_01 */
if (0x01 <= (interruptCounter2000ms_01 /3000))
{
  TASK_2000ms_01();  
  interruptCounter2000ms_01 = 0x00;
}

/* Trigger TASK_5000ms_01 */
if (0x01 <= (interruptCounter_5000ms_01 /5000))
{
 
  TASK_5000ms_01();  
  interruptCounter_5000ms_01 = 0x00;  
}

/* time loop interupt */
  if (interruptCounter_1ms > 1000) {
 
  //  portENTER_CRITICAL(&timerMux);
   interruptCounter_1ms = 0x00;
  //  portEXIT_CRITICAL(&timerMux);
 
  //  totalinterruptCounter_1s++;         	//counting total interrupt

  //  Serial.print(" Total interrupt number: ");
  //  Serial.println(totalinterruptCounter_1s);
 }

  // while(!timeClient.update()) {
  //   timeClient.forceUpdate();
  // }
//===================================

}

/* TASK_500ms_01 : Update cmd from App to server and control Replay */
void TASK_500ms_01(void) {

if (Firebase.getString(firebase_Automation_Pump_System, Path_Pump_System + "//" + Path_Cmd_PumpSys + "//" + Note_Cmd_PumpSys_Sts + "/" ))
{
      RawCmd_WaterPump_Sts = firebase_Automation_Pump_System.stringData();
      #if DEVELOPMENT_PHASE == ENABLE
      Serial.print("CMD_PUMP_STATUS: ");
      Serial.println(RawCmd_WaterPump_Sts);
      #endif
      if(0x31 == RawCmd_WaterPump_Sts[0])
      {
        digitalWrite(13, LOW);//turn ON
        Gbul_WaterPump_Sts = PUMP_TURN_ON;
      }
      else 
      {
        digitalWrite(13, HIGH); //turn OFF
        Gbul_WaterPump_Sts = PUMP_TURN_OFF;
      }
    }  
}

/* TASK_2000ms_01 : Update Water Pump Status to Server */
void TASK_2000ms_01(void) {
  // Serial.println("ON TASK_2000ms_01 ");
  if ( PUMP_TURN_ON == Gbul_WaterPump_Sts )
  {
     Serial.println("ON TASK_2000ms_01 SET ON ");
    /* Put ON status to firebase: firebase_Pump_Status */
    if (Firebase.setString(firebase_Automation_Pump_System, Path_Pump_System + "/" + Path_PumpStatus + "/" + Note_PumpStatus , " ON "))
    {
      #if DEVELOPMENT_PHASE == ENABLE  
      Serial.print("firebase_Pump_Status: ");
      Serial.println( "ON");
      #endif
    }
    else {
      /* Do Nothing */
    } 
  }
  else if ( PUMP_TURN_OFF == Gbul_WaterPump_Sts ){
    /* Put OFF status to firebase: firebase_Pump_Status */
    if (Firebase.setString(firebase_Automation_Pump_System, Path_Pump_System + "/" + Path_PumpStatus + "/" + Note_PumpStatus , "OFF "))
    {
      #if DEVELOPMENT_PHASE == ENABLE  
      Serial.print("firebase_Pump_Status: ");
      Serial.println( "OFF");
      #endif
    }
    else {
      /* Do Nothing */
    }
  }
  else if ( PUMP_INIT == Gbul_WaterPump_Sts ) {
    /* Put INIT status to firebase: firebase_Pump_Status */
    if (Firebase.setString(firebase_Automation_Pump_System, Path_Pump_System + "/" + Path_PumpStatus + "/" + Note_PumpStatus , "INIT"))
    {
      #if DEVELOPMENT_PHASE == ENABLE  
      Serial.print("firebase_Pump_Status: ");
      Serial.println( "INIT");
      #endif
    }
    else {
      /* Do Nothing */
    }    
  }

}

/* TASK_5000ms_01: Update RealTime data each 5000ms */
void TASK_5000ms_01 (void)
{
  timeClient.forceUpdate();
  if (timeClient.update())
  {

  #if DEVELOPMENT_PHASE == ENABLE    
  Serial.println("START UPDATE TIME/DATE ! ");
  #endif
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = timeClient.getFormattedDate();

  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  // Extract time
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);

//===================================
  if (Firebase.setString(firebase_Automation_Pump_System, Path_Pump_System + "/" + Path_Calendar + "/" + Path_Days + "/" + note_Day, dayStamp))
  {
    #if DEVELOPMENT_PHASE == ENABLE  
    Serial.print("DATE: ");
    Serial.println(dayStamp);
    #endif
  }

  if (Firebase.setString(firebase_Automation_Pump_System, Path_Pump_System + "/" + Path_Calendar + "/" + Path_Times + "/" + note_Hour, timeStamp))
  {
    #if DEVELOPMENT_PHASE == ENABLE  
    Serial.print("HOUR: ");
    Serial.println(timeStamp);
    #endif
  }
  }  
}
