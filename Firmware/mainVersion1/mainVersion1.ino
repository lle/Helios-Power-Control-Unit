/*
Keypad: Alexander Brevig (playground.arduino.cc/Code/Keypad)
LiquidCrystal: David A. Mellis 2008 (arduino.cc/en/Tutorial/LiquidCrystal)
Eeprom: Christopher Andrews 2015 (arduino.cc/en/Reference/EEPROM)
*/

#include <Keypad.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

//define pin
#define BUZZER_PIN 11
#define RELAY_PIN 13
#define LED_RED_PIN 10
#define LED_GREEN_PIN 9

//define states
#define BUZZER_ON HIGH
#define BUZZER_OFF LOW
#define RELAY_ON HIGH
#define RELAY_OFF LOW

//define state machine
#define STATE_START 1
#define STATE_INPUT 2
#define STATE_PROCESS_PW 3
#define STATE_PW_GOOD 4
#define STATE_PW_BAD 5
#define STATE_ON 6
#define STATE_OFF 7
#define STATE_ADMINLOGIN 8
#define STATE_PROCESS_ADMIN_PW 9
#define STATE_ADMIN_PW_GOOD 10
#define STATE_ADMINMODE 11
#define STATE_NEW_PW 12
#define STATE_NEW_TIMEOUT 13
#define STATE_WAIT_1SEC 99

//LCD Variable
#define LCD_DEFAULT 0
#define LCD_DONE 1

//=========================== Variables =======================
//read from eeprom
uint8_t power_defaultTimeout;
char password_current[5];

//user input
char password_enterred[5] = {'-', '-', '-', '-', '-'};
uint8_t password_inputCurrentIndex = 0;
//user menu 
int8_t admin_menuIndex = 0;
int8_t timeout_menuIndex = 0;

//state machine variable
long unsigned int wait_timeStamp = 0;
uint8_t CURRENT_STATE = STATE_START;
uint8_t PENDING_STATE = STATE_START;
uint8_t lcd_updateCompleted = LCD_DEFAULT;
long unsigned int powerOff_countdown_inSeconds;

//buzzer timing variable
uint16_t buzzerSch_numBeep = 0;
uint16_t buzzerSch_intervalBeep = 0;
long unsigned int buzzerSch_startTime = 0;
bool buzzerSch_isCompleted = true;
bool buzzerSch_beepState = BUZZER_OFF;

// LCD INIT 
LiquidCrystal lcd(A5, A4, A3, A2, A1, A0);

// KEYPAD INIT 
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {8, 7, 6, 5}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {4, 3, 2}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );


// ********************************** Functions ************************/
void buzzer_setState(uint8_t buzzer_newState)
{
  digitalWrite(BUZZER_PIN, buzzer_newState);
}

void relay_setState(uint8_t relay_newState)
{
  digitalWrite(RELAY_PIN, relay_newState);
}

bool password_addInput(char user_input)
{
  password_enterred[ password_inputCurrentIndex ] = user_input;
  password_inputCurrentIndex++;
  lcd.print( user_input );
  if(password_inputCurrentIndex >= 5)
  {
    password_inputCurrentIndex = 0;
    return true;
  }
  else
    return false;
}

void password_clearAll()
{
  for(int i=0; i<5; i++)
  {
    password_enterred[i] = '-';
    lcd.clear();
  }
}

bool password_process()
{
  bool isPassword_good = true;
  for(int i=0; i<5; i++)
  {
    if( password_enterred[i] != password_current[i] )
      isPassword_good = false;
  }
  if( isPassword_good == false )
  {
    password_clearAll();
    return false;
  }
  else
  {
    password_clearAll();
    return true;
  }
}

void update_adminpw()
{
  for(int i=0; i<5; i++)
  {
    EEPROM.write(i, password_enterred[i]);
  }
  lcd.clear(); lcd.setCursor(0,0); lcd.print("New PW Saved"); lcd.setCursor(0,1); lcd.print("Restarting...");
  buzzer_setState(BUZZER_ON); delay(10); buzzer_setState(BUZZER_OFF);
  delay(2000);
  asm volatile ("  jmp 0"); 
}

