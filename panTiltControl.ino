#include "GPIOServo.hpp"
#include "Buzzer.hpp"

//#define __DEBUG__

// uncomment one to select the way of control
//#define __JOYSTCIK__
//#define __NUNCHUK__
//#define __GOBLE__
#define __PS2_GAMEPAD__

//
#ifdef __NUNCHUK__
#include "Nunchuk.h"
//#define __NUNCHUK__MOTION // use Z button for motion control or laser target pointer
#endif

#ifdef __GOBLE__
#include "GoBLE.hpp"
//#define __SOFTWARE_SERIAL__
#endif
#define BAUD_RATE 115200

// I/O pins
#define FIRE_SERVO_PIN    2
#define PAN_SERVO_PIN     3
#define TILT_SERVO_PIN    4
#define LASER_POINT_PIN   6
#define BUZZER_PIN        7
#ifdef __JOYSTCIK__
#define JOYSTICK_X_PIN       A4
#define JOYSTICK_Y_PIN       A5
#define JOYSTICK_SWITCH_PIN  9
#endif
#ifdef __PS2_GAMEPAD__
#define PS2_ATT 10  // cs
#define PS2_CMD 11  // mosi
#define PS2_DAT 12  // miso
#define PS2_CLK 13  // sck
#endif // __PS2_GAMEPAD__


#ifdef __SOFTWARE_SERIAL__
#include <SoftwareSerial.h>
#define Console Serial
#define BT_RX_PIN 13
#define BT_TX_PIN 12
SoftwareSerial BlueTooth(BT_RX_PIN, BT_TX_PIN);
#ifdef __GOBLE__
_GoBLE<SoftwareSerial, HardwareSerial> Goble(BlueTooth, Console);
#endif //
#else
#define Console Serial
#define BlueTooth Serial
#ifdef __GOBLE__
_GoBLE<HardwareSerial, HardwareSerial> Goble(BlueTooth, Console);
#endif //
#endif // __SOFTWARE_SERIAL__
//
#define __CHECK_IDLE_TIME__
//
#define __UPWARD '1'
#define __DOWNWARD '2'
#define __RIGHT '3'
#define __LEFT '4'
#define __CENTER '7'
#define __FIRE '8'
#define __HALT 'h'
//
#define TRIGGER_OFF 40
#define TRIGGER_ON  0

#ifdef __PS2_GAMEPAD__
#include "PS2X_lib.h" //reference: http://www.billporter.info/
const int RUMBLE = true;
const int PRESSURES = false;
// gamepad variables
int gamepad_error;
byte gamepad_type;
byte gamepad_vibrate;
PS2X ps2x;
#endif //__PS2_GAMEPAD__

Buzzer buzzer(BUZZER_PIN);

GPIOservo fireServo(FIRE_SERVO_PIN); // to hit an end stop switch
GPIOservo panServo(PAN_SERVO_PIN);
GPIOservo tiltServo(TILT_SERVO_PIN);

const int panMax = 180;
const int panMid = 90;
const int panMin = 0;
//
const int tiltMax = 140;
const int tiltMid = 90;
const int tiltMin = 65;

#ifdef __GOBLE__
boolean revX = false;
boolean revY = false;
#endif

void setup() {
#ifdef __GOBLE__
  Goble.begin(BAUD_RATE);
#endif

#if defined(__DEBUG__) && defined(__SOFTWARE_SERIAL__) && defined(__AVR__)
  Console.begin(115200);
#else
  Console.begin(BAUD_RATE);
#endif

#ifdef __PS2_GAMEPAD__
  init_input_ps2();
#endif

  fireServo.attach();
  fireServo.write(TRIGGER_OFF);
  //
  panServo.attach();
  panServo.write(panMid);
  //
  tiltServo.attach();
  tiltServo.write(tiltMid);
  //
  pinMode(LASER_POINT_PIN, OUTPUT);
  digitalWrite(LASER_POINT_PIN, LOW);

#ifdef __JOYSTCIK__
  pinMode(JOYSTICK_SWITCH_PIN, INPUT_PULLUP);
#endif

#ifdef __NUNCHUK__
  nunchuk_init();
#endif

#ifdef __DEBUG__
  Console.println("console started");
  Console.println("in debugging mode");
#endif
  buzzer.beepShort();
}

