#include <SoftwareSerial.h>
#include <WiFiEsp.h>
#include <LiquidCrystal.h>
#include <Keypad.h>

/**------------------ WIFI Variables ------------------**/
const char *SSID     = "Twinkle";
const char *PASSWORD = "12345678";
const byte HTTP_PORT = 80;
const byte ESP_RX = 53;
const byte ESP_TX = 52;
const byte MSG_BUFFER = 128;
const int WIFI_CONNECT_DELAY = 2000;

//ESP's RX = 53 & ESP's TX = 52
SoftwareSerial ESPserial(ESP_TX, ESP_RX);
WiFiEspServer server(HTTP_PORT);
WiFiEspClient client;

// use a ring buffer to increase speed and reduce memory allocation
RingBuffer buf(8);
int status = WL_IDLE_STATUS;

unsigned int ack_slave = 0;
unsigned int syn = 0;
unsigned int ack_terminal = 0;
unsigned int syn_terminal = 0;

bool is_handshake_completed = false;
bool is_syn_sent = false;

byte incoming_byte;
char msg[MSG_BUFFER];
unsigned int msg_size = 0;
bool is_msg_buffer_used = false;
unsigned long msg_buffer_timer = 0;
const int MSG_BUFFER_TIMEOUT = 2000;

enum
{
  LAZY,
  ENGLISH,
  SPANISH,
  GUEST_ACK,
  ON_DEMAND
};
byte syn_state = LAZY;

const String H1 = "HTTP/1.1 200 OK\nContent-type:text/html\n\n<!DOCTYPE html><html><head>";
const String H2 = "<link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css\">";
const String H3 = "<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.2.0/jquery.min.js\"></script>";
const String H4 = "<script src=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js\"></script>";
const String H5 = "</head><body style=\"background-color: black; color: #b7a57a;\"><nav class=\"navbar navbar-inverse\"><div class=\"container-fluid\">";
const String H6 = "<div class=\"navbar-header\"><p class=\"navbar-brand\">Team Optimistic</p></div><ul class=\"nav navbar-nav\">";
const String H7 = "<li class=\"active\"><a href=\"/R\">Refresh Data</a></li><li class=\"dropdown\"><a class=\"dropdown-toggle\"";
const String H8 = "data-toggle=\"dropdown\" href=\"#\">Communication<span class=\"caret\"></span></a><ul class=\"dropdown-menu\">";
const String H9 = "<li><a>Master to Slave: ";
const String H10 = "</a></li><li><a>Slave to Master: ";
const String H11 = "</a></li><li><a>Master to Terminal: ";
const String H12 = "</a></li><li><a>Terminal to Master: ";
const String H13 = "</a></li></ul></li></ul></div></nav>";
const String H14 = "<div class=\"container-fluid text-center\">";
const String H15 = "<div class=\"jumbotron jumbotron-fluid\" style=\"background-color:#85754d;color:#fff;\"><h1>Smart Doorbell Dashboard</h1></div>";
const String H16 = "<div class =\"row\"><div class=\"col-sm-4\"><h2 class=\"alert alert-warning\">";
const String H17 = "&#8457;</h2><h2>Temperature</h2></div>";
const String H18 = "<div class=\"col-sm-4\"><h2 class=\"alert alert-warning\">";
const String H19 = "%</h2><h2>Humidity</h2></div>";
const String H20 = "<div class=\"col-sm-4\"><h2 class=\"alert alert-warning\">";
const String H21 = "</h2><h2>Rain</h2></div></div>";
const String H22 = "<div class=\"row\"><div class=\"col-sm-4\"><h2 class=\"alert alert-warning\">";
const String H23 = " ppm</h2><h2>Smoke</h2></div>";
const String H24 = "<div class=\"col-sm-4\"><h2 class=\"alert alert-warning\">";
const String H25 = " mg/m<sup>3</sup></h2><h2>Dust</h2></div>";
const String H26 = "<div class=\"col-sm-4\"><h2 class=\"alert alert-warning\">";
const String H27 = "</h2><h2>Light</h2></div></div>";
const String H28 = "<div class =\"row\"><div class=\"col-sm-4\"><h2 class=\"alert alert-warning\">";
const String H29 = " ppm</h2><h2>CO</h2></div>";
const String H30 = "<div class=\"col-sm-4\"><h2 class=\"alert alert-warning\">";
const String H31 = " ppm</h2><h2>CO2</h2></div><div class=\"col-sm-4\">";
const String H32 = "<h2 class=\"alert alert-warning\">";
const String H33 = " ppm</h2><h2>LPG</h2></div></div><hr style=\"border-top: 1px solid #85754d;\">";
const String H34 = "<p>&copy;2017 Copyright: Pouria &amp; Conard</p><div></body></html>";
/*******************************************************************************/
/*******************************************************************************/

