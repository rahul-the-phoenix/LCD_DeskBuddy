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
#define BUZZER_PIN          12

#define CLICK_BEEP_FREQ   1200
#define CLICK_BEEP_MS       80
#define LONG_PRESS_MS     1000

// ── Debounce delays ───────────────────────────────────────────────────────────
const unsigned long debounceDelay  = 200;
const unsigned long incDecDebounce = 120;

unsigned long lastButtonPress = 0;
unsigned long lastIncPress    = 0;
unsigned long lastDecPress    = 0;

// ── Auto-brightness ───────────────────────────────────────────────────────────
int  nightBrightness       = 300;
int  dayBrightness         = 1000;
int  nightStartHour        = 21;
int  morningEndHour        = 6;
bool autoBrightnessEnabled = true;
int lastAutoHour = -1; 
// ── Tick-tock variables ───────────────────────────────────────────────────────
int  tickFreq  = 1100;
int  tockFreq  =  950;
int  tickDurMs =   38;
int  tockDurMs =   28;
bool tickTurn  = true;

int  brightnessLevel = 500;
bool buzzerEnabled   = true;

float cachedTemp     = 0.0;
float cachedHumidity = 0.0;
unsigned long lastDHTRead = 0;

String customMessage      = "";
String customMessageLine2 = "";
int    messageStyle       = 0;
unsigned long lastAnimationUpdate = 0;
int    animationPosition  = 0;
String scrollText         = "";
bool   blinkState         = true;
unsigned long lastBlinkUpdate = 0;
int    animationSpeed     = 500;
int    textSpeed          = 150;
bool   systemOn           = true;
int    cursorPos          = 0;
int    cursorPos2         = 0;
int    charCount          = 0;
int    charCount2         = 0;
unsigned long lastCharTime  = 0;
unsigned long lastCharTime2 = 0;
bool   animationCompleted = false;
bool   line1Completed     = false;
bool   line2Completed     = false;
int    animationStage     = 0;

bool manualBrightnessOverride = false;
unsigned long manualOverrideEndTime = 0;
bool motivationMode         = false;
int  motivationStyle        = 0;
unsigned long lastMotivationChange = 0;
int  currentMotivationIndex = 0;

enum SystemMode { CLOCK_MODE, CLOCK2_MODE, ALARM_MODE, TIMER_MODE, MOTIVATION_MODE, STUDY_MODE };
SystemMode currentMode = CLOCK_MODE;
bool showModeSelectMessage = false;
unsigned long modeMessageStartTime = 0;

bool clock2Running    = false;
int  lastClock2Second = -1;

// ── EEPROM layout ─────────────────────────────────────────────────────────────
#define EEPROM_MAGIC      0xA5
#define EEPROM_ADDR_MAGIC    0
#define EEPROM_ADDR_HOUR     1
#define EEPROM_ADDR_MIN      2
#define EEPROM_ADDR_SEC      3
#define EEPROM_ADDR_ACTIVE   4
#define EEPROM_SIZE          8

bool alarmActive    = false;
int  alarmHour      = 8;
int  alarmMinute    = 0;
int  alarmSecond    = 0;
bool alarmTriggered = false;
unsigned long lastAlarmBeep  = 0;
bool alarmBlinkState         = false;
unsigned long lastAlarmBlink = 0;
int  lastDisplayAlarmSecond  = -1;

// ── FIX: Timer pause state tracked properly ───────────────────────────────────
bool timerRunning        = false;
bool timerPaused         = false;             // NEW: explicit pause flag
unsigned long timerStartTime   = 0;
unsigned long timerRemaining   = 0;
unsigned long timerPauseStart  = 0;           // NEW: when pause began
int  timerHours   = 0;
int  timerMinutes = 1;
int  timerSeconds = 0;
bool timerExpired = false;
unsigned long lastTimerBeep = 0;
bool timerBlinkState        = false;
int  lastDisplaySecond      = -1;

// ── STUDY MODE VARIABLES ──────────────────────────────────────────────────────
const int studyHourOptions[]   = {0,1,2,3,4,5,6,7,8,9};
const int studyHourCount       = 10;
const int studyMinOptions[]    = {2,20,30,40};
const int studyMinCount        = 4;
const int breakIntOptions[]    = {1,30,40,60,75,90};
const int breakIntCount        = 6;
const int breakDurOptions[]    = {1,5,10,15,30};
const int breakDurCount        = 5;

int studyHourIdx   = 1;
int studyMinIdx    = 0;
int breakIntIdx    = 1;
int breakDurIdx    = 1;

int studyDurHours        = 2;
int studyDurMinutes      = 0;
int breakIntervalMinutes = 30;
int breakDurationMinutes = 5;

bool  studyConfigured = false;
bool  studyRunning    = false;
bool  studyExpired    = false;
bool  studyPaused     = false;

unsigned long studyTotalMs   = 0;
unsigned long studyStartTime = 0;
unsigned long studyElapsedMs = 0;
unsigned long studyPausedAt  = 0;

unsigned long breakIntervalMs = 0;
unsigned long breakDurationMs = 0;

unsigned long nextBreakAt    = 0;
bool  inBreak                = false;
unsigned long breakStartTime = 0;      // when break was triggered
unsigned long breakActualStart = 0;    // FIX: when break display/countdown actually starts (after buzzer)
bool  breakIntroPlayed       = false;

// ── Non-blocking buzzer state for study mode ──────────────────────────────────
bool          studyBuzzerActive  = false;
int           studyBuzzerState   = 0;    // 1=break intro, 2=period start
unsigned long studyBuzzerStartMs = 0;
const unsigned long STUDY_BUZZER_DUR = 10000UL;  // 10 seconds

int  _lastStudySec = -1;
int  _lastBreakSec = -1;

unsigned long studyBlinkLast = 0;
bool studyBlinkState         = false;

// ─────────────────────────────────────────────────────────────────────────────
unsigned long discardPressStart = 0;
bool discardHeld      = false;
bool longPressHandled = false;

LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;
DHT dht(DHT_PIN, DHT_TYPE);
BluetoothSerial SerialBT;

const char* days[]    = {"SUN","MON","TUE","WED","THU","FRI","SAT"};
const char* months[]  = {"Jan","Feb","Mar","Apr","May","Jun",
                         "Jul","Aug","Sep","Oct","Nov","Dec"};
const char* months2[] = {"JAN","FEB","MAR","APR","MAY","JUN",
                         "JUL","AUG","SEP","OCT","NOV","DEC"};

