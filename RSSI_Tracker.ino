/* RSSI-Tracker
*  Version 1.0.7 19.09.2015
*  Copyright (C) 2013-2015 RSSI-Tracker Jörg Frede
*  based on code by Michael Heck Eigenbau Diversity und Antennentracker
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
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*  Dieses Programm ist Freie Software: Sie können es unter den Bedingungen
*  der GNU General Public License, wie von der Free Software Foundation,
*  Version 3 der Lizenz oder (nach Ihrer Wahl) jeder neueren
*  veröffentlichten Version, weiterverbreiten und/oder modifizieren.
*
*  Dieses Programm wird in der Hoffnung, dass es nützlich sein wird, aber
*  OHNE JEDE GEWÄHRLEISTUNG, bereitgestellt; sogar ohne die implizite
*  Gewährleistung der MARKTFÄHIGKEIT oder EIGNUNG FÜR EINEN BESTIMMTEN ZWECK.
*  Siehe die GNU General Public License für weitere Details.
*
*  Sie sollten eine Kopie der GNU General Public License zusammen mit diesem
*  Programm erhalten haben. Wenn nicht, siehe <http://www.gnu.org/licenses/>.
*/

#include <LCD4884.h>
#include <Servo.h>

//SETUP
const uint16_t wartezeit = 200;  // Default wartezeit in Millisekunden
const uint8_t schwellwert = 245; // Über diesem wert wird die Antenne nicht bewegt
const uint8_t maxh = 175;        // Winkel rechts
const uint8_t minh = 5;          // Winkel links
const uint8_t maxv = 120;        // Winkel oben
const uint8_t minv = 60;         // Winkel unten
const uint8_t rssi1 = 1;         // 1. RSSI Tracker Antenne
const uint8_t rssi2 = 2;         // 2. RSSI Fixe Antenne
const uint8_t ServoPortH = 10;   // Port für Hozizontalen Servo
const uint8_t ServoPortV = 12;   // Port für Vertikalen Servo
const uint8_t owinkel = 30;      // Öffnungswinkel der Tracking Antenne
#define SPKR 9                   // Port für Buzzer

//Variablen Initialisieren
uint32_t calibrate1 = 0;
uint32_t calibrate2 = 0;
uint16_t rssiTrack = 0;
uint16_t arrayTrack[100];
uint16_t rssiFix = 0;
uint16_t arrayFix[100];
uint16_t rssiTrackOld = 0;
uint16_t rssiFixOld = 0;
uint8_t hw = minh;
uint8_t vw = minv;
char richtung = 'L';
char Vert_richtung;
char buffer[10];
uint8_t loopVert = 0;
uint8_t loopHori = 0;
float faktor = 1;
Servo myservo1;  // Servo1 Objekt anlegen
Servo myservo2;  // Servo2 Objekt anlegen

void setup()
{
  myservo1.attach(ServoPortH);  // attaches the servo on pin to the servo object
  myservo2.attach(ServoPortV);  // attaches the servo on pin to the servo object

  Serial.begin(9600);     // Seriellen Port initialisieren

  // set the analog reference as built-in 2.56Volts
  analogReference(INTERNAL);

  do
  {
    lcd.LCD_init();
    lcd.LCD_clear();
    lcd.LCD_write_string(0, 0, "RSSI Tracker  calibrating...              Sender sollte in der naehe  sein!!!", MENU_NORMAL );

    FindTX();

    for (uint8_t i = 0; i < 100; i++)                        // RSSI Werte Kalibrieren
    {
      calibrate1 = calibrate1 + analogRead(rssi1);
      calibrate2 = calibrate2 + analogRead(rssi2);
      delay(25);
    }
    calibrate1 = calibrate1 / 100;
    calibrate2 = calibrate2 / 100;

    if ( ((calibrate1 + calibrate2) / 2 ) < 750 )            // Alarm anzeigen wenn kein Sender gefunden
    {
      lcd.LCD_init();
      lcd.LCD_clear();
      lcd.LCD_write_string(0, 0, "Kein Sender   gefunden!!!   Warte 10 Sek.", MENU_NORMAL );
      tone(SPKR, 988, 100);
      delay(130);
      noTone(SPKR);
      tone(SPKR, 440, 200);
      delay(10000);
      noTone(SPKR);
    }
  }
  while (((calibrate1 + calibrate2) / 2) < 300);          // Solange wir noch keine gültige Calibrierung haben noch mal widerholen

  tone(SPKR, 2093, 100);                                  // Ein wenig Musik um zu zeigen das wir erfolgreich Sender gefunden haben
  delay(130);
  noTone(SPKR);
  tone(SPKR, 2349, 100);
  delay(130);
  noTone(SPKR);
  tone(SPKR, 2637, 100);
  delay(130);
  noTone(SPKR);
}