/**------------------ Keypad Variables ------------------**/
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

//connect to the row pinouts of the keypad
byte rowPins[ROWS] = {25, 24, 23, 22};
bool isRequesting = false;

//connect to the column pinouts of the keypad
byte colPins[COLS] = {28, 27, 26};
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS);
/*******************************************************************************/
/*******************************************************************************/

/**------------------ LCD Variables ------------------**/
const int RS = 12, EN = 11, D4 = 5, D5 = 4, D6 = 3, D7 = 2;
const byte LED_COL = 20;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);
enum {
  CO,
  CO2,
  S,
  LPG,
  T,
  H,
  DT,
  LT,
  RN
};
/*******************************************************************************/
/*******************************************************************************/

/**------------------ LED Variables ------------------**/
const byte MOTION_LED = 30;

/**------------------ Buzzer Variables ------------------**/
const int BUZZER = 29;
const int BUZZER_TONE1 = 1200;
const int BUZZER_TONE2 = 1000;
const int BUZZER_DELAY = 250;
/*******************************************************************************/
/*******************************************************************************/

/**------------------ Display variables ------------------**/
String light = "", rain = "", temperature = "", humidity = "", lpg = "", co = "", co2 = "", smoke = "", dust = "";

/*******************************************************************************/
/*******************************************************************************/


void clearLCDRow(byte row)
{
  lcd.setCursor(0, row);
  for (byte i = 0; i < LED_COL; i++)
  {
    lcd.print(" ");
  }
}

void lcdPrint(int field, String data)
{
  switch (field)
  {
    case (CO):
      lcd.setCursor(3, 1);
      lcd.print("   ");
      lcd.setCursor(3, 1);
      lcd.print(data);
      break;
    case (CO2):
      lcd.setCursor(10, 1);
      lcd.print("    ");
      lcd.setCursor(10, 1);
      lcd.print(data);
      break;
    case (S):
      lcd.setCursor(17, 1);
      lcd.print("   ");
      lcd.setCursor(17, 1);
      lcd.print(data);
      break;
    case (LPG):
      lcd.setCursor(4, 2);
      lcd.print("    ");
      lcd.setCursor(4, 2);
      lcd.print(data);
      break;
    case (T):
      lcd.setCursor(11, 2);
      lcd.print("   ");
      lcd.setCursor(11, 2);
      lcd.print(data);
      break;
    case (H):
      lcd.setCursor(17, 2);
      lcd.print("   ");
      lcd.setCursor(17, 2);
      lcd.print(data);
      break;
    case (DT):
      lcd.setCursor(3, 3);
      lcd.print("    ");
      lcd.setCursor(3, 3);
      lcd.print(data);
      break;
    case (LT):
      lcd.setCursor(11, 3);
      lcd.print("  ");
      lcd.setCursor(11, 3);
      lcd.print(data);
      break;
    case (RN):
      lcd.setCursor(17, 3);
      lcd.print("   ");
      lcd.setCursor(17, 3);
      lcd.print(data);
  }
}