// ── Add all your motivation messages here ─────────────────────────────────────
// Motivation messages array
const char* motivationMessages[] = {
  "Focus On Logic@Build The Future",
  "Crack The GATE@Reach The IIT Tag",
  "Code Your Way@To The Top Tier",
  "Master Discrete@Math For The Win",
  "Dream Of IIT@Work For It Now",
  "OS Concepts Are@Key To Success",
  "Solve The PYQs@Rank Will Follow",
  "Data Structures@Fix All Tasks",
  "Be A Topper In@CS Engineering",
  "Algo Design Is@Your Best Tool",
  "Compilers Are@Logic Engines",
  "DBMS Mastery Is@Within Reach",
  "Keep On Coding@Keep Climbing",
  "GATE Exam 2027@Is Your Big Goal",
  "Believe In The@CS Power Today",
  "Revise Daily@Retain It Weekly",
  "TOC Is Hard@But You Are Too",
  "Network Layers@Lead To Glory",
  "Stay Calm Now@Solve The GATE",
  "IIT Dreams Are@Made Of Grit",
  "Digital Logic@Is Very Sharp",
  "Paging Is Easy@If You Study",
  "Graphs Lead To@The Right Path",
  "Trees Grow With@Your Efforts",
  "Success Is An@O(1) Operation",
  "Your Potential@Has No Limits",
  "Aptitude Is A@Scoring Zone",
  "Logic Gates@Open IIT Doors",
  "Stay Hungry@Stay A True Guy",
  "Rank Under 100@Is The Target",
  "Don't Stop Till@You Reach IIT",
  "Math Is The@Base Of All CS",
  "Practice Makes@You A Topper",
  "Study Very Hard@Code Smarter",
  "One Byte At@Time You Win It",
  "Recursion Leads@To The Peak",
  "Sort Your Life@Sort Your Rank",
  "Binary Search@For Your Goal",
  "Hashing Out@All The Trouble",
  "Cache Your@Knowledge Well",
  "Pointers Point@To The Trophy",
  "Deadlock Free@Study Mode Now",
  "Threads Of@Hard Work Pay",
  "SQL Queries@Unlock The Data",
  "Be The Master@Of The Automata",
  "Context Free@Focus Is The Key",
  "P Vs NP Is@A Big Mind Game",
  "Queue Up For@The Biggest Win",
  "Stack Up Marks@For Today Work",
  "Linked Lists@Lead To Success",
  "Heaps Of Joy@Await You At IIT",
  "Min Cut For@All Obstacles",
  "Max Flow For@Your Own Energy",
  "Be An Expert@In The C Coding",
  "Linux Kernels@Are A Cool Art",
  "Learn TCP For@The Better Link",
  "UDP Is Fast@Be Just Like That",
  "IP Address Of@Success Is IIT",
  "Routing Your@Way To Glory",
  "Bit By Bit@You Will Win It",
  "Boolean Algebra@Never Lies",
  "K Maps Simplify@Your Own Life",
  "MUX Your Time@For The Result",
  "Pipeline Your@Study Flow Now",
  "No Stalls In@Your Progress",
  "Branch Out To@The New Height",
  "RISC It All@For The Dream",
  "CISC Is Big@Just Stay Focused",
  "Memory Manage@Is Very Vital",
  "Virtual RAM@For Real Success",
  "Semaphores For@Clear Paths",
  "Mutex For Your@Focus Time Now",
  "Deadly Focus@No Deadlocks",
  "B Trees Help@You To Search",
  "B+ Trees@For Dense Ranks",
  "Normalise Your@Study Plan Now",
  "1NF To BCNF@Know It All Well",
  "ACID Traits@In Your Own Work",
  "Greedy Algos@For The Marks",
  "Dynamic Coding@For The Win",
  "Divide And@Conquer The GATE",
  "Backtrack To@Fix The Error",
  "Complexity Is@Just A Number",
  "Big O Of N@Is The Target",
  "Stay Linear@Stay Focused",
  "Exponentially@Grow Every Day",
  "Log Time Is@The Best Time",
  "NP Hard Is@Not Really Hard",
  "SAT Solvers@Fix The GATE",
  "Grammar Leads@To Languages",
  "Pushdown Your@Fears Today",
  "Turing Machine@Of Hard Work",
  "Decide To Be@The Best One",
  "Halting Is@Not An Option",
  "Learn To Code@Code To Learn",
  "CS Is The@Future Of All",
  "GATE Is Just@One Step Away",
  "IIT Bombay Is@Now Calling",
  "IIT Delhi Is@Your Vision",
  "IIT Madras Is@The Target",
  "IIT Kanpur@Awaits You Now",
  "IIT Kharagpur@Is The Goal",
  "IIT Roorkee@Awaits Logic",
  "IIT Guwahati@Looks So Great",
  "IISc Is The@Ultimate Aim",
  "Research Is@Your Passion",
  "Coding Is Your@Super Power",
  "Stay Humble@Work Much Harder",
  "Silence Your@Doubts Now",
  "Errors Lead@To Perfection",
  "Debug Your@Own Life Path",
  "Compile Your@Strength Now",
  "Run The Code@Of Success",
  "Execute Your@Plans Well",
  "Input Effort@Output IIT Seat",
  "No Bugs In@Your Strategy",
  "Logic Is The@Only Weapon",
  "Maths Is The@Queen Of GATE",
  "Probability@Of Winning One",
  "Stats Prove@You Are Good",
  "Matrices Are@Easy Points",
  "Eigen Values@Of Success",
  "Calculus For@Steady Growth",
  "Groups And@Rings Of Power",
  "Graph Theory@Is Beautiful",
  "Master Bayes@For The GATE",
  "Expectation@Is To Top GATE",
  "Variance In@Study Is Bad",
  "Stay Constant@Stay Very Ready",
  "Uniformly Work@Every Day",
  "Normalise The@Pressure Now",
  "Poisson Flow@Of The Wins",
  "Binomial Luck@Comes To All",
  "Set Theory@Is The Start",
  "Functions Are@Your Friends",
  "Relations Last@Long In GATE",
  "Lattices And@Posets Win It",
  "Groups Define@Your Circle",
  "Be A Real Pro@In Proposition",
  "Predicates@Are True For You",
  "Quantify Your@Hard Effort",
  "Validity Is@Your Strength",
  "Satisfy All@The GATE Rules",
  "Complexity@Of Dreams High",
  "Paging The@Future Today",
  "Segmentation@Of Your Goals",
  "Dirty Bits@In Your Prep",
  "Write Back@Your Success",
  "Write Through@The Struggle",
  "TLB Miss Is@Just A Slip",
  "Hit Ratio Of@Ninety Percent",
  "Locality Of@Focus Is Key",
  "Temporal Grit@Spatial Aim",
  "Bus Width@Of Your Brain",
  "Clock Speed@Of Your Logic",
  "Cycles Of@Constant Study",
  "Fetch The@Greatest Rank",
  "Decode The@GATE Pattern",
  "ALU Of Your@Mind Is Fast",
  "Registers@Of Memory Stay",
  "Direct Mapped@To The Goal",
  "Set Associative@Winning",
  "Fully Linked@To The Dream",
  "Page Faults@Teach Lessons",
  "Thrashing Is@Not Allowed",
  "Working Set@Of Winners",
  "Belady's Luck@Will Not Stop",
  "Optimal Prep@Leads To IIT",
  "LRU Means@Learn Recent",
  "FIFO For@All Your Tasks",
  "SJF For Your@Short Goals",
  "Round Robin@Your Subjects",
  "Priority Is@Always GATE",
  "Multi Level@Success Plan",
  "Deadlock Is@For The Weak",
  "Banker's Logic@Keeps Safety",
  "Safety State@Is IIT Seat",
  "Avoid All@The Wrong Paths",
  "Prevent Any@Lazy Habits",
  "Detection@Of Weak Areas",
  "Recovery@From Failures",
  "Disk Space@For Big Ideas",
  "RAID Your@Way To Glory",
  "Striping@The Competition",
  "Mirrored@In Your Efforts",
  "Checksum For@Your Growth",
  "Parity With@The Toppers",
  "Coding Theory@Is So Fun",
  "Hamming Way@To Perfection",
  "Distance@To IIT Is Short",
  "Error Free@Mindset Now",
  "Cycles Of@Constant Win",
  "Paths Lead@To The Summit",
  "Adjacency@To Greatness",
  "Degree Of@Success Is One",
  "Connected@To Your Dreams",
  "Isomorphic@To A Winner",
  "Planar Graphs@Of Progress",
  "Euler Path@To The Goal",
  "Hamiltonian@Strength Now",
  "Cliques Of@Top Engineers",
  "Cover All@The Syllabus",
  "Matching@Your Ambition",
  "Flow Through@The Hurdles",
  "Source Is@Your Hard Work",
  "Sink Is The@Final IIT",
  "Reach For@The Top Rank",
  "Failures Are@The Beta Test",
  "Sleep Is@For Non GATE Guy",
  "Coffee Runs@In My Veins",
  "Social Life@Segmentation Fault",
  "IIT Tag Or@Just A Rag",
  "Rank Decides@Your Worth",
  "Don't Cry@Just Dry Run",
  "Logic High@Feelings Low",
  "Love Is A@Recursive Trap",
  "No GF Only@BF Best Friend",
  "Study Now@Flex Later On",
  "Job Is Safe@But IIT Is Fire",
  "Be The Zero@One Percent Guy",
  "Pain Is Just@A Data Input",
  "Cry In Jaguar@Not In Bus",
  "IIT AIR@Feels Different",
  "Society@Wants Your Rank",
  "Parents Want@That IIT Tag",
  "Neighbors Envy@Is My Goal",
  "Ex GF Will@Regret Soon",
  "Burn The@Midnight Oil",
  "Grind Now@Shine Forever",
  "GATE Is A@Battlefield",
  "Be The King@Of Core Stuff",
  "Tech Is@The New Gold",
  "Silicon Valley@Calls You",
  "Start Up Is@A Post IIT",
  "Think Like@A Machine",
  "Be Cold As@A Processor",
  "Heart Is Just@A Pump Chill",
  "Emotions@Are Runtime Errors",
  "Hard Work@Beats Genius",
  "No Shortcut@To The Top",
  "Stairs To@IIT Are Steep",
  "Sweat In Peace@Win The War",
  "Focus Is@My Superpower",
  "Discipline@Is The Key",
  "Consistent@Is The Word",
  "Beat All@The Odds Today",
  "Impossible@Is Logic Error",
  "Win The Race@Be The Ace",
  "Top Rank@Or Nothing Now",
  "Eat Sleep@GATE Repeat",
  "Library Is@My Second Home",
  "Books Are@My Only Bae",
  "Pen Is My@Only Weapon",
  "Notes Are@My Treasure",
  "Summary@Is Not Enough",
  "Deep Dive@Into Syllabus",
  "Zero Days@Off Allowed",
  "One Goal@And One Vision",
  "Eyes On The@The IIT GATE",
  "Push Limits@Every Day",
  "Break Barriers@Not Hearts",
  "Silence Is@The Best Noise",
  "Success@Will Make A Roar",
  "Prove Them@All Wrong Now",
  "Watch Me@Reach The Top",
  "History@Is Made By Us",
  "Be A Legend@In The CS Field",
  "Future CEO@In The Making",
  "Code The@World Better",
  "Innovate Or@Get Deleted",
  "Stay Hungry@Stay Foolish",
  "Vision Of@A Top Topper",
  "Mindset Of@A Conqueror",
  "Born To@Crack The GATE",
  "Destined@For IIT Glory",
  "Master Of@All Subjects",
  "King Of The@The CS Kingdom",
  "Rule The@Tech World",
  "Binary@Is My Language",
  "Hex Life@Is A Rich Life",
  "Logic Over@Everything",
  "Reasoning@Is My Strength",
  "Puzzle Solver@By Birth",
  "Born For@The Engineer",
  "Building@The Next Thing",
  "Dream Big@Act Even Bigger",
  "Small Steps@For Big Impact",
  "Giant Leap@For My Career",
  "Path To@The IIT Is Clear",
  "Walk The@Talk Every Day",
  "No Excuses@Only Results",
  "Kill The@Laziness Now",
  "Destroy@Distractions",
  "Focus Like@A Laser Beam",
  "Sharp Mind@And Sharp Rank",
  "Bright Future@Awaits You",
  "Be The Light@Of Your Home",
  "Pride Of@The College",
  "Star Of@The Family Tree",
  "Legacy Starts@With The GATE",
  "IIT Is@Just Beginning",
  "Life Begins@At IIT Gates",
  "Magic Happens@In The Lab",
  "Creation@Is My Passion",
  "Art Of@Problem Solving",
  "Science@Of Winning Life",
  "Philosophy@Of A Topper",
  "Way Of The@Brave Warrior",
  "Code Of@Conduct To Win",
  "Protocol@For Success",
  "Standards@Are Set High",
  "Quality@Over Quantity",
  "Precision@Is Everything",
  "Accuracy@Is The Aim",
  "Speed Is@The Only Edge",
  "Efficiency@Is The Goal",
  "Optimize@Your Life Now",
  "Refactor@Your Habits",
  "Upgrade@Your Circle",
  "System Update@Is Required",
  "Reboot@Your Spirit Now",
  "Full Charge@Mode Is On",
  "Unlimited@True Potential",
  "Infinite@Possibilities",
  "End Game@Is IIT Madras",
  "Final Boss@Is GATE Exam",
  "Level Up@Every Week",
  "XP Gain@From Every Mock",
  "Skill Points@On The Maths",
  "Inventory@Of Formulae",
  "Quest For@The IIT Tag",
  "Mission@Is Accomplished",
  "Victory@Is My Destiny",
  "Crown Of@A GATE Topper",
  "Glory Is@Truly Eternal",
  "Hard Work@Will Never Fade",
  "Legendary@Status Is Aim",
  "Epic Win@In The Feb Exam",
  "Coolest Geek@In The Town",
  "Sartorial@The IIT Hoodie",
  "Brand Of@An IIT Student",
  "Worth Of@A Top Scholar",
  "Value Of@The Discipline",
  "Price Of@The Greatness",
  "Cost Of@The Mediocrity",
  "Avoid The@Average Life",
  "Be The@Extraordinary",
  "Phenomenal@Rank Is Coming",
  "Stellar@Performance",
  "Galactic@Huge Ambition",
  "Universal@Recognition",
  "Beyond@The Far Horizon",
  "Sky Is@Not The Limit",
  "IIT And@Far Beyond GATE",
  "Future@Is In Your Hands",
  "Write Your@Own Story Now",
  "Be The@True Protagonist",
  "Hero Of@Your Journey",
  "Success@Is Coming Home",
  "IIT Is@Calling You Now",
  "Wait For@The Result Day",
  "Tears Of@Joy Very Soon",
  "Smile Of@A Winner Now",
  "Peace At@The Highest Peak",
  "The End@Of The Grind",
  "Start Of@A New Life",
  "Welcome@To The IIT Life"
};
const int totalMessages = sizeof(motivationMessages) / sizeof(motivationMessages[0]);