void loop()
{
  getRssi();               // Neue werte hohlen

  DisplayRssi();
  lcd.LCD_write_string(0, 0, "Kein Tracking ", MENU_NORMAL );

  if (rssiTrack <= schwellwert)
  {
    trackHorizontal();
  }

  delay(wartezeit * faktor);

}

void trackHorizontal()
{
  do
  {
    getRssi();     // Neue werte hohlen

    if (rssiTrack <= 130 || (rssiTrack + 20) <=  rssiFix )              // Wenn das Signal der Tracker Antenne sauschlecht wird Notfall Scan
    {
      DisplayRssi();
      lcd.LCD_write_string(0, 0, "Notfall Scan! ", MENU_HIGHLIGHT );
      FindTX();
      faktor = 0.75;
    }
    else
    {
      if ( rssiTrack > rssiTrackOld )                              //RSSI Besser geworden min
      {
        Serial.print("besser : ");
        Serial.print(rssiTrackOld - rssiTrackOld);
        Serial.print(", faktor: ");
        Serial.print(faktor);
        Serial.print("\n");
        if (richtung == 'L')
        {
          Serial.println("links");
          if (faktor >= 0.1)
          {
            faktor = faktor * 0.5;
          }
          hw = hw + ( owinkel / 4 * faktor );
          richtung = 'L';
        }
        else if (richtung == 'R')
        {
          Serial.println("Richting = rechts");
          if (faktor >= 0.1)
          {
            faktor = faktor * 0.5;
          }
          hw = hw - ( owinkel / 4 * faktor );
          richtung = 'R';
        }
      }
      else if ( rssiTrack < rssiTrackOld )                 //RSSI schlechter geworden
      {
        Serial.print("schlechter : ");
        Serial.print(rssiTrackOld - rssiTrackOld);
        Serial.print(", faktor: ");
        Serial.print(faktor);
        Serial.print("\n");
        if ( rssiFix >= rssiFixOld )                        // Wenn die Fixe besser geworden ist oder gleich
        {
          if (richtung == 'R')
          {
            Serial.println("Richting = rechts");
            if (faktor <= 1)
            {
              faktor = faktor * 2;
            }
            hw = hw + ( owinkel / 4 * faktor) ;
            richtung = 'L';
          }
          else if (richtung == 'L')
          {
            Serial.println("Richting = links");
            if (faktor <= 1)
            {
              faktor = faktor * 2;
            }
            hw = hw - ( owinkel / 4 * faktor);
            richtung = 'R';
          }
        }
      }
      else                                                        //RSSI gleich
      {
        Serial.print("Wenig änderung : ");
        Serial.print(rssiTrackOld - rssiTrackOld);
        Serial.print(", faktor: ");
        Serial.print(faktor);
        Serial.print("\n");

        if (richtung == 'R')
        {
          Serial.println("Richtung = rechts");
          if (faktor <= 0.1 && loopHori >= 8)                    // wenn der Faktor gut ist und 8 mal Horizontal getrackt wurde.
          {
            trackVertikal();                    // Vertikal Tracken
          }
          if (faktor >= 0.1)
          {
            faktor = faktor * 0.5;
          }
          hw = hw - 1;
          richtung = 'R';
        }
        else if (richtung == 'L')
        {
          Serial.println("links");
          if (faktor <= 0.1 && loopHori >= 8)                    // wenn der Faktor gut ist und 8 mal Horizontal getrackt wurde.
          {
            trackVertikal();                    // Vertikal Tracken
          }
          if (faktor >= 0.1)
          {
            faktor = faktor * 0.5;
          }
          hw = hw + 1;
          richtung = 'L';
        }
      }

      DisplayRssi();
      lcd.LCD_write_string(0, 0, "Track Horizont", MENU_NORMAL );
    }

    hw = constrain(hw, minh, maxh);   // nicht über Servo limits bewegen!
    myservo1.write(hw);

    if (hw <= minh || hw >= maxh)          // Wenn der Servo an seinen Maxinalwert bewegt werden soll ein mal neu suchen.
    {
      lcd.LCD_clear();
      lcd.LCD_write_string(0, 2, "Reset H", MENU_NORMAL );
      FindTX();
      faktor = 1;
      return;
    }
    loopHori++;
    delay(wartezeit * faktor);
  }

  while (rssiTrack <= schwellwert);
  return;
}

