#include <HCSR04.h>
#include <ArduinoSort.h>
#include <QMC5883LCompass.h>
#include <L293.h>
#include <DFPlayerMini_Fast.h>
#include <SoftwareSerial.h>
#include <VarSpeedServo.h>

#define engineRigthEnable 5
#define engineRigthDir 4

#define engineLeftEnable 6
#define engineLeftDir 7

#define fullThrottle 255
#define halfThrottle 153
 
//3. SCL: compass analog input (A5)
//4. SDA: compass analog input (A4)

#define echoPin 10
#define triggerPin 11
#define servoPin 9
#define lightsPin 2
#define serialBaud 38400

#define firingPin A1

#define IgnitionSound 1
#define AccelCruiseSound 2
#define ReleaseIdleSound 3
#define ShutOffSound 4
#define idleFiringSound 5
#define movingFiringSound 6
#define stoppedFiringSound 7
#define patrollingSound 8

// DECLARATIONS

typedef struct
{
  char heading;
  short duration;
  bool valid;
} instruction;

char command;

short motorSpeedRigth;
short motorSpeedLeft;
short xValue;
short yValue;
short settingValue;
char movingDirection;

bool engineOn = false;
bool ledsOn = false;
bool moving = false;
short turretRotation;

unsigned long timer;

L293_twoWire rigthEngine(engineRigthEnable, engineRigthDir);
L293_twoWire leftEngine(engineLeftEnable, engineLeftDir);

QMC5883LCompass compass;

UltraSonicDistanceSensor distanceSensor(triggerPin, echoPin);

VarSpeedServo turretServo;

instruction path[30];

DFPlayerMini_Fast player;
SoftwareSerial playerSerial(A3, A2);
bool soundsOn = true;

// FUNCTIONS

void flushReceiveBuffer()
{
  delay(2);
  while (Serial.available() > 0)
    Serial.read();
}

bool checkInstruction(instruction i)
{
  if (i.heading == 'F' || i.heading == 'B' || i.heading == 'R' || i.heading == 'L')
  {
    if (i.duration > 0 && i.duration <= 400)
    {
      return true;
    }
  }
  return false;
}

void erasePath()
{
  for (int i = 0; i < 50; i++)
  {
    path[i].valid = false;
    path[i].heading = 0;
    path[i].duration = 0;
  }
  Serial.println(F("Path erased"));
  flushReceiveBuffer();
}

void processPath()
{
  short lastInstructionIndex = 0;
  while (true)
  {
    instruction temp;
    if (Serial.available() > 0)
    {
      char next = (char)Serial.read();

      if (next == 'E')
      {
        erasePath();
        return;
      }

      if (next != 'p')
      {
        temp.heading = next;
        timer = millis();
        while (Serial.available() < 3)
        {
          if (millis()-timer > 3000)
          {
            Serial.println(F("Path acquisition failed!"));
            return;  
          }
        }
        temp.duration = Serial.parseInt();
        if (checkInstruction(temp))
        {
          temp.valid = true;
          if (lastInstructionIndex < 29)
          {
            path[lastInstructionIndex] = temp;
            lastInstructionIndex++;
            continue;
          }
          else
          {
            Serial.println(F("Path too long, acquisition stopped!"));
            flushReceiveBuffer();
            return;
          }
        }
        else
        {
          Serial.println(F("Path not acceptable, acquisition stopped!"));
          erasePath();
          flushReceiveBuffer();
          return;
        }
      }
      else
      {
        flushReceiveBuffer();
        for (int i = 0; i < 50; i++)
        {
          if (path[i].valid == true)
          {
            Serial.println(path[i].heading);
            Serial.println(path[i].duration);
          }
        }
        return;
      }
    }
  }
}

bool checkStopOrder()
{
  if (Serial.available())
  {
    if (char(Serial.read()) == 'S')
    {
      return true;
    }
  }
  return false;
}