void lcdDisplay()
{
  lcdPrint(CO, co);
  lcdPrint(CO2, co2);
  lcdPrint(S, smoke);
  lcdPrint(LPG, lpg);
  lcdPrint(T, temperature);
  lcdPrint(H, humidity);
  lcdPrint(DT, dust);
  lcdPrint(LT, light);
  lcdPrint(RN, rain);
}

void lcdBoot()
{
  // set up the LCD's number of columns and rows:
  lcd.begin(20, 4);
  lcd.setCursor(0, 1);
  lcd.print("CO:");
  lcd.setCursor(7, 1);
  lcd.print("C2:");
  lcd.setCursor(15, 1);
  lcd.print("S:");
  lcd.setCursor(0, 2);
  lcd.print("LPG:");
  lcd.setCursor(9, 2);
  lcd.print("T:");
  lcd.setCursor(15, 2);
  lcd.print("H:");
  lcd.setCursor(0, 3);
  lcd.print("DT:");
  lcd.setCursor(8, 3);
  lcd.print("LT:");
  lcd.setCursor(14, 3);
  lcd.print("RN:");
}

void buzzerBoot()
{
  // Set buzzer - pin 9 as an output
  pinMode(BUZZER, OUTPUT);
}

void ledBoot()
{
  pinMode(MOTION_LED, OUTPUT);
}

void xbeeBoot()
{
  Serial.begin(9600);
  while (!Serial)
  {
    ;
  }
}

void printWifiStatus()
{
  // print the SSID of the network you're attached to
  // Serial.print("SSID: ");
  // Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  clearLCDRow(0);
  lcd.setCursor(0, 0);
  lcd.print("IP: ");
  lcd.print(ip);
}

void espBoot()
{
  status = WL_IDLE_STATUS;
  clearLCDRow(0);
  lcd.setCursor(0, 0);
  lcd.print("Initializing WIFI");
  // Start the software serial for communication with the ESP8266
  ESPserial.begin(9600);

  // initialize ESP module
  WiFi.init(&ESPserial);

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD)
  {
    clearLCDRow(0);
    lcd.setCursor(0, 0);
    lcd.print("No WiFi shield!");
    // don't continue
    // while (true);
  }

  // attempt to connect to WiFi network
  if (status != WL_CONNECTED)
  {
    clearLCDRow(0);
    lcd.setCursor(0, 0);
    lcd.print("Connect: "  );
    //for demo, comment when using home wifi
    //lcd.print("CJ's");
    //for actual home use, uncomment when using home wifi
    lcd.print(SSID);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(SSID, PASSWORD);
    delay(WIFI_CONNECT_DELAY);
  }

  if (status == WL_CONNECTED)
  {
    printWifiStatus();// start the web server on port 80
    server.begin();
    client = NULL;
  }
  else if (status != WL_CONNECTED)
  {
    clearLCDRow(0);
    lcd.setCursor(0, 0);
    lcd.print("WIFI OFF");
  }
}

void sendHttpResponse()
{
  client.println(H1);
  client.println(H2);
  client.println(H3);
  client.println(H4);
  client.println(H5);
  client.println(H6);
  client.print(H7);
  client.println(H8);
  client.print(H9);
  client.print(syn);
  client.println(H10);
  client.print(ack_slave);
  client.print(H11);
  client.print(syn_terminal);
  client.println(H12);
  client.print(ack_terminal);
  client.println(H13);
  client.println(H14);
  client.println(H15);
  client.print(H16);
  client.print(temperature);
  client.println(H17);
  client.println(H18);
  client.print(humidity);
  client.print(H19);
  client.print(H20);
  if (rain == "HVY")
  {
    client.print("HEAVY");
  }
  else
  {
    client.print(rain);
  }
  client.println(H21);
  client.print(H22);
  client.print(smoke);
  client.print(H23);
  client.print(H24);
  client.print(dust);
  client.println(H25);
  client.print(H26);
  if (light == "DK")
  {
    client.print("DARK");
  }
  else if (light == "DM")
  {
    client.print("DIM");
  }
  else if (light == "LT")
  {
    client.print("SEMI BRIGHT");
  }
  else if (light == "BR")
  {
    client.print("BRIGHT");
  }
  else if (light == "VB")
  {
    client.print("VERY BRIGHT");
  }
  client.println(H27);
  client.println(H28);
  client.print(co);
  client.println(H29);
  client.println(H30);
  client.print(co2);
  client.println(H31);
  client.println(H32);
  client.print(lpg);
  client.println(H33);
  client.println(H34);
}

