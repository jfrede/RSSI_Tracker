// Version 1.0.1 22.2.2014

#include <LCD4884.h>
#include <Servo.h>

//SETUP
int wartezeit = 200;  // Default wartezeit in Millisekunden
int schwellwert = 99;  // Über diesem wert wird die Antenne nicht bewegt
int maxh = 175;   // Winkel rechts
int minh = 5;     // Winkel links
int maxv = 120;   // Winkel oben
int minv = 60;    // Winkel unten
int rssi1 = 1;       // 1. RSSI Tracker Antenne
int rssi2 = 2;       // 2. RSSI Fixe Antenne
int ServoPortH = 10;  // Port für Hozizontalen Servo
int ServoPortV = 12;  // Port für Vertikalen Servo
#define SPKR 9        // Port für Buzzer
int owinkel = 45;     // Öffnungswinkel der Tracking Antenne


//Variablen Initialisieren
int calibrate1 = 0;
int calibrate2 = 0;
int rssiTrack = 0;
int rssiFix = 0;
int rssiTrackOld = 0;
int hw = minh;
int vw = minv;
char richtung = 'L';
char Vert_richtung;
char buffer[10];
int loopVert = 0;
int loopHori = 0;
float faktor = 1;
Servo myservo1;  // Servo1 Objekt anlegen
Servo myservo2;  // Servo2 Objekt anlegen

