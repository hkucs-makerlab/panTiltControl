#include <Servo.h>

// uncomment one to select the way of control
#define __JOYSTCIK__
//#define __NUNCHUK__
//#define __GOBLE__
//
#ifdef __NUNCHUK__
#include "Nunchuk.h"
#endif

#ifdef __GOBLE__
#include "GoBLE.hpp"
#endif

#ifdef __JOYSTCIK__
#define JOYSTICK_X_PIN       A0
#define JOYSTICK_Y_PIN       A1
#define JOYSTICK_SWITCH_PIN  9
#endif

//#define __DEBUG__
//#define __SOFTWARE_SERIAL__

#ifdef __SOFTWARE_SERIAL__
#include <SoftwareSerial.h>
#define Console Serial
#define BT_RX_PIN 13
#define BT_TX_PIN 12
SoftwareSerial BlueTooth(BT_RX_PIN, BT_TX_PIN);
#ifdef __GOBLE__  
_GoBLE<SoftwareSerial, HardwareSerial> Goble(BlueTooth, Console);
#endif
#else
#define Console Serial
#define BlueTooth Serial
#ifdef __GOBLE__  
_GoBLE<HardwareSerial, HardwareSerial> Goble(BlueTooth, Console);
#endif
#endif
//
#define BAUD_RATE 38400

//
#define __UPWARD '1'
#define __DOWNWARD '2'
#define __RIGHT '3'
#define __LEFT '4'
#define __CENTER '7'
#define __FIRE '8'
#define __HALT 'h'


boolean revX = false;
boolean revY = false;
const int panInterval = 5;
const int panMax = 180;
const int panMid = 90;
const int panMin = 0;
const int tiltInterval = 5;
const int tiltMax = 140;
const int tiltMid = 90;
const int tiltMin = 65;

int yy = tiltMid;
int xx = panMid;
#ifdef __DEBUG__
int yy2 = tiltMid;
int xx2 = panMid;
#endif

#define TRIGGER_OFF 40
#define TRIGGER_ON  0
#define FIRE_SERVO_PIN    2

//
#define PAN_SERVO_PIN     3
#define TILT_SERVO_PIN    4

Servo fireServo;  // to hit an end stop switch
Servo panServo;
Servo tiltServo;

void setup() {
#ifdef __GOBLE__  
  Goble.begin(BAUD_RATE);
#endif  
#ifdef __DEBUG__
  Console.begin(115200);
  Console.println("in debugging mode");
#endif

  fireServo.attach(FIRE_SERVO_PIN);
  fireServo.write(TRIGGER_OFF);
  panServo.attach(PAN_SERVO_PIN);
  tiltServo.attach(TILT_SERVO_PIN);

#ifdef __JOYSTCIK__
  pinMode(JOYSTICK_SWITCH_PIN, INPUT_PULLUP);
#endif

#ifdef __NUNCHUK__
  nunchuk_init();
#endif

#ifdef __DEBUG__
  Console.println("console started");
#endif

}

void loop() {
  static unsigned long prev_time = 0;
  static unsigned long prev_fire_time = 0;
  static unsigned long prev_turn_time = 0;
  static unsigned long prev_reset_time = 0;
  unsigned long cur_time;
  static char cmd[3] = {__CENTER, __CENTER, __HALT};
  int angle = TRIGGER_OFF;

  cur_time = millis();
#ifdef __NUNCHUK__
  check_nunchuk(cmd);
#elif defined __JOYSTCIK__
  check_joystick(cmd);
#elif defined __GOBLE__  
 check_goble(cmd);
#else
#endif
  if (cur_time - prev_turn_time >= 150) {
    prev_turn_time = cur_time;
    switch (cmd[0]) {
      case __UPWARD:
        if (yy + tiltInterval <= tiltMax)
          yy += tiltInterval;
        break;
      case __DOWNWARD:
        if (yy - tiltInterval >= tiltMin)
          yy -= tiltInterval;
        break;
      case __CENTER:
        yy = tiltMid;
        break;
    }

    switch (cmd[1]) {
      case __RIGHT:
        if (xx + panInterval <= panMax)
          xx += panInterval;
        break;
      case __LEFT:
        if (xx - panInterval >= panMin)
          xx -= panInterval;
        break;
      case __CENTER:
        xx  = panMid;
        break;
    }
  }
  //
  if (cmd[2] == __FIRE) {
    angle = TRIGGER_ON;
    prev_fire_time = cur_time;
  }
  if (cur_time - prev_fire_time >= 1000) {
    angle = TRIGGER_OFF;
  }
  fireServo.write(angle);

  //
  if (cur_time - prev_time >= 200) {
    prev_time = cur_time;
    panServo.write(xx);
    tiltServo.write(yy);
#ifdef __DEBUG__
    Console.print("cmd0: "); Console.print(cmd[0]);
    Console.print(", cmd1: "); Console.print(cmd[1]);
    Console.print(", cmd2: "); Console.println(cmd[2]);
    if (yy != yy2 || xx != xx2) {
      Console.print("yy=");
      Console.print(yy);
      Console.print(", xx=");
      Console.print(xx);
      Console.println();
      xx2 = xx;
      yy2 = yy;
    }
#endif
  }
  /*
    if ( cur_time - prev_reset_time >= 2000 ) {
      if (xx == panMin || xx == panMax) xx = panMid;
      if (yy == tiltMin || yy == tiltMax) yy = tiltMid;
      prev_reset_time = cur_time;
    }
  */
}

