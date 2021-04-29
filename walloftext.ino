#include <WiFi.h>
#include <ArduinoHttpClient.h>
#include "utils.h"
#include "credentials.h"
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#define PANEL_WIDTH 64
#define PANEL_HEIGHT 32
#define PANELS_NUMBER 1
#define PIN_E 32

#define PANE_WIDTH PANEL_WIDTH * PANELS_NUMBER
#define PANE_HEIGHT PANEL_HEIGHT

MatrixPanel_I2S_DMA *dma_display = nullptr;

#define FONT_SIZE 1 // FONT_SIZE x 8px

#define EMOJI_SIZE 30 //  expected resolution of the bmp files
#define EMOJI_LIMIT 10 // maximum amount of emojis

char serverAddress[] = "192.168.1.20";
int serverPort = 8000;
WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, serverPort);
int status = WL_IDLE_STATUS;

uint16_t color_black = dma_display->color565(0, 0, 0);
uint16_t color_blue = dma_display->color565(0, 0, 255);

void setup() {
  Serial.begin(115200);

  HUB75_I2S_CFG mxconfig;
  mxconfig.double_buff = true;
  mxconfig.mx_height = PANEL_HEIGHT;
  mxconfig.chain_length = PANELS_NUMBER;
  mxconfig.gpio.e = PIN_E;
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);

  Serial.println("Setup display");
  dma_display->begin();
  dma_display->fillScreen(color_black);
  dma_display->setTextSize(FONT_SIZE);
  dma_display->setTextWrap(true);
  dma_display->setTextColor(color_blue);
  dma_display->setCursor(1, 1);
  dma_display->setBrightness8(64);
  dma_display->print("On.");

  Serial.print("Connecting with Wifi: ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println("connected");

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

struct emoji {
  uint32_t unicode;
  unsigned int position;
  uint16_t pix[EMOJI_SIZE*EMOJI_SIZE];
};

emoji emojis[EMOJI_LIMIT];
unsigned short e = 0;

void loop() {
  dma_display->flipDMABuffer();
  dma_display->fillScreen(color_black);
  // Get the display content
  String text = fetch_text("content");

  char arr[1024];
  text.toCharArray(arr, 1024);

  unsigned int p = 0;
  for (int i = 0; i < 1024; i++, p++) {
    if (arr[i] == 0) {
      break;
    }
    if (arr[i] & 128) {
      bool is_emoji = 0;
      uint8_t len = 0;
      uint8_t mask = arr[i];
      while (mask & 128) {
        len++;
        mask = mask << 1;
      }
      if (len == 4) {
        is_emoji = 1;
      }

      char utf8char[4] = {0, 0, 0, 0};
      for (int c = 0; c < len; c++) {
        utf8char[c] = arr[i+c];
      }

      i += len-1;

      // TODO non-printable characters should probably discarded from the string

      if (is_emoji) {
        uint32_t unicode = utf8_to_unicode(utf8char);
        if (e < EMOJI_LIMIT) {
          emojis[e] = { unicode, p, 0 };
          e++;
        }
      }
    }
  }

  // Actually load the files
  for (int i = 0; i < e; i++) {
    get_emoji(&emojis[i]);
  }


  uint8_t brightness = fetch_brightness("brightness");
  dma_display->fillScreen(color_black);
  dma_display->setBrightness8(brightness);
  dma_display->setCursor(1, 1);
  //dma_display->println(arr);
  for (int i = 0; i < e; i++) {
    draw_emoji(1+i*32, 0, &emojis[i]);
  }
  dma_display->showDMABuffer();

  e = 0;
  delay(1000);
}

uint32_t utf8_to_unicode(char utf8char[4]) {
  uint32_t codepoint = 0;
  uint8_t len = 0;
  char mask = utf8char[0];
  do {
    len++;
    mask = mask << 1;
  } while (mask & 128);

  // first byte
  switch (len) {
    case 1:
      codepoint = utf8char[0] & 0b01111111;
      break;
    case 2:
      codepoint = (codepoint ^ (utf8char[0] & 0b00011111)) << 6;
      codepoint = (codepoint ^ (utf8char[1] & 0b00111111));
      break;
    case 3:
      codepoint = (codepoint ^ (utf8char[0] & 0b00001111)) << 6;
      codepoint = (codepoint ^ (utf8char[1] & 0b00111111)) << 6;
      codepoint = (codepoint ^ (utf8char[2] & 0b00111111));
      break;
    case 4:
      codepoint = (codepoint ^ (utf8char[0] & 0b00000111)) << 6;
      codepoint = (codepoint ^ (utf8char[1] & 0b00111111)) << 6;
      codepoint = (codepoint ^ (utf8char[2] & 0b00111111)) << 6;
      codepoint = (codepoint ^ (utf8char[3] & 0b00111111));
      break;
  }
  SerialPrintfln("utf8_to_unicode: UTF-8 %02x %02x %02x %02x should be U+%x", utf8char[0], utf8char[1], utf8char[2], utf8char[3], codepoint);
  return codepoint;
}

String fetch_text(String file) {
  client.get(file);
  int statusCode = client.responseStatusCode();
  String response = "";
  if (statusCode == 200) {
    response = client.responseBody();
    response.trim();
  }
  client.stop();
  SerialPrintfln("fetch_text: GET %s - %d - \"%s\"", file, statusCode, response);
  return response;
}

uint8_t fetch_brightness(String file) {
  client.get(file);
  int statusCode = client.responseStatusCode();
  String response = "64";
  if (statusCode == 200) {
    response = client.responseBody();
    response.trim();
  }
  client.stop();
  SerialPrintfln("fetch_brightness: GET %s - %d - \"%s\"", file, statusCode, response);
  return response.toInt();
}

void get_emoji(struct emoji *e) {
  char file[128];
  sprintf(file, "emojis/%x.bmp", e->unicode);
  client.get(file);
  int statusCode = client.responseStatusCode();
  SerialPrintfln("get_emoji: GET %s - %d", file, statusCode);

  if (statusCode == 200) {
    String response = client.responseBody();
    const char *str = response.c_str();
    str = str+66;
    for (int i = 0; i < EMOJI_SIZE*EMOJI_SIZE; i++) {
      int x = i % EMOJI_SIZE;
      int y = 31 - (i / EMOJI_SIZE);
      e->pix[i] = str[i*2+1];
      e->pix[i] = e->pix[i] << 8;
      e->pix[i] += str[i*2];
    }
  } else {
    for (int i = 0; i < EMOJI_SIZE*EMOJI_SIZE; i++) {
      int x = i % EMOJI_SIZE;
      int y = i / EMOJI_SIZE;
      e->pix[i] = 0;
    }
  }
  client.stop();
}

void draw_emoji(int xOff, int yOff, emoji *e) {
  SerialPrintfln("draw_emoji at text position: %d", e->position);
  for (int i = 0; i < EMOJI_SIZE*EMOJI_SIZE; i++) {
    int x = i % EMOJI_SIZE;
    int y = EMOJI_SIZE-(i/EMOJI_SIZE);
    dma_display->drawPixel(x + xOff, y + yOff, e->pix[i]);
  }
}