void serviceClient()
{
  // initialize the circular buffer
  buf.init();

  // loop while the client's connected
  while (client.connected())
  {
    // if there's bytes to read from the client,
    if (client.available())
    {
      char c = client.read();               // read a byte, then
      buf.push(c);                          // push it to the ring buffer

      // printing the stream to the serial monitor will slow down
      // the receiving of data from the ESP filling the serial buffer
      //Serial.write(c);

      // you got two newline characters in a row
      // that's the end of the HTTP request, so send a response
      if (buf.endsWith("\r\n\r\n"))
      {
        syn_terminal++;
        ack_terminal++;
        sendHttpResponse();
        break;
      }

      if (buf.endsWith("GET / R"))
      {
        if (!isRequesting)
        {
          isRequesting = true;
          syn_state = ON_DEMAND;
          sendSyn();
        }
      }
    }
  }
}

void listenClient()
{
  if (status == WL_CONNECTED)
  {
    client = server.available();  // listen for incoming clients
    // if you get a client,
    if (client)
    {
      serviceClient();
      // close the connection
      client.stop();
    }
  }
}

void buzz()
{
  tone(BUZZER, BUZZER_TONE1);
  delay(BUZZER_DELAY);
  noTone(BUZZER);
  delay(BUZZER_DELAY);

  tone(BUZZER, BUZZER_TONE2);
  delay(BUZZER_DELAY);
  noTone(BUZZER);
  delay(BUZZER_DELAY);

  tone(BUZZER, BUZZER_TONE1);
  delay(BUZZER_DELAY);
  noTone(BUZZER);
  delay(BUZZER_DELAY);

  tone(BUZZER, BUZZER_TONE2);
  delay(BUZZER_DELAY);
  noTone(BUZZER);
  delay(BUZZER_DELAY);
}

void detectedMotion()
{
  digitalWrite(MOTION_LED, HIGH);
}

void checkKeypad()
{
  char key = keypad.getKey();

  if (key != NO_KEY)
  {
    switch (key)
    {
      case '*':
        digitalWrite(MOTION_LED, LOW);
        break;

      case '1':
        syn_state = ENGLISH;
        sendSyn();
        break;

      case '2':
        syn_state = SPANISH;
        sendSyn();
        break;

      case '3':
        syn_state = GUEST_ACK;
        sendSyn();
        break;

      case '5':
        if (!isRequesting)
        {
          isRequesting = true;
          syn_state = ON_DEMAND;
          sendSyn();
        }
        break;

      case '#':
        espBoot();
        break;

      case '0':
        reboot();
    }
  }
}

void clearMsgBuffer()
{
  is_msg_buffer_used = false;
  memset(msg, 0, sizeof(msg));
  msg_size = 0;
  is_handshake_completed = false;
  is_syn_sent = false;
  syn_state = LAZY;
  isRequesting = false;
}

void checkMsgBuffer()
{
  if (is_msg_buffer_used && (millis() - msg_buffer_timer) >= MSG_BUFFER_TIMEOUT)
  {
    clearMsgBuffer();
  }
}

void mapLight (String lightData)
{
  switch (lightData.charAt(0))
  {
    case '0':
      light = "DK";
      break;
    case '1':
      light = "DM";
      break;
    case '2':
      light = "LT";
      break;
    case '3':
      light = "BR";
      break;
    case '4':
      light = "VB";
  }
}

