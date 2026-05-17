#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <DHT.h>
#include <BluetoothSerial.h>
#include <EEPROM.h>

#define I2C_SDA       21
#define I2C_SCL       22
#define DHT_PIN        4
#define DHT_TYPE    DHT11
#define BACKLIGHT_PIN  5
#define PWM_FREQ    5000

// Button pins
#define MODE_SELECT_PIN     32
#define SET_CONFIRM_PIN     33
#define INCREASE_PIN        25
#define DECREASE_PIN        26
#define DISCARD_PIN         27
#define BUZZER_FREQ       1200
// Buzzer pin
#define BUZZER_PIN          12

#define CLICK_BEEP_FREQ   1200
#define CLICK_BEEP_MS       80

#define LONG_PRESS_MS     1000

// ── FIX 1: Separate debounce delays ──────────────────────────────────────────
// Standard debounce for MODE_SELECT, SET_CONFIRM, DISCARD
const unsigned long debounceDelay     = 200;
// Faster debounce for INCREASE / DECREASE (nav / value-change buttons)
const unsigned long incDecDebounce    = 120;

// Last-press timestamps per button
unsigned long lastButtonPress         = 0;   // MODE_SELECT / SET_CONFIRM / DISCARD
unsigned long lastIncPress            = 0;   // INCREASE
unsigned long lastDecPress            = 0;   // DECREASE

// ── FIX 5: Auto-brightness variables (easy to change) ────────────────────────
int  nightBrightness    = 300;   // brightness from nightStartHour to morningEndHour
int  dayBrightness      = 1000;  // brightness during the rest of the day
int  nightStartHour     = 21;    // 9 PM  (24-hr)
int  morningEndHour     = 6;     // 6 AM  (24-hr, exclusive — i.e. < 6 = night)
bool autoBrightnessEnabled = true;

// ── FIX 6: Tick-tock beep variables ──────────────────────────────────────────
// Mechanical clock tic-tic sound:
//   "tic" = slightly higher freq, short
//   "toc" = slightly lower freq, shorter gap
int  tickFreq   = 1100;   // Hz  – "tic" tone
int  tockFreq   =  950;   // Hz  – "toc" tone
int  tickDurMs  =   38;   // ms  – "tic" duration  (snappy)
int  tockDurMs  =   28;   // ms  – "toc" duration  (softer)
bool tickTurn   = true;   // alternates tick / tock each second

int brightnessLevel = 500;
bool buzzerEnabled = true;

float cachedTemp     = 0.0;
float cachedHumidity = 0.0;
unsigned long lastDHTRead = 0;

String customMessage = "";
String customMessageLine2 = "";
int messageStyle = 0;
unsigned long lastAnimationUpdate = 0;
int animationPosition = 0;
String scrollText = "";
bool blinkState = true;
unsigned long lastBlinkUpdate = 0;
int animationSpeed = 500;
int textSpeed = 150;
bool systemOn = true;
int cursorPos = 0;
int cursorPos2 = 0;
int charCount = 0;
int charCount2 = 0;
unsigned long lastCharTime = 0;
unsigned long lastCharTime2 = 0;
bool animationCompleted = false;
bool line1Completed = false;
bool line2Completed = false;
int animationStage = 0;

bool motivationMode = false;
int motivationStyle = 0;
unsigned long lastMotivationChange = 0;
int currentMotivationIndex = 0;

enum SystemMode { CLOCK_MODE, CLOCK2_MODE, ALARM_MODE, TIMER_MODE, MOTIVATION_MODE, STUDY_MODE };
SystemMode currentMode = CLOCK_MODE;
bool showModeSelectMessage = false;
unsigned long modeMessageStartTime = 0;

bool clock2Running = false;

// CLOCK2 per-second beep tracking
int lastClock2Second = -1;

// ── FIX 4: EEPROM layout ─────────────────────────────────────────────────────
// Addr 0: magic byte (0xA5 = alarm saved)
// Addr 1: alarmHour
// Addr 2: alarmMinute
// Addr 3: alarmSecond
// Addr 4: alarmActive (0/1)
#define EEPROM_MAGIC      0xA5
#define EEPROM_ADDR_MAGIC    0
#define EEPROM_ADDR_HOUR     1
#define EEPROM_ADDR_MIN      2
#define EEPROM_ADDR_SEC      3
#define EEPROM_ADDR_ACTIVE   4
#define EEPROM_SIZE          8

bool alarmActive = false;
int alarmHour = 8;
int alarmMinute = 0;
int alarmSecond = 0;
bool alarmTriggered = false;
unsigned long lastAlarmBeep = 0;
bool alarmBlinkState = false;
unsigned long lastAlarmBlink = 0;
unsigned long lastAlarmSecondBeep = 0;
int lastDisplayAlarmSecond = -1;

bool timerRunning = false;
unsigned long timerStartTime = 0;
unsigned long timerRemaining = 0;
int timerHours = 0;
int timerMinutes = 1;
int timerSeconds = 0;
bool timerExpired = false;
unsigned long lastTimerBeep = 0;
bool timerBlinkState = false;
int lastDisplaySecond = -1;

// ── STUDY MODE VARIABLES ──────────────────────────────────────────────────────────────────────────────
int studyDurHours   = 0;
int studyDurMinutes = 25;

int breakIntervalMinutes = 25;
int breakDurationMinutes = 5;

bool studyRunning        = false;
bool studyExpired        = false;

unsigned long studyTotalMs        = 0;
unsigned long studyStartTime      = 0;
unsigned long studyElapsedMs      = 0;

unsigned long breakIntervalMs     = 0;
unsigned long breakDurationMs     = 0;

unsigned long nextBreakAt         = 0;
bool inBreak                      = false;
unsigned long breakStartTime      = 0;

bool studyBlinkState              = false;
unsigned long lastStudyBlink      = 0;

int lastStudyDisplaySecond        = -1;
int lastBreakDisplaySecond        = -1;
// ─────────────────────────────────────────────────────────────────────────────

unsigned long discardPressStart   = 0;
bool discardHeld                  = false;
bool longPressHandled             = false;

LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;
DHT dht(DHT_PIN, DHT_TYPE);
BluetoothSerial SerialBT;

const char* days[]    = {"SUN","MON","TUE","WED","THU","FRI","SAT"};
const char* months[]  = {"Jan","Feb","Mar","Apr","May","Jun",
                         "Jul","Aug","Sep","Oct","Nov","Dec"};
const char* months2[] = {"JAN","FEB","MAR","APR","MAY","JUN",
                         "JUL","AUG","SEP","OCT","NOV","DEC"};

