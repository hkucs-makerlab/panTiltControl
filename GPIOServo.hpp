#ifndef __GPIO_SERVO__
#define __GPIO_SERVO__
#if defined(ESP32)
#include <ESP32Servo.h> //https://github.com/RoboticsBrno/ServoESP32
#else
#include <Servo.h>
#endif


class GPIOservo {
  private:

  public:
    GPIOservo(): angle(0), rev(false), attached(false), pinIndex(-1) {

    }
    GPIOservo(int pinIndex ): GPIOservo() {
      this->pinIndex = pinIndex;
#if defined(ESP32)
      // calling in constructor not working in AVR mcu
      this->attach();
#endif
    }
    bool attach() {
      if (pinIndex < 0) return false;
      if (attached) {
        gpioServos.detach();
      }
      gpioServos.attach(pinIndex, gpioServoMin, gpioServoMax);
      gpioServos.write(angle);
      prevTime = millis();
      return attached = true;
    }
    
    bool attach(uint8_t pinIndex) {
      this->pinIndex = pinIndex;
      return attach();
    }

    void write(int angle) {
      if (!attached) {
        Serial.println("failed, not attach to a pin!");
        return;
      }
      gpioServos.write(angle);
      if (angle != this->angle) this->angle = angle;
    }

    bool move(int targetAngle) {
      if (angle == targetAngle)  {
        //Serial.println(String("angle: ") + angle + String(", targetAngle: ") + targetAngle);
        return true;
      }
      currentTime = millis();
      if (currentTime - prevTime >= angleTimeGap) {
        long diffTime = currentTime - prevTime;
        int diffAngle = diffTime / angleTimeGap;
        prevTime = currentTime;
        //Serial.println(String("angle: ") + angle + String(", targetAngle: ") + targetAngle + String(", diff angle: ") + diffAngle);
        this->write(angle);
        if (targetAngle < angle) {
          angle -= diffAngle;
          if (angle < 0) angle = 0;
        } else {
          angle += diffAngle;
          if (angle > 180) angle = 180;
        }
      }
      return false;
    }

    void sweep() {
      currentTime = millis();
      if (currentTime - prevTime >= angleTimeGap) {
        long diffTime = currentTime - prevTime;
        int diffAngle = diffTime / angleTimeGap;
        //Serial.println(String("angle: ") + angle + String(" ,diff time: ") + diffTime + String(", diff angle: ") + diffAngle);
        prevTime = currentTime;
        this->write(angle);
        if (rev) {
          angle -= diffAngle;
          if (angle < 0) {
            rev = false;
            angle = 0;
          }
        } else {
          angle += diffAngle;
          if (angle > 180) {
            rev = true;
            angle = 180;
          }
        } //rev
      }
    }
  private:

    //    const uint8_t servoGpioPins[11] = {
    //      4, 5, 12, 13, 14,
    //      15, 25, 26, 27, 32,
    //      33
    //    };
    int pinIndex;
    const int gpioServoMin = 500;
    const int gpioServoMax = 2600;
    Servo gpioServos;
    //
    const short angleTimeGap = 5;
    int angle;
    bool rev;
    long prevTime, currentTime;
    //
    bool attached;
};
#endif