void loop() {
  //static unsigned long prev_time = 0;
  static unsigned long prev_fire_time = 0;
  unsigned long cur_time;
  static char cmd[3] = {__CENTER, __CENTER, __HALT};
  int angle = TRIGGER_OFF;

  cur_time = millis();

#ifdef __CHECK_IDLE_TIME__
  static long idle_timeout = 0;
  if ((cur_time - idle_timeout) > 60000) {
    panServo.detach();
    tiltServo.detach();
    fireServo.detach();
    digitalWrite(LASER_POINT_PIN, LOW);
    idle_timeout = cur_time;
  }
#endif

#ifdef __NUNCHUK__
  check_nunchuk(cmd);
#elif defined __JOYSTCIK__
  check_joystick(cmd);
#elif defined __GOBLE__
  check_goble(cmd);
#elif defined __PS2_GAMEPAD__
  check_ps2_gamepad(cmd);
#else
#error "No control method defined!"
#endif

#ifdef __CHECK_IDLE_TIME__
  if (cmd[0] != __HALT) tiltServo.attach();
  if (cmd[1] != __HALT) panServo.attach();
  if (cmd[2] != __HALT) fireServo.attach();
#endif

  switch (cmd[0]) {
    case __UPWARD:
      tiltServo.move(tiltMax);
      break;
    case __DOWNWARD:
      tiltServo.move(tiltMin);
      break;
    case __CENTER:
      tiltServo.write(tiltMid);
      break;
  }

  switch (cmd[1]) {
    case __RIGHT:
      panServo.move(panMin);
      break;
    case __LEFT:
      panServo.move(panMax);
      break;
    case __CENTER:
      panServo.write(panMid);
      break;
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
#ifdef __DEBUG__
  if (0) {
    static long last_debug_time = 0;
    long now = millis();
    if (now - 1000 > last_debug_time) {
      Console.print("cmd0: "); Console.print(cmd[0]);
      Console.print(", cmd1: "); Console.print(cmd[1]);
      Console.print(", cmd2: "); Console.println(cmd[2]);
      last_debug_time = now;
    }
  }
#endif
}


#ifdef __PS2_GAMEPAD__
void init_input_ps2()
{
  for (int i = 0; i < 3; i++)
  {
    gamepad_error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_ATT, PS2_DAT, true, true);
    if (gamepad_error == 0)
    {
      Console.println("Found Controller, configured successful;\n");
      break;
    }
    delay(1000);
  }
  if (gamepad_error == 1)
  {
    Console.print("No PS2 controller found: ");
    Console.println(gamepad_error);
    aborted();
  }
  else if (gamepad_error == 2)
  {
    Console.print("PS2 Controller found but not accepting commands: ");
    Console.println(gamepad_error);
    aborted();
  }

  //verify the gamepad type
  gamepad_type = ps2x.readType();
  if (gamepad_type == 0)
    Console.println("Unknown PS2 Controller type found");
  else if (gamepad_type == 1)
    Console.println("DualShock Controller found");
  else if (gamepad_type == 2)
    Console.println("GuitarHero Controller found");
  else if (gamepad_type == 3)
    Console.println("Wireless Sony DualShock Controller found");

  //turn off gamepad vibration
  gamepad_vibrate = 0;
}

void check_ps2_gamepad(char *cmd) {
  static long last_ps2_gamepad_time = 0;

  int joystickX1, joystickY1;
  int joystickX2, joystickY2;
  long now = millis();
  if (now - 20 > last_ps2_gamepad_time) {
    ps2x.read_gamepad(false, gamepad_vibrate);
    joystickX1 = ps2x.Analog(PSS_LX);
    joystickY1 = ps2x.Analog(PSS_LY);
    joystickX2 = ps2x.Analog(PSS_RX);
    joystickY2 = ps2x.Analog(PSS_RY);
#ifdef __DEBUG__
    Console.println("joystickY: " + String(joystickY) + ", joystickX: " + String(joystickX));
#endif
    if (joystickY1 > 210 || joystickY2 > 210 || ps2x.Button(PSB_PAD_DOWN)) {
      cmd[0] = __UPWARD ;
    } else if (joystickY1 < 90 || joystickY2 < 90 || ps2x.Button(PSB_PAD_UP)) {
      cmd[0] = __DOWNWARD;
    } else {
      cmd[0] = __HALT;
    }

    if (joystickX1 > 190 || joystickX2 > 190 || ps2x.Button(PSB_PAD_RIGHT)) {
      cmd[1] = __RIGHT;
    } else if (joystickX1 < 50 || joystickX2 < 50 || ps2x.Button(PSB_PAD_LEFT) ) {
      cmd[1] = __LEFT ;
    } else  {
      cmd[1] = __HALT;
    }

    if (ps2x.Button(PSB_L1) || ps2x.Button(PSB_R1)) {
      static long last_laser_time = 0;
      if (now - 500 > last_laser_time) {
        int value = digitalRead(LASER_POINT_PIN);
        digitalWrite(LASER_POINT_PIN, !value );
        last_laser_time = now;
      }
#ifdef __DEBUG__
      Console.println("PSB_L1 pressed");
#endif
    }

    if (ps2x.Button(PSB_L2) || ps2x.Button(PSB_R2)) {
      cmd[2] = __FIRE;
#ifdef __DEBUG__
      Console.println("PSB_L2 pressed");
#endif
    } else {
      cmd[2] = __HALT;
    }

    if (ps2x.Button(PSB_START)) {
      cmd[0] = __CENTER;
      cmd[1] = __CENTER;
      cmd[2] = __HALT;
      return;
    }
    last_ps2_gamepad_time = now;
  }
}
#endif //__PS2_GAMEPAD__