void buzzer_scheduleBeep(uint16_t numBeep, uint16_t intervalBeep)
{
  buzzerSch_numBeep = numBeep * 2; //must be multiplied by 2 since we need 2 stage: ON + OFF
  buzzerSch_intervalBeep = intervalBeep;
  buzzerSch_startTime = millis();
  buzzerSch_isCompleted = false;
}

void buzzer_runSchedule()
{
  if( buzzerSch_isCompleted == false && millis() - buzzerSch_intervalBeep > buzzerSch_startTime)
  {
    buzzerSch_startTime = millis(); //update new start time
    buzzerSch_numBeep--;    //decrement number of beep
    if(buzzerSch_beepState == BUZZER_OFF) 
    { 
      digitalWrite(LED_RED_PIN, HIGH);
      buzzerSch_beepState = BUZZER_ON;
      buzzer_setState(BUZZER_ON);
    }
    else
    {
      digitalWrite(LED_RED_PIN, LOW);
      buzzerSch_beepState = BUZZER_OFF;
      buzzer_setState(BUZZER_OFF);
    }

    //when completed, set everything to off
    if(buzzerSch_numBeep <= 0)
    {
      digitalWrite(LED_RED_PIN, LOW);
      buzzer_setState(BUZZER_OFF);
      buzzerSch_beepState = BUZZER_OFF;
      buzzerSch_numBeep = 0;
      buzzerSch_intervalBeep = 0;
      buzzerSch_isCompleted = true;
    }
  }
}

void timeoutMenu_update(int8_t offset)
{
  timeout_menuIndex = timeout_menuIndex + offset;
  //deal with min/max menu
  if      (timeout_menuIndex<0) { timeout_menuIndex = 5; }
  else if (timeout_menuIndex>5) { timeout_menuIndex = 0; }
  
  //generate menu
  if      (timeout_menuIndex == 0) { lcd.setCursor(0,1); lcd.print("15 minutes    "); }
  else if (timeout_menuIndex == 1) { lcd.setCursor(0,1); lcd.print("30 minutes    "); }
  else if (timeout_menuIndex == 2) { lcd.setCursor(0,1); lcd.print("45 minutes    "); }
  else if (timeout_menuIndex == 3) { lcd.setCursor(0,1); lcd.print("60 minutes    "); }
  else if (timeout_menuIndex == 4) { lcd.setCursor(0,1); lcd.print("90 minutes    "); }
  else if (timeout_menuIndex == 5) { lcd.setCursor(0,1); lcd.print("120 minutes   "); }
}

void timeoutMenu_select()
{
  lcd.clear(); lcd.setCursor(0,0); lcd.print("Time Updated"); lcd.setCursor(0,1); lcd.print("Restarting...");
  switch(timeout_menuIndex)
  {
    case 0:
      EEPROM.write(5, 15); break;
    case 1:
      EEPROM.write(5, 30); break;
    case 2:
      EEPROM.write(5, 45); break;
    case 3:
      EEPROM.write(5, 60); break;
    case 4:
      EEPROM.write(5, 90); break;
    case 5:
      EEPROM.write(5, 120); break;
  }
  buzzer_setState(BUZZER_ON); delay(10); buzzer_setState(BUZZER_OFF); 
  delay(2000);
  asm volatile ("  jmp 0");       
}

void adminMenu_update(int8_t offset)
{
  admin_menuIndex = admin_menuIndex + offset;
  //deal with min/max menu
  if      (admin_menuIndex<0) { admin_menuIndex = 2; }
  else if (admin_menuIndex>2) { admin_menuIndex = 0; }
  //generate menu
  if      (admin_menuIndex == 0) { lcd.setCursor(0,1); lcd.print("New Password    "); }
  else if (admin_menuIndex == 1) { lcd.setCursor(0,1); lcd.print("Change Timout   "); }
  else if (admin_menuIndex == 2) { lcd.setCursor(0,1); lcd.print("Exit            "); }
}