void followPath()
{
  player.play(patrollingSound);
  int i = 0;
  int azimuth;
  double distance;
  bool following;
  instruction temp;
  delay(2300);

  while (path[i].valid == true)
  {

    temp = path[i];
    switch (temp.heading)
    {
    case 'F':
    {
      double targetDistance = getDistance() - temp.duration;
      Serial.print(F("Target distance = "));
      Serial.println(targetDistance);

      while (true)
      {
        if (checkStopOrder())
        {
          stopEngines();
          flushReceiveBuffer();
          return;
        }
        distance = getDistance();
        if (distance < 20 || distance <= targetDistance)
        {
          stopEngines();
          following = false;
          break;
        }
        if (following == false)
        {
          moveForward(100);
          following = true;
        }
      }
      break;
    }
    case 'B':
    {
      double targetDistance = getDistance() + temp.duration;
      Serial.print(F("Target distance = "));
      Serial.println(targetDistance);

      while (true)
      {
        if (checkStopOrder())
        {
          stopEngines();
          flushReceiveBuffer();
          return;
        }
        distance = getDistance();
        if (distance >= 400 || distance >= targetDistance)
        {
          stopEngines();
          following = false;
          break;
        }
        if (following == false)
        {
          moveBackward(100);
          following = true;
        }
      }
      break;
    }
    case 'R':
    {
      compass.read();
      int initialAzimuth = (compass.getAzimuth());
      Serial.print(F("Initial Azimuth = "));
      Serial.println(initialAzimuth);
      int targetHeading = (initialAzimuth + temp.duration) % 360;
      Serial.print(F("Target heading = "));
      Serial.println(targetHeading);

      while (true)
      {
        if (checkStopOrder())
        {
          stopEngines();
          flushReceiveBuffer();
          return;
        }
        compass.read();
        azimuth = compass.getAzimuth();
        if (rigthTargetHeadingCheck(azimuth,targetHeading))
        {
          stopEngines();
          following = false;
          break;
        }
        if (following == false)
        {
          rotateRigth(70);
          following = true;
        }
      }
      break;
    }
    case 'L':
    {
      compass.read();
      int initialAzimuth = (compass.getAzimuth());
      Serial.print(F("Initial Azimuth = "));
      Serial.println(initialAzimuth);
      int targetHeading = (initialAzimuth - temp.duration)%360;
      Serial.print(F("Target heading = "));
      Serial.println(targetHeading);

      while (true)
      {
        if (checkStopOrder())
        {
          stopEngines();
          flushReceiveBuffer();
          return;
        }
        compass.read();
        azimuth = compass.getAzimuth();
        if (leftTargetHeadingCheck(azimuth,targetHeading))
        {
          stopEngines();
          following = false;
          break;
        }
        if (following == false)
        {
          rotateLeft(70);
          following = true;
        }
      }
      break;
    }
    }
    i++;
  }
  flushReceiveBuffer();
}

bool rigthTargetHeadingCheck(int actual, int target)
{
  if(target < actual)
  {
    return false;
  }
  if(actual >= target) return true;

  return false;
}

bool leftTargetHeadingCheck(int actual, int target)
{
  if(target > actual)
  {
    return false;
  }
  if(actual <= target) return true;

  return false;
}

int getDistance()
{
  int distances[10];
  for(int i = 0; i < 10; i++)
  {
    distances[i] = distanceSensor.measureDistanceCm();
  }
  sortArray(distances,10);
  int distance = distances[4];
  if(distance < 0) return 400;
  else return distance;
}

void switchEngine()
{
  if (engineOn == false)
  {
    digitalWrite(lightsPin, HIGH);
    ledsOn = true;
    player.play(IgnitionSound);
    delay(2450);
  }
  else
  {
    if (moving == true)
    {
      stopEngines();
    }
    player.play(ShutOffSound);
    digitalWrite(lightsPin, LOW);
    ledsOn = false;
    delay(2450);
  }

  engineOn = !engineOn;
  Serial.println("Switched Engine");
  flushReceiveBuffer();
}

