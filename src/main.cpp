#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>
#include <AnimatedGIF.h>
#include <XPT2046_Touchscreen.h>

#define SD_CS 5
#define TOUCH_CS 33
#define TOUCH_IRQ 36

TFT_eSPI tft = TFT_eSPI();
AnimatedGIF gif;
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

File gifFile;

int currentGif = 0;
int gifCount = 0;
String gifList[50];

// ================== GIF CALLBACK ==================
void *GIFOpenFile(const char *fname, int32_t *pSize) {
  gifFile = SD.open(fname);
  if (!gifFile) return NULL;
  *pSize = gifFile.size();
  return (void *)&gifFile;
}

void GIFCloseFile(void *pHandle) {
  File *f = static_cast<File *>(pHandle);
  if (f) f->close();
}

int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
  File *f = static_cast<File *>(pFile->fHandle);
  return f->read(pBuf, iLen);
}

int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition) {
  File *f = static_cast<File *>(pFile->fHandle);
  f->seek(iPosition);
  return iPosition;
}

void GIFDraw(GIFDRAW *pDraw) {
  if (pDraw->y >= tft.height()) return;

  uint16_t *lineBuffer = (uint16_t *)malloc(pDraw->iWidth * 2);
  if (!lineBuffer) return;

  for (int i = 0; i < pDraw->iWidth; i++) {
    int idx = pDraw->pPixels[i];
    uint8_t r = pDraw->pPalette[idx * 3];
    uint8_t g = pDraw->pPalette[idx * 3 + 1];
    uint8_t b = pDraw->pPalette[idx * 3 + 2];
    lineBuffer[i] = tft.color565(r, g, b);
  }

  tft.pushImage(pDraw->x, pDraw->y, pDraw->iWidth, 1, lineBuffer);
  free(lineBuffer);
}

// ================== GIF LIST ==================
void loadGifList() {
  File root = SD.open("/gifs");
  while (true) {
    File f = root.openNextFile();
    if (!f) break;
    String name = f.name();
    name.toLowerCase();
    if (name.endsWith(".gif")) {
      gifList[gifCount++] = "/gifs/" + name;
    }
    f.close();
  }
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  ts.begin();
  ts.setRotation(1);

  if (!SD.begin(SD_CS)) {
    tft.drawString("SD ERROR", 50, 120, 4);
    while (1);
  }

  gif.begin(LITTLE_ENDIAN_PIXELS);
  loadGifList();
}

// ================== LOOP ==================
void loop() {
  if (gifCount == 0) return;

  gif.open(gifList[currentGif].c_str(), GIFOpenFile, GIFCloseFile,
           GIFReadFile, GIFSeekFile, GIFDraw);

  while (gif.playFrame(true)) {
    if (ts.touched()) {
      TS_Point p = ts.getPoint();
      if (p.x < 160) {
        currentGif = (currentGif - 1 + gifCount) % gifCount;
      } else {
        currentGif = (currentGif + 1) % gifCount;
      }
      gif.close();
      delay(300);
      return;
    }
  }

  gif.close();
}