void trackVertikal()
{
  loopVert = 0;
  loopHori = 0;
  do
  {
    getRssi();    // Neue werte hohlen

    if (rssiTrack <= 50)
    {
      vw = (minv + maxv) / 2;
      myservo2.write(vw);
      return;
    }
    else
    {
      DisplayRssi();
      lcd.LCD_write_string(0, 0, "Track Vertikal", MENU_NORMAL );
    }

    if (rssiTrack >= rssiTrackOld)
    {
      if (Vert_richtung == 'O')
      {
        if (faktor >= 0.1)
        {
          faktor = faktor * 0.9;
        }
        vw = vw - ( owinkel / 3 * faktor );
        Vert_richtung = 'O';
      }
      else
      {
        if (faktor >= 0.1)
        {
          faktor = faktor * 0.9;
        }
        vw = vw + ( owinkel / 3 * faktor );
        Vert_richtung = 'U';
      }
    }
    else
    {
      if ( rssiFix >= rssiFixOld )                        // Wenn die Fix besser geworden ist oder gleich
      {
        if (Vert_richtung == 'U')
        {
          if (faktor <= 2)
          {
            faktor = faktor * 1.1;
          }
          vw = vw - ( owinkel / 3 * faktor);
          Vert_richtung = 'O';
        }
        else
        {
          if (faktor <= 2)
          {
            faktor = faktor * 1.1;
          }
          vw = vw + ( owinkel / 3 * faktor);
          Vert_richtung = 'U';
        }
      }
      DisplayRssi();
      lcd.LCD_write_string(0, 0, "Track Vertikal", MENU_NORMAL );
    }

    vw = constrain(vw, minv, maxv);   // nicht über Servo limits bewegen!
    myservo2.write(vw);

    if (vw <= minv || vw >= maxv)
    {
      lcd.LCD_clear();
      lcd.LCD_write_string(0, 2, "Reset V", MENU_NORMAL );
      vw = (minv + maxv) / 2;
      return;
    }
    delay(wartezeit * faktor);

    loopVert++;
  }
  while (loopVert > 3 && rssiTrack <= schwellwert);
  return;
}


void sortTrack() {
  uint16_t out, in, swapper;
  for (out = 0 ; out < 10; out++) { // outer loop
    for (in = out; in < (10 - 1); in++)  { // inner loop
      if ( arrayTrack[in] > arrayTrack[in + 1] ) { // out of order?
        // swap them:
        swapper = arrayTrack[in];
        arrayTrack[in] = arrayTrack[in + 1];
        arrayTrack[in + 1] = swapper;
      }
    }
  }
}

void sortFix() {
  uint16_t out, in, swapper;
  for (out = 0 ; out < 10; out++) { // outer loop
    for (in = out; in < (10 - 1); in++)  { // inner loop
      if ( arrayFix[in] > arrayFix[in + 1] ) { // out of order?
        // swap them:
        swapper = arrayFix[in];
        arrayFix[in] = arrayFix[in + 1];
        arrayFix[in + 1] = swapper;
      }
    }
  }
}

void getRssi()
{
  rssiTrackOld = rssiTrack;
  rssiFixOld = rssiFix;

  for (int i = 0; i < 10; i++)                            // 10 mal RSSI Werte holen und Median ausrechnen bilden um Rauschen zu verringern
  {
    arrayTrack[i] = analogRead(rssi1);
    arrayFix[i] = analogRead(rssi2);
    delay(5);
  }
  sortFix();
  sortTrack();

  rssiTrack = arrayTrack[5];                             //  Median Werte merken.
  rssiFix = arrayFix[5];

  if ( rssiTrack > calibrate1 + 5 )                       // wenn der aktuelle RSSI wert 5 über der calibrierung ist die Calibrierung um 1 erhöhen.
  {
    calibrate1++;
  }
  if ( rssiFix > calibrate2 + 5 )
  {
    calibrate2++;
  }

  //Info to searial port
  Serial.print("rssiTrackRaw: ");
  Serial.print(rssiTrack);
  Serial.print(", rssiFixRaw: ");
  Serial.print(rssiFix);

  rssiTrack = map(rssiTrack, 0, calibrate1, 0, 255);  // Neue zwischen 0 und 255 begrenzen
  rssiFix = map(rssiFix, 0, calibrate2, 0, 255);
  rssiTrack = constrain(rssiTrack, 0, 255);
  rssiFix = constrain(rssiFix, 0, 255);

  //Info to searial port
  Serial.print(", rssiTrackNormal: ");
  Serial.print(rssiTrack);
  Serial.print(", rssiFixNormal: ");
  Serial.print(rssiFix);
  Serial.print("\n");


  if (((rssiTrack + rssiFix) / 2) <= 130)             // Alarm machen wenn das Signal schlecht wird
  {
    tone(SPKR, 2960);
  }
  else                                               // Alarm aus wenn es wieder gut ist
  {
    noTone(SPKR);
  }

  return;
}