void switchLeds()
{

  if (ledsOn == false)
  {
    digitalWrite(lightsPin, HIGH);
  }
  else
  {
    digitalWrite(lightsPin, LOW);
  }
  ledsOn = !ledsOn;
}

void moveForward(int throttle)
{
  if (moving == false)
  {
    player.play(AccelCruiseSound);
    for (int i = 0; i <= throttle; i++)
    {
      rigthEngine.forward(i);
      leftEngine.forward(i);
      delay(2);
    }
  }
  else
  {
    rigthEngine.forward(throttle);
    leftEngine.forward(throttle);
  }
  moving = true;
}

void moveBackward(int throttle)
{
  if (moving == false)
  {
    player.play(AccelCruiseSound);
    for (int i = 0; i <= throttle; i++)
    {
      rigthEngine.back(i);
      leftEngine.back(i);
      delay(2);
    }
  }
  else
  {
    rigthEngine.back(throttle);
    leftEngine.back(throttle);
  }
  moving = true;
}

void rotateRigth(int throttle)
{
  if (moving == false)
  {
    player.play(AccelCruiseSound);
    for (int i = 0; i <= throttle; i++)
    {
      rigthEngine.back(i);
      leftEngine.forward(i);
      delay(2);
    }
  }
  else
  {
    rigthEngine.stop();
    leftEngine.forward(throttle);
  }
  moving = true;
}

void rotateLeft(int throttle)
{
  if (moving == false)
  {
    player.play(AccelCruiseSound);
    for (int i = 0; i <= throttle; i++)
    {
      rigthEngine.forward(i);
      leftEngine.back(i);
      delay(2);
    }
  }
  else
  {
    rigthEngine.forward(throttle);
    leftEngine.stop();
  }
  moving = true;
}

void stopEngines()
{
  if(moving == true)
  {
    moving = false;
    player.play(ReleaseIdleSound);  
  }
  rigthEngine.stop();
  leftEngine.stop();
}

void analogMove()
{
  xValue = Serial.parseInt();
  yValue = Serial.parseInt();

  if (yValue < 120)
  {
    settingValue = map(yValue,119,0,0,255);
    motorSpeedRigth = settingValue;
    motorSpeedLeft = settingValue;
    movingDirection = 'F';
  }
  else if (yValue > 134)
  {
    settingValue = map(yValue,135,255,0,255);
    motorSpeedRigth = settingValue;
    motorSpeedLeft = settingValue;
    movingDirection = 'B';
  }
  else
  {
    motorSpeedRigth = 0;
    motorSpeedLeft = 0;
    movingDirection = 'S';
  }

  if (xValue < 120)
  {
    settingValue = map(xValue,119,0,0,255);
    motorSpeedRigth = motorSpeedRigth + settingValue;
    motorSpeedLeft = motorSpeedLeft - settingValue;
    if(movingDirection == 'S') movingDirection = 'L';
    
    if (motorSpeedRigth > 255)
    {
      motorSpeedRigth = 255;
    }
    if (motorSpeedLeft < 0)
    {
      motorSpeedLeft = 0 ;
    }
  }
  else if (xValue > 134)
  {
    settingValue = map(xValue,135,255,0,255);
    motorSpeedRigth = motorSpeedRigth - settingValue;
    motorSpeedLeft = motorSpeedLeft + settingValue;
    if(movingDirection == 'S') movingDirection = 'R';
    
    if (motorSpeedRigth < 0) {
      motorSpeedRigth = 0;
    }
    if (motorSpeedLeft > 255) {
      motorSpeedLeft = 255;
    }
  }

  if (motorSpeedRigth < 60)
  {
    motorSpeedRigth = 0;
  }
  if (motorSpeedLeft < 60)
  {
    motorSpeedLeft = 0;
  }
  
  if(motorSpeedRigth == 0 && motorSpeedLeft == 0)
  {
    if(moving == true)
    {
      moving = false;
      player.play(ReleaseIdleSound);
    }
    rigthEngine.stop();
    leftEngine.stop();
    return;
  }
  else
  {
    if(moving == false)
    {
      moving = true;
      player.play(AccelCruiseSound);
    }
  
    if(movingDirection == 'F')
    {
      rigthEngine.forward(motorSpeedRigth);
      leftEngine.forward(motorSpeedLeft);
    }
    else if(movingDirection == 'B')
    {
      rigthEngine.back(motorSpeedRigth);
      leftEngine.back(motorSpeedLeft);
    }
    else if(movingDirection == 'R')
    {
      rigthEngine.back(motorSpeedLeft);
      leftEngine.forward(motorSpeedLeft);
    }
    else if(movingDirection == 'L')
    {
      rigthEngine.forward(motorSpeedRigth);
      leftEngine.back(motorSpeedRigth);
    }
  }
}