void mapRain(String rainData)
{
  switch (rainData.charAt(0))
  {
    case '0':
      rain = "HVY";
      break;
    case '1':
      rain = "WET";
      break;
    case '2':
      rain = "DRY";
  }
}

void translate()
{
  char default_data[10];
  byte index = 0;
  memset(default_data, 0, sizeof(default_data));
  for (int i = 0 ; i < msg_size; i++)
  {
    char c = msg[i];
    switch (c)
    {
      case 'Z':
        break;

      case 'L':
        break;

      case 'R':
        mapLight(default_data);
        index = 0;
        memset(default_data, 0, sizeof(default_data));
        break;

      case 'T':
        mapRain(default_data);
        index = 0;
        memset(default_data, 0, sizeof(default_data));
        break;

      case 'H':
        temperature  = default_data;
        index = 0;
        memset(default_data, 0, sizeof(default_data));
        break;

      case 'G':
        humidity = default_data;
        index = 0;
        memset(default_data, 0, sizeof(default_data));
        break;

      case 'C':
        lpg = default_data;
        index = 0;
        memset(default_data, 0, sizeof(default_data));
        break;

      case 'E':
        co = default_data;
        index = 0;
        memset(default_data, 0, sizeof(default_data));
        break;

      case 'N':
        smoke = default_data;
        index = 0;
        memset(default_data, 0, sizeof(default_data));
        break;

      case 'D':
        co2 = default_data;
        index = 0;
        memset(default_data, 0, sizeof(default_data));
        break;

      case 'S':
        dust = default_data;
        index = 0;
        memset(default_data, 0, sizeof(default_data));
        isRequesting = false;
        break;

      default:
        default_data[index] = c;
        index++;
    }
  }
}

void decodeMsg()
{
  switch (msg[0])
  {
    case 'B':
      buzz();
      break;

    case 'M':
      detectedMotion();
      break;

    case 'Z':
      translate();
      lcdDisplay();
  }
  clearMsgBuffer();
}

void sendAck()
{
  Serial.write('K');
}

void sendSyn()
{
  Serial.write('O');
}

void checkMsg()
{
  // see if there's incoming serial data:
  if (syn_state == LAZY && (Serial.available() > 0))
  {
    incoming_byte = Serial.read();
    if (!is_handshake_completed)
    {
      sendAck();
      ack_slave++;
      is_handshake_completed = true;
      msg_buffer_timer = millis();
      is_msg_buffer_used = true;
    }
    else
    {
      char c = (char)incoming_byte;
      if (c == 'S')
      {
        msg[msg_size] = c;
        msg_size++;
        decodeMsg();
      }
      else
      {
        msg[msg_size] = c;
        msg_size++;
      }
    }
  }
}

void sendMsg()
{
  if (syn_state != LAZY && (Serial.available() > 0))
  {
    incoming_byte = Serial.read();

    if (((char)incoming_byte) == 'K')
    {

      switch (syn_state)
      {
        case ENGLISH:

          Serial.write('E');
          Serial.write('S');
          break;

        case SPANISH:
          Serial.write('P');
          Serial.write('S');
          break;

        case GUEST_ACK:
          Serial.write('A');
          Serial.write('S');
          break;

        case ON_DEMAND:
          Serial.write('D');
          Serial.write('S');
      }
      syn++;
      is_syn_sent = false;
      syn_state = LAZY;
    }
  }
}

void reboot()
{
  lcdBoot();
  ledBoot();
  buzzerBoot();
  xbeeBoot();
  espBoot();

}

void setup()
{
  reboot();
}

void loop()
{
  checkMsg();
  //clear message buffer to prevent collision and weird behavior
  checkMsgBuffer();
  checkKeypad();
  listenClient();
  sendMsg();
}
