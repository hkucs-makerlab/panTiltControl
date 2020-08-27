#include <Servo.h>
#include "GoBLE.hpp"

#define __SOFTWARE_SERIAL__
#ifdef __SOFTWARE_SERIAL__
#include <SoftwareSerial.h>
#define __DEBUG__
#define Console Serial
#define BT_RX_PIN A4
#define BT_TX_PIN A5
SoftwareSerial BlueTooth(BT_RX_PIN, BT_TX_PIN);
_GoBLE<SoftwareSerial, HardwareSerial> Goble(BlueTooth, Console);
#else
#define Console Serial
#define BlueTooth Serial
_GoBLE<HardwareSerial, HardwareSerial> Goble(BlueTooth, Console);
#endif
//
#define BAUD_RATE 38400


//
#define __FORWARD '1'
#define __BACKWARD '2'
#define __RIGHT '3'
#define __LEFT '4'
#define __FIRE '8'
#define __CENTER '7'
#define __HALT 'h'


boolean revX = false;
boolean revY = false;
const int panInterval = 10;
const int tiltInterval = 10;

int yy = 90;
int xx = 90;
#ifdef __DEBUG__
int yy2 = 90;
int xx2 = 90;
#endif

#define TRIGGER_OFF 40
#define FIRE_PIN 2
#define PAN_PIN 3
#define TILT_PIN 4

Servo fireServo;  // to hit an end stop switch
Servo panServo;
Servo tiltServo;

void setup() {
  Goble.begin(BAUD_RATE);
#ifdef __DEBUG__
  Console.begin(115200);
  Console.println("in debugging mode");
#endif

  fireServo.attach(FIRE_PIN);
  fireServo.write(TRIGGER_OFF);
  panServo.attach(PAN_PIN);
  tiltServo.attach(TILT_PIN);

#ifdef __DEBUG__
  Console.println("console started");
#endif

}

void loop() {
  static unsigned long prev_time = 0;
  static unsigned long prev_fire_time = 0;
  unsigned long cur_time;
  char cmd;
  int angle=TRIGGER_OFF;

  cur_time = millis();
  cmd = check_goble();
  if (cur_time - prev_time >= 200) {
    prev_time = cur_time;
    switch (cmd) {
      case __FORWARD:
        if (yy + tiltInterval <= 180)
          yy += 10;
        break;
      case __BACKWARD:
        if (yy - tiltInterval >= 0)
          yy -= 10;
        break;
      case __RIGHT:
        if (xx + panInterval <= 180)
          xx += 10;
        break;
      case __LEFT:
        if (xx - panInterval >= 0)
          xx -= 10;
        break;
      case __CENTER:
        xx = yy = 90;
        break;
    }
#ifdef __DEBUG__
    Console.print("cmd: "); Console.println(cmd);
#endif
  }
#ifdef __DEBUG__
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
  if (cmd == __FIRE) {
    angle = 0;
    prev_fire_time = cur_time;
  } else if (cur_time - prev_fire_time >= 1000) {
    angle = TRIGGER_OFF;
  }
  fireServo.write(angle);
  panServo.write(xx);
  tiltServo.write(yy);
}

char check_goble() {

  static char cmd = __HALT;

  int joystickX = 0;
  int joystickY = 0;

  if (Goble.available()) {
    joystickX = Goble.readJoystickX();
    joystickY = Goble.readJoystickY();

    if (joystickX > 190) {
      cmd = revY ? __BACKWARD : __FORWARD;
    } else if (joystickX < 80) {
      cmd = revY ? __FORWARD : __BACKWARD;
    } else if (joystickY > 190) {
      cmd = revX ? __LEFT : __RIGHT;
    } else if (joystickY < 80) {
      cmd = revX ?   __RIGHT : __LEFT;
    } else
      cmd = __HALT;

    if (Goble.readSwitchUp() == PRESSED) {
      //cmd = '1';
      cmd = __FORWARD;
    } else if (Goble.readSwitchDown() == PRESSED) {
      //cmd = '2';
      cmd = __BACKWARD;
    } else if (Goble.readSwitchLeft() == PRESSED) {
      //cmd = '3';
      cmd = __RIGHT;
    } else if (Goble.readSwitchRight() == PRESSED) {
      //cmd = '4';
      cmd = __LEFT;
    } else if (Goble.readSwitchSelect() == PRESSED) {
      //cmd = '5';
      revY = !revY;
    } else if (Goble.readSwitchStart() == PRESSED) {
      //cmd = '6';
      revX = !revX;
    } else if (Goble.readSwitchAction() == PRESSED) {
      //cmd = '7';
      cmd = __CENTER;
    } else if (Goble.readSwitchMid() == PRESSED) {
      //cmd = '8';
      cmd = __FIRE;   
    } 
  }

  return cmd;
}
