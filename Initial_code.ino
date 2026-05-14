#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <DHT.h>

#define I2C_SDA       21
#define I2C_SCL       22
#define DHT_PIN        4
#define DHT_TYPE    DHT11
#define BACKLIGHT_PIN  5
#define PWM_FREQ    5000

int brightnessLevel = 1000;

LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;
DHT dht(DHT_PIN, DHT_TYPE);

const char* days[]   = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
const char* months[] = {"Jan","Feb","Mar","Apr","May","Jun",
                        "Jul","Aug","Sep","Oct","Nov","Dec"};

int scaleToPWM(int level) {
  if (level <= 1)    return 0;
  if (level >= 1000) return 255;
  return map(level, 1, 1000, 1, 255);
}

void setBacklight(int level) {
  ledcWrite(BACKLIGHT_PIN, scaleToPWM(level));
}

void setup() {
  Serial.begin(115200);
  delay(500);

  ledcAttach(BACKLIGHT_PIN, PWM_FREQ, 8);
  setBacklight(brightnessLevel);

  Wire.begin(I2C_SDA, I2C_SCL);
  delay(100);

  dht.begin();
  delay(1000);

  Serial.println("── I2C Scan ──");
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0)
      Serial.printf("  Found: 0x%02X\n", addr);
  }
  Serial.println("──────────────");

  lcd.init();
  lcd.init();
  delay(50);
  lcd.backlight();
  lcd.clear();

  if (!rtc.begin()) {
    Serial.println("RTC not found!");
    lcd.setCursor(0, 0);
    lcd.print("RTC Error!");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("WARNING: RTC lost power!");
  }

  Serial.println("Ready!");
  Serial.println("SET:HH,MM,SS,DD,MON,YYYY  → set time");
  Serial.println("1-1000                     → brightness");

  lcd.setCursor(0, 0);
  lcd.print("RAHUL'S  CLOCK");
  lcd.setCursor(3, 1);
  lcd.print("ESP32  v4.0");
  delay(1800);
  lcd.clear();
}

float cachedTemp     = 0.0;
float cachedHumidity = 0.0;
unsigned long lastDHTRead = 0;

void updateDHT() {
  unsigned long nowMs = millis();
  if (nowMs - lastDHTRead < 2000) return;
  lastDHTRead = nowMs;

  float t  = dht.readTemperature();
  float hh = dht.readHumidity();

  if (!isnan(t))  cachedTemp     = t;
  if (!isnan(hh)) cachedHumidity = hh;
}

void loop() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.startsWith("SET:")) {
      cmd.remove(0, 4);
      int rH, rM, rS, rD, rMo, rYr;
      sscanf(cmd.c_str(), "%d,%d,%d,%d,%d,%d", &rH, &rM, &rS, &rD, &rMo, &rYr);
      rtc.adjust(DateTime(rYr, rMo, rD, rH, rM, rS));
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

  updateDHT();

  DateTime rtcNow  = rtc.now();
  bool showColon   = (rtcNow.second() % 2 == 0);

  int dispTemp     = (int)cachedTemp;
  int dispHumidity = (int)cachedHumidity;

  // ── AM/PM logic ──
  int hour24  = rtcNow.hour();
  String ampm = (hour24 < 12) ? "AM" : "PM";
  int hour12  = hour24 % 12;
  if (hour12 == 0) hour12 = 12;

  String sep  = showColon ? ":" : " ";
  String hStr = (hour12 < 10) ? "0" + String(hour12) : String(hour12);
  String mStr = (rtcNow.minute() < 10) ? "0" + String(rtcNow.minute()) : String(rtcNow.minute());
  String sStr = (rtcNow.second() < 10) ? "0" + String(rtcNow.second()) : String(rtcNow.second());

  // ── LINE 0: "05:09:23AM|32°C" ──
  // col 0-1  : hour  "05"
  // col 2    : sep   ":"
  // col 3-4  : min   "09"
  // col 5    : sep   ":"
  // col 6-7  : sec   "23"
  // col 8-9  : AM/PM
  // col 10   : |
  // col 11   : space
  // col 12-15: "32°C"

  lcd.setCursor(0, 0);
  lcd.print(hStr + sep + mStr + " "+  ampm);  // 10 chars

  lcd.setCursor(9, 0);
  lcd.print(" T=");

  if (dispTemp < 10) lcd.print(" ");
  lcd.print(dispTemp);
  lcd.print((char)223);   // °
  lcd.print("C");

  // ── LINE 1: "Thu,May 15  69%" ──
  // col 0-2  : day   "Thu"
  // col 3    : ","
  // col 4-6  : month "May"
  // col 7    : space
  // col 8-9  : date  "15"
  // col 10-11: "  "  padding
  // col 12-15: "69%" or " 9%"

  String dowStr  = String(days[rtcNow.dayOfTheWeek()]);
  String monStr  = String(months[rtcNow.month() - 1]);
  String dayDStr = (rtcNow.day() < 10) ? "0" + String(rtcNow.day()) : String(rtcNow.day());

  lcd.setCursor(0, 1);
  lcd.print(dowStr);       // "Thu"
  lcd.print(",");          // ","
  lcd.print(monStr);       // "May"
  //lcd.print(" ");
  lcd.print(dayDStr);      // "15"
  lcd.print("  ");         // padding to col 12
  lcd.print("H=");

  if (dispHumidity < 10) lcd.print(" ");
  lcd.print(dispHumidity);
  lcd.print("%");

  Serial.printf("Time: %s:%s:%s %s | Temp: %d°C | %s,%s %s | Hum: %d%%\n",
                hStr.c_str(), mStr.c_str(), sStr.c_str(), ampm.c_str(),
                dispTemp,
                dowStr.c_str(), monStr.c_str(), dayDStr.c_str(),
                dispHumidity);

  delay(1000);
}
