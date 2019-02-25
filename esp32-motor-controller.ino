/*
 * Required libraries:
 * https://github.com/me-no-dev/AsyncTCP/archive/master.zip
 * https://github.com/me-no-dev/ESPAsyncWebServer/archive/master.zip
  */

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ESPmDNS.h>

const char* ssid = "MSHomeNetwork24";
const char* password = "5q14tbwht0";

const char* startPage = "";
char outputBuffer[512];

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const int led = 13;

double xPosition = 0;
double yPosition = 0;
double zPosition = 0;
double rotationAngle = 0;

void announceCurrentState() {
  putAllInfoToOutput();
  ws.textAll(outputBuffer);
}

void changePosition(double x, double y, double z, double a) {
  xPosition = x;
  yPosition = y;
  zPosition = z;
  rotationAngle = a;
  Serial.printf("Position change: x = %.2f; y = %.2f; z = %.2f, a = %.2f\n", x, y, z, a);
  announceCurrentState();
  // TODO: motors logic
}

void handleNotFound(AsyncWebServerRequest *request) {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  request->send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void putAllInfoToOutput() {
  sprintf(outputBuffer, "{\"type\": \"state\", \"x\": %.2f, \"y\": %.2f, \"z\": %.2f, \"a\": %.2f}", xPosition, yPosition, zPosition, rotationAngle);
}



double parseDouble(char* data, int& offset, bool& valid) {
  offset = 0;
  valid = false;
  //Serial.print("Parsing number: '");
  //Serial.println(data);
  
  bool signFound = false;
  bool dotFound = false;
  bool firstDigitFound = false;
  while(true) {
    char current = data[offset];
    if (!signFound && !dotFound && !firstDigitFound && current == ' ') {
      ++offset;
      continue;
    }
    
    if (!signFound && !firstDigitFound && !dotFound) {
      if (current == '-' || current == '+') {
        signFound = true;
        ++offset;
        continue;
      }
    }
    if (!dotFound) {
      if (current == '.' || current == ',') {
        dotFound = true;
        if (current == ',') {
          data[offset] = '.';
        }
        ++offset;
        continue;
      }
    }
    if (current >= '0' && current <= '9') {
      firstDigitFound = true;
      ++offset;
      continue;
    } else {
      break;
    }
  }

  if (firstDigitFound) {
    valid = true;
    char buf = data[offset];
    data[offset] = 0;
    double result = atof(data);
    data[offset] = buf;
    return result;
  } else {
    valid = false;
    offset = 0;
    return 0;
  }
}

char parseAxisName(char* data, int& offset, bool& valid) {
  offset = 0;
  valid = true;
  while (true) {
    if (data[offset] == ' ') {
      ++offset;
      continue;
    }
    if (data[offset] == 'X' || data[offset] == 'x') {
      ++offset;
      return 'x';
    } else if (data[offset] == 'Y' || data[offset] == 'y')  {
      ++offset;
      return 'y';
    } else if (data[offset] == 'Z' || data[offset] == 'z') {
      ++offset;
      return 'z';
    } else if (data[offset] == 'A' || data[offset] == 'a') {
      ++offset;
      return 'a';
    } else {
      valid = false;
      offset = 0;
      return 0;
    }
  }
}

void processAppend(char* data) {
  int position = 0;
  double deltaX = 0;
  double deltaY = 0;
  double deltaZ = 0;
  double deltaA = 0;
  
  bool changeFound = false;
  while (true) {
    if (data[position] == ' ') {
      ++position;
      continue;
    }

    if (data[position] == 0) {
      break;
    }
    
    int offset = 0;
    bool valid = false;

    char axis = parseAxisName(data + position, offset, valid);

    if (valid) {
      position += offset;
      
      double value = parseDouble(data + position, offset, valid);
      
      if (valid) {
        position += offset;
        switch (axis) {
          case 'x':
            deltaX = value;
            break;
          case 'y':
            deltaY = value;
            break;
          case 'z':
            deltaZ = value;
            break;
          case 'a':
            deltaA = value;
            break;
        }
        changeFound = true;
      } else {
        Serial.printf("Incorrect value at position %i: \"%s\"\n", position, data);
        return;
      }
    } else {
      Serial.printf("Incorrect axis name at position %i: \"%s\"\n", position, data);
      return;
    }
  }

  if (changeFound) {
    changePosition(xPosition + deltaX, yPosition + deltaY, zPosition + deltaZ, rotationAngle + deltaA);
  }
}

void processSet(char* data) {
  int position = 0;
  double deltaX = xPosition;
  double deltaY = yPosition;
  double deltaZ = zPosition;
  double deltaA = rotationAngle;
  
  bool changeFound = false;
  while (true) {
    if (data[position] == ' ') {
      ++position;
      continue;
    }

    if (data[position] == 0) {
      break;
    }
    
    int offset = 0;
    bool valid = false;
    char axis = parseAxisName(data + position, offset, valid);

    if (valid) {
      position += offset;
      
      double value = parseDouble(data + position, offset, valid);
      
      if (valid) {
        position += offset;
        switch (axis) {
          case 'x':
            deltaX = value;
            break;
          case 'y':
            deltaY = value;
            break;
          case 'z':
            deltaZ = value;
            break;
          case 'a':
            deltaA = value;
            break;
        }
        changeFound = true;
      } else {
        Serial.printf("Incorrect value at position %i: \"%s\"\n", position, data);
        return;
      }
    } else {
      Serial.printf("Incorrect axis name at position %i: \"%s\"\n", position, data);
      return;
    }
  }

  if (changeFound) {
    changePosition(deltaX, deltaY, deltaZ, deltaA);
  }
}

void onMessage(AsyncWebSocket * server, AsyncWebSocketClient * client, char* data, size_t len) {
  if (strncmp(data, "getInfo", 7) == 0) {
    //Serial.print("Processing getInfo for ");
    //Serial.println(client->id());
    putAllInfoToOutput();
    client->text(outputBuffer);
  } else if (strncmp(data, "append", 6) == 0) {
    processAppend(data + 6 );
  } else if (strncmp(data, "set", 3) == 0) {
    processSet(data + 3);
  }
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    Serial.print("Websocket client connection received ");
    Serial.println(client->id());
    putAllInfoToOutput();
    client->text(outputBuffer);
  } else if(type == WS_EVT_DISCONNECT){
    Serial.print("Client disconnected ");
    Serial.println(client->id());
  } else if(type == WS_EVT_DATA){
    //data packet
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);
      if(info->opcode == WS_TEXT){
        data[len] = 0;
        Serial.printf("%s\n", (char*)data);
        onMessage(server, client, (char*)data, len);
      } else {
        for(size_t i=0; i < info->len; i++){
          Serial.printf("%02x ", data[i]);
        }
        Serial.printf("\n");
      }
      if(info->opcode == WS_TEXT) {
        
        //client->text("I got your text message");
      } else {
        //client->binary("I got your binary message");
      }
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if(info->index == 0){
        if(info->num == 0) {
          Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
        }
        Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);
      if(info->message_opcode == WS_TEXT){
        data[len] = 0;
        Serial.printf("%s\n", (char*)data);
      } else {
        for(size_t i=0; i < len; i++){
          Serial.printf("%02x ", data[i]);
        }
        Serial.printf("\n");
      }

      if((info->index + len) == info->len){
        Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if(info->final){
          Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          if(info->message_opcode == WS_TEXT) {
            //client->text("I got your text message");
          } else {
            //client->binary("I got your binary message");
          }
        }
      }
    }
  }
}

void setup(void) {
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);

  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  
  //WiFi.softAP(ssid, password);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.print("Connecting to WiFi ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  //Serial.println("Started AP");
  Serial.print("IP address: ");
  //Serial.println(WiFi.softAPIP());
  Serial.println(WiFi.localIP());
  
  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  //server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  //server.handleClient();
}