const char* motivationMessages[] = {
  "focus on logic@build the future",
  "crack the gate@reach the iit tag",
  "code your way@to the top tier",
  "master discrete@math for the win",
  "dream of iit@work for it now",
  "os concepts are@key to success",
  "solve the pyqs@rank will follow",
  "data structures@fix all tasks",
  "be a topper in@cs engineering",
  "algo design is@your best tool",
  "compilers are@logic engines",
  "dbms mastery is@within reach",
  "keep on coding@keep climbing",
  "gate exam 2027@is your big goal",
  "believe in the@cs power today",
  "revise daily@retain it weekly",
  "toc is hard@but you are too",
  "network layers@lead to glory",
  "stay calm now@solve the gate",
  "iit dreams are@made of grit",
  "digital logic@is very sharp",
  "paging is easy@if you study",
  "graphs lead to@the right path",
  "trees grow with@your efforts",
  "success is an@o1 operation",
  "your potential@has no limits",
  "aptitude is a@scoring zone",
  "logic gates@open iit doors",
  "stay hungry@stay a true guy",
  "rank under 100@is the target",
  "dont stop till@you reach iit",
  "math is the@base of all cs",
  "practice makes@you a topper",
  "study very hard@code smarter",
  "one byte at@time you win it",
  "recursion leads@to the peak",
  "sort your life@sort your rank",
  "binary search@for your goal",
  "hashing out@all the trouble",
  "cache your@knowledge well",
  "pointers point@to the trophy",
  "deadlock free@study mode now",
  "threads of@hard work pay",
  "sql queries@unlock the data",
  "be the master@of the automata",
  "context free@focus is the key",
  "p vs np is@a big mind game",
  "queue up for@the biggest win",
  "stack up marks@for today work",
  "linked lists@lead to success",
  "heaps of joy@await you at iit",
  "min cut for@all obstacles",
  "max flow for@your own energy",
  "be an expert@in the c coding",
  "linux kernels@are a cool art",
  "learn tcp for@the better link",
  "udp is fast@be just like that",
  "ip address of@success is iit",
  "routing your@way to glory",
  "bit by bit@you will win it",
  "boolean algebra@never lies",
  "k maps simplify@your own life",
  "mux your time@for the result",
  "pipeline your@study flow now",
  "no stalls in@your progress",
  "branch out to@the new height",
  "risc it all@for the dream",
  "cisc is big@just stay focused",
  "memory manage@is very vital",
  "virtual ram@for real success",
  "semaphores for@clear paths",
  "mutex for your@focus time now",
  "deadly focus@no deadlocks",
  "b trees help@you to search",
  "b plus trees@for dense ranks",
  "normalise your@study plan now",
  "1nf to bcnf@know it all well",
  "acid traits@in your own work",
  "greedy algos@for the marks",
  "dynamic coding@for the win",
  "divide and@conquer the gate",
  "backtrack to@fix the error",
  "complexity is@just a number",
  "big o of n@is the target",
  "stay linear@stay focused",
  "exponentially@grow every day",
  "log time is@the best time",
  "np hard is@not really hard",
  "sat solvers@fix the gate",
  "grammar leads@to languages",
  "pushdown your@fears today",
  "turing machine@of hard work",
  "decide to be@the best one",
  "halting is@not an option",
  "learn to code@code to learn",
  "cs is the@future of all",
  "gate is just@one step away",
  "iit bombay is@now calling",
  "iit delhi is@your vision",
  "iit madras is@the target",
  "iit kanpur@awaits you now",
  "iit kharagpur@is the goal",
  "iit roorkee@awaits logic",
  "iit guwahati@looks so great",
  "iisc is the@ultimate aim",
  "research is@your passion",
  "coding is your@super power",
  "stay humble@work much harder",
  "silence your@doubts now",
  "errors lead@to perfection",
  "debug your@own life path",
  "compile your@strength now",
  "run the code@of success",
  "execute your@plans well",
  "input effort@output iit seat",
  "no bugs in@your strategy",
  "logic is the@only weapon",
  "maths is the@queen of gate",
  "probability@of winning one",
  "stats prove@you are good",
  "matrices are@easy points",
  "eigen values@of success",
  "calculus for@steady growth",
  "groups and@rings of power",
  "graph theory@is beautiful",
  "master bayes@for the gate",
  "expectation@is to top gate",
  "variance in@study is bad",
  "stay constant@stay very ready",
  "uniformly work@every day",
  "normalise the@pressure now",
  "poisson flow@of the wins",
  "binomial luck@comes to all",
  "set theory@is the start",
  "functions are@your friends",
  "relations last@long in gate",
  "lattices and@posets win it",
  "groups define@your circle",
  "be a real pro@in proposition",
  "predicates@are true for you",
  "quantify your@hard effort",
  "validity is@your strength",
  "satisfy all@the gate rules",
  "complexity@of dreams high",
  "paging the@future today",
  "segmentation@of your goals",
  "dirty bits@in your prep",
  "write back@your success",
  "write through@the struggle",
  "tlb miss is@just a slip",
  "hit ratio of@ninety percent",
  "locality of@focus is key",
  "temporal grit@spacial aim",
  "bus width@of your brain",
  "clock speed@of your logic",
  "cycles of@constant study",
  "fetch the@greatest rank",
  "decode the@gate pattern",
  "alu of your@mind is fast",
  "registers@of memory stay",
  "direct mapped@to the goal",
  "set associative@winning",
  "fully linked@to the dream",
  "page faults@teach lessons",
  "thrashing is@not allowed",
  "working set@of winners",
  "beladys luck@will not stop",
  "optimal prep@leads to iit",
  "lru means@learn recent",
  "fifo for@all your tasks",
  "sjf for your@short goals",
  "round robin@your subjects",
  "priority is@always gate",
  "multi level@success plan",
  "deadlock is@for the weak",
  "bankers logic@keeps safety",
  "safety state@is iit seat",
  "avoid all@the wrong paths",
  "prevent any@lazy habits",
  "detection@of weak areas",
  "recovery@from failures",
  "disk space@for big ideas",
  "raid your@way to glory",
  "striping@the competition",
  "mirrored@in your efforts",
  "checksum for@your growth",
  "parity with@the toppers",
  "coding theory@is so fun",
  "hamming way@to perfection",
  "distance@to iit is short",
  "error free@mindset now",
  "cycles of@constant win",
  "paths lead@to the summit",
  "adjacency@to greatness",
  "degree of@success is one",
  "connected@to your dreams",
  "isomorphic@to a winner",
  "planar graphs@of progress",
  "euler path@to the goal",
  "hamiltonian@strength now",
  "cliques of@top engineers",
  "cover all@the syllabus",
  "matching@your ambition",
  "flow through@the hurdles",
  "source is@your hard work",
  "sink is the@final iit",
  "reach for@the top rank",
  "failures are@the beta test",
  "sleep is@for non gate guy",
  "coffee runs@in my veins",
  "social life@segment fault",
  "iit tag or@just a rag",
  "rank decides@your worth",
  "dont cry@just dry run",
  "logic high@feelings low",
  "love is a@recursive trap",
  "no gf only@bf best friend",
  "study now@flex later on",
  "job is safe@but iit is fire",
  "be the zero@one percent guy",
  "pain is just@a data input",
  "cry in jaguar@not in bus",
  "iit air@feels different",
  "society@wants your rank",
  "parents want@that iit tag",
  "neighbors envy@is my goal",
  "ex gf will@regret soon",
  "burn the@midnight oil",
  "grind now@shine forever",
  "gate is a@battlefield",
  "be the king@of core stuff",
  "tech is@the new gold",
  "silicon valley@calls you",
  "start up is@a post iit",
  "think like@a machine",
  "be cold as@a processor",
  "heart is just@a pump chill",
  "emotions@are run errors",
  "hard work@beats genius",
  "no shortcut@to the top",
  "stairs to@iit are steep",
  "sweat in peace@win the war",
  "focus is@my superpower",
  "discipline@is the key",
  "consistent@is the word",
  "beat all@the odds today",
  "impossible@is logic error",
  "win the race@be the ace",
  "top rank@or nothing now",
  "eat sleep@gate repeat",
  "library is@my second home",
  "books are@my only bae",
  "pen is my@only weapon",
  "notes are@my treasure",
  "summary@is not enough",
  "deep dive@into syllabus",
  "zero days@off allowed",
  "one goal@and one vision",
  "eyes on the@the iit gate",
  "push limits@every day",
  "break barriers@not hearts",
  "silence is@the best noise",
  "success@will make a roar",
  "prove them@all wrong now",
  "watch me@reach the top",
  "history@is made by us",
  "be a legend@in the cs field",
  "future ceo@in the making",
  "code the@world better",
  "innovate or@get deleted",
  "stay hungry@stay foolish",
  "vision of@a top topper",
  "mindset of@a conqueror",
  "born to@crack the gate",
  "destined@for iit glory",
  "master of@all subjects",
  "king of the@the cs kingdom",
  "rule the@tech world",
  "binary@is my language",
  "hex life@is a rich life",
  "logic over@everything",
  "reasoning@is my strength",
  "puzzle solver@by birth",
  "born for@the engineer",
  "building@the next thing",
  "dream big@act even bigger",
  "small steps@for big impact",
  "giant leap@for my career",
  "path to@the iit is clear",
  "walk the@talk every day",
  "no excuses@only results",
  "kill the@laziness now",
  "destroy@distractions",
  "focus like@a laser beam",
  "sharp mind@and sharp rank",
  "bright future@awaits you",
  "be the light@of your home",
  "pride of@the college",
  "star of@the family tree",
  "legacy starts@with the gate",
  "iit is@just beginning",
  "life begins@at iit gates",
  "magic happens@in the lab",
  "creation@is my passion",
  "art of@problem solving",
  "science@of winning life",
  "philosophy@of a topper",
  "way of the@brave warrior",
  "code of@conduct to win",
  "protocol@for success",
  "standards@are set high",
  "quality@over quantity",
  "precision@is everything",
  "accuracy@is the aim",
  "speed is@the only edge",
  "efficiency@is the goal",
  "optimize@your life now",
  "refactor@your habits",
  "upgrade@your circle",
  "system update@is required",
  "reboot@your spirit now",
  "full charge@mode is on",
  "unlimited@true potential",
  "infinite@possibilities",
  "end game@is iit madras",
  "final boss@is gate exam",
  "level up@every week",
  "xp gain@from every mock",
  "skill points@on the maths",
  "inventory@of formulae",
  "quest for@the iit tag",
  "mission@is accomplished",
  "victory@is my destiny",
  "crown of@a gate topper",
  "glory is@truly eternal",
  "hard work@will never fade",
  "legendary@status is aim",
  "epic win@in the feb exam",
  "coolest geek@in the town",
  "sartorial@the iit hoodie",
  "brand of@an iit student",
  "worth of@a top scholar",
  "value of@the discipline",
  "price of@the greatness",
  "cost of@the mediocrity",
  "avoid the@average life",
  "be the@extraordinary",
  "phenomenal@rank is coming",
  "stellar@performance",
  "galactic@huge ambition",
  "universal@recognition",
  "beyond@the far horizon",
  "sky is@not the limit",
  "iit and@far beyond gate",
  "future@is in your hands",
  "write your@own story now",
  "be the@true protagonist",
  "hero of@your journey",
  "success@is coming home",
  "iit is@calling you now",
  "wait for@the result day",
  "tears of@joy very soon",
  "smile of@a winner now",
  "peace at@the highest peak",
  "the end@of the grind",
  "start of@a new life",
  "welcome@to the iit life"
};

const int totalMessages = sizeof(motivationMessages) / sizeof(motivationMessages[0]);

// ── EEPROM HELPERS ────────────────────────────────────────────────────────────

void saveAlarmToEEPROM() {
  EEPROM.write(EEPROM_ADDR_MAGIC,  EEPROM_MAGIC);
  EEPROM.write(EEPROM_ADDR_HOUR,   alarmHour);
  EEPROM.write(EEPROM_ADDR_MIN,    alarmMinute);
  EEPROM.write(EEPROM_ADDR_SEC,    alarmSecond);
  EEPROM.write(EEPROM_ADDR_ACTIVE, alarmActive ? 1 : 0);
  EEPROM.commit();
}

void loadAlarmFromEEPROM() {
  if (EEPROM.read(EEPROM_ADDR_MAGIC) == EEPROM_MAGIC) {
    alarmHour   = EEPROM.read(EEPROM_ADDR_HOUR);
    alarmMinute = EEPROM.read(EEPROM_ADDR_MIN);
    alarmSecond = EEPROM.read(EEPROM_ADDR_SEC);
    alarmActive = (EEPROM.read(EEPROM_ADDR_ACTIVE) == 1);
    alarmTriggered = false;
    Serial.println("→ Alarm loaded from EEPROM");
  } else {
    Serial.println("→ No alarm in EEPROM");
  }
}

void clearAlarmFromEEPROM() {
  EEPROM.write(EEPROM_ADDR_MAGIC,  0x00);
  EEPROM.write(EEPROM_ADDR_ACTIVE, 0);
  EEPROM.commit();
}

// ── FIX 5: Auto-brightness apply ─────────────────────────────────────────────

void applyAutoBrightness() {
  if (!autoBrightnessEnabled || !systemOn) return;
  DateTime now = rtc.now();
  int h = now.hour();
  // Night = nightStartHour(21) <= h  OR  h < morningEndHour(6)
  bool isNight = (h >= nightStartHour) || (h < morningEndHour);
  int target = isNight ? nightBrightness : dayBrightness;
  if (brightnessLevel != target) {
    brightnessLevel = target;
    setBacklight(brightnessLevel);
  }
}

// ── HELPER FUNCTIONS ──────────────────────────────────────────────────────────

int scaleToPWM(int level) {
  if (level <= 1)    return 0;
  if (level >= 1000) return 255;
  return map(level, 1, 1000, 1, 255);
}

void setBacklight(int level) {
  if (!systemOn) level = 0;
  ledcWrite(BACKLIGHT_PIN, scaleToPWM(level));
}

void updateSpeed() {
  textSpeed = map(animationSpeed, 1, 1000, 2000, 10);
  if (textSpeed < 5)    textSpeed = 5;
  if (textSpeed > 2000) textSpeed = 2000;
}

void splitMessage(String msg) {
  int atPos = msg.indexOf('@');
  if (atPos != -1) {
    customMessage = msg.substring(0, atPos);
    customMessageLine2 = msg.substring(atPos + 1);
    if (customMessage.length() > 16)      customMessage      = customMessage.substring(0, 16);
    if (customMessageLine2.length() > 16) customMessageLine2 = customMessageLine2.substring(0, 16);
  } else {
    customMessage = msg;
    customMessageLine2 = "";
    if (customMessage.length() > 16) customMessage = customMessage.substring(0, 16);
  }
}