void rotateTurret()
{
  short rotation = Serial.parseInt();
  turretRotation = 180 - rotation - 10;
  turretServo.write(turretRotation,90);
}

void fire()
{
  if (moving == false)
  {
    if (engineOn == false)
      player.play(stoppedFiringSound);
    else
      player.play(idleFiringSound);
    if (turretRotation >= 30 && turretRotation <= 130)
    {
      delay(30);
      rigthEngine.back(180);
      leftEngine.back(180);
      digitalWrite(firingPin, HIGH);
      delay(120);
      digitalWrite(firingPin, LOW);
      rigthEngine.stop();
      leftEngine.stop();
      return;
    }
  }
  else
    player.play(movingFiringSound);
  delay(50);
  digitalWrite(firingPin, HIGH);
  delay(200);
  digitalWrite(firingPin, LOW);
}

void switchSounds()
{
  if(soundsOn == true)
  {
    player.volume(0);
    soundsOn = false;
    Serial.println(F("Sounds off"));  
  }
  else
  {
    player.volume(30);
    soundsOn = true;
    Serial.println(F("Sounds on")); 
  }
}

void setup()
{

  Serial.begin(serialBaud);
  Serial.setTimeout(10);

  soundsOn = true;
  playerSerial.begin(9600);
  playerSerial.setTimeout(10);
  player.begin(playerSerial);
  player.volume(30);
  player.EQSelect(5);

  turretServo.attach(servoPin);

  compass.init();
  compass.setCalibration(0, 1990, 0, 2307, -2658, 0);

  pinMode(engineRigthEnable, OUTPUT);
  pinMode(engineRigthDir, OUTPUT);

  pinMode(engineLeftEnable, OUTPUT);
  pinMode(engineLeftDir, OUTPUT);

  pinMode(firingPin, OUTPUT);
  pinMode(lightsPin, OUTPUT);
  analogWrite(engineRigthEnable, LOW);
  analogWrite(engineLeftEnable, LOW);

  turretServo.write(80, 90);
  turretRotation = 80;
}

void loop()
{
  if (Serial.available() > 0)
  {
    char command = (char)Serial.read();

    switch (command)
    {
    case 'I':
      {
        switchEngine();
        break;
      }

    case 'T':
      {
        rotateTurret();
        break;
      }

    case 'P':
      {
        processPath();
        break; 
      }

    case 'L':
      {
        switchLeds();
        break;
      }

    case 'F':
      {
        fire();
        break;
      }
      
    case 'M':
      {
        switchSounds();
        break;
      }
      
    case 'S':
          {
            stopEngines();
            break;
          }

    default:
    {
      if (engineOn == true)
      {
        switch (command)
        {
          case 'X':
          {
            analogMove();
            break;
          }
          
          case 'E':
          {
            followPath();
            break;
          }

          default:
          {
          }
          
          }
        }
      }
      break;
    }
  }
}
