#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

int brightnessLevel  = 1000;
const int BACKLIGHT_PIN = 5;
const int PWM_FREQ      = 5000;

unsigned long lastSecond = 0;
unsigned long lastBlink  = 0;
bool colonOn             = true;

int hour24     = 14;
int minuteVal  = 34;
int secondVal  = 0;
int dayDate    = 12;
int monthVal   = 9;
int yearVal    = 2026;
int weekdayVal = 2;

const char* dayNames[] = {"SUN","MON","TUE","WED","THU","FRI","SAT"};

int currentTemp = 28;
int tempDir     = 1;

byte degSym[8]  = {0b00110,0b01001,0b01001,0b00110,0b00000,0b00000,0b00000,0b00000};
byte barFull[8] = {0b11111,0b11111,0b11111,0b11111,0b11111,0b11111,0b11111,0b11111};

// ════════════════════════════════════════

int scaleToPWM(int level) {
  if (level <= 1)    return 0;
  if (level >= 1000) return 255;
  return map(level, 1, 1000, 1, 255);
}

void setBacklight(int level) {
  ledcWrite(BACKLIGHT_PIN, scaleToPWM(level));
}

int daysInMonth(int m, int y) {
  if (m == 2) return (y % 4 == 0) ? 29 : 28;
  if (m == 4 || m == 6 || m == 9 || m == 11) return 30;
  return 31;
}

void updateDateTime() {
  secondVal++;
  if (secondVal >= 60) {
    secondVal = 0; minuteVal++;
    if (minuteVal >= 60) {
      minuteVal = 0; hour24++;
      if (hour24 >= 24) {
        hour24 = 0;
        weekdayVal = (weekdayVal + 1) % 7;
        dayDate++;
        if (dayDate > daysInMonth(monthVal, yearVal)) {
          dayDate = 1; monthVal++;
          if (monthVal > 12) { monthVal = 1; yearVal++; }
        }
      }
    }
  }
}

void updateTemp() {
  if (random(0, 10) == 0) tempDir = (random(0, 2) == 0) ? -1 : 1;
  currentTemp += random(0, 2) * tempDir;
  if (currentTemp > 42) { currentTemp = 42; tempDir = -1; }
  if (currentTemp < 18) { currentTemp = 18; tempDir =  1; }
}

void drawTimeOnly() {
  int h12 = hour24 % 12;
  if (h12 == 0) h12 = 12;
  const char* ampm = (hour24 < 12) ? "AM" : "PM";

  char timeBuf[9];
  if (colonOn)
    sprintf(timeBuf, "%02d:%02d %s", h12, minuteVal, ampm);
  else
    sprintf(timeBuf, "%02d %02d %s", h12, minuteVal, ampm);

  lcd.setCursor(0, 0);
  lcd.print(timeBuf);   // 8 chars: "02:34 AM"
}

void drawDisplay() {
  // LINE 0: "02:34 AM █28°C  "
  // col:     0123456 7 8 9 ...
  // "02:34 AM" = 8 chars (col 0–7)
  // col 8 = space
  // col 9 = █
  // col 10–15 = temp

  drawTimeOnly();              // col 0–7

  lcd.setCursor(8, 0);
  lcd.print(" ");              // col 8  = space
  lcd.write(byte(1));          // col 9  = █ divider
  lcd.print(" ");              // col 10 = space
  if (currentTemp < 10) lcd.print(" ");
  lcd.print(currentTemp);      // col 11–12
  lcd.write(byte(0));          // col 13 = °
  lcd.print("C");              // col 14
  lcd.print(" ");              // col 15 clear

  // LINE 1: "12/09/26 █  WED "
  // col:     01234567 8 9 ...
  int yy = yearVal % 100;
  char dateBuf[9];
  sprintf(dateBuf, "%02d/%02d/%02d", dayDate, monthVal, yy);

  lcd.setCursor(0, 1);
  lcd.print(dateBuf);          // col 0–7
  lcd.print(" ");              // col 8  = space
  lcd.write(byte(1));          // col 9  = █ divider
  lcd.print("  ");             // col 10–11 = gap
  lcd.print(dayNames[weekdayVal]); // col 12–14
  lcd.print(" ");              // col 15 clear
}

// ════════════════════════════════════════

void setup() {
  Serial.begin(115200);
  ledcAttach(BACKLIGHT_PIN, PWM_FREQ, 8);

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, degSym);
  lcd.createChar(1, barFull);
  setBacklight(brightnessLevel);
  lcd.clear();

  // Splash screen
  lcd.setCursor(1, 0);
  lcd.print("RAHUL'S  CLOCK");
  lcd.setCursor(3, 1);
  lcd.print("ESP32  v2.0");
  delay(1800);
  lcd.clear();

  Serial.println("ESP32 Clock Ready");
  Serial.println("SET:HH,MM,SS,DD,MON,YYYY,WD  →  time set");
  Serial.println("1-1000                        →  brightness");
}

void loop() {
  unsigned long now = millis();

  // ── Serial commands ──
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.startsWith("SET:")) {
      cmd.remove(0, 4);
      int h, m, s, d, mo, yr, wd;
      sscanf(cmd.c_str(), "%d,%d,%d,%d,%d,%d,%d", &h,&m,&s,&d,&mo,&yr,&wd);
      hour24=h; minuteVal=m; secondVal=s;
      dayDate=d; monthVal=mo; yearVal=yr; weekdayVal=wd;
      Serial.println("Time updated!");
    } else {
      int v = cmd.toInt();
      if (v >= 1 && v <= 1000) {
        brightnessLevel = v;
        setBacklight(v);
        Serial.printf("Brightness: %d/1000\n", v);
      }
    }
  }

  
  if (now - lastBlink >= 500) {
    lastBlink = now;
    colonOn   = !colonOn;
    drawTimeOnly();
  }


  if (now - lastSecond >= 1000) {
    lastSecond = now;
    updateDateTime();
    updateTemp();
    drawDisplay();

    Serial.printf("%02d:%02d %s | T=%dC | %02d/%02d/%02d %s\n",
      hour24 % 12 == 0 ? 12 : hour24 % 12,
      minuteVal,
      hour24 < 12 ? "AM" : "PM",
      currentTemp,
      dayDate, monthVal, yearVal % 100,
      dayNames[weekdayVal]);
  }
}
