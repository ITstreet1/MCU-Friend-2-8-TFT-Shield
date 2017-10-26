
#include <Adafruit_GFX.h> //biblioteka za grafićki prikaz
#include <MCUFRIEND_kbv.h> //biblioteka zadužena za komunikaciju sa ekranom
#include <TouchScreen.h> //biblioteka za komunikaciju sa digitajzerom
#include <SD.h> //biblioteka za komunikaciju sa sd slotom
MCUFRIEND_kbv tft;

#define LCD_CS A3 // Chip Select ide na  Analog 3
#define LCD_CD A2 // Command/Data ide na Analog 2
#define LCD_WR A1 // LCD Write ide na Analog 1
#define LCD_RD A0 // LCD Read ide na Analog 0
#define LCD_RESET A4 // Može i na Arduino reset pin

#define SD_CS 10  // dodeljivanje pina sd slotu
#define NAMEMATCH ""        // "" bilo koje ime
#define PALETTEDEPTH   0     // bez podrske za Palette mod
char namebuf[32] = "/";   //BMP fajlovi u root-u
File root;
int pathlen;

uint8_t YP = A1;  // mora biti analogni pin
uint8_t XM = A2;  // mora biti analogni pin
uint8_t YM = 7;   // može biti digitalni pin
uint8_t XP = 6;   // može biti digitalni pin

//vrednosti unete nakon kalibracije ekrana - ovde dodajemo vrednosti dobijene kalibracijom
uint16_t TS_LEFT = 182; 
uint16_t TS_RT  = 908;
uint16_t TS_TOP = 932;
uint16_t TS_BOT = 193;

uint16_t xpos, ypos, zpos;  // koordinate ekrana

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300); // koordinate ekrana, 300 je vrednost otpornosti samog ekrana
TSPoint tp;

//dodajemo nazive bojama radi lakšeg snalaženja
#define BLACK   0x0000
#define GREY    0x8410
#define ORANGE  0xFA60
#define LIME    0X07FF
#define BLUE    0x001F
#define RED     0xF800
#define AQUA    0x04FF
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF



boolean buttonEnabled = true;

void setup() {
  Serial.begin(9600); //pokrećemo serial monitor radi lakšeg praćenja eventualnih grešaka
  SD.begin(SD_CS); //pokrećemo sd slot
  root = SD.open(namebuf); //otvaramo sd slot
  pathlen = strlen(namebuf);

  //pokrećemo ekran, postavljamo ga u landscape mod i bojimo u belo
  tft.reset();
  tft.begin(0x4747); //ID ekrana (HX8347-D)
  tft.setRotation(1); //landscape, a rotaciju biramo između 1, 2, 3 i 4
  tft.fillScreen(0xF9A0);

  //prvo polje, sve funkcije se mogu naći u linku ka gethub-u u okviru tekst u Svetu Kompjutera, ili na zvaničnoj Adafruit stranici
  tft.fillRoundRect(30,10,260,105,25,0x033F);
  tft.drawRoundRect(30,10,260,105,25,BLACK);
  tft.setCursor(45,18);
  tft.setTextColor(BLACK);
  tft.setTextSize(3);
  tft.print("Naslovnica");
  tft.setCursor(45,45);
  tft.setTextSize(3);
  tft.print("Sveta");
  tft.setCursor(45,75);
  tft.setTextSize(3);
  tft.print("Kompjutera");

  //drugo polje
  tft.fillRect(30,140,55,74,0x1480);
  tft.drawRect(30,140,55,74,BLACK);
  tft.setCursor(32,142);
  tft.setTextColor(BLACK);
  tft.setTextSize(10);
  tft.print("X");
  tft.setCursor(120,140);
  tft.setTextColor(BLACK);
  tft.setTextSize(3);
  tft.print("X marks");
  tft.setCursor(120,180);
  tft.setTextColor(BLACK);
  tft.setTextSize(3);
  tft.print("the spot!");
}
void loop() {
  TSPoint p = ts.getPoint(); //čekamo dodir na ekranu i pretvaramo u promenljivu
  if (p.z >ts.pressureThreshhold){
    zpos = p.z;
    //mapiranje ekrana po osama uz podatke dobijene kalibracijom
    xpos = map(p.y, TS_LEFT, TS_RT, 0, tft.width());
    ypos = map(p.x, TS_TOP, TS_BOT, 0, tft.height());
    //pratimo vrednosti po osama
    Serial.print("X= ");
    Serial.println(xpos);
    Serial.print("Y= ");
    Serial.println(ypos);
    Serial.print("Z= ");
    Serial.println(zpos);
    delay(1000);
    //ako dodir postoji
    if(xpos>30&&xpos<85 && ypos>140&&ypos<214 && buttonEnabled){
      buttonEnabled = false;
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);
      pinMode(XP, OUTPUT);
      pinMode(YM, OUTPUT);
      tft.setRotation(0); //okrećemo ekran
      showBMP("image.bmp",0,0);//iscrtavamo sliku sa kartice
      Serial.println("Naslovnica iscrtana uspešno");
      delay(1000);
      }
      delay(10);
    }
}
//ceo kod ispod je neophodan za pravilan rad ekrana i njega ostavljamo kako jeste. Sve u vezi njega je na arduino forumu a ovde bismo samo izgubili vreme
#define BMPIMAGEOFFSET 54
#define BUFFPIXEL      20