void adminMenu_select()
{
  if(admin_menuIndex == 0)
  {
    lcd.clear(); lcd.setCursor(0,0); lcd.print("Enter New PW:"); lcd.setCursor(0,1);
    CURRENT_STATE = STATE_NEW_PW;
  }
  else if(admin_menuIndex == 1)
  {
    CURRENT_STATE = STATE_NEW_TIMEOUT;
  }
  else if(admin_menuIndex == 2)
  {
    asm volatile ("  jmp 0"); 
  }
}

void lcd_showTimeLeft()
{
  static long unsigned int timeLeft_lastUpdate_timeStamp;
  if( millis() - timeLeft_lastUpdate_timeStamp > 1000) //update every second only
  {
    lcd.setCursor(0,1);
    lcd.print("Time Left: ");
    lcd.print(powerOff_countdown_inSeconds);
    lcd.print("  ");
    

    //warning for timeout
    if(powerOff_countdown_inSeconds == 59) //at 60 seconds
      buzzer_scheduleBeep(2, 100);
    else if(powerOff_countdown_inSeconds == 30)
      buzzer_scheduleBeep(3, 100);
    else if(powerOff_countdown_inSeconds == 15)
      buzzer_scheduleBeep(6, 50);
    else if(powerOff_countdown_inSeconds >= 1 && powerOff_countdown_inSeconds < 10)
      buzzer_scheduleBeep(1, 20);
    else if(powerOff_countdown_inSeconds == 0)
      CURRENT_STATE = STATE_OFF;

    powerOff_countdown_inSeconds--;
    timeLeft_lastUpdate_timeStamp = millis();
  }
}

void firstRun()
{
  EEPROM.write(0, '1'); //password characters
  EEPROM.write(1, '2');
  EEPROM.write(2, '3');
  EEPROM.write(3, '4');
  EEPROM.write(4, '5');
  EEPROM.write(5, 60); //default timeout = 60
}

// ********************************** SETUP *********************/
void setup()
{  
  /***************************
  If this is the first time you are running the code, 
  run the function firstRun() to flash the EEPROM with the default password "12345" 
  and default timeout =  60 minutes
  */
  /***************************/
  //firstRun();
  /***************************/
  
  char eepromTest[5];
  for(int i=0; i<5; i++)
  {
    password_current[i] = EEPROM.read(i);     //load stored password
  }
  power_defaultTimeout = EEPROM.read(5);

  
  pinMode(LED_GREEN_PIN, OUTPUT); //LED 2
  pinMode(LED_RED_PIN, OUTPUT);   //LED 1
  pinMode(BUZZER_PIN, OUTPUT);    //Buzzer
  pinMode(RELAY_PIN, OUTPUT);     //Relay

  digitalWrite(LED_RED_PIN, HIGH);      
  digitalWrite(LED_GREEN_PIN, HIGH);
  delay(50);
  buzzer_setState(BUZZER_ON);           //Then Buzz
  delay(50);
  buzzer_setState(BUZZER_OFF);          //Then Flash Green
  delay(50);
  digitalWrite(LED_GREEN_PIN, LOW);
  digitalWrite(LED_RED_PIN, LOW);
  
  
  //Init LCD Init
  lcd.begin(16, 2); 
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Loading");
  lcd.setCursor(0,1);
  for(int i=0; i<16; i++)
  {
    lcd.print(char(255));
    delay(33);
  }
  delay(500);

  //Introduction
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Helios Machine");
  lcd.setCursor(0, 1);
  lcd.print("Control - V1.0");
  delay(1500);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("by   Black Cat");
  lcd.setCursor(0, 1);
  lcd.print("     Robotics");
  delay(1500);
}