// ═════════════════════════════════════════════════════════════════════════════
//  EEPROM HELPERS
// ═════════════════════════════════════════════════════════════════════════════

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
    alarmHour      = EEPROM.read(EEPROM_ADDR_HOUR);
    alarmMinute    = EEPROM.read(EEPROM_ADDR_MIN);
    alarmSecond    = EEPROM.read(EEPROM_ADDR_SEC);
    alarmActive    = (EEPROM.read(EEPROM_ADDR_ACTIVE) == 1);
    alarmTriggered = false;
    Serial.println("-> Alarm loaded from EEPROM");
  } else {
    Serial.println("-> No alarm in EEPROM");
  }
}

// ═════════════════════════════════════════════════════════════════════════════
//  AUTO-BRIGHTNESS
// ═════════════════════════════════════════════════════════════════════════════

void applyAutoBrightness() {
    if (!autoBrightnessEnabled || !systemOn) return;

    DateTime now = rtc.now();
    int h = now.hour();

    // শুধু যখন ঘণ্টা বদলায় তখনই check করো
    if (h == lastAutoHour) return;  // same hour, কিছু করো না

    // শুধু trigger hour এ fire করো
    if (h == morningEndHour) {          // সকাল ৬টা
        lastAutoHour    = h;
        brightnessLevel = dayBrightness;   // 1000
        setBacklight(brightnessLevel);

    } else if (h == nightStartHour) {   // রাত ৯টা
        lastAutoHour    = h;
        brightnessLevel = nightBrightness; // 300
        setBacklight(brightnessLevel);
    }
    // অন্য সময়ে কিছুই করবে না → manual change টিকে থাকবে
}

// ═════════════════════════════════════════════════════════════════════════════
//  HELPERS
// ═════════════════════════════════════════════════════════════════════════════

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
    customMessage      = msg.substring(0, atPos);
    customMessageLine2 = msg.substring(atPos + 1);
    if (customMessage.length()      > 16) customMessage      = customMessage.substring(0, 16);
    if (customMessageLine2.length() > 16) customMessageLine2 = customMessageLine2.substring(0, 16);
  } else {
    customMessage      = msg;
    customMessageLine2 = "";
    if (customMessage.length() > 16) customMessage = customMessage.substring(0, 16);
  }
}

// ═════════════════════════════════════════════════════════════════════════════
//  BUZZER
// ═════════════════════════════════════════════════════════════════════════════

void buttonClickBeep() {
  if (!buzzerEnabled) return;
  tone(BUZZER_PIN, CLICK_BEEP_FREQ);
  delay(CLICK_BEEP_MS);
  noTone(BUZZER_PIN);
}

void tickTockBeep() {
  if (!buzzerEnabled) return;
  if (tickTurn) {
    tone(BUZZER_PIN, tickFreq); delay(tickDurMs); noTone(BUZZER_PIN);
  } else {
    tone(BUZZER_PIN, tockFreq); delay(tockDurMs); noTone(BUZZER_PIN);
  }
  tickTurn = !tickTurn;
}

void buzzerBeep() {
  if (!buzzerEnabled) return;
  tone(BUZZER_PIN, BUZZER_FREQ); delay(100); noTone(BUZZER_PIN);
}

void buzzerContinuousStart() {
  if (!buzzerEnabled) return;
  tone(BUZZER_PIN, BUZZER_FREQ);
}

void buzzerContinuousStop() {
  noTone(BUZZER_PIN);
}

// ═══════════════════════════════════════════════════════════════════════════
//  NON-BLOCKING 10-SECOND BUZZER FOR STUDY MODE
// ═══════════════════════════════════════════════════════════════════════════

void startStudyBuzzer(int state) {
  studyBuzzerActive  = true;
  studyBuzzerState   = state;
  studyBuzzerStartMs = millis();
  if (buzzerEnabled) {
    tone(BUZZER_PIN, BUZZER_FREQ);
  }
}

// Returns true when buzzer duration is complete
// FIX: Save state BEFORE resetting, return completed state via out param
bool tickStudyBuzzer(int &completedState) {
  completedState = 0;
  if (!studyBuzzerActive) return false;

  if (millis() - studyBuzzerStartMs >= STUDY_BUZZER_DUR) {
    noTone(BUZZER_PIN);
    completedState    = studyBuzzerState;  // save before reset
    studyBuzzerActive = false;
    studyBuzzerState  = 0;
    return true;
  }
  return false;
}

// ═════════════════════════════════════════════════════════════════════════════
//  CLOCK2 DISPLAY
// ═════════════════════════════════════════════════════════════════════════════

void displayClock2() {
  DateTime rtcNow = rtc.now();
  bool showColon  = ((millis() % 1000) < 500);
  char sep        = showColon ? ':' : ' ';
  int  hour24     = rtcNow.hour();
  const char* ampm = (hour24 < 12) ? "AM" : "PM";
  int  hour12     = hour24 % 12;
  if (hour12 == 0) hour12 = 12;

  int curSec = rtcNow.second();
  if (curSec != lastClock2Second) { lastClock2Second = curSec; tickTockBeep(); }

  lcd.setCursor(0, 0);
  char line0[17];
  snprintf(line0, 17, "  %02d%c%02d%c%02d %s   ", hour12, sep, rtcNow.minute(), sep, rtcNow.second(), ampm);
  lcd.print(line0);

  int dispTemp     = (int)cachedTemp;
  int dispHumidity = (int)cachedHumidity;
  char humStr[5], tempStr[5];
  if (dispHumidity < 10) snprintf(humStr,  5, " %d%%", dispHumidity);
  else                   snprintf(humStr,  5, "%d%%",  dispHumidity);
  if (dispTemp < 10) snprintf(tempStr, 5, " %d%c", dispTemp, (char)223);
  else               snprintf(tempStr, 5, "%d%c",  dispTemp, (char)223);

  lcd.setCursor(0, 1);
  char line1[17];
  snprintf(line1, 17, "%s%02d %s %s%s",
           months2[rtcNow.month() - 1], rtcNow.day(),
           days[rtcNow.dayOfTheWeek()], humStr, tempStr);
  lcd.print(line1);
}

// ═════════════════════════════════════════════════════════════════════════════
//  QUICK SETTINGS MENU
// FIX: Added checkAlarm() inside loops so alarm is not missed
// ═════════════════════════════════════════════════════════════════════════════

