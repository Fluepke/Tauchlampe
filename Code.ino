//Pins, an die Hardware angeschlossen ist:
#define KEEPALIVE 2
#define PLUSBUTTON 4
#define MINUSBUTTON 5
#define LAMPEPWM 6
#define AKKU 0

const int signalHoehe = 20; //Höhe auf der Plus - und Minussymbol dargestellt werden
#define DEBOUNCEDELAY 100L //Zeit bis auf Taster reagiert wird
#define READINGS 10 //Lesevorgänge, um Analogwert zu runden

#include "U8glib.h"
//U8GLIB_SSD1306_128X64 u8g(10, 9, 12, 11, 13);	// SW SPI Com: SCK = 13, MOSI = 11, CS = 10, A0 = 9
//oder
//U8GLIB_SSD1306_128X64 u8g(10, 9, 8);		// HW SPI Com: CS = 10, A0 = 9, RS/DC = 8(Hardware Pins are  SCK = 13 and MOSI = 11)
//oder
//U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NO_ACK);  
U8GLIB_SSD1306_128X64 u8g(10, 9, 12, 11, 13);

struct Debounce {
  byte input;
  boolean btnState;
  boolean lastBtnState;
  long lastDebounceTime;
} Buttons[] = {
  { PLUSBUTTON, 0, 0, 0 },
  { MINUSBUTTON, 0, 0, 0 }
};

byte lampenHelligkeit = 0;

void setup() { //Hardware initialisieren
  pinMode(KEEPALIVE, OUTPUT);
  pinMode(PLUSBUTTON, INPUT);
  pinMode(MINUSBUTTON, INPUT);
  digitalWrite(KEEPALIVE, HIGH);
  setPwmFrequency(LAMPEPWM, 32);
  Serial.begin(9600);
  
  u8g.firstPage();  
  do { //Picture Loop, in ihr wird auf das Display gezeichnet
    u8g.setFont(u8g_font_unifontr);
    u8g.drawStr( 0, 20, "Fish Blaster");  //Splashscreen zeichnen
    u8g.drawStr( 0, 35, "Beta 1.0");
    u8g.drawStr( 0, 64, "Fabian Luepke");
  } while( u8g.nextPage() );
    
 delay (500);
}

void loop() {
  analogWrite(LAMPEPWM, lampenHelligkeit); //Lampenhelligkeit per PWM ausgeben
  if (Serial.available() > 1) { //Auf Befehle via Bluetooth / Serial Port reagieren
    switch (Serial.read()) {
      case 'o':
        if (Serial.read() == 'f') {
          Serial.println("ausschalten");
          delay(2);
          digitalWrite(KEEPALIVE, LOW); //Geht Keepalive auf LOW, schaltet sich alles aus.
        }
        break;
      case 'b':
        if (Serial.read() == 'a') {
          Serial.println(analogRead(AKKU)); //In Zukunft: Mittelwert bilden
        }
        break;
      case 'd':
        lampenHelligkeit = Serial.read();
        break;
    }
  }
  
  //Schalter entprellt einlesen
  for (byte i = 0; i < sizeof(Buttons) / sizeof(Buttons[0]); i++)
  {
    //Wenn sich der Status der Schalter seit dem letzen Einlesen geändert hat, wird die aktuelle Zeit gespeichert
    if ((Buttons[i].lastBtnState = digitalRead(Buttons[i].input)) != Buttons[i].lastBtnState) {
      //Serial.println("Status getoggled");
      Buttons[i].lastDebounceTime = millis();
    }
    //Wenn der Schaltzustand seit der Zeitspanne DEBOUNCEDELAY vorliegt, kann er als stabil betrachtet werden:
    if ((millis() - Buttons[i].lastDebounceTime) > DEBOUNCEDELAY)  {
      if (!Buttons[i].lastBtnState)
        buttonBetaetigt(Buttons[i].input); //Aktion aufgrund des Schalters auslösen
      Buttons[i].lastBtnState = true; //Schalter "zurücksetzen"
      Buttons[i].lastDebounceTime = millis(); 
    }
  }
  
  //Infos anzeigen
  u8g.firstPage();  
  do { //Picture Loop, in ihr wird auf das Display gezeichnet
    //Akkuspannung ermitteln und anzeigen
    u8g.setFont(u8g_font_unifontr);
    char StrBuffer[5];
    float spannung = (float)((float)((float)analogRead(AKKU) / (float)1024) * (float)14); //Spannung berechnen
    //Hinweis: In der zukünftigen Version wird der Mttelwert der Spannung über eine Zeitspanne bestimmt.
    String string = dtostrf(spannung, 4, 1, StrBuffer); //in ein String umwandeln
    string = "Bat:" + string + "V"; //Endgültigen String zusammensetzen
    char buffer[10];
    string.toCharArray(buffer, 10); //in ein Char Array umwandeln
    u8g.drawStr( 0, 15, buffer); //auf das Display zeichnen
    
    const int hoehe = 45;
    //Balken mit Lampenhelligkeit darstellen
    u8g.drawHLine (2, hoehe, 124);
    u8g.drawHLine (2, hoehe+5, 124);
    u8g.drawVLine (0, hoehe+2, 2);
    u8g.drawVLine (127, hoehe+2, 2);
    u8g.drawPixel (1, hoehe+1);
    u8g.drawPixel (1, hoehe+4);
    u8g.drawPixel (126, hoehe+1);
    u8g.drawPixel (126, hoehe+4);
    
    int balkenlaenge = ((float)lampenHelligkeit / (float)256)*(float)126;
    u8g.drawHLine (2, hoehe+2, balkenlaenge);
    u8g.drawHLine (2, hoehe+3, balkenlaenge);
    
    char StrBuffer1[5];
    float helligkeit = ((float)lampenHelligkeit / (float)255)*(float)100; //Helligkeit in Prozent bestimmen
    String string1 = dtostrf(helligkeit, 4, 1, StrBuffer1);
    string1 += "%"; //Darzustellenden String zusammensetzen
    char buffer1[6];
    string1.toCharArray(buffer1, 6); //In ein Char Array umwandeln
    u8g.drawStr(64 - ((float)u8g.getStrWidth(buffer1) / (float)2), 64, buffer1); //und darstellen!
    
    //Ein Plus zeichnen, wen der "+" Knopf betätigt wird
    if (!digitalRead(PLUSBUTTON)) //Negation, weil Knopf gegen GND schaltet
      drawPlus();
    //Minusbutton nicht behandelt, da in aktuellem Hardware Setup nicht angeschlossen
  } while( u8g.nextPage() );
}

void drawMinus() {
  for (int i = 7; i<= 14; i++) {
    u8g.drawHLine(53, signalHoehe + i, 22);
  }
}

void drawPlus() {
  drawMinus();
  for (int i = 60; i <= 67; i++) {
    u8g.drawVLine(i, signalHoehe, 22);
  }
}

//In dieser Methode wird auf betätigte Buttons reagiert
void buttonBetaetigt(byte input)
{
  switch (input) { 
    case PLUSBUTTON:
      Serial.println("mehr Licht!");
      if (lampenHelligkeit < 252)
        lampenHelligkeit+=4;
      else
        lampenHelligkeit = 255;
      break;
    default:
      //Serial.println("Error");
      break;
  }
}

void setPwmFrequency(int pin, int divisor) {
  byte mode;
  if(pin == 5 || pin == 6 || pin == 9 || pin == 10) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if(pin == 5 || pin == 6) {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    } else {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  } else if(pin == 3 || pin == 11) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x7; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}
