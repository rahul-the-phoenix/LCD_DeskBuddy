#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>

#define I2C_SDA 21
#define I2C_SCL 22

const int BACKLIGHT_PIN = 5;
const int PWM_FREQ      = 5000;
int brightnessLevel     = 1000;

LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;

const char* days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

// ════════════════════════════════════════

int scaleToPWM(int level) {
  if (level <= 1)    return 0;
  if (level >= 1000) return 255;
  return map(level, 1, 1000, 1, 255);
}

void setBacklight(int level) {
  ledcWrite(BACKLIGHT_PIN, scaleToPWM(level));
}

String formatTime(DateTime dt) {
  int hour = dt.hour();
  int minute = dt.minute();
  String ampm = (hour < 12) ? "AM" : "PM";
  int displayHour = hour % 12;
  if (displayHour == 0) displayHour = 12;
  String hourStr   = (displayHour < 10) ? "0" + String(displayHour) : String(displayHour);
  String minuteStr = (minute < 10)      ? "0" + String(minute)      : String(minute);
  return hourStr + ":" + minuteStr + " " + ampm;
}

String formatDate(DateTime dt) {
  String day   = (dt.day()   < 10) ? "0" + String(dt.day())   : String(dt.day());
  String month = (dt.month() < 10) ? "0" + String(dt.month()) : String(dt.month());
  String year  = String(dt.year()).substring(2);
  return day + "/" + month + "/" + year;
}

// ════════════════════════════════════════

void setup() {
  Serial.begin(115200);
  delay(500);

  // PWM backlight on GPIO 5
  ledcAttach(BACKLIGHT_PIN, PWM_FREQ, 8);
  setBacklight(brightnessLevel);

  Wire.begin(I2C_SDA, I2C_SCL);
  delay(100);

  // I2C scan
  Serial.println("── I2C Scan ──");
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0)
      Serial.printf("  Found: 0x%02X\n", addr);
  }
  Serial.println("──────────────");

  // LCD init
  lcd.init();
  lcd.init();
  delay(50);
  lcd.backlight();
  lcd.clear();

  // RTC init
  if (!rtc.begin()) {
    Serial.println("RTC not found!");
    lcd.setCursor(0, 0);
    lcd.print("RTC Error!");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("WARNING: RTC lost power!");
    // time adjust করা হচ্ছে না — saved time রাখা হচ্ছে
  }

  Serial.println("Ready!");
  Serial.println("SET:HH,MM,SS,DD,MON,YYYY  →  set time");
  Serial.println("1-1000                     →  brightness");

  lcd.setCursor(0, 0);
  lcd.print("RAHUL'S  CLOCK");
  lcd.setCursor(3, 1);
  lcd.print("ESP32  v3.0");
  delay(1800);
  lcd.clear();
}

void loop() {
  // Serial commands
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.startsWith("SET:")) {
      cmd.remove(0, 4);
      int h, m, s, d, mo, yr;
      sscanf(cmd.c_str(), "%d,%d,%d,%d,%d,%d", &h, &m, &s, &d, &mo, &yr);
      rtc.adjust(DateTime(yr, mo, d, h, m, s));
      Serial.println("RTC updated!");
    } else {
      int v = cmd.toInt();
      if (v >= 1 && v <= 1000) {
        brightnessLevel = v;
        setBacklight(v);
        Serial.printf("Brightness: %d/1000\n", v);
      }
    }
  }

  DateTime now = rtc.now();

  // LINE 0: "02:34 AM  |  28°C"
  // col:     0         9 10
  lcd.setCursor(0, 0);
  lcd.print(formatTime(now));        // col 0–7  "02:34 AM"
  lcd.setCursor(9, 0);
  lcd.print("|");                    // col 9  divider
  lcd.setCursor(11, 0);
  int temp = (int)rtc.getTemperature();
  if (temp < 10) lcd.print(" ");
  lcd.print(temp);
  lcd.print((char)223);              // ° symbol
  lcd.print("C ");

  // LINE 1: "14/05/26  |  Wed"
  lcd.setCursor(0, 1);
  lcd.print(formatDate(now));        // col 0–7  "14/05/26"
  lcd.setCursor(9, 1);
  lcd.print("|");                    // col 9  divider
  lcd.setCursor(11, 1);
  lcd.print(days[now.dayOfTheWeek()]);
  lcd.print("  ");

  delay(1000);
}