uint16_t read16(File& f) {
    uint16_t result;         // read little-endian
    f.read(&result, sizeof(result));
    return result;
}

uint32_t read32(File& f) {
    uint32_t result;
    f.read(&result, sizeof(result));
    return result;
}

uint8_t showBMP(char *nm, int x, int y)
{
    File bmpFile;
    int bmpWidth, bmpHeight;    // W+H in pixels
    uint8_t bmpDepth;           // Bit depth (currently must be 24, 16, 8, 4, 1)
    uint32_t bmpImageoffset;    // Start of image data in file
    uint32_t rowSize;           // Not always = bmpWidth; may have padding
    uint8_t sdbuffer[3 * BUFFPIXEL];    // pixel in buffer (R+G+B per pixel)
    uint16_t lcdbuffer[(1 << PALETTEDEPTH) + BUFFPIXEL], *palette = NULL;
    uint8_t bitmask, bitshift;
    boolean flip = true;        // BMP is stored bottom-to-top
    int w, h, row, col, lcdbufsiz = (1 << PALETTEDEPTH) + BUFFPIXEL, buffidx;
    uint32_t pos;               // seek position
    boolean is565 = false;      //

    uint16_t bmpID;
    uint16_t n;                 // blocks read
    uint8_t ret;

    if ((x >= tft.width()) || (y >= tft.height()))
        return 1;               // off screen

    bmpFile = SD.open(nm);      // Parse BMP header
    bmpID = read16(bmpFile);    // BMP signature
    (void) read32(bmpFile);     // Read & ignore file size
    (void) read32(bmpFile);     // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile);       // Start of image data
    (void) read32(bmpFile);     // Read & ignore DIB header size
    bmpWidth = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    n = read16(bmpFile);        // # planes -- must be '1'
    bmpDepth = read16(bmpFile); // bits per pixel
    pos = read32(bmpFile);      // format
    if (bmpID != 0x4D42) ret = 2; // bad ID
    else if (n != 1) ret = 3;   // too many planes
    else if (pos != 0 && pos != 3) ret = 4; // format: 0 = uncompressed, 3 = 565
    else if (bmpDepth < 16 && bmpDepth > PALETTEDEPTH) ret = 5; // palette 
    else {
        bool first = true;
        is565 = (pos == 3);               // ?already in 16-bit format
        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * bmpDepth / 8 + 3) & ~3;
        if (bmpHeight < 0) {              // If negative, image is in top-down order.
            bmpHeight = -bmpHeight;
            flip = false;
        }
        w = bmpWidth;
        h = bmpHeight;
        if ((x + w) >= tft.width())       // Crop area to be loaded
            w = tft.width() - x;
        if ((y + h) >= tft.height())      //
            h = tft.height() - y;
        // Set TFT address window to clipped image bounds
        tft.setAddrWindow(x, y, x + w - 1, y + h - 1);
        for (row = 0; row < h; row++) { // For each scanline...
            // Seek to start of scan line.  It might seem labor-
            // intensive to be doing this on every line, but this
            // method covers a lot of gritty details like cropping
            // and scanline padding.  Also, the seek only takes
            // place if the file position actually needs to change
            // (avoids a lot of cluster math in SD library).
            uint8_t r, g, b, *sdptr;
            int lcdidx, lcdleft;
            if (flip)   // Bitmap is stored bottom-to-top order (normal BMP)
                pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
            else        // Bitmap is stored top-to-bottom
                pos = bmpImageoffset + row * rowSize;
            if (bmpFile.position() != pos) { // Need seek?
                bmpFile.seek(pos);
                buffidx = sizeof(sdbuffer); // Force buffer reload
            }

            for (col = 0; col < w; ) {  //pixels in row
                lcdleft = w - col;
                if (lcdleft > lcdbufsiz) lcdleft = lcdbufsiz;
                for (lcdidx = 0; lcdidx < lcdleft; lcdidx++) { // buffer at a time
                    uint16_t color;
                    // Time to read more pixel data?
                    if (buffidx >= sizeof(sdbuffer)) { // Indeed
                        bmpFile.read(sdbuffer, sizeof(sdbuffer));
                        buffidx = 0; // Set index to beginning
                        r = 0;
                    }
                    switch (bmpDepth) {          // Convert pixel from BMP to TFT format
                        case 24:
                            b = sdbuffer[buffidx++];
                            g = sdbuffer[buffidx++];
                            r = sdbuffer[buffidx++];
                            color = tft.color565(r, g, b);
                            break;
                        case 16:
                            b = sdbuffer[buffidx++];
                            r = sdbuffer[buffidx++];
                            if (is565)
                                color = (r << 8) | (b);
                            else
                                color = (r << 9) | ((b & 0xE0) << 1) | (b & 0x1F);
                            break;
                        case 1:
                        case 4:
                        case 8:
                            if (r == 0)
                                b = sdbuffer[buffidx++], r = 8;
                            color = palette[(b >> bitshift) & bitmask];
                            r -= bmpDepth;
                            b <<= bmpDepth;
                            break;
                    }
                    lcdbuffer[lcdidx] = color;

                }
                tft.pushColors(lcdbuffer, lcdidx, first);
                first = false;
                col += lcdidx;
            }           // end cols
        }               // end rows
        tft.setAddrWindow(0, 0, tft.width() - 1, tft.height() - 1); //restore full screen
        ret = 0;        // good render
    }
    bmpFile.close();
    return (ret);
}
