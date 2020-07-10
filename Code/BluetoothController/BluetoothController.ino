#include <HCSR04.h>
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

#define DeployVoice 1
#define IgnitionSound 2
#define AccelCruiseSound 3
#define ReleaseIdleSound 4
#define CruiseSound 5
#define ShutOffSound 6
#define idleFiringSound 7
#define movingFiringSound 8
#define stoppedFiringSound 9

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
short lastInstructionIndex = 0;

DFPlayerMini_Fast player;
SoftwareSerial playerSerial(A3,A2);

// FUNCTIONS

void flushReceiveBuffer()
{
  delay(2);
  while(Serial.available() > 0)
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

void processPath()
{
  while (true)
  {
    instruction temp;
    if (Serial.available() > 0)
    {
      char next = (char)Serial.read();
      if (next != 'p')
      {
        temp.heading = next;
        while(Serial.available() < 3)
        {
        }
        //temp.duration = short(short(Serial.read()) * 100 + short(Serial.read()) * 10 + short(Serial.read()));
        temp.duration = Serial.parseInt();
        temp.valid = true;
        if (checkInstruction(temp))
        {
          if (lastInstructionIndex < 49)
          {
            lastInstructionIndex++;
            path[lastInstructionIndex] = temp;
            Serial.print("Direzione acquisita: ");
            Serial.println(temp.heading);
            Serial.print("Distanza acquisita: ");
            Serial.println(temp.duration);
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
          lastInstructionIndex = 0;
          flushReceiveBuffer();
          return;
        }
      }
      else
      {
        flushReceiveBuffer();
        return;
      }
    }
  }
}

bool checkStopOrder()
{
   if(Serial.available())
    {
      if(char(Serial.read()) == 'S')
      {
        return true;  
      }
    }
  return false;
}

void followPath()
{
  short i = 0;
  short azimuth;
  double distance;
  instruction temp;
  
  while(path[i].valid == true)
  {
    
    temp = path[i];
    switch(temp.heading)
    {
      case 'F':
      {
        double initialDistance = distanceSensor.measureDistanceCm();
        
        while(true)
        {
          if(checkStopOrder())
          {
            stopEngines();
            flushReceiveBuffer();
            return;  
          }
          distance = distanceSensor.measureDistanceCm();
          if(distance < 15 || distance <= initialDistance - temp.duration)
          {
            stopEngines();
            break;
          }
          moveForward(halfThrottle);
        }
        break;
      }
      case 'B':
      {
        double initialDistance = distanceSensor.measureDistanceCm();
        
        while(true)
        {
          if(checkStopOrder())
          {
            stopEngines();
            flushReceiveBuffer();
            return;  
          }
          distance = distanceSensor.measureDistanceCm();
          if(distance >= 400 || distance >= initialDistance + temp.duration)
          {
            stopEngines();
            break;
          }
          moveBackward(halfThrottle);
        }
        break;
      }
      case 'R':
      {
        compass.read();
        int initialAzimuth = (compass.getAzimuth())%360 ;
        int targetHeading = (initialAzimuth + temp.duration)%360;
        
        while(true)
        {
          if(checkStopOrder())
          {
            stopEngines();
            flushReceiveBuffer();
            return;  
          }
          azimuth = compass.getAzimuth();
          if(azimuth >= targetHeading)
          {
            stopEngines();
            break;
          }
          rotateRigth(100);
        }
        break;
      }
      case 'L':
      {
        compass.read();
        int initialAzimuth = (compass.getAzimuth())%360;
        int targetHeading = initialAzimuth - temp.duration;
        if (targetHeading <  0) targetHeading += 360;
        
        while(true)
        {
          if(checkStopOrder())
          {
            stopEngines();
            flushReceiveBuffer();
            return;  
          }
          azimuth = compass.getAzimuth();
          if(azimuth <= targetHeading)
          {
            stopEngines();
            break;
          }
          rotateRigth(100);
        }
        break;
      }
    }
    i++;
  }
  flushReceiveBuffer();
}

void switchEngine()
{
  if (engineOn == false)
  {
    digitalWrite(lightsPin, HIGH);
    ledsOn = true;
    /*player.play(DeployVoice);
    delay(2500);*/
    player.play(IgnitionSound);
    }
  else
  {
    if (moving == true){
        stopEngines();
        delay(2450);
      }
    player.play(ShutOffSound);
    digitalWrite(lightsPin, LOW);
    ledsOn = false;
  }

  engineOn = !engineOn;
  Serial.println("Switched Engine");
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
  if(moving == false)
  {
    player.play(AccelCruiseSound);
    for(int i = 0;i <= throttle; i++)
    {
      rigthEngine.forward(i);
      leftEngine.forward(i);
      delay(10);
    }
  }
  else
  {
    rigthEngine.forward(throttle);
    leftEngine.forward(throttle);
  }
  moving = true;
  //Serial.println("moveForward");
}

void moveForwardRigth(int throttle)
{
  if(moving == false)
  {
    player.play(AccelCruiseSound);
    for(int i = 0;i <= throttle; i++)
    {
      rigthEngine.forward(i/2);
      leftEngine.forward(i);
      delay(10);
    }
  }
  else
  {
    rigthEngine.forward(throttle/2);
    leftEngine.forward(throttle);
  }
  moving = true;
  Serial.println("moveForwardRigth");
}

void moveForwardLeft(int throttle)
{
  if(moving == false)
  {
    player.play(AccelCruiseSound);
    for(int i = 0;i <= throttle; i++)
    {
      rigthEngine.forward(i);
      leftEngine.forward(i/2);
      delay(10);
    }
  }
  else
  {
    rigthEngine.forward(throttle);
    leftEngine.forward(throttle/2);
  }
  moving = true;
  Serial.println("moveForwardLeft");
}

void moveBackward(int throttle)
{
  if(moving == false)
  {
    player.play(AccelCruiseSound);
    for(int i = 0;i <= throttle; i++)
    {
      rigthEngine.back(i);
      leftEngine.back(i);
      delay(10);
    }
  }
  else
  {
    rigthEngine.back(throttle);
    leftEngine.back(throttle);
  }
  moving = true;
  Serial.println("moveBackward");
}

void moveBackwardRigth(int throttle)
{
  if(moving == false)
  {
    player.play(AccelCruiseSound);
    for(int i = 0;i <= throttle; i++)
    {
      rigthEngine.back(i/2);
      leftEngine.back(i);
      delay(10);
    }
  }
  else
  {
    rigthEngine.back(throttle/2);
    leftEngine.back(throttle);
  }
  moving = true;
  Serial.println("moveBackwardRigth");
}

void moveBackwardLeft(int throttle)
{
  if(moving == false)
  {
    player.play(AccelCruiseSound);
    for(int i = 0;i <= throttle; i++)
    {
      rigthEngine.back(i);
      leftEngine.back(i/2);
      delay(10);
    }
  }
  else
  {
    rigthEngine.back(throttle);
    leftEngine.back(throttle/2);
  }
  moving = true;
  Serial.println("moveBackwardLeft");
}

void rotateRigth(int throttle)
{
  if(moving == false)
  {
    player.play(AccelCruiseSound);
    for(int i = 0;i <= throttle; i++)
    {
      rigthEngine.back(i);
      leftEngine.forward(i);
      delay(10);
    }
  }
  else
  {
    rigthEngine.back(throttle);
    leftEngine.forward(throttle);
  }
  moving = true;
  Serial.println("rotateRigth");
}

void rotateLeft(int throttle)
{
  if(moving == false)
  {
    player.play(AccelCruiseSound);
    for(int i = 0;i <= throttle; i++)
    {
      rigthEngine.forward(i);
      leftEngine.back(i);
      delay(10);
    }
  }
  else
  {
    rigthEngine.forward(throttle);
    leftEngine.back(throttle);
  }
  moving = true;
  Serial.println("rotateLeft");
}

void stopEngines()
{
  rigthEngine.stop();
  leftEngine.stop();
  moving = false;
  player.play(ReleaseIdleSound);
  Serial.println("stopEngines");
}

void rotateTurret()
{
  short rotation;
  while(true)
  {
    if (Serial.available() > 1)
    {
      rotation = Serial.parseInt();
      break;
    }
  }
  turretRotation = 180 - rotation -10;
  turretServo.write(turretRotation,50);
  //String rotatingString = "Rotating turret to ";
  //n  Serial.println(rotatingString + rotation);
}

void fire()
{
  if(moving == false)
  {
    if(engineOn == false) player.play(stoppedFiringSound);
    else player.play(idleFiringSound);
    if(turretRotation >= 40 && turretRotation <= 120)
    {
      rigthEngine.back(fullThrottle);
      leftEngine.back(fullThrottle);
      delay(50);
      digitalWrite(firingPin,HIGH);
      delay(200);
      digitalWrite(firingPin,LOW);
      rigthEngine.stop();
      leftEngine.stop();
      return;
    }
  }
  else player.play(movingFiringSound);
  delay(50);
  digitalWrite(firingPin,HIGH);
  delay(200);
  digitalWrite(firingPin,LOW);
}

void setSpeedSetting()
{
  while(true)
  {
    if(Serial.available() > 2)
    {
      short value = Serial.parseInt();
      speedSetting = map(value, 0, 0, 100, 255);
      flushReceiveBuffer();
      String setting = "Throttle set to: ";
      Serial.println(setting + speedSetting);
      return;  
    }  
  }
}

void setup()
{

  Serial.begin(serialBaud);
  
  playerSerial.begin(9600);
  player.begin(playerSerial);
  player.volume(30);
  player.EQSelect(5);

  turretServo.attach(servoPin);

  compass.init();

  speedSetting = halfThrottle;

  pinMode(engineRigthEnable, OUTPUT);
  pinMode(engineRigthDir, OUTPUT);

  pinMode(engineLeftEnable, OUTPUT);
  pinMode(engineLeftDir, OUTPUT);

  pinMode(firingPin, OUTPUT);
  pinMode(lightsPin, OUTPUT);
  analogWrite(engineRigthEnable, LOW);
  analogWrite(engineLeftEnable, LOW);

  turretServo.write(80,50);
  turretRotation = 80;
}

void loop()
{
  if (Serial.available() > 0)
  {

    char command = (char)Serial.read();

    if (command == 'I')
    {
      switchEngine();
    }
    else if (command == 'T')
    {
      rotateTurret();
    }
    else if (command == 'P')
    {
      processPath();
    }
    else if (command == 'L')
    {
      switchLeds();
    }
    else if (command == 'F')
    {
      fire();
    }
    else if (command == 'V')
    {
      setSpeedSetting();
    }
    else
    {
      if (engineOn == true)
      {
        if (command == '1')
        {
          moveForward(speedSetting);
        }
        else if (command == '2')
        {
          rotateRigth(speedSetting);
        }
        else if (command == '3')
        {
          moveBackward(speedSetting);
        }
        else if (command == '4')
        {
          rotateLeft(speedSetting);
        }
        else if (command == '0')
        {
          stopEngines();
        }
        else if (command == '5')
        {
          moveForwardRigth(speedSetting);
        }
        else if (command == '6')
        {
          moveBackwardRigth(speedSetting);
        }

        else if (command == '7')
        {
          moveBackwardLeft(speedSetting);
        }

        else if (command == '8')
        {
          moveForwardLeft(speedSetting);
        }

        else if(command == 'E')
        {
          followPath();
        }
      }
    }
  }
}