// ── BUZZER FUNCTIONS ──────────────────────────────────────────────────────────

void buttonClickBeep() {
  if (!buzzerEnabled) return;
  tone(BUZZER_PIN, CLICK_BEEP_FREQ);
  delay(CLICK_BEEP_MS);
  noTone(BUZZER_PIN);
}

// ── FIX 6: Mechanical tick-tock beep ─────────────────────────────────────────
// Called every second in place of the old buzzerShortBeep() for timer/study/clock2/alarm countdown
void tickTockBeep() {
  if (!buzzerEnabled) return;
  if (tickTurn) {
    tone(BUZZER_PIN, tickFreq);
    delay(tickDurMs);
    noTone(BUZZER_PIN);
  } else {
    tone(BUZZER_PIN, tockFreq);
    delay(tockDurMs);
    noTone(BUZZER_PIN);
  }
  tickTurn = !tickTurn;   // alternate each call
}

void buzzerBeep() {
  if (!buzzerEnabled) return;
  tone(BUZZER_PIN, BUZZER_FREQ);
  delay(100);
  noTone(BUZZER_PIN);
}

void buzzerContinuousStart() {
  if (!buzzerEnabled) return;
  tone(BUZZER_PIN, BUZZER_FREQ);
}

void buzzerContinuousStop() {
  noTone(BUZZER_PIN);
}

void buzzer10SecBeep() {
  if (!buzzerEnabled) return;
  tone(BUZZER_PIN, BUZZER_FREQ);
  unsigned long start = millis();
  while (millis() - start < 10000) delay(10);
  noTone(BUZZER_PIN);
}

// ── CLOCK2 DISPLAY ────────────────────────────────────────────────────────────
void displayClock2() {
  DateTime rtcNow = rtc.now();
  bool showColon = ((millis() % 1000) < 500);
  char sep = showColon ? ':' : ' ';

  int hour24 = rtcNow.hour();
  const char* ampm = (hour24 < 12) ? "AM" : "PM";
  int hour12 = hour24 % 12;
  if (hour12 == 0) hour12 = 12;

  int dispTemp     = (int)cachedTemp;
  int dispHumidity = (int)cachedHumidity;

  // Per-second tick-tock in CLOCK2
  int curSec = rtcNow.second();
  if (curSec != lastClock2Second) {
    lastClock2Second = curSec;
    tickTockBeep();   // FIX 6
  }

  // LINE 0: "  12:34:56 AM   "
  lcd.setCursor(0, 0);
  char line0[17];
  sprintf(line0, "  %02d%c%02d%c%02d %s   ",
          hour12, sep, rtcNow.minute(), sep, rtcNow.second(), ampm);
  lcd.print(line0);

  // LINE 1: "MAY17 SUN 89%30°"
  lcd.setCursor(0, 1);
  char humStr[5], tempStr[5];
  if (dispHumidity < 10) sprintf(humStr, " %d%%", dispHumidity);
  else                   sprintf(humStr, "%d%%",  dispHumidity);
  if (dispTemp < 10) sprintf(tempStr, " %d%c", dispTemp, (char)223);
  else               sprintf(tempStr, "%d%c",  dispTemp, (char)223);

  char line1[17];
  sprintf(line1, "%s%02d %s %s%s",
          months2[rtcNow.month() - 1],
          rtcNow.day(),
          days[rtcNow.dayOfTheWeek()],
          humStr,
          tempStr);
  lcd.print(line1);
}

// ── QUICK SETTINGS MENU ───────────────────────────────────────────────────────

void quickSettingsMenu() {
  buttonClickBeep();

  int selectedOption = 0;
  bool menuActive = true;
  unsigned long lastNav = millis();
  const unsigned long navDelay = 200;

  lcd.clear();

  while (menuActive) {
    lcd.setCursor(0, 0);
    lcd.print(selectedOption == 0 ? ">1.BRIGHTNESS   " : " 1.BRIGHTNESS   ");
    lcd.setCursor(0, 1);
    lcd.print(selectedOption == 1 ? ">2.BUZZER       " : " 2.BUZZER       ");

    if (millis() - lastNav > navDelay) {
      if (digitalRead(INCREASE_PIN) == LOW || digitalRead(DECREASE_PIN) == LOW) {
        lastNav = millis(); buttonClickBeep();
        selectedOption = 1 - selectedOption;
      }
      if (digitalRead(SET_CONFIRM_PIN) == LOW) {
        lastNav = millis(); buttonClickBeep(); menuActive = false;
      }
      if (digitalRead(DISCARD_PIN) == LOW) {
        lastNav = millis(); buttonClickBeep(); lcd.clear(); return;
      }
    }
    delay(20);
  }

  lcd.clear();

  if (selectedOption == 0) {
    bool brightnessActive = true;
    unsigned long lastBtnTime = millis();
    const unsigned long btnDelay = 150;

    while (brightnessActive) {
      lcd.setCursor(0, 0);
      lcd.print("BRIGHTNESS ADJ  ");
      lcd.setCursor(0, 1);
      int barLen = map(brightnessLevel, 0, 1000, 0, 10);
      lcd.print("[");
      for (int i = 0; i < 10; i++) lcd.print(i < barLen ? (char)255 : ' ');
      lcd.print("]");
      char bBuf[5];
      sprintf(bBuf, "%4d", brightnessLevel);
      lcd.print(bBuf);

      if (millis() - lastBtnTime > btnDelay) {
        if (digitalRead(INCREASE_PIN) == LOW) {
          lastBtnTime = millis(); buttonClickBeep();
          brightnessLevel += 50;
          if (brightnessLevel > 1000) brightnessLevel = 1000;
          if (systemOn) setBacklight(brightnessLevel);
        }
        if (digitalRead(DECREASE_PIN) == LOW) {
          lastBtnTime = millis(); buttonClickBeep();
          brightnessLevel -= 50;
          if (brightnessLevel < 0) brightnessLevel = 0;
          if (systemOn) setBacklight(brightnessLevel);
        }
        if (digitalRead(SET_CONFIRM_PIN) == LOW || digitalRead(DISCARD_PIN) == LOW) {
          lastBtnTime = millis(); buttonClickBeep(); brightnessActive = false;
        }
      }
      delay(20);
    }
  } else {
    bool buzzerMenuActive = true;
    unsigned long lastBtnTime = millis();
    const unsigned long btnDelay = 200;

    while (buzzerMenuActive) {
      lcd.setCursor(0, 0);
      lcd.print("BUZZER TOGGLE   ");
      lcd.setCursor(0, 1);
      if (buzzerEnabled) lcd.print("STATUS:  ON  >  ");
      else               lcd.print("STATUS:  OFF >  ");

      if (millis() - lastBtnTime > btnDelay) {
        if (digitalRead(INCREASE_PIN) == LOW || digitalRead(DECREASE_PIN) == LOW) {
          lastBtnTime = millis();
          buzzerEnabled = !buzzerEnabled;
          if (buzzerEnabled) {
            tone(BUZZER_PIN, CLICK_BEEP_FREQ);
            delay(CLICK_BEEP_MS * 2);
            noTone(BUZZER_PIN);
          }
          Serial.printf("→ Buzzer: %s\n", buzzerEnabled ? "ON" : "OFF");
          SerialBT.printf("→ Buzzer: %s\n", buzzerEnabled ? "ON" : "OFF");
        }
        if (digitalRead(SET_CONFIRM_PIN) == LOW || digitalRead(DISCARD_PIN) == LOW) {
          lastBtnTime = millis(); buttonClickBeep(); buzzerMenuActive = false;
        }
      }
      delay(20);
    }
  }

  lcd.clear();
}

// ── MOTIVATION MODE ───────────────────────────────────────────────────────────

void startMotivationMode(int style) {
  motivationMode = true;
  motivationStyle = style;
  messageStyle = style;
  currentMotivationIndex = random(0, totalMessages);
  lastMotivationChange = millis();

  animationPosition = 0;
  cursorPos = 0; cursorPos2 = 0;
  charCount = 0; charCount2 = 0;
  animationCompleted = false;
  line1Completed = false; line2Completed = false;
  animationStage = 0;

  String msg = String(motivationMessages[currentMotivationIndex]);
  splitMessage(msg);

  if (style == 3) {
    customMessage = msg;
    customMessageLine2 = "";
    if (customMessage.indexOf('@') != -1) customMessage.replace("@", "");
    scrollText = customMessage + "    ";
  }

  lcd.clear();
}

void changeMotivationMessage() {
  currentMotivationIndex = random(0, totalMessages);
  String msg = String(motivationMessages[currentMotivationIndex]);
  splitMessage(msg);

  animationPosition = 0;
  cursorPos = 0; cursorPos2 = 0;
  charCount = 0; charCount2 = 0;
  animationCompleted = false;
  line1Completed = false; line2Completed = false;
  animationStage = 0;

  if (motivationStyle == 3) {
    customMessage = msg;
    customMessageLine2 = "";
    if (customMessage.indexOf('@') != -1) customMessage.replace("@", "");
    scrollText = customMessage + "    ";
  }

  lcd.clear();
}

// ── TEXT DISPLAY STYLES ───────────────────────────────────────────────────────

void displayStyle1() {
  lcd.setCursor(0, 0);
  lcd.print(customMessage);
  for (int i = customMessage.length(); i < 16; i++) lcd.print(" ");
  lcd.setCursor(0, 1);
  if (customMessageLine2.length() > 0) {
    lcd.print(customMessageLine2);
    for (int i = customMessageLine2.length(); i < 16; i++) lcd.print(" ");
  } else {
    lcd.print("                ");
  }
}

void displayStyle2() {
  int spaces1 = (16 - customMessage.length()) / 2;
  if (spaces1 < 0) spaces1 = 0;
  lcd.setCursor(spaces1, 0);
  lcd.print(customMessage);
  for (int i = customMessage.length(); i < 16; i++) lcd.print(" ");
  lcd.setCursor(0, 1);
  if (customMessageLine2.length() > 0) {
    int spaces2 = (16 - customMessageLine2.length()) / 2;
    if (spaces2 < 0) spaces2 = 0;
    lcd.setCursor(spaces2, 1);
    lcd.print(customMessageLine2);
    for (int i = customMessageLine2.length(); i < 16; i++) lcd.print(" ");
  } else {
    lcd.print("                ");
  }
}

