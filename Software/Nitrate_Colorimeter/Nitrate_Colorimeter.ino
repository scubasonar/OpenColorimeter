#include <ESP8266WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>

#define wifi_ssid ""
#define wifi_password ""

#define mqtt_server "192.168.1.17"
#define mqtt_user ""
#define mqtt_password ""

//!!!!! channel E responds most to nitrate testing

// ------------- command parser
const byte numChars = 128;
char receivedChars[numChars];
char tempChars[numChars];        // temporary array for use when parsing
bool dataGood = false;

      // variables to hold the parsed data
char messageFromPC[numChars] = {0};
int integerFromPC = 0;
float floatFromPC = 0.0;

const byte SENSOR_COUNT = 18;
unsigned long int lightValues[SENSOR_COUNT];

boolean newData = false;
// -------------- command parser

WiFiClient espClient;
PubSubClient client(espClient);
char message_buff[100];   // used for MQTT subscription
const int AVG_SAMPLES = 20;
const int PIN_LED = 0;
long lastMsg = 0; 


void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  pinMode(PIN_LED, OUTPUT);
  analogWriteFreq(300);
  analogWrite(PIN_LED, 0);
  
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);

  
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// receive color data from UART port 
// The color data is a string of comma separated values. 
void recvColorData() {
    static byte ndx = 0;
    char endMarker = '\n';
    char rc;
    int commaCnt = 0;
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();
        //Serial.print(rc);
          if (rc != endMarker) {
              receivedChars[ndx] = rc;
              ndx++;
              if(rc == ',')
                commaCnt++;
              if (ndx >= numChars) {
                  ndx = numChars - 1;
              }
          }
          else {
              receivedChars[ndx] = '\0'; // terminate the string
               receivedChars[127] = '\0'; // terminate the string
              ndx = 0;
              if(commaCnt == 17) // only send good data...
                newData = true;
              //Serial.print("rx: ");
              //Serial.println(receivedChars);
          }
        }
}

// parse serial commands for color color data  !!!! PROBLEM IS IN HERE
int parseData() {      // split the data into its parts
  char * strStart;
  char * strEnd;
  int i = 0;
  int commaCnt = 0;


 // ------- DATA VALIDATION START
  if(strcmp("ERROR", tempChars) == 0)
  {
    //Serial.println("received error");
    return -1;
  }
  
  Serial.println(tempChars);
    
  for(i = 0; i < SENSOR_COUNT; i++)
    lightValues[i] == 0;

  for(i = 0; i < numChars; i++)
  {   
      if(tempChars[i] == '\0')
        break;
      else
        if(tempChars[i] == ',')
          commaCnt++;
      
      dataGood = false;
      if(commaCnt == SENSOR_COUNT - 1)
      {
        dataGood = true;
      }
  }

  // ------- DATA VALIDATION END
  
  // ------- DATA PARSING 
  strStart = tempChars;
  for(i = 0; i < SENSOR_COUNT; i++)
   {
    lightValues[i] = strtoul(strStart, &strEnd, 0);
    strStart = strEnd;
    if(strStart[0] == ',')
      strStart++;
    if(strStart[0] == ' ')
      strStart++;
   }

    return 0;
}

void loop() {
  unsigned long value = 0;
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  analogWrite(PIN_LED, 100);
  Serial.println("ATINTTIME=250\r\n");
  delay(500);
  Serial.println("ATGAIN=1\r\n");
  delay(5000);
  int i = 0;
  int k = 10;

  // collect and average 5 samples for the color we are intested in
  // in this case it is lightValues[17] which is the one that responds
  // to the nitrate solution
  for(i = 0; i < 5; k= 10)
  {
      client.loop();
      delay(250);
      Serial.print("ATDATA\r\n");
      delay(900);
      recvColorData();
      
      while((newData == false) && k > 0)
      {
        recvColorData();
        k--;
        delay(100);
      }
      if (newData == true) {
       
        newData = false;
        strcpy(tempChars, receivedChars);  // this temporary copy is necessary to protect the original data
                                         //   because strtok() used in parseData() replaces the commas with \0
        if(!parseData())
        {
           value += lightValues[17];
           i++;
           client.publish("debug", String(i).c_str(), true); 
           //publishColors();
        }
        
      }
      
  }

  client.publish("debug", String(i).c_str(), true); 
  value /= 5;
  
  publishColors();

  client.publish("debug", "Cooldown mode", true); 
  analogWrite(PIN_LED, 0);
  for(i=0; i < 20; i++)
  {
    delay(100);
    client.loop();
  }
}

// public eash sensor color to it's own MQTT topic, making subscribing easy
void publishColors()
{
   client.publish("sensor/color_r", String(lightValues[0]).c_str(), true);
   client.publish("sensor/color_s", String(lightValues[1]).c_str(), true);
   client.publish("sensor/color_t", String(lightValues[2]).c_str(), true);
   client.publish("sensor/color_u", String(lightValues[3]).c_str(), true);
   client.publish("sensor/color_v", String(lightValues[4]).c_str(), true);
   client.publish("sensor/color_w", String(lightValues[5]).c_str(), true);
   client.publish("sensor/color_j", String(lightValues[6]).c_str(), true);
   client.publish("sensor/color_i", String(lightValues[7]).c_str(), true);
   client.publish("sensor/color_g", String(lightValues[8]).c_str(), true);
   client.publish("sensor/color_h", String(lightValues[9]).c_str(), true);
   client.publish("sensor/color_k", String(lightValues[10]).c_str(), true);
   client.publish("sensor/color_l", String(lightValues[11]).c_str(), true);
   client.publish("sensor/color_d", String(lightValues[12]).c_str(), true);
   client.publish("sensor/color_c", String(lightValues[13]).c_str(), true);
   client.publish("sensor/color_a", String(lightValues[14]).c_str(), true);
   client.publish("sensor/color_b", String(lightValues[15]).c_str(), true);
   client.publish("sensor/color_e", String(lightValues[16]).c_str(), true);
   client.publish("sensor/color_f", String(lightValues[17]).c_str(), true);
}

void showParsedData() {
  for(int i = 0; i < SENSOR_COUNT; i++)
  {
      Serial.print(lightValues[i]);
      Serial.print(" ");    
  }
  Serial.println("\r\n");

}