void setup()
{
  myservo1.attach(ServoPortH);  // attaches the servo on pin to the servo object
  myservo2.attach(ServoPortV);  // attaches the servo on pin to the servo object

  Serial.begin(9600);     // Seriellen Port initialisieren

  do
  {
    lcd.LCD_init();
    lcd.LCD_clear();
    lcd.LCD_write_string(0, 0, "RSSI Tracker  calibrating...              Sender sollte in der naehe  sein!!!", MENU_NORMAL );

    FindTX();

    for (int i = 0; i < 100; i++)                        // RSSI Werte Kalibrieren
    {
      calibrate1 = calibrate1 + analogRead(rssi1);
      calibrate2 = calibrate2 + analogRead(rssi2);
      delay(25);
    }
    calibrate1 = calibrate1 / 100;
    calibrate2 = calibrate2 / 100;

    if ( ((calibrate1 + calibrate2) / 2 ) < 150 )            // Alarm anzeigen wenn kein Sender gefunden
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
  } while (((calibrate1 + calibrate2) / 2) < 150);        // Solange wir noch keine gültige Calibrierung haben noch mal widerholen

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

    if (rssiTrack <= 48 || (rssiTrack + 20) <=  rssiFix )              // Wenn das Signal der Tracker Antenne sauschlecht wird Notfall Scan
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
        Serial.println("besser");
        Serial.println(rssiTrackOld - rssiTrackOld);
        Serial.println("faktor");
        Serial.println(faktor);

        if (richtung == 'L')
        {
          Serial.println("links");
          if (faktor >= 0.1)
          {
            faktor = faktor * 0.65;
          }
          hw = hw + ( owinkel / 4 * faktor );
          richtung = 'L';
        }
        else if (richtung == 'R')
        {
          Serial.println("rechts");
          if (faktor >= 0.1)
          {
            faktor = faktor * 0.65;
          }
          hw = hw - ( owinkel / 4 * faktor );
          richtung = 'R';
        }
      }
      else if ( rssiTrack < rssiTrackOld)                 //RSSI schlechter geworden
      {
        Serial.println("schlechter");
        Serial.println(rssiTrackOld - rssiTrackOld);
        Serial.println("faktor");
        Serial.println(faktor);
        if (richtung == 'R')
        {
          Serial.println("rechts");
          if (faktor <= 1)
          {
            faktor = faktor * 2;
          }
          hw = hw + ( owinkel / 4 * faktor) ;
          richtung = 'L';
        }
        else if (richtung == 'L')
        {
          Serial.println("links");
          if (faktor <= 1)
          {
            faktor = faktor * 2;
          }
          hw = hw - ( owinkel / 4 * faktor);
          richtung = 'R';
        }
      }
      else                                                        //RSSI gleich
      {
        Serial.println("wenig änderung");
        Serial.println(rssiTrackOld - rssiTrackOld);
        Serial.println("faktor");
        Serial.println(faktor);
        if (richtung == 'R')
        {
          Serial.println("rechts");
          if (faktor <= 0.1 && loopHori >= 8)                    // wenn der Faktor gut ist und 8 mal Horizontal getrackt wurde.
          {
            trackVertikal();                    // Horizontal Tracken
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
            trackVertikal();                    // Horizontal Tracken
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

void getRssi()
{
  rssiTrackOld = rssiTrack;

  for (int i = 0; i < 10; i++)                        // 10 mal RSSI Werte holen und mittelwert bilden um Rauschen zu verringern
  {
    rssiTrack = rssiTrack + analogRead(rssi1);
    rssiFix = rssiFix + analogRead(rssi2);
    delay(5);
  }
  rssiTrack = rssiTrack / 10;
  rssiFix = rssiFix / 10;

  if ( rssiTrack > calibrate1 + 5 )                       // wenn der aktuelle RSSI wert 5 über der calibrierung ist die Calibrierung um 1 erhöhen.
  {
    calibrate1++;
  }
  if ( rssiFix > calibrate2 + 5 )
  {
    calibrate2++;
  }

  rssiTrack = map(rssiTrack, 0, calibrate1, 0, 100);  // Neue zwischen 0 und 100 begrenzen
  rssiFix = map(rssiFix, 0, calibrate2, 0, 100);
  rssiTrack = constrain(rssiTrack, 0, 100);
  rssiFix = constrain(rssiFix, 0, 100);

  if (((rssiTrack + rssiFix) / 2) <= 55)             // Alarm machen wenn das Signal schlecht wird
  {
    tone(SPKR, 2960);
  }
  else                                               // Alarm aus wenn es wieder gut ist
  {
    noTone(SPKR);
  }

  //Info to searial port
  //Serial.println("rssiTrack:");
  //Serial.println(rssiTrack);
  //Serial.println("rssiFix:");
  //Serial.println(rssiFix);
  //Serial.println(" ");

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
  int maxr = 0;
  int maxw = 0;
  if ( hw >= ((minh + maxh) / 2 ) )     // wenn wir weite also die Mitte nach Links stehen
  {
    hw = maxh;                      // Horizontalen Servo auf minimim
    myservo1.write(hw);
    vw = minv + maxv / 2;           // Vertikal Servo auf Mitte
    myservo2.write(vw);
    delay(200);                     // 0.2 Sekunden warten bis Servos Position ereicht hat

    Serial.println("Sweep Horizontal");

    for (hw = maxh; hw > minh ; hw--) {         //ein mal über den kompletten Horizont schwenken
      myservo1.write(hw);
      rssiTrack =  analogRead(rssi1);
      Serial.println(vw);
      Serial.println(rssiTrack);
      if (rssiTrack >= maxr)                     //Wenn wir einen neuen Maximal Wert haben diesen Merken
      {
        maxr = rssiTrack;
        maxw = hw;
      }
      delay(10);
    }
    hw = maxw;                                 // Antenne auf den höchsten Wert setzten
    hw = constrain(hw, minh, maxh);           // nicht über Servo limits bewegen!
    myservo1.write(hw);
  }
  else
  {
    hw = minh;                      // Horizontalen Servo auf minimim
    myservo1.write(hw);
    vw = minv + maxv / 2;           // Vertikal Servo auf Mitte
    myservo2.write(vw);
    delay(200);                     // 0.2 Sekunden warten bis Servos Position ereicht hat
    Serial.println("Sweep Horizontal");

    for (hw = minh; hw < maxh ; hw++) {         //ein mal über den kompletten Horizont schwenken
      myservo1.write(hw);
      rssiTrack =  analogRead(rssi1);
      Serial.println(vw);
      Serial.println(rssiTrack);
      if (rssiTrack >= maxr)                     //Wenn wir einen neuen Maximal Wert haben diesen Merken
      {
        maxr = rssiTrack;
        maxw = hw;
      }
      delay(10);
    }
    hw = maxw;                                 // Antenne auf den höchsten Wert setzten
    hw = constrain(hw, minh, maxh);   // nicht über Servo limits bewegen!
    myservo1.write(hw);
  }

  vw = minv;                      // Vertikalen Servo auf minimim und 0.5 Sekunden wartn bis der da ist.
  myservo2.write(vw);
  delay(400);
  maxr = 0;
  maxw = 0;

  Serial.println("Sweep Vertikal");                    // Den ganzen spass noch mal Vertikal
  for (vw = minv; vw <= maxv ; vw++) {
    myservo2.write(vw);
    rssiTrack =  analogRead(rssi1);
    Serial.println(vw);
    Serial.println(rssiTrack);
    if (rssiTrack > maxr)                       //Wenn wir einen neuen Maximal Wert haben diesen Merken
    {
      maxw = vw;
      maxr = rssiTrack;
    }
    delay(10);
  }
  vw = maxw;                                   // Antenne auf den höchsten Wert setzten
  vw = constrain(vw, minv, maxv);   // nicht über Servo limits bewegen!
  myservo2.write(vw);
}