void quickSettingsMenu() {
  buttonClickBeep();
  int selectedOption = 0;
  bool menuActive    = true;
  unsigned long lastNav = millis();
  const unsigned long navDelay = 200;
  lcd.clear();

  while (menuActive) {
    // FIX: Check alarm inside blocking menu
    checkAlarm();
    if (alarmTriggered) { lcd.clear(); return; }

    lcd.setCursor(0, 0);
    lcd.print(selectedOption == 0 ? ">1.BRIGHTNESS   " : " 1.BRIGHTNESS   ");
    lcd.setCursor(0, 1);
    lcd.print(selectedOption == 1 ? ">2.BUZZER       " : " 2.BUZZER       ");

    if (millis() - lastNav > navDelay) {
      if (digitalRead(INCREASE_PIN) == LOW || digitalRead(DECREASE_PIN) == LOW) {
        lastNav = millis(); buttonClickBeep(); selectedOption = 1 - selectedOption;
      }
      if (digitalRead(SET_CONFIRM_PIN) == LOW) { lastNav = millis(); buttonClickBeep(); menuActive = false; }
      if (digitalRead(DISCARD_PIN)     == LOW) { lastNav = millis(); buttonClickBeep(); lcd.clear(); return; }
    }
    delay(20);
  }
  lcd.clear();

  if (selectedOption == 0) {
    bool brightnessActive = true;
    unsigned long lastBtnTime = millis();
    const unsigned long btnDelay = 150;
    while (brightnessActive) {
      checkAlarm();
      if (alarmTriggered) { lcd.clear(); return; }

      lcd.setCursor(0, 0); lcd.print("BRIGHTNESS ADJ  ");
      lcd.setCursor(0, 1);
      int barLen = map(brightnessLevel, 0, 1000, 0, 10);
      lcd.print("[");
      for (int i = 0; i < 10; i++) lcd.print(i < barLen ? (char)255 : ' ');
      lcd.print("]");
      char bBuf[5]; snprintf(bBuf, 5, "%4d", brightnessLevel); lcd.print(bBuf);
      if (millis() - lastBtnTime > btnDelay) {
        // When user manually changes brightness
if (digitalRead(INCREASE_PIN) == LOW) {
  lastBtnTime = millis(); buttonClickBeep();
  brightnessLevel += 50; if (brightnessLevel > 1000) brightnessLevel = 1000;
  if (systemOn) setBacklight(brightnessLevel);
  // Set manual override for 10 seconds
  manualBrightnessOverride = true;
  manualOverrideEndTime = millis() + 10000;
}
        if (digitalRead(DECREASE_PIN) == LOW) {
          lastBtnTime = millis(); buttonClickBeep();
          brightnessLevel -= 50; if (brightnessLevel < 0) brightnessLevel = 0;
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
      checkAlarm();
      if (alarmTriggered) { lcd.clear(); return; }

      lcd.setCursor(0, 0); lcd.print("BUZZER TOGGLE   ");
      lcd.setCursor(0, 1);
      if (buzzerEnabled) lcd.print("STATUS:  ON  >  ");
      else               lcd.print("STATUS:  OFF >  ");
      if (millis() - lastBtnTime > btnDelay) {
        if (digitalRead(INCREASE_PIN) == LOW || digitalRead(DECREASE_PIN) == LOW) {
          lastBtnTime = millis(); buzzerEnabled = !buzzerEnabled;
          if (buzzerEnabled) { tone(BUZZER_PIN, CLICK_BEEP_FREQ); delay(CLICK_BEEP_MS * 2); noTone(BUZZER_PIN); }
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

// ═════════════════════════════════════════════════════════════════════════════
//  MOTIVATION MODE
// ═════════════════════════════════════════════════════════════════════════════

void startMotivationMode(int style) {
  motivationMode = true; motivationStyle = style; messageStyle = style;
  currentMotivationIndex = random(0, totalMessages);
  lastMotivationChange = millis();
  animationPosition = 0; cursorPos = 0; cursorPos2 = 0;
  charCount = 0; charCount2 = 0; animationCompleted = false;
  line1Completed = false; line2Completed = false; animationStage = 0;
  String msg = String(motivationMessages[currentMotivationIndex]);
  splitMessage(msg);
  if (style == 3) {
    customMessage = msg; customMessageLine2 = "";
    if (customMessage.indexOf('@') != -1) customMessage.replace("@", " ");
    scrollText = customMessage + "    ";
  }
  lcd.clear();
}

void changeMotivationMessage() {
  currentMotivationIndex = random(0, totalMessages);
  String msg = String(motivationMessages[currentMotivationIndex]);
  splitMessage(msg);
  animationPosition = 0; cursorPos = 0; cursorPos2 = 0;
  charCount = 0; charCount2 = 0; animationCompleted = false;
  line1Completed = false; line2Completed = false; animationStage = 0;
  if (motivationStyle == 3) {
    customMessage = msg; customMessageLine2 = "";
    if (customMessage.indexOf('@') != -1) customMessage.replace("@", " ");
    scrollText = customMessage + "    ";
  }
  lcd.clear();
}

// ═════════════════════════════════════════════════════════════════════════════
//  TEXT DISPLAY STYLES
// ═════════════════════════════════════════════════════════════════════════════

void displayStyle1() {
  lcd.setCursor(0, 0); lcd.print(customMessage);
  for (int i = customMessage.length(); i < 16; i++) lcd.print(" ");
  lcd.setCursor(0, 1);
  if (customMessageLine2.length() > 0) {
    lcd.print(customMessageLine2);
    for (int i = customMessageLine2.length(); i < 16; i++) lcd.print(" ");
  } else lcd.print("                ");
}

void displayStyle2() {
  int s1 = max(0, (int)(16 - customMessage.length()) / 2);
  lcd.setCursor(s1, 0); lcd.print(customMessage);
  for (int i = s1 + customMessage.length(); i < 16; i++) lcd.print(" ");
  // FIX: Clear left side too
  for (int i = 0; i < s1; i++) { lcd.setCursor(i, 0); lcd.print(" "); }
  lcd.setCursor(0, 1);
  if (customMessageLine2.length() > 0) {
    int s2 = max(0, (int)(16 - customMessageLine2.length()) / 2);
    for (int i = 0; i < s2; i++) lcd.print(" ");
    lcd.setCursor(s2, 1); lcd.print(customMessageLine2);
    for (int i = s2 + customMessageLine2.length(); i < 16; i++) lcd.print(" ");
  } else lcd.print("                ");
}

void displayStyle3() {
  // FIX: Guard against empty scrollText
  if (scrollText.length() == 0) { lcd.setCursor(0,0); lcd.print("                "); return; }
  lcd.setCursor(0, 0);
  int sLen = scrollText.length();
  String d = "";
  for (int i = 0; i < 16; i++) {
    d += scrollText[(animationPosition + i) % sLen];
  }
  lcd.print(d);
  lcd.setCursor(0, 1); lcd.print("                ");
  if (millis() - lastAnimationUpdate > textSpeed) {
    lastAnimationUpdate = millis();
    animationPosition = (animationPosition + 1) % sLen;
  }
}

void displayStyle4() {
  if (millis() - lastBlinkUpdate > textSpeed) { lastBlinkUpdate = millis(); blinkState = !blinkState; }
  if (blinkState) { displayStyle2(); }
  else { lcd.setCursor(0,0); lcd.print("                "); lcd.setCursor(0,1); lcd.print("                "); }
}

void displayStyle5() {
  if (!animationCompleted) {
    if (animationStage == 0) {
      if (millis() - lastAnimationUpdate > textSpeed) {
        lastAnimationUpdate = millis();
        if (cursorPos <= (int)customMessage.length()) cursorPos++;
        if (cursorPos > (int)customMessage.length()) { line1Completed = true; animationStage = 1; cursorPos2 = 0; lastAnimationUpdate = millis(); }
      }
      int s = max(0, (int)(16 - customMessage.length()) / 2);
      // Clear row first
      lcd.setCursor(0, 0); lcd.print("                ");
      lcd.setCursor(s, 0); lcd.print(customMessage.substring(0, cursorPos));
      if (cursorPos < (int)customMessage.length()) { lcd.setCursor(s + cursorPos, 0); lcd.print("_"); }
      lcd.setCursor(0, 1); lcd.print("                ");
    } else if (animationStage == 1 && customMessageLine2.length() > 0) {
      if (millis() - lastAnimationUpdate > textSpeed) {
        lastAnimationUpdate = millis();
        if (cursorPos2 <= (int)customMessageLine2.length()) cursorPos2++;
        if (cursorPos2 > (int)customMessageLine2.length()) { line2Completed = true; animationCompleted = true; }
      }
      int s1 = max(0, (int)(16 - customMessage.length()) / 2);
      lcd.setCursor(0, 0); lcd.print("                ");
      lcd.setCursor(s1, 0); lcd.print(customMessage);
      int s2 = max(0, (int)(16 - customMessageLine2.length()) / 2);
      lcd.setCursor(0, 1); lcd.print("                ");
      lcd.setCursor(s2, 1); lcd.print(customMessageLine2.substring(0, cursorPos2));
      if (cursorPos2 < (int)customMessageLine2.length()) { lcd.setCursor(s2 + cursorPos2, 1); lcd.print("_"); }
    } else if (animationStage == 1) animationCompleted = true;
  } else { displayStyle2(); }
}

void displayStyle6() {
  if (!animationCompleted) {
    if (animationStage == 0) {
      if (millis() - lastCharTime > (unsigned long)textSpeed && charCount < (int)customMessage.length()) { lastCharTime = millis(); charCount++; }
      if (charCount >= (int)customMessage.length()) { line1Completed = true; animationStage = 1; charCount2 = 0; lastCharTime = millis(); }
      int s = max(0, (int)(16 - customMessage.length()) / 2);
      lcd.setCursor(0, 0); lcd.print("                ");
      lcd.setCursor(s, 0);
      lcd.print(customMessage.substring(0, charCount));
      for (int i = charCount; i < (int)customMessage.length(); i++) lcd.print("_");
      lcd.setCursor(0, 1); lcd.print("                ");
    } else if (animationStage == 1 && customMessageLine2.length() > 0) {
      if (millis() - lastCharTime > (unsigned long)textSpeed && charCount2 < (int)customMessageLine2.length()) { lastCharTime = millis(); charCount2++; }
      if (charCount2 >= (int)customMessageLine2.length()) { line2Completed = true; animationCompleted = true; }
      int s1 = max(0, (int)(16 - customMessage.length()) / 2);
      lcd.setCursor(0, 0); lcd.print("                ");
      lcd.setCursor(s1, 0); lcd.print(customMessage);
      int s2 = max(0, (int)(16 - customMessageLine2.length()) / 2);
      lcd.setCursor(0, 1); lcd.print("                ");
      lcd.setCursor(s2, 1);
      lcd.print(customMessageLine2.substring(0, charCount2));
      for (int i = charCount2; i < (int)customMessageLine2.length(); i++) lcd.print("_");
    } else if (animationStage == 1) animationCompleted = true;
  } else { displayStyle2(); }
}

// ═════════════════════════════════════════════════════════════════════════════
//  ALARM SETUP
// ═════════════════════════════════════════════════════════════════════════════

void setupAlarm() {
  int tempHour12 = alarmHour % 12; if (tempHour12 == 0) tempHour12 = 12;
  bool tempIsPM  = (alarmHour >= 12);
  int  tempMinute = alarmMinute;
  int  settingStep = 0;
  unsigned long lastBlinkTime = 0; bool fieldVisible = true;
  lcd.clear();

  while (currentMode == ALARM_MODE) {
    checkAlarm();
    if (alarmTriggered) { lcd.clear(); return; }

    if (digitalRead(DISCARD_PIN)     == LOW && millis() - lastButtonPress > debounceDelay) { lastButtonPress = millis(); buttonClickBeep(); currentMode = CLOCK_MODE; lcd.clear(); return; }
    if (digitalRead(MODE_SELECT_PIN) == LOW && millis() - lastButtonPress > debounceDelay) { lastButtonPress = millis(); buttonClickBeep(); currentMode = CLOCK_MODE; lcd.clear(); return; }
    if (millis() - lastIncPress > incDecDebounce && digitalRead(INCREASE_PIN) == LOW) {
      lastIncPress = millis(); buttonClickBeep();
      if (settingStep == 0) { tempHour12++; if (tempHour12 > 12) tempHour12 = 1; }
      else if (settingStep == 1) { tempMinute = (tempMinute + 1) % 60; }
      else { tempIsPM = !tempIsPM; }
    }
    if (millis() - lastDecPress > incDecDebounce && digitalRead(DECREASE_PIN) == LOW) {
      lastDecPress = millis(); buttonClickBeep();
      if (settingStep == 0) { tempHour12--; if (tempHour12 < 1) tempHour12 = 12; }
      else if (settingStep == 1) { tempMinute = (tempMinute - 1 + 60) % 60; }
      else { tempIsPM = !tempIsPM; }
    }
    if (millis() - lastButtonPress > debounceDelay && digitalRead(SET_CONFIRM_PIN) == LOW) {
      lastButtonPress = millis(); buttonClickBeep();
      if (settingStep < 2) { settingStep++; }
      else {
        alarmHour   = tempIsPM ? (tempHour12 == 12 ? 12 : tempHour12 + 12) : (tempHour12 == 12 ? 0 : tempHour12);
        alarmMinute = tempMinute; alarmSecond = 0;
        alarmActive = true; alarmTriggered = false;
        saveAlarmToEEPROM();
        lcd.clear(); return;
      }
    }
    if (millis() - lastBlinkTime > 500) { lastBlinkTime = millis(); fieldVisible = !fieldVisible; }
    char hBuf[3], mBuf[3];
    snprintf(hBuf, 3, "%02d", tempHour12); snprintf(mBuf, 3, "%02d", tempMinute);
    const char* ap = tempIsPM ? "PM" : "AM";
    lcd.setCursor(0, 0); lcd.print("SET ALARM TIME  ");
    lcd.setCursor(0, 1);
    lcd.print(settingStep == 0 ? (fieldVisible ? hBuf : "  ") : hBuf);
    lcd.print(":");
    lcd.print(settingStep == 1 ? (fieldVisible ? mBuf : "  ") : mBuf);
    lcd.print(" ");
    lcd.print(settingStep == 2 ? (fieldVisible ? ap : "  ") : ap);
    lcd.print("        ");
    delay(50);
  }
}

// ═════════════════════════════════════════════════════════════════════════════
//  TIMER
//  FIX: Pause logic completely rewritten — no double subtract
// ═════════════════════════════════════════════════════════════════════════════

void setupTimer() {
  int step = 0, tH = timerHours, tM = timerMinutes, tS = timerSeconds;
  unsigned long lastBlinkTime = 0; bool fieldVisible = true;
  lcd.clear();

  while (currentMode == TIMER_MODE) {
    checkAlarm();
    if (alarmTriggered) { lcd.clear(); return; }

    if (digitalRead(DISCARD_PIN)     == LOW && millis() - lastButtonPress > debounceDelay) { lastButtonPress = millis(); buttonClickBeep(); currentMode = CLOCK_MODE; lcd.clear(); return; }
    if (digitalRead(MODE_SELECT_PIN) == LOW && millis() - lastButtonPress > debounceDelay) { lastButtonPress = millis(); buttonClickBeep(); currentMode = CLOCK_MODE; lcd.clear(); return; }
    if (millis() - lastIncPress > incDecDebounce && digitalRead(INCREASE_PIN) == LOW) {
      lastIncPress = millis(); buttonClickBeep();
      switch (step) { case 0: tH = (tH+1)%24; break; case 1: tM = (tM+1)%60; break; case 2: tS = (tS+1)%60; break; }
    }
    if (millis() - lastDecPress > incDecDebounce && digitalRead(DECREASE_PIN) == LOW) {
      lastDecPress = millis(); buttonClickBeep();
      switch (step) { case 0: tH = (tH-1+24)%24; break; case 1: tM = (tM-1+60)%60; break; case 2: tS = (tS-1+60)%60; break; }
    }
    if (millis() - lastButtonPress > debounceDelay && digitalRead(SET_CONFIRM_PIN) == LOW) {
      lastButtonPress = millis(); buttonClickBeep();
      if (step < 2) { step++; }
      else {
        timerHours = tH; timerMinutes = tM; timerSeconds = tS;
        timerRemaining = ((unsigned long)timerHours*3600UL + timerMinutes*60UL + timerSeconds) * 1000UL;
        timerRunning = false; timerPaused = false; timerExpired = false;
        lcd.clear(); runTimer(); return;
      }
    }
    if (millis() - lastBlinkTime > 500) { lastBlinkTime = millis(); fieldVisible = !fieldVisible; }
    char hB[3], mB[3], sB[3];
    snprintf(hB, 3, "%02d", tH); snprintf(mB, 3, "%02d", tM); snprintf(sB, 3, "%02d", tS);
    lcd.setCursor(0, 0); lcd.print("SET TIMER       ");
    lcd.setCursor(0, 1);
    lcd.print(step == 0 ? (fieldVisible ? hB : "  ") : hB); lcd.print(":");
    lcd.print(step == 1 ? (fieldVisible ? mB : "  ") : mB); lcd.print(":");
    lcd.print(step == 2 ? (fieldVisible ? sB : "  ") : sB); lcd.print("        ");
    delay(50);
  }
}

void runTimer() {
  lcd.clear();
  timerRunning   = true;
  timerPaused    = false;
  timerExpired   = false;
  timerStartTime = millis();
  lastDisplaySecond = -1;

  while (currentMode == TIMER_MODE && !timerExpired) {
    checkAlarm();
    if (alarmTriggered) { timerRunning = false; buzzerContinuousStop(); lcd.clear(); return; }

    if (digitalRead(DISCARD_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
      lastButtonPress = millis(); buttonClickBeep();
      timerRunning = false; timerPaused = false; timerExpired = false;
      buzzerContinuousStop(); currentMode = CLOCK_MODE; lcd.clear(); return;
    }
    if (digitalRead(MODE_SELECT_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
      lastButtonPress = millis(); buttonClickBeep();
      timerRunning = false; timerPaused = false; timerExpired = false;
      buzzerContinuousStop(); currentMode = CLOCK_MODE; lcd.clear(); return;
    }

    // FIX: Proper pause/resume — remaining is snapshot at pause time, no double subtract
    if (millis() - lastButtonPress > debounceDelay && digitalRead(SET_CONFIRM_PIN) == LOW) {
      lastButtonPress = millis(); buttonClickBeep();
      if (timerRunning && !timerPaused) {
        // Pause: snapshot remaining
        timerRemaining -= (millis() - timerStartTime);
        timerPaused = true;
        timerRunning = false;
      } else if (timerPaused) {
        // Resume
        timerStartTime = millis();
        timerRunning = true;
        timerPaused = false;
      }
    }

    if (timerRunning && !timerPaused) {
      unsigned long elapsed = millis() - timerStartTime;
      if (elapsed >= timerRemaining) {
        timerExpired   = true;
        timerRunning   = false;
        timerPaused    = false;
        timerRemaining = 0;
        lastTimerBeep  = millis();
        buzzerContinuousStart();
        lcd.clear();
      }
    }

    // Display
    unsigned long dispRemaining = timerPaused ? timerRemaining
                                              : (timerRunning ? timerRemaining - (millis() - timerStartTime) : timerRemaining);
    if (timerRunning && !timerPaused) {
      // Tick per second
      DateTime now = rtc.now();
      int cs = now.second();
      if (cs != lastDisplaySecond) { lastDisplaySecond = cs; tickTockBeep(); }
    }

    int h = dispRemaining / 3600000UL;
    int m = (dispRemaining % 3600000UL) / 60000UL;
    int s = (dispRemaining % 60000UL) / 1000UL;
    lcd.setCursor(0, 0);
    if (timerPaused) lcd.print("TIMER: PAUSED   ");
    else             lcd.print("TIMER:          ");
    lcd.setCursor(7, 0);
    char ts[9];
    if (h > 0) snprintf(ts, 9, "%02d:%02d:%02d", h, m, s);
    else       snprintf(ts, 6, "%02d:%02d", m, s);
    lcd.print(ts);

    DateTime now2 = rtc.now();
    int nH12 = now2.hour() % 12; if (nH12 == 0) nH12 = 12;
    lcd.setCursor(0, 1); lcd.print("NOW: ");
    char ns[13]; snprintf(ns, 13, "%02d:%02d:%02d %s", nH12, now2.minute(), now2.second(), (now2.hour()>=12)?"PM":"AM");
    lcd.print(ns);
    delay(10);
  }

  // Timer expired screen
  while (timerExpired && currentMode == TIMER_MODE) {
    checkAlarm();
    if (alarmTriggered) { buzzerContinuousStop(); timerExpired = false; lcd.clear(); return; }

    if (digitalRead(DISCARD_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
      lastButtonPress = millis(); buttonClickBeep();
      buzzerContinuousStop(); timerExpired = false; currentMode = CLOCK_MODE; lcd.clear(); return;
    }
    if (digitalRead(MODE_SELECT_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
      lastButtonPress = millis(); buttonClickBeep();
      buzzerContinuousStop(); timerExpired = false; currentMode = CLOCK_MODE; lcd.clear(); return;
    }
    if (millis() - lastTimerBeep > 500) { lastTimerBeep = millis(); timerBlinkState = !timerBlinkState; }
    lcd.setCursor(0, 0);
    if (timerBlinkState) lcd.print("    TIME UP!    "); else lcd.print("                ");
    DateTime now = rtc.now();
    lcd.setCursor(0, 1);
    char ns[17]; snprintf(ns, 17, "NOW: %02d:%02d:%02d", now.hour(), now.minute(), now.second());
    lcd.print(ns);
    delay(10);
  }
}

// ═════════════════════════════════════════════════════════════════════════════
//  ALARM CHECK & DISPLAY
// ═════════════════════════════════════════════════════════════════════════════

void checkAlarm() {
  if (!alarmActive || alarmTriggered) return;
  DateTime now = rtc.now();
  if (now.hour() == alarmHour && now.minute() == alarmMinute && now.second() == 0) {
    alarmTriggered = true; alarmActive = false;
    saveAlarmToEEPROM();
    lastAlarmBeep = millis(); lastAlarmBlink = millis();
    // FIX: Stop study buzzer before starting alarm buzzer
    studyBuzzerActive = false;
    studyRunning      = false;   // FIX: halt study session variables too
    noTone(BUZZER_PIN);
    buzzerContinuousStart();
    lcd.clear();
  }
}

void displayAlarmMode() {
  DateTime now = rtc.now();
  int cs = now.second();
  if (alarmActive && !alarmTriggered && cs != lastDisplayAlarmSecond) { lastDisplayAlarmSecond = cs; tickTockBeep(); }
  int dH12 = alarmHour % 12; if (dH12 == 0) dH12 = 12;
  const char* ap = (alarmHour >= 12) ? "PM" : "AM";
  lcd.setCursor(0, 0);
  char alStr[17]; snprintf(alStr, 17, "ALARM: %02d:%02d %s ", dH12, alarmMinute, ap);
  lcd.print(alStr);
  int nH12 = now.hour() % 12; if (nH12 == 0) nH12 = 12;
  const char* nap = (now.hour() >= 12) ? "PM" : "AM";
  lcd.setCursor(0, 1);
  char ns[17]; snprintf(ns, 17, "NOW: %02d:%02d:%02d %s", nH12, now.minute(), now.second(), nap);
  lcd.print(ns);
}

void displayAlarmScreen() {
  unsigned long lastBlink = 0;
  bool blinkL = false;
  while (alarmTriggered) {
    unsigned long cm = millis();
    auto stopAlarm = [&]() {
      buzzerContinuousStop(); alarmTriggered = false; alarmActive = false;
      saveAlarmToEEPROM(); lcd.clear();
    };
    if (digitalRead(DISCARD_PIN)     == LOW && cm - lastButtonPress > debounceDelay) { lastButtonPress = cm; buttonClickBeep(); stopAlarm(); return; }
    if (digitalRead(MODE_SELECT_PIN) == LOW && cm - lastButtonPress > debounceDelay) { lastButtonPress = cm; buttonClickBeep(); stopAlarm(); return; }
    if (digitalRead(SET_CONFIRM_PIN) == LOW && cm - lastButtonPress > debounceDelay) { lastButtonPress = cm; buttonClickBeep(); stopAlarm(); return; }
    if (cm - lastBlink >= 500) { lastBlink = cm; blinkL = !blinkL; }
    lcd.setCursor(0, 0);
    if (blinkL) lcd.print("  ** ALARM! **  "); else lcd.print("                ");
    DateTime now = rtc.now();
    lcd.setCursor(0, 1);
    char ns[17]; snprintf(ns, 17, "NOW: %02d:%02d:%02d", now.hour(), now.minute(), now.second());
    lcd.print(ns);
    delay(10);
  }
}

// ═════════════════════════════════════════════════════════════════════════════
//  STUDY MODE — SETUP SCREENS
// ═════════════════════════════════════════════════════════════════════════════

bool studyExitCheck() {
  if (digitalRead(DISCARD_PIN)     == LOW && millis() - lastButtonPress > debounceDelay) { lastButtonPress = millis(); buttonClickBeep(); return true; }
  if (digitalRead(MODE_SELECT_PIN) == LOW && millis() - lastButtonPress > debounceDelay) { lastButtonPress = millis(); buttonClickBeep(); return true; }
  return false;
}

bool setupStudyDuration() {
  int step = 0;
  int tHIdx = studyHourIdx, tMIdx = studyMinIdx;
  unsigned long lastBlink = 0; bool fv = true;
  lcd.clear();

  while (true) {
    checkAlarm(); if (alarmTriggered) return false;
    if (studyExitCheck()) return false;
    if (millis() - lastIncPress > incDecDebounce && digitalRead(INCREASE_PIN) == LOW) {
      lastIncPress = millis(); buttonClickBeep();
      if (step == 0) tHIdx = (tHIdx + 1) % studyHourCount;
      else           tMIdx = (tMIdx + 1) % studyMinCount;
    }
    if (millis() - lastDecPress > incDecDebounce && digitalRead(DECREASE_PIN) == LOW) {
      lastDecPress = millis(); buttonClickBeep();
      if (step == 0) tHIdx = (tHIdx - 1 + studyHourCount) % studyHourCount;
      else           tMIdx = (tMIdx - 1 + studyMinCount)  % studyMinCount;
    }
    if (millis() - lastButtonPress > debounceDelay && digitalRead(SET_CONFIRM_PIN) == LOW) {
      lastButtonPress = millis(); buttonClickBeep();
      if (step == 0) { step = 1; fv = true; lastBlink = millis(); }
      else {
        studyHourIdx  = tHIdx; studyMinIdx    = tMIdx;
        studyDurHours = studyHourOptions[tHIdx]; studyDurMinutes = studyMinOptions[tMIdx];
        lcd.clear(); return true;
      }
    }
    if (millis() - lastBlink > 500) { lastBlink = millis(); fv = !fv; }
    char hBuf[3], mBuf[3];
    snprintf(hBuf, 3, "%02d", studyHourOptions[tHIdx]);
    snprintf(mBuf, 3, "%02d", studyMinOptions[tMIdx]);
    lcd.setCursor(0, 0); lcd.print("STUDY DURATION  ");
    lcd.setCursor(0, 1);
    lcd.print("HR:"); lcd.print(step == 0 ? (fv ? hBuf : "  ") : hBuf);
    lcd.print("  MIN:"); lcd.print(step == 1 ? (fv ? mBuf : "  ") : mBuf);
    lcd.print("   ");
    delay(50);
  }
}

bool setupBreakInterval() {
  int tIdx = breakIntIdx;
  unsigned long lastBlink = 0; bool fv = true;
  lcd.clear();

  while (true) {
    checkAlarm(); if (alarmTriggered) return false;
    if (studyExitCheck()) return false;
    if (millis() - lastIncPress > incDecDebounce && digitalRead(INCREASE_PIN) == LOW) {
      lastIncPress = millis(); buttonClickBeep();
      tIdx = (tIdx + 1) % breakIntCount;
    }
    if (millis() - lastDecPress > incDecDebounce && digitalRead(DECREASE_PIN) == LOW) {
      lastDecPress = millis(); buttonClickBeep();
      tIdx = (tIdx - 1 + breakIntCount) % breakIntCount;
    }
    if (millis() - lastButtonPress > debounceDelay && digitalRead(SET_CONFIRM_PIN) == LOW) {
      lastButtonPress = millis(); buttonClickBeep();
      breakIntIdx = tIdx; breakIntervalMinutes = breakIntOptions[tIdx];
      lcd.clear(); return true;
    }
    if (millis() - lastBlink > 500) { lastBlink = millis(); fv = !fv; }
    char buf[3]; snprintf(buf, 3, "%02d", breakIntOptions[tIdx]);
    lcd.setCursor(0, 0); lcd.print("BREAK INTERVAL  ");
    lcd.setCursor(0, 1);
    lcd.print("MIN: "); lcd.print(fv ? buf : "  "); lcd.print("           ");
    delay(50);
  }
}

bool setupBreakDuration() {
  int tIdx = breakDurIdx;
  unsigned long lastBlink = 0; bool fv = true;
  lcd.clear();

  while (true) {
    checkAlarm(); if (alarmTriggered) return false;
    if (studyExitCheck()) return false;
    if (millis() - lastIncPress > incDecDebounce && digitalRead(INCREASE_PIN) == LOW) {
      lastIncPress = millis(); buttonClickBeep();
      tIdx = (tIdx + 1) % breakDurCount;
    }
    if (millis() - lastDecPress > incDecDebounce && digitalRead(DECREASE_PIN) == LOW) {
      lastDecPress = millis(); buttonClickBeep();
      tIdx = (tIdx - 1 + breakDurCount) % breakDurCount;
    }
    if (millis() - lastButtonPress > debounceDelay && digitalRead(SET_CONFIRM_PIN) == LOW) {
      lastButtonPress = millis(); buttonClickBeep();
      breakDurIdx = tIdx; breakDurationMinutes = breakDurOptions[tIdx];
      lcd.clear(); return true;
    }
    if (millis() - lastBlink > 500) { lastBlink = millis(); fv = !fv; }
    char buf[3]; snprintf(buf, 3, "%02d", breakDurOptions[tIdx]);
    lcd.setCursor(0, 0); lcd.print("BREAK DURATION  ");
    lcd.setCursor(0, 1);
    lcd.print("BREAK: "); lcd.print(fv ? buf : "  "); lcd.print(" MIN        ");
    delay(50);
  }
}

// ── Start study session ───────────────────────────────────────────────────────
void startStudySession() {
  studyTotalMs    = ((unsigned long)studyDurHours   * 3600UL
                   + (unsigned long)studyDurMinutes * 60UL) * 1000UL;
  breakIntervalMs = (unsigned long)breakIntervalMinutes * 60UL * 1000UL;
  breakDurationMs = (unsigned long)breakDurationMinutes * 60UL * 1000UL;

  if (studyTotalMs == 0)    { currentMode = CLOCK_MODE; lcd.clear(); return; }
  if (breakIntervalMs == 0) breakIntervalMs = studyTotalMs;

  studyRunning      = true;
  studyExpired      = false;
  studyConfigured   = true;
  inBreak           = false;
  breakIntroPlayed  = false;
  studyElapsedMs    = 0;
  studyStartTime    = millis();
  nextBreakAt       = studyStartTime + breakIntervalMs;
  _lastStudySec     = -1;
  _lastBreakSec     = -1;
  studyBlinkLast    = millis();
  studyBlinkState   = false;

  studyBuzzerActive = false;
  studyBuzzerState  = 0;

  lcd.clear();
  Serial.println("-> Study session started");
  SerialBT.println("-> Study session started");
}

// ═════════════════════════════════════════════════════════════════════════════
//  STUDY TICK  —  non-blocking, called every loop()
//  FIX: Break timing uses separate breakActualStart variable
//       tickStudyBuzzer() now returns completed state via out param
// ═════════════════════════════════════════════════════════════════════════════

void tickStudy() {
  if (!studyRunning && !studyExpired) return;

  // ── Non-blocking study buzzer tick ───────────────────────────────────────
  if (studyBuzzerActive) {
    int completedState = 0;
    bool done = tickStudyBuzzer(completedState);
    if (done) {
      if (completedState == 1) {
        // Break intro buzzer finished — now start break countdown
        breakIntroPlayed  = true;
        breakActualStart  = millis();   // FIX: real break countdown starts NOW
        _lastBreakSec     = -1;
        if (currentMode == STUDY_MODE) lcd.clear();
      } else if (completedState == 2) {
        // Period-start buzzer finished — resume study display
        _lastStudySec = -1;
        if (currentMode == STUDY_MODE) lcd.clear();
      }
    }
    return;  // Yield while buzzer is active
  }

  // ── Alarm check ──────────────────────────────────────────────────────────
  checkAlarm();
  if (alarmTriggered) {
    studyRunning      = false;
    studyBuzzerActive = false;
    noTone(BUZZER_PIN);
    return;
  }

  unsigned long nowMs = millis();
  studyElapsedMs = nowMs - studyStartTime;

  // ── Session expired ───────────────────────────────────────────────────────
  if (!studyExpired && studyElapsedMs >= studyTotalMs) {
    studyExpired      = true;
    studyRunning      = false;
    studyBuzzerActive = false;
    noTone(BUZZER_PIN);
    buzzerContinuousStart();
    studyBlinkLast  = millis();
    studyBlinkState = false;
    lcd.clear();
    return;
  }

  // ── Break START trigger ───────────────────────────────────────────────────
  if (!inBreak && !studyExpired && nowMs >= nextBreakAt) {
    inBreak          = true;
    breakStartTime   = nowMs;
    breakIntroPlayed = false;
    _lastBreakSec    = -1;

    if (currentMode == STUDY_MODE) {
      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("  BREAK TIME!   ");
      lcd.setCursor(0, 1); lcd.print(" BUZZER ALERT   ");
    }
    startStudyBuzzer(1);
    return;
  }

  // ── Break intro waiting for buzzer ───────────────────────────────────────
  if (inBreak && !breakIntroPlayed) return;

  // ── Inside break (after intro buzzer) ────────────────────────────────────
  if (inBreak) {
    // FIX: Use breakActualStart for elapsed, not breakStartTime + 10000
    unsigned long breakElapsed = millis() - breakActualStart;

    if (breakElapsed >= breakDurationMs) {
      // Break over
      inBreak          = false;
      breakIntroPlayed = false;
      _lastStudySec    = -1;
      _lastBreakSec    = -1;
      nextBreakAt      = millis() + breakIntervalMs;
      studyStartTime   = millis() - studyElapsedMs;  // keep elapsed accurate

      if (currentMode == STUDY_MODE) {
        lcd.clear();
        lcd.setCursor(0, 0); lcd.print("PERIOD STARTING ");
        lcd.setCursor(0, 1); lcd.print(" BUZZER ALERT   ");
      }
      startStudyBuzzer(2);
      return;
    }

    // Per-second tick during break
    DateTime bNow = rtc.now();
    int bSec = bNow.second();
    if (bSec != _lastBreakSec) { _lastBreakSec = bSec; tickTockBeep(); }

    if (currentMode != STUDY_MODE) return;

    unsigned long breakRemMs = breakDurationMs - breakElapsed;
    int bM = breakRemMs / 60000UL;
    int bS = (breakRemMs % 60000UL) / 1000UL;

    if (millis() - studyBlinkLast > 500) { studyBlinkLast = millis(); studyBlinkState = !studyBlinkState; }
    lcd.setCursor(0, 0);
    if (studyBlinkState) lcd.print("  BREAK TIME    "); else lcd.print("                ");

    int cH12 = bNow.hour() % 12; if (cH12 == 0) cH12 = 12;
    const char* cap = (bNow.hour() >= 12) ? "PM" : "AM";
    lcd.setCursor(0, 1);
    char brk[17];
    snprintf(brk, 17, "B=%02d:%02d %02d:%02d%s", bM, bS, cH12, bNow.minute(), cap);
    lcd.print(brk);
    return;
  }

  // ── Normal study period ───────────────────────────────────────────────────
  DateTime sNow = rtc.now();
  int sSec = sNow.second();
  if (sSec != _lastStudySec) { _lastStudySec = sSec; tickTockBeep(); }

  if (currentMode != STUDY_MODE) return;

  unsigned long elH  = studyElapsedMs / 3600000UL;
  unsigned long elM  = (studyElapsedMs % 3600000UL) / 60000UL;
  unsigned long totH = studyTotalMs / 3600000UL;
  unsigned long totM = (studyTotalMs % 3600000UL) / 60000UL;

  char stdBuf[17];
  snprintf(stdBuf, 17, "STD=%02lu:%02lu (%01lu:%02lu)", elH, elM, totH, totM);
  lcd.setCursor(0, 0); lcd.print(stdBuf);

  unsigned long toBreak = (nextBreakAt > millis()) ? (nextBreakAt - millis()) : 0;
  int tbM = toBreak / 60000UL;
  int tbS = (toBreak % 60000UL) / 1000UL;
  int cH12 = sNow.hour() % 12; if (cH12 == 0) cH12 = 12;
  const char* cap2 = (sNow.hour() >= 12) ? "PM" : "AM";
  lcd.setCursor(0, 1);
  char brkBuf[17];
  snprintf(brkBuf, 17, "BK=%02d:%02d %02d:%02d%s", tbM, tbS, cH12, sNow.minute(), cap2);
  lcd.print(brkBuf);
}

// ── Study TIME UP screen (non-blocking) ──────────────────────────────────────
void tickStudyExpired() {
  if (!studyExpired) return;
  if (millis() - studyBlinkLast > 500) { studyBlinkLast = millis(); studyBlinkState = !studyBlinkState; }

  if (currentMode == STUDY_MODE) {
    lcd.setCursor(0, 0);
    if (studyBlinkState) lcd.print("    TIME UP!    "); else lcd.print("                ");
    DateTime now = rtc.now();
    lcd.setCursor(0, 1);
    char ns[17]; //snprintf(ns, 17, "NOW: %02d:%02d:%02d", now.hour(), now.minute(), now.second());
    snprintf(ns, 17, "WELL DONE CHAMP!");
    lcd.print(ns);
  }

  if (millis() - lastButtonPress > debounceDelay) {
    bool anyBtn = (digitalRead(DISCARD_PIN)     == LOW ||
                   digitalRead(MODE_SELECT_PIN) == LOW ||
                   digitalRead(SET_CONFIRM_PIN) == LOW);
    if (anyBtn) {
      lastButtonPress = millis(); buttonClickBeep();
      buzzerContinuousStop();
      studyExpired      = false;
      studyRunning      = false;
      studyConfigured   = false;
      studyBuzzerActive = false;
      noTone(BUZZER_PIN);
      lcd.clear();
    }
  }
}

// ═════════════════════════════════════════════════════════════════════════════
//  COMMAND PROCESSOR
//  FIX: settimer via BT/Serial now sets up timer non-blocking (no runTimer call)
// ═════════════════════════════════════════════════════════════════════════════

void processCommand(String cmd) {
  cmd.trim();
  if (cmd == "home") {
    motivationMode = false; messageStyle = 0; customMessage = ""; customMessageLine2 = "";
    systemOn = true; currentMode = CLOCK_MODE; clock2Running = false;
    alarmTriggered = false; timerExpired = false; timerRunning = false; timerPaused = false;
    studyRunning = false; studyExpired = false; studyConfigured = false;
    studyBuzzerActive = false;
    buzzerContinuousStop(); setBacklight(brightnessLevel); lcd.clear(); lcd.backlight();
    Serial.println("-> Clock mode"); SerialBT.println("-> Clock mode"); return;
  }
  if (cmd == "offme") {
    motivationMode = false; systemOn = false;
    setBacklight(0); lcd.noBacklight(); lcd.clear();
    Serial.println("-> System OFF"); SerialBT.println("-> System OFF"); return;
  }
  if (cmd == "onme") {
    systemOn = true; setBacklight(brightnessLevel); lcd.backlight(); lcd.clear();
    messageStyle = 0; motivationMode = false;
    Serial.println("-> System ON"); SerialBT.println("-> System ON"); return;
  }

  auto intParam = [&](int start) -> int {
    int cp = cmd.indexOf(')');
    return (cp != -1) ? cmd.substring(start, cp).toInt() : -1;
  };

  if (cmd.startsWith("bright("))      { int v = intParam(7);  if (v >= 0 && v <= 1000) { brightnessLevel = v; if (systemOn) setBacklight(v); } return; }
  if (cmd.startsWith("speed("))       { int v = intParam(6);  if (v >= 1 && v <= 1000) { animationSpeed = v; updateSpeed(); } return; }
  if (cmd.startsWith("nightbright(")) { int v = intParam(12); if (v >= 0 && v <= 1000) nightBrightness = v; return; }
  if (cmd.startsWith("daybright("))   { int v = intParam(10); if (v >= 0 && v <= 1000) dayBrightness   = v; return; }
  if (cmd.startsWith("nightstart("))  { int v = intParam(11); if (v >= 0 && v <= 23)   nightStartHour  = v; return; }
  if (cmd.startsWith("morningend("))  { int v = intParam(11); if (v >= 0 && v <= 23)   morningEndHour  = v; return; }
  if (cmd == "autobright on")  { autoBrightnessEnabled = true;  return; }
  if (cmd == "autobright off") { autoBrightnessEnabled = false; return; }
  if (cmd.startsWith("tickfreq("))  { int v = intParam(9); if (v > 0) tickFreq  = v; return; }
  if (cmd.startsWith("tockfreq("))  { int v = intParam(9); if (v > 0) tockFreq  = v; return; }
  if (cmd.startsWith("tickdur("))   { int v = intParam(8); if (v > 0) tickDurMs = v; return; }
  if (cmd.startsWith("tockdur("))   { int v = intParam(8); if (v > 0) tockDurMs = v; return; }
  if (cmd == "buzzeron")  { buzzerEnabled = true;  Serial.println("-> Buzzer ON");  SerialBT.println("-> Buzzer ON");  return; }
  if (cmd == "buzzeroff") { buzzerEnabled = false; noTone(BUZZER_PIN); Serial.println("-> Buzzer OFF"); SerialBT.println("-> Buzzer OFF"); return; }

  if (cmd.startsWith("mv(")) {
    int cp = cmd.indexOf(')');
    if (cp != -1) {
      int style = cmd.substring(3, cp).toInt();
      if (style==2||style==4||style==5||style==6) { currentMode = CLOCK_MODE; startMotivationMode(style); }
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
        animationPosition=0; cursorPos=0; cursorPos2=0; charCount=0; charCount2=0;
        animationCompleted=false; line1Completed=false; line2Completed=false; animationStage=0;
        splitMessage(msg);
        if (style == 3) {
          customMessage = msg; customMessageLine2 = "";
          if (customMessage.indexOf('@') != -1) customMessage.replace("@", " ");
        }
        scrollText = customMessage + "    "; lcd.clear();
      }
    }
    return;
  }
  if (cmd.startsWith("SET:")) {
    cmd.remove(0, 4); int rH,rM,rS,rD,rMo,rYr;
    sscanf(cmd.c_str(), "%d,%d,%d,%d,%d,%d", &rH,&rM,&rS,&rD,&rMo,&rYr);
    rtc.adjust(DateTime(rYr,rMo,rD,rH,rM,rS));
    Serial.println("-> RTC updated!"); SerialBT.println("-> RTC updated!"); return;
  }
  if (cmd.startsWith("setalarm:")) {
    cmd.remove(0, 9); int aH,aM,aS; sscanf(cmd.c_str(), "%d,%d,%d", &aH,&aM,&aS);
    alarmHour=aH; alarmMinute=aM; alarmSecond=aS; alarmActive=true; alarmTriggered=false;
    saveAlarmToEEPROM(); return;
  }
  if (cmd == "alarmon")  { alarmActive = true;  alarmTriggered = false; saveAlarmToEEPROM(); return; }
  if (cmd == "alarmoff") { alarmActive = false; alarmTriggered = false; buzzerContinuousStop(); saveAlarmToEEPROM(); return; }

  // FIX: settimer via BT/Serial — don't call blocking runTimer(), just configure
  if (cmd.startsWith("settimer:")) {
    cmd.remove(0, 9); int tH,tM,tS; sscanf(cmd.c_str(), "%d,%d,%d", &tH,&tM,&tS);
    timerHours=tH; timerMinutes=tM; timerSeconds=tS;
    timerRemaining = ((unsigned long)timerHours*3600UL + timerMinutes*60UL + timerSeconds)*1000UL;
    timerRunning=false; timerPaused=false; timerExpired=false;
    timerStartTime = millis();
    currentMode=TIMER_MODE;
    // Timer will start when user presses SET button in timer mode
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(" -TIMER MODE-   ");
    lcd.setCursor(0,1); lcd.print(" ENTER TO START ");
    Serial.println("-> Timer set, press SET to start"); SerialBT.println("-> Timer set, press SET to start");
    return;
  }
  if (cmd == "timerstop") {
    timerRunning=false; timerPaused=false; timerExpired=false;
    buzzerContinuousStop(); currentMode=CLOCK_MODE; lcd.clear(); return;
  }

  // Default: show as text message
  if (cmd.length() > 0) {
    motivationMode=false; messageStyle=2; currentMode=CLOCK_MODE;
    splitMessage(cmd); lcd.clear(); return;
  }
}

// ═════════════════════════════════════════════════════════════════════════════
//  SETUP
// ═════════════════════════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_Clock");
  delay(500);

  EEPROM.begin(EEPROM_SIZE);
  //randomSeed(analogRead(0));
    randomSeed(analogRead(34) + analogRead(35) + analogRead(36) + analogRead(0) +  millis());

  pinMode(MODE_SELECT_PIN, INPUT_PULLUP);
  pinMode(SET_CONFIRM_PIN, INPUT_PULLUP);
  pinMode(INCREASE_PIN,    INPUT_PULLUP);
  pinMode(DECREASE_PIN,    INPUT_PULLUP);
  pinMode(DISCARD_PIN,     INPUT_PULLUP);
  pinMode(BUZZER_PIN,      OUTPUT);
  noTone(BUZZER_PIN);

  ledcAttach(BACKLIGHT_PIN, PWM_FREQ, 8);
  setBacklight(brightnessLevel);

  Wire.begin(I2C_SDA, I2C_SCL);
  delay(100);
  dht.begin();
  delay(1000);

  Serial.println("-- I2C Scan --");
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) Serial.printf("  Found: 0x%02X\n", addr);
  }
  Serial.println("--------------");

  lcd.init(); lcd.init(); delay(50);
  lcd.backlight(); lcd.clear();

  if (!rtc.begin()) { lcd.setCursor(0,0); lcd.print("RTC Error!"); while (1); }

  loadAlarmFromEEPROM();

  lcd.setCursor(0, 0); lcd.print("RAHUL'S  CLOCK");
  lcd.setCursor(3, 1); lcd.print("ESP32  v24.2");
  delay(1800);
  lcd.clear();
}

// ═════════════════════════════════════════════════════════════════════════════
//  DHT CACHE
// ═════════════════════════════════════════════════════════════════════════════

void updateDHT() {
  if (millis() - lastDHTRead < 2000) return;
  lastDHTRead = millis();
  float t = dht.readTemperature(), hh = dht.readHumidity();
  if (!isnan(t))  cachedTemp     = t;
  if (!isnan(hh)) cachedHumidity = hh;
}

// ═════════════════════════════════════════════════════════════════════════════
//  MAIN LOOP
// ═════════════════════════════════════════════════════════════════════════════

void loop() {
  yield();

  if (SerialBT.available() > 0) { String cmd = SerialBT.readStringUntil('\n'); processCommand(cmd); }
  if (Serial.available()   > 0) { String cmd = Serial.readStringUntil('\n');   processCommand(cmd); }

  // ── Alarm check ───────────────────────────────────────────────────────────
  checkAlarm();
  if (alarmTriggered) { displayAlarmScreen(); delay(10); return; }

  // ── Study expired screen ──────────────────────────────────────────────────
  if (studyExpired) {
    tickStudyExpired();
    if (studyExpired) { delay(10); return; }
  }

  // ── Study background tick ─────────────────────────────────────────────────
  if (studyRunning) tickStudy();

  // ── Auto-brightness ───────────────────────────────────────────────────────
  applyAutoBrightness();

  // ── DISCARD long-press → Quick Settings ──────────────────────────────────
  if (!studyRunning) {
    bool discardNow = (digitalRead(DISCARD_PIN) == LOW);
    if (discardNow && !discardHeld) { discardHeld = true; discardPressStart = millis(); longPressHandled = false; }
    if (discardHeld && discardNow && !longPressHandled) {
      if (millis() - discardPressStart >= LONG_PRESS_MS) {
        longPressHandled = true; lastButtonPress = millis(); quickSettingsMenu();
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
        motivationMode = false; messageStyle = 0; lastClock2Second = -1;
        showModeSelectMessage = true; modeMessageStartTime = millis();
        lcd.clear(); lcd.setCursor(0,0); lcd.print(" -CLOCK2 MODE-  ");

      } else if (currentMode == CLOCK2_MODE) {
        currentMode = ALARM_MODE; clock2Running = false;
        motivationMode = false; messageStyle = 0;
        showModeSelectMessage = true; modeMessageStartTime = millis();
        lastDisplayAlarmSecond = -1;
        lcd.clear(); lcd.setCursor(0,0); lcd.print(" -ALARM MODE-   ");

      } else if (currentMode == ALARM_MODE) {
        currentMode = TIMER_MODE;
        showModeSelectMessage = true; modeMessageStartTime = millis();
        lcd.clear(); lcd.setCursor(0,0); lcd.print(" -TIMER MODE-   ");

      } else if (currentMode == TIMER_MODE) {
        currentMode = MOTIVATION_MODE;
        timerRunning = false; timerPaused = false; timerExpired = false;
        buzzerContinuousStop();
        showModeSelectMessage = true; modeMessageStartTime = millis();
        lcd.clear(); lcd.setCursor(0,0); lcd.print("-MOTIVATN MODE- ");

      } else if (currentMode == MOTIVATION_MODE) {
        currentMode = STUDY_MODE;
        motivationMode = false; messageStyle = 0;
        showModeSelectMessage = true; modeMessageStartTime = millis();
        lcd.clear(); lcd.setCursor(0,0);
        if (studyRunning) lcd.print(" STD RUNNING... ");
        else              lcd.print(" -STUDY MODE-   ");

      } else if (currentMode == STUDY_MODE) {
        currentMode = CLOCK_MODE;
        showModeSelectMessage = false;
        lcd.clear();
      }
    }

    // DISCARD short press
    if (digitalRead(DISCARD_PIN) == LOW && !longPressHandled) {
      lastButtonPress = millis(); buttonClickBeep();
      if (alarmActive && !alarmTriggered) {
        alarmActive = false; saveAlarmToEEPROM();
        Serial.println("-> Alarm discarded"); SerialBT.println("-> Alarm discarded");
        lcd.clear(); lcd.setCursor(0,0); lcd.print("  ALARM CLEARED ");
        delay(1000); lcd.clear();
      } else if (studyRunning && currentMode == STUDY_MODE) {
        studyRunning      = false;
        studyExpired      = false;
        studyConfigured   = false;
        studyBuzzerActive = false;
        buzzerContinuousStop();
        noTone(BUZZER_PIN);
        currentMode = CLOCK_MODE; lcd.clear();
      } else if (currentMode != CLOCK_MODE) {
        currentMode = CLOCK_MODE; clock2Running = false;
        alarmTriggered = false; timerExpired = false;
        timerRunning = false; timerPaused = false;
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
      if (alarmActive && !alarmTriggered) displayAlarmMode();
      else if (!alarmActive) {
        lcd.setCursor(0,0); lcd.print(" -ALARM MODE-   ");
        lcd.setCursor(0,1); lcd.print(" ENTER TO SET   ");
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
        lcd.setCursor(0,0); lcd.print(" -TIMER MODE-   ");
        lcd.setCursor(0,1);
        if (timerRemaining > 0) {
          unsigned long dr = timerRemaining;
          int h = dr/3600000UL, m=(dr%3600000UL)/60000UL, s=(dr%60000UL)/1000UL;
          char ts[12]; snprintf(ts, 12, " READY:%02d:%02d:%02d", h, m, s); lcd.print(ts);
        } else lcd.print(" ENTER TO SET   ");
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
      if (!studyConfigured && !studyRunning && !studyExpired) {
        lcd.setCursor(0,0); lcd.print(" -STUDY MODE-   ");
        lcd.setCursor(0,1); lcd.print(" ENTER TO START ");
        if (digitalRead(SET_CONFIRM_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
          lastButtonPress = millis(); buttonClickBeep();
          if (setupStudyDuration() && setupBreakInterval() && setupBreakDuration()) {
            startStudySession();
          } else {
            currentMode = CLOCK_MODE; lcd.clear();
          }
        }
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
      DateTime rtcNow  = rtc.now();
      bool showColon   = ((millis() % 1000) < 500);
      char sep         = showColon ? ':' : ' ';
      int hour24       = rtcNow.hour();
      const char* ampm = (hour24 < 12) ? "AM" : "PM";
      int hour12       = hour24 % 12; if (hour12 == 0) hour12 = 12;
      int dispTemp     = (int)cachedTemp;
      int dispHumidity = (int)cachedHumidity;

      lcd.setCursor(0, 0);
      char row0[17];
      snprintf(row0, 17, "%02d%c%02d %s T=%c%2d%cC", hour12, sep, rtcNow.minute(), ampm,
               (dispTemp < 10 ? ' ' : 0), dispTemp, (char)223);
      // simpler safe approach:
      lcd.print(hour12 < 10 ? "0" : ""); lcd.print(hour12);
      lcd.print(sep);
      lcd.print(rtcNow.minute() < 10 ? "0" : ""); lcd.print(rtcNow.minute());
      lcd.print(" "); lcd.print(ampm);
      lcd.print(" T=");
      if (dispTemp < 10) lcd.print(" ");
      lcd.print(dispTemp); lcd.print((char)223); lcd.print("C");

      lcd.setCursor(0, 1);
      lcd.print(months[rtcNow.month()-1]);
      if (rtcNow.day() < 10) lcd.print("0");
      lcd.print(rtcNow.day());
      lcd.print(" "); lcd.print(days[rtcNow.dayOfTheWeek()]);
      lcd.print("  H=");
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