void DisplayRssi()
{
  //Display rssi values
  lcd.LCD_write_string(0, 1, "              RSSI T:       RSSI F:       Winkel H:     Winkel V:     ", MENU_NORMAL );
  itoa(rssiTrack, buffer, 10);
  lcd.LCD_write_string(45, 2, buffer, MENU_NORMAL );
  itoa(rssiFix, buffer, 10);
  lcd.LCD_write_string(45, 3, buffer, MENU_NORMAL );
  itoa(hw, buffer, 10);
  lcd.LCD_write_string(55, 4, buffer, MENU_NORMAL );
  itoa(vw, buffer, 10);
  lcd.LCD_write_string(55, 5, buffer, MENU_NORMAL );
}

void FindTX() // mal gucken wo wir den besten emfang haben.
{
  // Erst mal ermitteln ob wir von links ode rechts anfangen zu suchen.
  uint16_t maxr = 0;
  uint8_t maxw = 0;
  uint8_t oldhw = hw;
  uint8_t divhw = 0;
  if ( hw >= ((minh + maxh) / 2 ) )     // Wenn wir weiter als die Mitte nach links stehen
  {
    hw = maxh;                          // Horizontalen Servo auf maximum
    myservo1.write(hw);
    vw = minv + maxv / 2;               // Vertikal Servo auf Mitte
    myservo2.write(vw);
    delay((maxh - oldhw) * 3);          // 3ms pro ° Warten bis der Servo da ist

    Serial.println("Sweep Horizontal");

    for (hw = maxh; hw > minh ; hw--) { // Ein mal über den kompletten Horizont schwenken
      myservo1.write(hw);
      delay(5);                         // Servo ein wenig zeit geben zum drehen
      rssiTrack =  analogRead(rssi1);
      Serial.print("Winkel,");
      Serial.print(hw);
      Serial.print(",wert,");
      Serial.print(rssiTrack);
      Serial.print("\n");
      if (rssiTrack > maxr)             // Wenn wir einen neuen maximal Wert haben diesen merken
      {
        maxr = rssiTrack;
        maxw = hw;
      }
    }
    divhw = maxw - hw;
    hw = maxw;                          // Antenne auf den höchsten Wert setzten
    hw = constrain(hw, minh, maxh);     // nicht über Servo Limits bewegen!
    myservo1.write(hw);
  }
  else                                  // Andernfals waren wir rechts von der Mitte
  {
    hw = minh;                          // Horizontalen Servo auf minimim
    myservo1.write(hw);
    vw = (minv + maxv) / 2;             // Vertikal Servo auf Mitte
    myservo2.write(vw);
    delay((oldhw - minh) * 3);          // 3ms pro ° Warten bis der Servo da ist
    Serial.println("Sweep Horizontal");

    for (hw = minh; hw < maxh ; hw++) { // Ein mal über den kompletten Horizont schwenken
      myservo1.write(hw);
      delay(5);                         // Servo ein wenig zeit geben zum drehen
      rssiTrack =  analogRead(rssi1);
      Serial.print("Winkel,");
      Serial.print(hw);
      Serial.print(",wert,");
      Serial.print(rssiTrack);
      Serial.print("\n");
      if (rssiTrack > maxr)             // Wenn wir einen neuen maximal Wert haben diesen Merken
      {
        maxr = rssiTrack;
        maxw = hw;
      }
    }
    divhw = hw - maxw;
    hw = maxw;                          // Antenne auf den höchsten Wert setzten
    hw = constrain(hw, minh, maxh);     // nicht über Servo Limits bewegen!
    myservo1.write(hw);
  }

  vw = minv;                            // Vertikalen Servo auf minimim
  myservo2.write(vw);

  if (((minv + maxv) / 2 - minv) > divhw)  // Je nach dem ob es länger dauert den horizontalen oder vertikalen Servo zu bewegen
  {
    delay(((minv + maxv) / 2 - minv) * 3); // 3ms pro ° Warten bis der vertikale Servo da ist
  }
  else
  {
    delay(divhw * 3);                    // 3ms pro ° Warten bis der horizontale Servo da ist
  }

  maxr = 0;
  maxw = 0;

  Serial.println("Sweep Vertikal");     // Den ganzen spass noch mal Vertikal
  for (vw = minv; vw <= maxv ; vw++) {
    myservo2.write(vw);
    delay(5);
    rssiTrack =  analogRead(rssi1);
    Serial.print("Winkel,");
    Serial.print(vw);
    Serial.print(",wert,");
    Serial.print(rssiTrack);
    Serial.print("\n");
    if (rssiTrack > maxr)               // Wenn wir einen neuen Maximal Wert haben diesen Merken
    {
      maxw = vw;
      maxr = rssiTrack;
    }
  }
  vw = maxw;                            // Antenne auf den höchsten Wert setzten
  vw = constrain(vw, minv, maxv);       // nicht über Servo limits bewegen!
  myservo2.write(vw);
}
