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
#define serialBaud 115200
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

bool engineOn = false;
bool ledsOn = false;
bool moving = false;
short turretRotation;
short speedSetting;

unsigned long timer;

L293_twoWire rigthEngine(engineRigthEnable, engineRigthDir);
L293_twoWire leftEngine(engineLeftEnable, engineLeftDir);

QMC5883LCompass compass;

UltraSonicDistanceSensor distanceSensor(triggerPin, echoPin);

VarSpeedServo turretServo;

instruction path[50];

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
  Serial.println("Path erased");
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
            Serial.println("Path acquisition failed!");
            return;  
          }
        }
        temp.duration = Serial.parseInt();
        if (checkInstruction(temp))
        {
          temp.valid = true;
          if (lastInstructionIndex < 49)
          {
            path[lastInstructionIndex] = temp;
            lastInstructionIndex++;
            continue;
          }
          else
          {
            Serial.println("Path too long, acquisition stopped!");
            flushReceiveBuffer();
            return;
          }
        }
        else
        {
          Serial.println("Path not acceptable, acquisition stopped!");
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
      Serial.print("Target distance = ");
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
      Serial.print("Target distance = ");
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
      Serial.print("Initial Azimuth = ");
      Serial.println(initialAzimuth);
      int targetHeading = (initialAzimuth + temp.duration) % 360;
      Serial.print("Target heading = ");
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
      Serial.print("Initial Azimuth = ");
      Serial.println(initialAzimuth);
      int targetHeading = (initialAzimuth - temp.duration)%360;
      Serial.print("Target heading = ");
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
  }
  else
  {
    if (moving == true)
    {
      stopEngines();
      delay(2450);
    }
    player.play(ShutOffSound);
    digitalWrite(lightsPin, LOW);
    ledsOn = false;
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
  Serial.println("SwitchedLeds");
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

void moveForwardRigth(int throttle)
{
  if (moving == false)
  {
    player.play(AccelCruiseSound);
    for (int i = 0; i <= throttle; i++)
    {
      rigthEngine.forward(i / 2);
      leftEngine.forward(i);
      delay(2);
    }
  }
  else
  {
    rigthEngine.forward(throttle / 2);
    leftEngine.forward(throttle);
  }
  moving = true;
}

void moveForwardLeft(int throttle)
{
  if (moving == false)
  {
    player.play(AccelCruiseSound);
    for (int i = 0; i <= throttle; i++)
    {
      rigthEngine.forward(i);
      leftEngine.forward(i / 2);
      delay(2);
    }
  }
  else
  {
    rigthEngine.forward(throttle);
    leftEngine.forward(throttle / 2);
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

void moveBackwardRigth(int throttle)
{
  if (moving == false)
  {
    player.play(AccelCruiseSound);
    for (int i = 0; i <= throttle; i++)
    {
      rigthEngine.back(i / 2);
      leftEngine.back(i);
      delay(2);
    }
  }
  else
  {
    rigthEngine.back(throttle / 2);
    leftEngine.back(throttle);
  }
  moving = true;
}

void moveBackwardLeft(int throttle)
{
  if (moving == false)
  {
    player.play(AccelCruiseSound);
    for (int i = 0; i <= throttle; i++)
    {
      rigthEngine.back(i);
      leftEngine.back(i / 2);
      delay(2);
    }
  }
  else
  {
    rigthEngine.back(throttle);
    leftEngine.back(throttle / 2);
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
  rigthEngine.stop();
  leftEngine.stop();
  moving = false;
  player.play(ReleaseIdleSound);
}

void rotateTurret()
{
  short rotation;
  while (true)
  {
    if (Serial.available() > 1)
    {
      rotation = Serial.parseInt();
      break;
    }
  }
  turretRotation = 180 - rotation - 10;
  turretServo.write(turretRotation, 90);
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
      rigthEngine.back(180);
      leftEngine.back(180);
      digitalWrite(firingPin, HIGH);
      delay(180);
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

void setSpeedSetting()
{
  timer = millis();
  while (true)
  {
    if (millis()-timer > 3000)
    {
      Serial.println("Speed setting failed!");
      return;  
    }
    if (Serial.available() > 1)
    {
      int value = Serial.parseInt();
      speedSetting = map(value, 0, 100, 0, 255);
      flushReceiveBuffer();
      Serial.print("Throttle set to: ");
      Serial.println(speedSetting);
      return;
    }
  }
}

void switchSounds()
{
  if(soundsOn == true)
  {
    player.volume(0);
    soundsOn = false;
    Serial.println("Sounds off");  
  }
  else
  {
    player.volume(30);
    soundsOn = true;
    Serial.println("Sounds on"); 
  }
}

void setup()
{

  Serial.begin(serialBaud);

  soundsOn = true;
  playerSerial.begin(9600);
  player.begin(playerSerial);
  player.volume(30);
  player.EQSelect(5);

  turretServo.attach(servoPin);

  compass.init();
  compass.setCalibration(0, 1990, 0, 2307, -2658, 0);

  speedSetting = halfThrottle;

  pinMode(engineRigthEnable, OUTPUT);
  pinMode(engineRigthDir, OUTPUT);

  pinMode(engineLeftEnable, OUTPUT);
  pinMode(engineLeftDir, OUTPUT);

  pinMode(firingPin, OUTPUT);
  pinMode(lightsPin, OUTPUT);
  analogWrite(engineRigthEnable, LOW);
  analogWrite(engineLeftEnable, LOW);

  turretServo.write(80, 50);
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

    case 'V':
      {
        setSpeedSetting();
        break;
      }

    case 'M':
      {
        switchSounds();
        break;
      }

    default:
    {
      if (engineOn == true)
      {
        switch (command)
        {
        case '1':
          {
            moveForward(speedSetting);
            break;
          }
        case '2':
          {
            rotateRigth(speedSetting);
            break;
          }
        case '3':
          {
            moveBackward(speedSetting);
            break;
          }
        case '4':
          {
            rotateLeft(speedSetting);
            break;
          }
        case '0':
          {
            stopEngines();
            break;
          }
        case '5':
          {
            moveForwardRigth(speedSetting);
            break;
          }
        case '6':
          {
            moveBackwardRigth(speedSetting);
            break;
          }

        case '7':
          {
            moveBackwardLeft(speedSetting);
            break;
          }

        case '8':
          {
            moveForwardLeft(speedSetting);
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