void displayStyle3() {
  lcd.setCursor(0, 0);
  String displayMsg = scrollText.substring(animationPosition, animationPosition + 16);
  if (displayMsg.length() < 16) displayMsg += scrollText.substring(0, 16 - displayMsg.length());
  lcd.print(displayMsg);
  lcd.setCursor(0, 1);
  lcd.print("                ");
  if (millis() - lastAnimationUpdate > textSpeed) {
    lastAnimationUpdate = millis();
    animationPosition++;
    if (animationPosition >= (int)scrollText.length()) animationPosition = 0;
  }
}

void displayStyle4() {
  if (millis() - lastBlinkUpdate > textSpeed) {
    lastBlinkUpdate = millis();
    blinkState = !blinkState;
  }
  if (blinkState) {
    int spaces1 = (16 - customMessage.length()) / 2;
    if (spaces1 < 0) spaces1 = 0;
    lcd.setCursor(spaces1, 0);
    lcd.print(customMessage);
    for (int i = customMessage.length(); i < 16; i++) lcd.print(" ");
    lcd.setCursor(0, 1);
    if (customMessageLine2.length() > 0) {
      int spaces2 = (16 - customMessageLine2.length()) / 2;
      if (spaces2 < 0) spaces2 = 0;
      lcd.setCursor(spaces2, 1);
      lcd.print(customMessageLine2);
      for (int i = customMessageLine2.length(); i < 16; i++) lcd.print(" ");
    } else {
      lcd.print("                ");
    }
  } else {
    lcd.setCursor(0, 0); lcd.print("                ");
    lcd.setCursor(0, 1); lcd.print("                ");
  }
}

void displayStyle5() {
  if (!animationCompleted) {
    if (animationStage == 0) {
      if (millis() - lastAnimationUpdate > textSpeed) {
        lastAnimationUpdate = millis();
        if (cursorPos <= (int)customMessage.length()) cursorPos++;
        if (cursorPos > (int)customMessage.length()) {
          line1Completed = true;
          animationStage = 1;
          cursorPos2 = 0;
          lastAnimationUpdate = millis();
        }
      }
      int spaces1 = (16 - customMessage.length()) / 2;
      if (spaces1 < 0) spaces1 = 0;
      lcd.setCursor(spaces1, 0);
      if (cursorPos <= (int)customMessage.length()) {
        lcd.print(customMessage.substring(0, cursorPos));
        if (cursorPos < (int)customMessage.length()) {
          lcd.setCursor(spaces1 + cursorPos, 0);
          lcd.print("_");
        }
      }
      for (int i = spaces1 + cursorPos + 1; i < 16; i++) lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print("                ");
    } else if (animationStage == 1 && customMessageLine2.length() > 0) {
      if (millis() - lastAnimationUpdate > textSpeed) {
        lastAnimationUpdate = millis();
        if (cursorPos2 <= (int)customMessageLine2.length()) cursorPos2++;
        if (cursorPos2 > (int)customMessageLine2.length()) {
          line2Completed = true;
          animationCompleted = true;
        }
      }
      int spaces1 = (16 - customMessage.length()) / 2;
      if (spaces1 < 0) spaces1 = 0;
      lcd.setCursor(spaces1, 0);
      lcd.print(customMessage);
      for (int i = customMessage.length(); i < 16; i++) lcd.print(" ");
      int spaces2 = (16 - customMessageLine2.length()) / 2;
      if (spaces2 < 0) spaces2 = 0;
      lcd.setCursor(spaces2, 1);
      if (cursorPos2 <= (int)customMessageLine2.length()) {
        lcd.print(customMessageLine2.substring(0, cursorPos2));
        if (cursorPos2 < (int)customMessageLine2.length()) {
          lcd.setCursor(spaces2 + cursorPos2, 1);
          lcd.print("_");
        }
      }
      for (int i = spaces2 + cursorPos2 + 1; i < 16; i++) lcd.print(" ");
    } else if (animationStage == 1 && customMessageLine2.length() == 0) {
      animationCompleted = true;
    }
  } else {
    int spaces1 = (16 - customMessage.length()) / 2;
    if (spaces1 < 0) spaces1 = 0;
    lcd.setCursor(spaces1, 0);
    lcd.print(customMessage);
    for (int i = customMessage.length(); i < 16; i++) lcd.print(" ");
    lcd.setCursor(0, 1);
    if (customMessageLine2.length() > 0) {
      int spaces2 = (16 - customMessageLine2.length()) / 2;
      if (spaces2 < 0) spaces2 = 0;
      lcd.setCursor(spaces2, 1);
      lcd.print(customMessageLine2);
      for (int i = customMessageLine2.length(); i < 16; i++) lcd.print(" ");
    } else {
      lcd.print("                ");
    }
  }
}

void displayStyle6() {
  if (!animationCompleted) {
    if (animationStage == 0) {
      if (millis() - lastCharTime > textSpeed && charCount < (int)customMessage.length()) {
        lastCharTime = millis();
        charCount++;
      }
      if (charCount >= (int)customMessage.length()) {
        line1Completed = true;
        animationStage = 1;
        charCount2 = 0;
        lastCharTime = millis();
      }
      int spaces1 = (16 - customMessage.length()) / 2;
      if (spaces1 < 0) spaces1 = 0;
      lcd.setCursor(spaces1, 0);
      String displayMsg = customMessage.substring(0, charCount);
      lcd.print(displayMsg);
      for (int i = displayMsg.length(); i < (int)customMessage.length(); i++) lcd.print("_");
      for (int i = spaces1 + customMessage.length(); i < 16; i++) lcd.print(" ");
      lcd.setCursor(0, 1); lcd.print("                ");
    } else if (animationStage == 1 && customMessageLine2.length() > 0) {
      if (millis() - lastCharTime > textSpeed && charCount2 < (int)customMessageLine2.length()) {
        lastCharTime = millis();
        charCount2++;
      }
      if (charCount2 >= (int)customMessageLine2.length()) {
        line2Completed = true;
        animationCompleted = true;
      }
      int spaces1 = (16 - customMessage.length()) / 2;
      if (spaces1 < 0) spaces1 = 0;
      lcd.setCursor(spaces1, 0);
      lcd.print(customMessage);
      for (int i = customMessage.length(); i < 16; i++) lcd.print(" ");
      int spaces2 = (16 - customMessageLine2.length()) / 2;
      if (spaces2 < 0) spaces2 = 0;
      lcd.setCursor(spaces2, 1);
      String displayMsg2 = customMessageLine2.substring(0, charCount2);
      lcd.print(displayMsg2);
      for (int i = displayMsg2.length(); i < (int)customMessageLine2.length(); i++) lcd.print("_");
      for (int i = spaces2 + customMessageLine2.length(); i < 16; i++) lcd.print(" ");
    } else if (animationStage == 1 && customMessageLine2.length() == 0) {
      animationCompleted = true;
    }
  } else {
    int spaces1 = (16 - customMessage.length()) / 2;
    if (spaces1 < 0) spaces1 = 0;
    lcd.setCursor(spaces1, 0);
    lcd.print(customMessage);
    for (int i = customMessage.length(); i < 16; i++) lcd.print(" ");
    lcd.setCursor(0, 1);
    if (customMessageLine2.length() > 0) {
      int spaces2 = (16 - customMessageLine2.length()) / 2;
      if (spaces2 < 0) spaces2 = 0;
      lcd.setCursor(spaces2, 1);
      lcd.print(customMessageLine2);
      for (int i = customMessageLine2.length(); i < 16; i++) lcd.print(" ");
    } else {
      lcd.print("                ");
    }
  }
}

// ── ALARM SETUP ───────────────────────────────────────────────────────────────