// ********************************** MAIN LOOP *********************/
void loop()
{
  char key;
  bool diuUser_input_fiveNumber;
  bool isPasswordCorrect;

  buzzer_runSchedule();

  //STATE MACHINE
  switch(CURRENT_STATE)
  {
    case STATE_START:
      //Execution
      lcd.clear(); lcd.setCursor(0,0); lcd.print("Enter Password");
      CURRENT_STATE = STATE_INPUT;
      lcd.setCursor(0,1);
      break;
      
    case STATE_INPUT:
      //Execution
      key = keypad.getKey();
      diuUser_input_fiveNumber = false;
      if(key) 
      { 
        if(key=='*' && password_inputCurrentIndex == 0)
        {
          lcd.clear(); lcd.setCursor(0,0); lcd.print("Admin Login"); lcd.setCursor(0,1); lcd.print("PW:");
          CURRENT_STATE = STATE_ADMINLOGIN;
          buzzer_scheduleBeep(3, 75);
        }
        else
        {
          diuUser_input_fiveNumber = password_addInput(key); 
          buzzer_scheduleBeep(1, 10);
        }
      }
      //Transition Out
      if( diuUser_input_fiveNumber == true )
        CURRENT_STATE = STATE_PROCESS_PW;
      break;

    case STATE_ADMINLOGIN:
      //Execution
      key = keypad.getKey();
      diuUser_input_fiveNumber = false;
      if(key) 
      { 
        diuUser_input_fiveNumber = password_addInput(key); 
        buzzer_scheduleBeep(1, 10);
      }
      //Transition Out
      if( diuUser_input_fiveNumber == true )
        CURRENT_STATE = STATE_PROCESS_ADMIN_PW;
      break;

    case STATE_PROCESS_PW:
      //Serial.println("State: PROCESS_PW");
      //Execution
      isPasswordCorrect = false;
      isPasswordCorrect = password_process();
      //Transition Out
      if( isPasswordCorrect == true )
      {
        CURRENT_STATE = STATE_PW_GOOD;
        digitalWrite(LED_GREEN_PIN, HIGH);
      }
      else
        CURRENT_STATE = STATE_PW_BAD;
      break;

    case STATE_PROCESS_ADMIN_PW:
      //Execution
      isPasswordCorrect = false;
      isPasswordCorrect = password_process();
      //Transition Out
      if( isPasswordCorrect == true )
        CURRENT_STATE = STATE_ADMIN_PW_GOOD;
      else
        CURRENT_STATE = STATE_PW_BAD;
      break;

    case STATE_PW_GOOD:
      //Execution
      lcd.clear(); lcd.setCursor(0,0); lcd.print("Password OK"); lcd.setCursor(0,1); lcd.print("Powering On");
      relay_setState(RELAY_ON);
      digitalWrite(LED_GREEN_PIN, LOW);
      buzzer_scheduleBeep(2, 50);
      powerOff_countdown_inSeconds = power_defaultTimeout * 60; //defaultTimeOut(minutes) x 60(sec) = total seconds timeout
      //wait state
      wait_timeStamp = millis();
      PENDING_STATE = STATE_ON;
      CURRENT_STATE = STATE_WAIT_1SEC;
      break;

    case STATE_ADMIN_PW_GOOD:
      //Execution
      lcd.clear(); lcd.setCursor(0,0); lcd.print("Password OK"); lcd.setCursor(0,1); lcd.print("Admin Mode");
      buzzer_scheduleBeep(2, 50);
      //wait state
      wait_timeStamp = millis();
      PENDING_STATE = STATE_ADMINMODE;
      CURRENT_STATE = STATE_WAIT_1SEC;
      break;

    case STATE_ADMINMODE:
      //Execution
      if(lcd_updateCompleted != LCD_DONE)
      {
        lcd_updateCompleted = LCD_DONE;
        lcd.clear(); lcd.setCursor(0,0); lcd.print("<- *  MENU  # ->");
        adminMenu_update(0); 
      }
      key = keypad.getKey();
      if(key) //de-bouncer
      {
        //Transition Out
        if( key == '#' )
        {
          adminMenu_update(1); 
          buzzer_scheduleBeep(1, 10); 
        }
        else if( key == '*' )
        {
          adminMenu_update(-1);
          buzzer_scheduleBeep(1, 10);
        }
        else if( key == '0' )
        {
          adminMenu_select();
          buzzer_scheduleBeep(1, 10);
          lcd_updateCompleted = LCD_DEFAULT;
        }
      }
      break;
    
    case STATE_PW_BAD:
      //Execution
       lcd.clear(); lcd.setCursor(0,0); lcd.print("Bad Password");
      relay_setState(RELAY_OFF);
      buzzer_scheduleBeep(5, 50);
      //wait state
      wait_timeStamp = millis();
      PENDING_STATE = STATE_START;
      CURRENT_STATE = STATE_WAIT_1SEC;
      break;

    case STATE_ON:
      //Execution
      if(lcd_updateCompleted != LCD_DONE)
      {
        lcd_updateCompleted = LCD_DONE;
        lcd.clear(); lcd.setCursor(0,0); lcd.print("# to Shutdown"); lcd.setCursor(0,1);
      }
      lcd_showTimeLeft();
      key = keypad.getKey();
      if(key) //de-bouncer
      {
        //Transition Out
        if( key == '#' ) 
        {
          lcd_updateCompleted = LCD_DEFAULT;
          CURRENT_STATE = STATE_OFF; 
        }
      }
      break;

    case STATE_OFF:
      //Execution
      lcd.clear(); lcd.setCursor(0,0); lcd.print("Power Off");
      relay_setState(RELAY_OFF);
      digitalWrite(LED_GREEN_PIN, LOW);
      buzzer_scheduleBeep(2, 50);
      //go into wait state
      wait_timeStamp = millis();
      PENDING_STATE = STATE_START;
      CURRENT_STATE = STATE_WAIT_1SEC;
      break;

    case STATE_WAIT_1SEC: /* NON LOCKING DELAY */
      if(millis() - wait_timeStamp > 2000)
      {
        //Serial.println("State: WAIT");
        CURRENT_STATE = PENDING_STATE;
      }
      break;

    case STATE_NEW_PW:
      //Execution
      key = keypad.getKey();
      diuUser_input_fiveNumber = false;
      if(key) 
      { 
        if(key=='*' || key=='#')
        {
          lcd.clear(); lcd.setCursor(0,0); lcd.print("Cancelled"); 
          buzzer_setState(BUZZER_ON); delay(10); buzzer_setState(BUZZER_OFF);
          delay(2000);
          asm volatile ("  jmp 0"); 
        }
        else
        {
          diuUser_input_fiveNumber = password_addInput(key); 
          buzzer_scheduleBeep(1, 10);
        }
      }
      //Transition Out
      if( diuUser_input_fiveNumber == true )
        update_adminpw();
      break;

    case STATE_NEW_TIMEOUT:
      //Execution
      if(lcd_updateCompleted != LCD_DONE)
      {
        lcd_updateCompleted = LCD_DONE;
        lcd.clear(); lcd.setCursor(0,0); lcd.print("<- *  TIME  # ->");
        timeoutMenu_update(0); 
      }
      key = keypad.getKey();
      if(key) //de-bouncer
      {
        //Transition Out
        if( key == '#' )
        {
          timeoutMenu_update(1); 
          buzzer_scheduleBeep(1, 10); 
        }
        else if( key == '*' )
        {
          timeoutMenu_update(-1);
          buzzer_scheduleBeep(1, 10);
        }
        else if( key == '0' )
        {
          timeoutMenu_select();
          buzzer_scheduleBeep(1, 10);
          lcd_updateCompleted = LCD_DEFAULT;
        }
      }
      break;
    
    default:
      lcd.clear(); lcd.setCursor(0,0); lcd.print("Error: "); lcd.setCursor(0,1); lcd.print("State Unknown");
      break;
  }
}