#ifdef __NUNCHUK__
void check_nunchuk(char *cmd) {
  int joystickX, joystickY, switchState;
  String msg;

  if (!nunchuk_read()) {
    return;
  }

  //nunchuk_print();
  if (nunchuk_buttonC() && nunchuk_buttonZ()) {
    cmd[0] = __CENTER;
    cmd[1] = __CENTER;
    cmd[2] = __HALT;
    return;
  }

  if (nunchuk_buttonC()) {
    //Console.println("Pressed button C");
    float pitch_angle = nunchuk_pitch() * 180 / M_PI;
    if (pitch_angle >= -90 && pitch_angle <= 90) {
      joystickY = map(pitch_angle, 60, -90, 255, 0);
    } else {
      joystickY = 128;
    }
    float roll_angle = nunchuk_roll() * 180 / M_PI;
    if (roll_angle >= -90 && roll_angle <= 90) {
      joystickX = map(roll_angle, -90, 90, 0, 255);
    } else {
      joystickY = 128;
    }
    //msg = "Pitch: " + String(pitch_angle) + ", Roll: " + String(roll_angle);
    //Console.println(msg);
  } else {
    joystickX = map(nunchuk_joystickX(), -126, 127, 0, 255);
    joystickY = map(nunchuk_joystickY(), -126, 127, 0, 255);
  }
  //msg = "X: " + String(joystickX) + " ,Y:" + String(joystickY);
  //Console.println(msg);
  if (joystickY > 210) {
    cmd[0] = __UPWARD ;
  } else if (joystickY < 90) {
    cmd[0] = __DOWNWARD;
  } else {
    cmd[0] = __HALT;
  }
  if (joystickX > 190) {
    cmd[1] = __LEFT;
  } else if (joystickX < 50) {
    cmd[1] = __RIGHT ;
  } else  {
    cmd[1] = __HALT;
  }
  if (nunchuk_buttonZ()) {
    //Console.println("Pressed button Z");
    cmd[2] = __FIRE;
  } else {
    cmd[2] = __HALT;
  }
}
#endif

#ifdef __JOYSTCIK__
void check_joystick( char *cmd) {
  int tilt, pan, fire;

  pan = map(analogRead(JOYSTICK_X_PIN), 0, 1023, 0, 255);
  if (pan > 190) {
    cmd[0] =  __UPWARD;
  } else if (pan < 50) {
    cmd[0] = __DOWNWARD ;
  } else {
    cmd[0] = __HALT;
  }

  tilt = map(analogRead(JOYSTICK_Y_PIN), 0, 1023, 0, 255);
  if (tilt > 190) {
    cmd[1] = __LEFT;
  } else if (tilt < 50) {
    cmd[1] = __RIGHT ;
  } else  {
    cmd[1] = __HALT;
  }

  fire = digitalRead(JOYSTICK_SWITCH_PIN);
  if (fire == 0) {
    cmd[2] = __FIRE;
  } else {
    cmd[2] = __HALT;
  }
}
#endif

#ifdef __GOBLE__ 
void check_goble(char *cmd) {
  int joystickX = 0;
  int joystickY = 0;

  if (!Goble.available()) {
    return;
  }
  joystickX = Goble.readJoystickX();
  joystickY = Goble.readJoystickY();

  if (joystickX > 190) {
    cmd[0] = revY ? __UPWARD : __DOWNWARD;
  } else if (joystickX < 80) {
    cmd[0] = revY ? __DOWNWARD : __UPWARD;
  } else if (Goble.readSwitchUp() == PRESSED) {
    //cmd = '1';
    cmd[0] = revY ? __UPWARD : __DOWNWARD;
  } else if (Goble.readSwitchDown() == PRESSED) {
    //cmd = '2';
    cmd[0] = revY ? __DOWNWARD : __UPWARD;
  } else {
    cmd[0] = __HALT;
  }

  if (joystickY > 190) {
    cmd[1] = revX ? __LEFT : __RIGHT;
  } else if (joystickY < 80) {
    cmd[1] = revX ?   __RIGHT : __LEFT;
  } else if (Goble.readSwitchLeft() == PRESSED) {
    //cmd = '3';
    cmd[0] = revX ? __LEFT : __RIGHT;
  } else if (Goble.readSwitchRight() == PRESSED) {
    //cmd = '4';
    cmd[0] =  revX ?   __RIGHT : __LEFT;
  } else  {
    cmd[1] = __HALT;
  }

  if (Goble.readSwitchSelect() == PRESSED) {
    //cmd = '5';
    revY = !revY;
  } else if (Goble.readSwitchStart() == PRESSED) {
    //cmd = '6';
    revX = !revX;
  }

  if (Goble.readSwitchAction() == PRESSED) {
    //cmd = '7';
    cmd[0] = __CENTER;
    cmd[1] = __CENTER;
  }

  if (Goble.readSwitchMid() == PRESSED) {
    //cmd = '8';
    cmd[2] = __FIRE;
  } else {
    cmd[2] = __HALT;
  }
}
#endif