void setupAlarm() {
  int tempHour12 = alarmHour % 12;
  if (tempHour12 == 0) tempHour12 = 12;
  bool tempIsPM  = (alarmHour >= 12);
  int tempMinute = alarmMinute;

  int settingStep = 0;
  bool settingComplete = false;
  unsigned long lastBlinkTime = 0;
  bool fieldVisible = true;

  lcd.clear();

  while (!settingComplete && currentMode == ALARM_MODE) {
    if (digitalRead(DISCARD_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
      lastButtonPress = millis(); buttonClickBeep();
      currentMode = CLOCK_MODE; lcd.clear(); return;
    }
    if (digitalRead(MODE_SELECT_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
      lastButtonPress = millis(); buttonClickBeep();
      currentMode = CLOCK_MODE; lcd.clear(); return;
    }
    if (millis() - lastIncPress > incDecDebounce) {
      if (digitalRead(INCREASE_PIN) == LOW) {
        lastIncPress = millis(); buttonClickBeep();
        if (settingStep == 0) { tempHour12++; if (tempHour12 > 12) tempHour12 = 1; }
        else if (settingStep == 1) { tempMinute = (tempMinute + 1) % 60; }
        else { tempIsPM = !tempIsPM; }
      }
    }
    if (millis() - lastDecPress > incDecDebounce) {
      if (digitalRead(DECREASE_PIN) == LOW) {
        lastDecPress = millis(); buttonClickBeep();
        if (settingStep == 0) { tempHour12--; if (tempHour12 < 1) tempHour12 = 12; }
        else if (settingStep == 1) { tempMinute = (tempMinute - 1 + 60) % 60; }
        else { tempIsPM = !tempIsPM; }
      }
    }
    if (millis() - lastButtonPress > debounceDelay) {
      if (digitalRead(SET_CONFIRM_PIN) == LOW) {
        lastButtonPress = millis(); buttonClickBeep();
        if (settingStep < 2) { settingStep++; }
        else {
          alarmHour   = tempIsPM ? (tempHour12 == 12 ? 12 : tempHour12 + 12)
                                 : (tempHour12 == 12 ? 0  : tempHour12);
          alarmMinute = tempMinute;
          alarmSecond = 0;
          alarmActive = true;
          alarmTriggered = false;
          saveAlarmToEEPROM();   // FIX 4
          settingComplete = true;
          lcd.clear(); return;
        }
      }
    }

    if (millis() - lastBlinkTime > 500) {
      lastBlinkTime = millis();
      fieldVisible = !fieldVisible;
    }

    lcd.setCursor(0, 0); lcd.print("SET ALARM TIME  ");

    char hBuf[3], mBuf[3];
    sprintf(hBuf, "%02d", tempHour12);
    sprintf(mBuf, "%02d", tempMinute);
    const char* ampmStr = tempIsPM ? "PM" : "AM";

    lcd.setCursor(0, 1);
    if (settingStep == 0) lcd.print(fieldVisible ? hBuf : "  ");
    else                  lcd.print(hBuf);
    lcd.print(":");
    if (settingStep == 1) lcd.print(fieldVisible ? mBuf : "  ");
    else                  lcd.print(mBuf);
    lcd.print(" ");
    if (settingStep == 2) lcd.print(fieldVisible ? ampmStr : "  ");
    else                  lcd.print(ampmStr);
    lcd.print("        ");

    delay(50);
  }
}

void setupTimer() {
  int settingStep = 0;
  int tempHour = timerHours, tempMinute = timerMinutes, tempSecond = timerSeconds;
  bool settingComplete = false;
  unsigned long lastBlinkTime = 0;
  bool fieldVisible = true;

  lcd.clear();
  while (!settingComplete && currentMode == TIMER_MODE) {
    if (digitalRead(DISCARD_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
      lastButtonPress = millis(); buttonClickBeep();
      currentMode = CLOCK_MODE; lcd.clear(); return;
    }
    if (digitalRead(MODE_SELECT_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
      lastButtonPress = millis(); buttonClickBeep();
      currentMode = CLOCK_MODE; lcd.clear(); return;
    }
    if (millis() - lastIncPress > incDecDebounce) {
      if (digitalRead(INCREASE_PIN) == LOW) {
        lastIncPress = millis(); buttonClickBeep();
        switch (settingStep) {
          case 0: tempHour   = (tempHour   + 1) % 24; break;
          case 1: tempMinute = (tempMinute + 1) % 60; break;
          case 2: tempSecond = (tempSecond + 1) % 60; break;
        }
      }
    }
    if (millis() - lastDecPress > incDecDebounce) {
      if (digitalRead(DECREASE_PIN) == LOW) {
        lastDecPress = millis(); buttonClickBeep();
        switch (settingStep) {
          case 0: tempHour   = (tempHour   - 1 + 24) % 24; break;
          case 1: tempMinute = (tempMinute - 1 + 60) % 60; break;
          case 2: tempSecond = (tempSecond - 1 + 60) % 60; break;
        }
      }
    }
    if (millis() - lastButtonPress > debounceDelay) {
      if (digitalRead(SET_CONFIRM_PIN) == LOW) {
        lastButtonPress = millis(); buttonClickBeep();
        if (settingStep < 2) { settingStep++; }
        else {
          timerHours = tempHour; timerMinutes = tempMinute; timerSeconds = tempSecond;
          timerRemaining = (timerHours * 3600L + timerMinutes * 60 + timerSeconds) * 1000L;
          timerRunning = false; timerExpired = false;
          settingComplete = true; lcd.clear(); runTimer(); return;
        }
      }
    }

    if (millis() - lastBlinkTime > 500) {
      lastBlinkTime = millis();
      fieldVisible = !fieldVisible;
    }

    lcd.setCursor(0, 0); lcd.print("SET TIMER       ");

    char hBuf[3], mBuf[3], sBuf[3];
    sprintf(hBuf, "%02d", tempHour);
    sprintf(mBuf, "%02d", tempMinute);
    sprintf(sBuf, "%02d", tempSecond);

    lcd.setCursor(0, 1);
    if (settingStep == 0) lcd.print(fieldVisible ? hBuf : "  ");
    else lcd.print(hBuf);
    lcd.print(":");
    if (settingStep == 1) lcd.print(fieldVisible ? mBuf : "  ");
    else lcd.print(mBuf);
    lcd.print(":");
    if (settingStep == 2) lcd.print(fieldVisible ? sBuf : "  ");
    else lcd.print(sBuf);
    lcd.print("        ");

    delay(50);
  }
}

void runTimer() {
  lcd.clear();
  timerRunning = true; timerExpired = false;
  timerStartTime = millis(); lastDisplaySecond = -1;

  while (currentMode == TIMER_MODE && !timerExpired) {
    if (digitalRead(DISCARD_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
      lastButtonPress = millis(); buttonClickBeep();
      timerRunning = false; timerExpired = false;
      buzzerContinuousStop(); currentMode = CLOCK_MODE; lcd.clear(); return;
    }
    if (digitalRead(MODE_SELECT_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
      lastButtonPress = millis(); buttonClickBeep();
      timerRunning = false; timerExpired = false;
      buzzerContinuousStop(); currentMode = CLOCK_MODE; lcd.clear(); return;
    }
    if (millis() - lastButtonPress > debounceDelay) {
      if (digitalRead(SET_CONFIRM_PIN) == LOW) {
        lastButtonPress = millis(); buttonClickBeep();
        if (timerRunning) {
          timerRunning = false;
          timerRemaining -= (millis() - timerStartTime);
        } else {
          timerStartTime = millis(); timerRunning = true;
        }
      }
    }
    if (timerRunning && !timerExpired) {
      long elapsed = millis() - timerStartTime;
      if (elapsed >= (long)timerRemaining) {
        timerExpired = true; timerRunning = false; timerRemaining = 0;
        lastTimerBeep = millis(); buzzerContinuousStart(); lcd.clear();
      } else {
        timerRemaining -= elapsed; timerStartTime = millis();
      }
    }
    DateTime now = rtc.now();
    int currentSecond = now.second();
    if (timerRunning && !timerExpired && currentSecond != lastDisplaySecond) {
      lastDisplaySecond = currentSecond;
      tickTockBeep();   // FIX 6
    }
    unsigned long dr = timerRemaining;
    int h = dr / 3600000, m = (dr % 3600000) / 60000, s = (dr % 60000) / 1000;
    lcd.setCursor(0, 0); lcd.print("TIMER: ");
    char timeStr[9];
    if (h > 0) sprintf(timeStr, "%02d:%02d:%02d", h, m, s);
    else       sprintf(timeStr, "%02d:%02d", m, s);
    lcd.print(timeStr); lcd.print("        ");
    lcd.setCursor(0, 1); lcd.print("NOW: ");
    int nowH12 = now.hour() % 12;
    if (nowH12 == 0) nowH12 = 12;
    char nowStr[13];
    sprintf(nowStr, "%02d:%02d:%02d %s", nowH12, now.minute(), now.second(),
            (now.hour() >= 12) ? "PM" : "AM");
    lcd.print(nowStr);
    delay(10);
  }

  while (timerExpired && currentMode == TIMER_MODE) {
    if (digitalRead(DISCARD_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
      lastButtonPress = millis(); buttonClickBeep();
      buzzerContinuousStop(); timerExpired = false;
      currentMode = CLOCK_MODE; lcd.clear(); return;
    }
    if (digitalRead(MODE_SELECT_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
      lastButtonPress = millis(); buttonClickBeep();
      buzzerContinuousStop(); timerExpired = false;
      currentMode = CLOCK_MODE; lcd.clear(); return;
    }
    if (millis() - lastTimerBeep > 500) { lastTimerBeep = millis(); timerBlinkState = !timerBlinkState; }
    lcd.setCursor(0, 0);
    if (timerBlinkState) lcd.print("    TIME UP!    ");
    else                 lcd.print("                ");
    DateTime now = rtc.now();
    lcd.setCursor(0, 1);
    char nowStr[17];
    sprintf(nowStr, "NOW: %02d:%02d:%02d", now.hour(), now.minute(), now.second());
    lcd.print(nowStr);
    delay(10);
  }
}

// ── ALARM CHECK & DISPLAY ─────────────────────────────────────────────────────

// FIX 3: checkAlarm now triggers from ANY mode (called in main loop unconditionally)
void checkAlarm() {
  if (!alarmActive || alarmTriggered) return;
  DateTime now = rtc.now();
  if (now.hour() == alarmHour && now.minute() == alarmMinute && now.second() == 0) {
    alarmTriggered = true;
    alarmActive = false;
    saveAlarmToEEPROM();   // FIX 4: save deactivated state
    lastAlarmBeep = millis(); lastAlarmBlink = millis();
    buzzerContinuousStart(); lcd.clear();
  }
}

void displayAlarmMode() {
  DateTime now = rtc.now();
  int currentSecond = now.second();
  if (alarmActive && !alarmTriggered && currentSecond != lastDisplayAlarmSecond) {
    lastDisplayAlarmSecond = currentSecond;
    tickTockBeep();   // FIX 6
  }
  int dispH12 = alarmHour % 12;
  if (dispH12 == 0) dispH12 = 12;
  const char* ampm = (alarmHour >= 12) ? "PM" : "AM";
  lcd.setCursor(0, 0);
  char alarmStr[16];
  sprintf(alarmStr, "ALARM: %02d:%02d %s", dispH12, alarmMinute, ampm);
  lcd.print(alarmStr); lcd.print(" ");
  int nowH12 = now.hour() % 12;
  if (nowH12 == 0) nowH12 = 12;
  const char* nowAP = (now.hour() >= 12) ? "PM" : "AM";
  lcd.setCursor(0, 1);
  char nowStr[17];
  sprintf(nowStr, "NOW: %02d:%02d:%02d %s", nowH12, now.minute(), now.second(), nowAP);
  lcd.print(nowStr);
}

// FIX 3: Alarm triggered screen — reachable from any mode
void displayAlarmScreen() {
  unsigned long lastBlinkTime = 0;
  bool blinkStateLocal = false;
  while (alarmTriggered) {
    unsigned long currentMillis = millis();

    // FIX 2: DISCARD cancels the alarm from any mode
    if (digitalRead(DISCARD_PIN) == LOW && currentMillis - lastButtonPress > debounceDelay) {
      lastButtonPress = currentMillis; buttonClickBeep();
      buzzerContinuousStop(); alarmTriggered = false;
      // Save discard to EEPROM (alarm no longer active)
      alarmActive = false;
      saveAlarmToEEPROM();   // FIX 4
      lcd.clear(); return;
    }
    if (digitalRead(MODE_SELECT_PIN) == LOW && currentMillis - lastButtonPress > debounceDelay) {
      lastButtonPress = currentMillis; buttonClickBeep();
      buzzerContinuousStop(); alarmTriggered = false;
      alarmActive = false;
      saveAlarmToEEPROM();   // FIX 4
      lcd.clear(); return;
    }
    if (digitalRead(SET_CONFIRM_PIN) == LOW && currentMillis - lastButtonPress > debounceDelay) {
      lastButtonPress = currentMillis; buttonClickBeep();
      buzzerContinuousStop(); alarmTriggered = false;
      alarmActive = false;
      saveAlarmToEEPROM();   // FIX 4
      lcd.clear(); return;
    }
    if (currentMillis - lastBlinkTime >= 500) {
      lastBlinkTime = currentMillis; blinkStateLocal = !blinkStateLocal;
    }
    lcd.setCursor(0, 0);
    if (blinkStateLocal) lcd.print("  ** ALARM! **  ");
    else                 lcd.print("                ");
    DateTime now = rtc.now();
    lcd.setCursor(0, 1);
    char nowStr[17];
    sprintf(nowStr, "NOW: %02d:%02d:%02d", now.hour(), now.minute(), now.second());
    lcd.print(nowStr);
    delay(10);
  }
}

// ── STUDY MODE ────────────────────────────────────────────────────────────────

bool studyExitPressed() {
  if (digitalRead(DISCARD_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
    lastButtonPress = millis(); buttonClickBeep(); return true;
  }
  if (digitalRead(MODE_SELECT_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
    lastButtonPress = millis(); buttonClickBeep(); return true;
  }
  return false;
}

bool setupStudyDuration(int &outH, int &outM) {
  int step = 0;
  int tH = outH, tM = outM;
  unsigned long lastBlinkTime = 0;
  bool fieldVisible = true;

  lcd.clear();
  while (true) {
    if (studyExitPressed()) { currentMode = CLOCK_MODE; lcd.clear(); return false; }
    if (millis() - lastIncPress > incDecDebounce) {
      if (digitalRead(INCREASE_PIN) == LOW) {
        lastIncPress = millis(); buttonClickBeep();
        if (step == 0) tH = (tH + 1) % 24;
        else           tM = (tM + 1) % 60;
      }
    }
    if (millis() - lastDecPress > incDecDebounce) {
      if (digitalRead(DECREASE_PIN) == LOW) {
        lastDecPress = millis(); buttonClickBeep();
        if (step == 0) tH = (tH - 1 + 24) % 24;
        else           tM = (tM - 1 + 60) % 60;
      }
    }
    if (millis() - lastButtonPress > debounceDelay) {
      if (digitalRead(SET_CONFIRM_PIN) == LOW) {
        lastButtonPress = millis(); buttonClickBeep();
        if (step == 0) { step = 1; fieldVisible = true; lastBlinkTime = millis(); }
        else { outH = tH; outM = tM; lcd.clear(); return true; }
      }
    }

    if (millis() - lastBlinkTime > 500) { lastBlinkTime = millis(); fieldVisible = !fieldVisible; }

    char hBuf[3], mBuf[3];
    sprintf(hBuf, "%02d", tH);
    sprintf(mBuf, "%02d", tM);

    lcd.setCursor(0, 0); lcd.print("STUDY DURATION  ");
    lcd.setCursor(0, 1);
    lcd.print("HR:");
    if (step == 0) lcd.print(fieldVisible ? hBuf : "  ");
    else           lcd.print(hBuf);
    lcd.print("  MIN:");
    if (step == 1) lcd.print(fieldVisible ? mBuf : "  ");
    else           lcd.print(mBuf);
    lcd.print("   ");

    delay(50);
  }
}

bool setupBreakInterval(int &outM) {
  int tM = outM;
  unsigned long lastBlinkTime = 0;
  bool fieldVisible = true;

  lcd.clear();
  while (true) {
    if (studyExitPressed()) { currentMode = CLOCK_MODE; lcd.clear(); return false; }
    if (millis() - lastIncPress > incDecDebounce) {
      if (digitalRead(INCREASE_PIN) == LOW) {
        lastIncPress = millis(); buttonClickBeep();
        tM = (tM + 1) % 60;
        if (tM == 0) tM = 1;
      }
    }
    if (millis() - lastDecPress > incDecDebounce) {
      if (digitalRead(DECREASE_PIN) == LOW) {
        lastDecPress = millis(); buttonClickBeep();
        tM--;
        if (tM < 1) tM = 59;
      }
    }
    if (millis() - lastButtonPress > debounceDelay) {
      if (digitalRead(SET_CONFIRM_PIN) == LOW) {
        lastButtonPress = millis(); buttonClickBeep();
        outM = tM; lcd.clear(); return true;
      }
    }

    if (millis() - lastBlinkTime > 500) { lastBlinkTime = millis(); fieldVisible = !fieldVisible; }

    lcd.setCursor(0, 0); lcd.print("BREAK INTERVAL  ");
    lcd.setCursor(0, 1);
    lcd.print("MIN: ");
    if (fieldVisible) { char buf[3]; sprintf(buf, "%02d", tM); lcd.print(buf); }
    else              { lcd.print("  "); }
    lcd.print("           ");
    delay(50);
  }
}

bool setupOneField(const char* title, const char* prompt, int &outVal, int maxVal) {
  int tV = outVal;
  unsigned long lastBlinkTime = 0;
  bool fieldVisible = true;

  lcd.clear();
  while (true) {
    if (studyExitPressed()) { currentMode = CLOCK_MODE; lcd.clear(); return false; }
    if (millis() - lastIncPress > incDecDebounce) {
      if (digitalRead(INCREASE_PIN) == LOW) {
        lastIncPress = millis(); buttonClickBeep();
        tV = (tV + 1) % (maxVal + 1);
        if (tV == 0) tV = 1;
      }
    }
    if (millis() - lastDecPress > incDecDebounce) {
      if (digitalRead(DECREASE_PIN) == LOW) {
        lastDecPress = millis(); buttonClickBeep();
        tV--;
        if (tV < 1) tV = maxVal;
      }
    }
    if (millis() - lastButtonPress > debounceDelay) {
      if (digitalRead(SET_CONFIRM_PIN) == LOW) {
        lastButtonPress = millis(); buttonClickBeep();
        outVal = tV; lcd.clear(); return true;
      }
    }

    if (millis() - lastBlinkTime > 500) { lastBlinkTime = millis(); fieldVisible = !fieldVisible; }

    lcd.setCursor(0, 0);
    lcd.print(title);
    for (int i = strlen(title); i < 16; i++) lcd.print(" ");

    lcd.setCursor(0, 1);
    lcd.print(prompt);
    if (fieldVisible) { char buf[3]; sprintf(buf, "%02d", tV); lcd.print(buf); }
    else              { lcd.print("  "); }
    lcd.print(" MIN        ");
    delay(50);
  }
}

void runStudyMode() {
  if (!setupStudyDuration(studyDurHours, studyDurMinutes)) return;
  if (!setupBreakInterval(breakIntervalMinutes)) return;
  if (!setupOneField("BREAK DURATION  ", "BREAK: ", breakDurationMinutes, 60)) return;

  studyTotalMs    = ((unsigned long)studyDurHours   * 3600UL
                   + (unsigned long)studyDurMinutes * 60UL) * 1000UL;
  breakIntervalMs = (unsigned long)breakIntervalMinutes * 60UL * 1000UL;
  breakDurationMs = (unsigned long)breakDurationMinutes * 60UL * 1000UL;

  if (studyTotalMs == 0) { currentMode = CLOCK_MODE; lcd.clear(); return; }
  if (breakIntervalMs == 0) breakIntervalMs = studyTotalMs;

  studyRunning         = true;
  studyExpired         = false;
  inBreak              = false;
  studyElapsedMs       = 0;
  studyStartTime       = millis();
  nextBreakAt          = studyStartTime + breakIntervalMs;
  lastStudyDisplaySecond = -1;
  lastBreakDisplaySecond = -1;

  lcd.clear();
  Serial.println("→ Study session started");
  SerialBT.println("→ Study session started");

  while (currentMode == STUDY_MODE && studyRunning && !studyExpired) {
    if (studyExitPressed()) {
      studyRunning = false; buzzerContinuousStop();
      currentMode = CLOCK_MODE; lcd.clear(); return;
    }

    // FIX 3: check alarm even inside study mode
    checkAlarm();
    if (alarmTriggered) {
      studyRunning = false; buzzerContinuousStop();
      displayAlarmScreen();
      currentMode = CLOCK_MODE; lcd.clear(); return;
    }

    unsigned long nowMs = millis();
    studyElapsedMs = nowMs - studyStartTime;

    if (studyElapsedMs >= studyTotalMs) {
      studyExpired = true; studyRunning = false; break;
    }

    // Per-second tick-tock during STUDY period
    if (!inBreak) {
      DateTime nowRtc = rtc.now();
      int curSec = nowRtc.second();
      if (curSec != lastStudyDisplaySecond) {
        lastStudyDisplaySecond = curSec;
        tickTockBeep();   // FIX 6
      }
    }

    // Break trigger
    if (!inBreak && nowMs >= nextBreakAt) {
      inBreak = true; breakStartTime = nowMs;
      lastBreakDisplaySecond = -1;
      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("  BREAK TIME!  ");
      lcd.setCursor(0, 1); lcd.print(" BUZZER ALERT  ");
      buzzer10SecBeep();
      lcd.clear();

      unsigned long breakBlinkLast = millis();
      bool breakBlink = true;

      while (currentMode == STUDY_MODE) {
        if (studyExitPressed()) {
          buzzerContinuousStop(); studyRunning = false;
          currentMode = CLOCK_MODE; lcd.clear(); return;
        }

        // FIX 3: alarm check during break
        checkAlarm();
        if (alarmTriggered) {
          buzzerContinuousStop(); studyRunning = false;
          displayAlarmScreen();
          currentMode = CLOCK_MODE; lcd.clear(); return;
        }

        long breakRawElapsed = (long)(millis() - breakStartTime) - 10000L;
        if (breakRawElapsed < 0) breakRawElapsed = 0;

        if ((unsigned long)breakRawElapsed >= breakDurationMs) {
          inBreak = false; lastStudyDisplaySecond = -1; lastBreakDisplaySecond = -1;
          nextBreakAt = millis() + breakIntervalMs;
          lcd.clear();
          lcd.setCursor(0, 0); lcd.print("PERIOD STARTING ");
          lcd.setCursor(0, 1); lcd.print(" BUZZER ALERT  ");
          buzzer10SecBeep();
          lcd.clear(); break;
        }

        // Per-second tick-tock DURING break countdown
        DateTime breakNowRtc = rtc.now();
        int breakCurSec = breakNowRtc.second();
        if (breakCurSec != lastBreakDisplaySecond) {
          lastBreakDisplaySecond = breakCurSec;
          tickTockBeep();   // FIX 6
        }

        unsigned long breakRemMs = breakDurationMs - (unsigned long)breakRawElapsed;
        int bH = breakRemMs / 3600000UL;
        int bM = (breakRemMs % 3600000UL) / 60000UL;
        int bS = (breakRemMs % 60000UL) / 1000UL;

        if (millis() - breakBlinkLast > 500) { breakBlinkLast = millis(); breakBlink = !breakBlink; }
        lcd.setCursor(0, 0);
        if (breakBlink) lcd.print("  BREAK TIME    ");
        else            lcd.print("                ");
        lcd.setCursor(0, 1);
        char bBuf[17];
        sprintf(bBuf, "BREAK: %02d:%02d:%02d", bH, bM, bS);
        lcd.print(bBuf);
        delay(10);
      }

      studyStartTime = millis() - studyElapsedMs;
      continue;
    }

    // Normal study display
    unsigned long toBreak = (nextBreakAt > millis()) ? (nextBreakAt - millis()) : 0;
    int bH2 = toBreak / 3600000UL;
    int bM2 = (toBreak % 3600000UL) / 60000UL;
    int bS2 = (toBreak % 60000UL)   / 1000UL;

    DateTime now = rtc.now();
    lcd.setCursor(0, 0);
    char nowBuf[17];
    sprintf(nowBuf, "NOW: %02d:%02d:%02d", now.hour(), now.minute(), now.second());
    lcd.print(nowBuf);
    lcd.setCursor(0, 1);
    char brBuf[17];
    sprintf(brBuf, "BREAK %02d:%02d:%02d", bH2, bM2, bS2);
    lcd.print(brBuf);
    delay(10);
  }

  // Time Up screen
  if (studyExpired) {
    buzzerContinuousStart();
    unsigned long tuBlink = millis();
    bool tuState = true;
    lcd.clear();
    while (currentMode == STUDY_MODE) {
      if (studyExitPressed()) {
        buzzerContinuousStop(); studyExpired = false;
        currentMode = CLOCK_MODE; lcd.clear(); return;
      }
      if (millis() - tuBlink > 500) { tuBlink = millis(); tuState = !tuState; }
      lcd.setCursor(0, 0);
      if (tuState) lcd.print("    TIME UP!    ");
      else         lcd.print("                ");
      DateTime now = rtc.now();
      lcd.setCursor(0, 1);
      char nowBuf[17];
      sprintf(nowBuf, "NOW: %02d:%02d:%02d", now.hour(), now.minute(), now.second());
      lcd.print(nowBuf);
      delay(10);
    }
    buzzerContinuousStop();
  }

  studyRunning = false; studyExpired = false;
  currentMode = CLOCK_MODE; lcd.clear();
}

// ── COMMAND PROCESSOR ─────────────────────────────────────────────────────────

void processCommand(String cmd) {
  cmd.trim();

  if (cmd == "home") {
    motivationMode = false; messageStyle = 0;
    customMessage = ""; customMessageLine2 = "";
    systemOn = true; currentMode = CLOCK_MODE; clock2Running = false;
    alarmTriggered = false; timerExpired = false; timerRunning = false;
    studyRunning = false; studyExpired = false;
    buzzerContinuousStop(); setBacklight(brightnessLevel);
    lcd.clear(); lcd.backlight();
    Serial.println("→ Clock mode"); SerialBT.println("→ Clock mode"); return;
  }
  if (cmd == "offme") {
    motivationMode = false; systemOn = false;
    setBacklight(0); lcd.noBacklight(); lcd.clear();
    Serial.println("→ System OFF"); SerialBT.println("→ System OFF"); return;
  }
  if (cmd == "onme") {
    systemOn = true; setBacklight(brightnessLevel);
    lcd.backlight(); lcd.clear(); messageStyle = 0; motivationMode = false;
    Serial.println("→ System ON"); SerialBT.println("→ System ON"); return;
  }
  if (cmd.startsWith("bright(")) {
    int cp = cmd.indexOf(')');
    if (cp != -1) {
      int v = cmd.substring(7, cp).toInt();
      if (v >= 0 && v <= 1000) { brightnessLevel = v; if (systemOn) setBacklight(v); }
    }
    return;
  }
  if (cmd.startsWith("speed(")) {
    int cp = cmd.indexOf(')');
    if (cp != -1) {
      int v = cmd.substring(6, cp).toInt();
      if (v >= 1 && v <= 1000) { animationSpeed = v; updateSpeed(); }
    }
    return;
  }
  // FIX 5: BT commands to change auto-brightness values
  if (cmd.startsWith("nightbright(")) {
    int cp = cmd.indexOf(')');
    if (cp != -1) { int v = cmd.substring(12, cp).toInt(); if (v >= 0 && v <= 1000) nightBrightness = v; }
    return;
  }
  if (cmd.startsWith("daybright(")) {
    int cp = cmd.indexOf(')');
    if (cp != -1) { int v = cmd.substring(10, cp).toInt(); if (v >= 0 && v <= 1000) dayBrightness = v; }
    return;
  }
  if (cmd.startsWith("nightstart(")) {
    int cp = cmd.indexOf(')');
    if (cp != -1) { int v = cmd.substring(11, cp).toInt(); if (v >= 0 && v <= 23) nightStartHour = v; }
    return;
  }
  if (cmd.startsWith("morningend(")) {
    int cp = cmd.indexOf(')');
    if (cp != -1) { int v = cmd.substring(11, cp).toInt(); if (v >= 0 && v <= 23) morningEndHour = v; }
    return;
  }
  if (cmd == "autobright on")  { autoBrightnessEnabled = true;  return; }
  if (cmd == "autobright off") { autoBrightnessEnabled = false; return; }
  // FIX 6: BT commands to tweak tick-tock values
  if (cmd.startsWith("tickfreq(")) {
    int cp = cmd.indexOf(')');
    if (cp != -1) { int v = cmd.substring(9, cp).toInt(); if (v > 0) tickFreq = v; }
    return;
  }
  if (cmd.startsWith("tockfreq(")) {
    int cp = cmd.indexOf(')');
    if (cp != -1) { int v = cmd.substring(9, cp).toInt(); if (v > 0) tockFreq = v; }
    return;
  }
  if (cmd.startsWith("tickdur(")) {
    int cp = cmd.indexOf(')');
    if (cp != -1) { int v = cmd.substring(8, cp).toInt(); if (v > 0) tickDurMs = v; }
    return;
  }
  if (cmd.startsWith("tockdur(")) {
    int cp = cmd.indexOf(')');
    if (cp != -1) { int v = cmd.substring(8, cp).toInt(); if (v > 0) tockDurMs = v; }
    return;
  }
  if (cmd == "buzzeron")  { buzzerEnabled = true;  Serial.println("→ Buzzer ON");  SerialBT.println("→ Buzzer ON");  return; }
  if (cmd == "buzzeroff") { buzzerEnabled = false; noTone(BUZZER_PIN); Serial.println("→ Buzzer OFF"); SerialBT.println("→ Buzzer OFF"); return; }
  if (cmd.startsWith("mv(")) {
    int cp = cmd.indexOf(')');
    if (cp != -1) {
      int style = cmd.substring(3, cp).toInt();
      if (style == 2 || style == 4 || style == 5 || style == 6) {
        currentMode = CLOCK_MODE; startMotivationMode(style);
      }
    }
    return;
  }
  if (cmd.startsWith("text(")) {
    int cp = cmd.indexOf(')');
    if (cp != -1) {
      int style = cmd.substring(5, cp).toInt();
      String msg = cmd.substring(cp + 1);
      if (style >= 1 && style <= 6) {
        motivationMode = false; messageStyle = style; currentMode = CLOCK_MODE;
        animationPosition = 0; cursorPos = 0; cursorPos2 = 0;
        charCount = 0; charCount2 = 0; animationCompleted = false;
        line1Completed = false; line2Completed = false; animationStage = 0;
        splitMessage(msg);
        if (style == 3) { customMessage = msg; customMessageLine2 = ""; if (customMessage.indexOf('@') != -1) customMessage.replace("@",""); }
        scrollText = customMessage + "    ";
        lcd.clear();
      }
    }
    return;
  }
  if (cmd.startsWith("SET:")) {
    cmd.remove(0, 4);
    int rH, rM, rS, rD, rMo, rYr;
    sscanf(cmd.c_str(), "%d,%d,%d,%d,%d,%d", &rH, &rM, &rS, &rD, &rMo, &rYr);
    rtc.adjust(DateTime(rYr, rMo, rD, rH, rM, rS));
    Serial.println("→ RTC updated!"); SerialBT.println("→ RTC updated!"); return;
  }
  if (cmd.startsWith("setalarm:")) {
    cmd.remove(0, 9); int aH, aM, aS;
    sscanf(cmd.c_str(), "%d,%d,%d", &aH, &aM, &aS);
    alarmHour = aH; alarmMinute = aM; alarmSecond = aS;
    alarmActive = true; alarmTriggered = false;
    saveAlarmToEEPROM();   // FIX 4
    return;
  }
  if (cmd == "alarmon")  { alarmActive = true;  alarmTriggered = false; saveAlarmToEEPROM(); return; }
  if (cmd == "alarmoff") {
    alarmActive = false; alarmTriggered = false;
    buzzerContinuousStop();
    saveAlarmToEEPROM();   // FIX 4
    return;
  }
  if (cmd.startsWith("settimer:")) {
    cmd.remove(0, 9); int tH, tM, tS;
    sscanf(cmd.c_str(), "%d,%d,%d", &tH, &tM, &tS);
    timerHours = tH; timerMinutes = tM; timerSeconds = tS;
    timerRemaining = (timerHours * 3600L + timerMinutes * 60 + timerSeconds) * 1000L;
    timerRunning = false; timerExpired = false;
    currentMode = TIMER_MODE; lcd.clear(); runTimer(); return;
  }
  if (cmd == "timerstop") {
    timerRunning = false; timerExpired = false; buzzerContinuousStop();
    currentMode = CLOCK_MODE; lcd.clear(); return;
  }
  if (cmd.length() > 0) {
    motivationMode = false; messageStyle = 2; currentMode = CLOCK_MODE;
    splitMessage(cmd); lcd.clear(); return;
  }
}

// ── SETUP ─────────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_Clock");
  delay(500);

  EEPROM.begin(EEPROM_SIZE);   // FIX 4

  randomSeed(analogRead(0));

  pinMode(MODE_SELECT_PIN, INPUT_PULLUP);
  pinMode(SET_CONFIRM_PIN, INPUT_PULLUP);
  pinMode(INCREASE_PIN,    INPUT_PULLUP);
  pinMode(DECREASE_PIN,    INPUT_PULLUP);
  pinMode(DISCARD_PIN,     INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);

  ledcAttach(BACKLIGHT_PIN, PWM_FREQ, 8);
  setBacklight(brightnessLevel);

  Wire.begin(I2C_SDA, I2C_SCL);
  delay(100);
  dht.begin();
  delay(1000);

  Serial.println("── I2C Scan ──");
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) Serial.printf("  Found: 0x%02X\n", addr);
  }
  Serial.println("──────────────");

  lcd.init(); lcd.init(); delay(50);
  lcd.backlight(); lcd.clear();

  if (!rtc.begin()) {
    lcd.setCursor(0, 0); lcd.print("RTC Error!"); while (1);
  }

  loadAlarmFromEEPROM();   // FIX 4: restore alarm after power-on

  lcd.setCursor(0, 0); lcd.print("RAHUL'S  CLOCK");
  lcd.setCursor(3, 1); lcd.print("ESP32  v23.0");
  delay(1800);
  lcd.clear();
}

// ── DHT CACHE ─────────────────────────────────────────────────────────────────

void updateDHT() {
  if (millis() - lastDHTRead < 2000) return;
  lastDHTRead = millis();
  float t = dht.readTemperature(), hh = dht.readHumidity();
  if (!isnan(t))  cachedTemp     = t;
  if (!isnan(hh)) cachedHumidity = hh;
}

// ── MAIN LOOP ─────────────────────────────────────────────────────────────────

void loop() {
  if (SerialBT.available() > 0) { String cmd = SerialBT.readStringUntil('\n'); processCommand(cmd); }
  if (Serial.available()   > 0) { String cmd = Serial.readStringUntil('\n');   processCommand(cmd); }

  // FIX 3: Check alarm globally — fires from ANY mode
  checkAlarm();

  // If alarm triggered, show alarm screen regardless of current mode
  if (alarmTriggered) {
    displayAlarmScreen();
    delay(10);
    return;
  }

  // FIX 5: Auto-brightness — check every loop iteration
  applyAutoBrightness();

  // ── DISCARD long-press → Quick Settings ──────────────────────────────────
  if (!studyRunning) {
    bool discardNow = (digitalRead(DISCARD_PIN) == LOW);
    if (discardNow && !discardHeld) {
      discardHeld = true; discardPressStart = millis(); longPressHandled = false;
    }
    if (discardHeld && discardNow && !longPressHandled) {
      if (millis() - discardPressStart >= LONG_PRESS_MS) {
        longPressHandled = true; lastButtonPress = millis();
        quickSettingsMenu();
      }
    }
    if (!discardNow) discardHeld = false;
  }

  // ── MODE SELECT cycling ───────────────────────────────────────────────────
  if (millis() - lastButtonPress > debounceDelay) {
    if (digitalRead(MODE_SELECT_PIN) == LOW) {
      lastButtonPress = millis(); buttonClickBeep();

      if (currentMode == CLOCK_MODE) {
        currentMode = CLOCK2_MODE; clock2Running = false;
        motivationMode = false; messageStyle = 0;
        lastClock2Second = -1;
        showModeSelectMessage = true; modeMessageStartTime = millis();
        lcd.clear(); lcd.setCursor(0, 0); lcd.print(" -CLOCK2 MODE-  ");

      } else if (currentMode == CLOCK2_MODE) {
        currentMode = ALARM_MODE; clock2Running = false;
        motivationMode = false; messageStyle = 0;
        showModeSelectMessage = true; modeMessageStartTime = millis();
        lastDisplayAlarmSecond = -1;
        lcd.clear(); lcd.setCursor(0, 0); lcd.print(" -ALARM MODE-   ");

      } else if (currentMode == ALARM_MODE) {
        currentMode = TIMER_MODE;
        showModeSelectMessage = true; modeMessageStartTime = millis();
        lcd.clear(); lcd.setCursor(0, 0); lcd.print(" -TIMER MODE-   ");

      } else if (currentMode == TIMER_MODE) {
        currentMode = MOTIVATION_MODE;
        timerRunning = false; timerExpired = false; buzzerContinuousStop();
        showModeSelectMessage = true; modeMessageStartTime = millis();
        lcd.clear(); lcd.setCursor(0, 0); lcd.print("-MOTIVATN MODE- ");

      } else if (currentMode == MOTIVATION_MODE) {
        currentMode = STUDY_MODE;
        motivationMode = false; messageStyle = 0;
        showModeSelectMessage = true; modeMessageStartTime = millis();
        lcd.clear(); lcd.setCursor(0, 0); lcd.print(" -STUDY MODE-   ");

      } else if (currentMode == STUDY_MODE) {
        currentMode = CLOCK_MODE;
        showModeSelectMessage = false;
        studyRunning = false; studyExpired = false; buzzerContinuousStop();
        lcd.clear();
      }
    }

    // FIX 2: DISCARD short press → cancel alarm OR back to CLOCK
    if (digitalRead(DISCARD_PIN) == LOW && !longPressHandled) {
      lastButtonPress = millis(); buttonClickBeep();
      // If alarm active (set but not yet triggered), discard it
      if (alarmActive && !alarmTriggered) {
        alarmActive = false;
        saveAlarmToEEPROM();   // FIX 4
        Serial.println("→ Alarm discarded");
        SerialBT.println("→ Alarm discarded");
        lcd.clear();
        lcd.setCursor(0, 0); lcd.print("  ALARM CLEARED ");
        delay(1000);
        lcd.clear();
      } else if (currentMode != CLOCK_MODE) {
        currentMode = CLOCK_MODE; clock2Running = false;
        alarmTriggered = false; timerExpired = false; timerRunning = false;
        studyRunning = false; studyExpired = false;
        motivationMode = false; messageStyle = 0;
        buzzerContinuousStop(); lcd.clear();
      }
    }
  }

  // Mode banner timeout
  if (showModeSelectMessage && (millis() - modeMessageStartTime > 1500)) {
    showModeSelectMessage = false; lcd.clear();
  }

  if (!systemOn) { delay(100); return; }

  // ── CLOCK2 ────────────────────────────────────────────────────────────────
  if (currentMode == CLOCK2_MODE) {
    if (!showModeSelectMessage) { updateDHT(); displayClock2(); }
    delay(10); return;
  }

  // ── ALARM ─────────────────────────────────────────────────────────────────
  if (currentMode == ALARM_MODE) {
    if (!showModeSelectMessage) {
      if (digitalRead(SET_CONFIRM_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
        lastButtonPress = millis(); buttonClickBeep(); setupAlarm(); lcd.clear();
      }
      if (alarmActive && !alarmTriggered) { displayAlarmMode(); }
      else if (!alarmActive) {
        lcd.setCursor(0, 0); lcd.print(" -ALARM MODE-   ");
        lcd.setCursor(0, 1); lcd.print(" ENTER TO SET   ");
      }
    }
    delay(10); return;
  }

  // ── TIMER ─────────────────────────────────────────────────────────────────
  if (currentMode == TIMER_MODE) {
    if (!showModeSelectMessage) {
      if (digitalRead(SET_CONFIRM_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
        lastButtonPress = millis(); buttonClickBeep(); setupTimer(); lcd.clear();
      } else {
        lcd.setCursor(0, 0); lcd.print(" -TIMER MODE-   ");
        lcd.setCursor(0, 1);
        if (!timerRunning && !timerExpired && timerRemaining > 0 &&
            !(timerHours==0 && timerMinutes==1 && timerSeconds==0)) {
          char ts[9];
          int h = timerRemaining/3600000, m=(timerRemaining%3600000)/60000, s=(timerRemaining%60000)/1000;
          sprintf(ts, "%02d:%02d:%02d", h, m, s);
          lcd.print(" READY: "); lcd.print(ts);
        } else {
          lcd.print(" ENTER TO SET   ");
        }
      }
    }
    delay(10); return;
  }

  // ── MOTIVATION ────────────────────────────────────────────────────────────
  if (currentMode == MOTIVATION_MODE) {
    if (!showModeSelectMessage) {
      if (!motivationMode) { animationSpeed = 890; updateSpeed(); startMotivationMode(5); }
      if (millis() - lastMotivationChange > 25000) { lastMotivationChange = millis(); changeMotivationMessage(); }
      switch (messageStyle) {
        case 1: displayStyle1(); break; case 2: displayStyle2(); break;
        case 3: displayStyle3(); break; case 4: displayStyle4(); break;
        case 5: displayStyle5(); break; case 6: displayStyle6(); break;
      }
    }
    delay(10); return;
  }

  // ── STUDY ─────────────────────────────────────────────────────────────────
  if (currentMode == STUDY_MODE) {
    if (!showModeSelectMessage) {
      if (digitalRead(SET_CONFIRM_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
        lastButtonPress = millis(); buttonClickBeep(); runStudyMode(); lcd.clear();
      } else {
        lcd.setCursor(0, 0); lcd.print(" -STUDY MODE-   ");
        lcd.setCursor(0, 1); lcd.print(" ENTER TO START ");
      }
    }
    delay(10); return;
  }

  // ── CLOCK (default) ───────────────────────────────────────────────────────
  if (currentMode == CLOCK_MODE) {
    updateDHT();
    if (motivationMode && messageStyle != 0 && millis() - lastMotivationChange > 25000) {
      lastMotivationChange = millis(); changeMotivationMessage();
    }
    if (messageStyle == 0) {
      DateTime rtcNow = rtc.now();
      bool showColon = ((millis() % 1000) < 500);
      String sep = showColon ? ":" : " ";
      int hour24 = rtcNow.hour();
      String ampm = (hour24 < 12) ? "AM" : "PM";
      int hour12 = hour24 % 12;
      if (hour12 == 0) hour12 = 12;
      String hStr = (hour12 < 10) ? "0" + String(hour12) : String(hour12);
      String mStr = (rtcNow.minute() < 10) ? "0" + String(rtcNow.minute()) : String(rtcNow.minute());
      int dispTemp = (int)cachedTemp, dispHumidity = (int)cachedHumidity;

      lcd.setCursor(0, 0);
      lcd.print(hStr + sep + mStr + " " + ampm);
      lcd.setCursor(9, 0); lcd.print(" T=");
      if (dispTemp < 10) lcd.print(" ");
      lcd.print(dispTemp); lcd.print((char)223); lcd.print("C");

      String dowStr = String(days[rtcNow.dayOfTheWeek()]);
      String monStr = String(months[rtcNow.month() - 1]);
      String dayDStr = (rtcNow.day() < 10) ? "0" + String(rtcNow.day()) : String(rtcNow.day());
      lcd.setCursor(0, 1);
      lcd.print(monStr); lcd.print(dayDStr); lcd.print(" ");
      lcd.print(dowStr); lcd.print("  H=");
      if (dispHumidity < 10) lcd.print(" ");
      lcd.print(dispHumidity); lcd.print("%");
    } else {
      switch (messageStyle) {
        case 1: displayStyle1(); break; case 2: displayStyle2(); break;
        case 3: displayStyle3(); break; case 4: displayStyle4(); break;
        case 5: displayStyle5(); break; case 6: displayStyle6(); break;
      }
    }
  }

  delay(10);
}