#ifdef __NUNCHUK__
void check_nunchuk(char *cmd) {
  static long last_nunchuk_time = 0;

  int joystickX, joystickY, switchState;
  String msg;
  long now = millis();
  if (now - 200 > last_nunchuk_time) {
    if (!nunchuk_read()) {
      return;
    }
    if (nunchuk_buttonC() && nunchuk_buttonZ()) {
      cmd[0] = __CENTER;
      cmd[1] = __CENTER;
      cmd[2] = __HALT;
      return;
    }
    if (nunchuk_buttonZ()) {
      //Console.println("Pressed button Z");
#ifdef __NUNCHUK__MOTION
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
      //Console.println("Pitch: " + String(pitch_angle) + ", Roll: " + String(roll_angle));
#else //__NUNCHUK__MOTION
      static long last_laser_time = 0;
      if (now - 500 > last_laser_time) {
        int value = digitalRead(LASER_POINT_PIN);
        digitalWrite(LASER_POINT_PIN, !value );
        last_laser_time = now;
      }
#endif  //__NUNCHUK__MOTION
    } else {
      joystickX = map(nunchuk_joystickX(), -126, 127, 0, 255);
      joystickY = map(nunchuk_joystickY(), -126, 127, 0, 255);
    }

    if (joystickY > 210) {
      cmd[0] = __UPWARD ;
    } else if (joystickY < 90) {
      cmd[0] = __DOWNWARD;
    } else {
      cmd[0] = __HALT;
    }
    if (joystickX > 190) {
      cmd[1] = __RIGHT;
    } else if (joystickX < 50) {
      cmd[1] = __LEFT ;
    } else  {
      cmd[1] = __HALT;
    }
    if (nunchuk_buttonC()) {
      cmd[2] = __FIRE;
    } else {
      cmd[2] = __HALT;
    }
    last_nunchuk_time = now;
  }
}
#endif

#ifdef __JOYSTCIK__
void check_joystick( char *cmd) {
  static long last_joystick_time = 0;
  int tilt, pan, fire;
  long now = millis();
  if (now - 200 > last_joystick_time) {
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
    last_joystick_time = now;
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

  if (joystickY > 190) {
    cmd[0] = revY ? __UPWARD : __DOWNWARD;
  } else if (joystickY < 80) {
    cmd[0] = revY ? __DOWNWARD : __UPWARD;
  } else if (Goble.readSwitchUp() == PRESSED) {
    cmd[0] = revY ? __UPWARD : __DOWNWARD;
  } else if (Goble.readSwitchDown() == PRESSED) {
    cmd[0] = revY ? __DOWNWARD : __UPWARD;
  } else {
    cmd[0] = __HALT;
  }

  if (joystickX > 190) {
    cmd[1] = revX ? __LEFT : __RIGHT;
  } else if (joystickX < 80) {
    cmd[1] = revX ? __RIGHT : __LEFT;
  } else if (Goble.readSwitchLeft() == PRESSED) {
    cmd[1] = revX ? __RIGHT : __LEFT;
  } else if (Goble.readSwitchRight() == PRESSED) {
    cmd[1] =  revX ? __LEFT : __RIGHT;
  } else  {
    cmd[1] = __HALT;
  }

  if (Goble.readSwitchSelect() == PRESSED) {
    int value = digitalRead(LASER_POINT_PIN);
    digitalWrite(LASER_POINT_PIN, !value );
    //revY = !revY;
  } else if (Goble.readSwitchStart() == PRESSED) {
    //revX = !revX;
  }

  if (Goble.readSwitchAction() == PRESSED) {
    cmd[0] = __CENTER;
    cmd[1] = __CENTER;
  }

  if (Goble.readSwitchMid() == PRESSED) {
    cmd[2] = __FIRE;
  } else {
    cmd[2] = __HALT;
  }
}
#endif

void aborted()
{
  Console.println("Program aborted!");
  buzzer.beepError();
  while (1)
    ;
}
