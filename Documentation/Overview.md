# Table of contents

- [Introduction](#Introduction)
- [Model construction](#Body)
- [Sound System](#Sound)
- [Motors System](#Motors)
- [Power Source](#Power)
- [Compass module](#Compass)
- [Microcontroller](#Microcontroller)
- [Other components](#Other)

## Introduction <a name="Introduction"></a>

The idea is to create a dynamic model of the Sci-Fi Armoured Personnel Carrier Razorback, from the WarHammer 40,000 game series.

|**Model to reproduce**|
|:--:|
|<img src="https://github.com/dadit97/Razorback-Project/blob/master/Images/Razorback.jpg" width="400">|

The model, in addiction to being remotely controlled, should be able to follow a path he was instructed to patrol, using a compass as navigation aid and a distance sensor to avoid collisions.
This is an electronic project, so, the model will be simplified to some extent, but the objective is to create something that looks pretty similar.

## Model construction <a name="Body"></a>

To build the chassis, the protection plates and the wheels I used some woodworking machines that I have access to. The dimensions have been estabilished by scaling some images knowing the total length and width of the model.


|**Main structure**||
|:--:|--|
|<img src="https://github.com/dadit97/Razorback-Project/blob/master/Images/Main_Structure.jpg" width="1400">|Inside the structure two pulleys were placed to reduce the speed of the main motors, these will transmit the power to the geared wheels,which were taken from an old Meccano kit. A structure to hold the motors in place has been installed in the middle of the body.|


|**Distance Sensor**||
|:--:|--|
|<img src="https://github.com/dadit97/Razorback-Project/blob/master/Images/Incapsulated_Sensor.jpg" width="300">|The distance sensor had to be placed on the front, so it was closed in a proper case.|


Then the turret was built creating the servomotor housing inside and creating room for the firing LEDs. Unfortunately, the micro servo doesn't have a robust axle, so the connection is not the best, but for its purpose is enough.

The remaining of the building work were just the various decorations to make it similar to the target.


|**Components used and result after spray painting**|
|:--:|
|<img src="https://github.com/dadit97/Razorback-Project/blob/master/Images/Pieces.jpg" width="400">  <img src="https://github.com/dadit97/Razorback-Project/blob/master/Images/Result.jpg" width="400">|

## Power Source <a name="Power"></a>

|**Battery**||
|:--:|--|
|<img src="https://github.com/dadit97/Razorback-Project/blob/master/Images/Battery.jpg" width="250">|The model will be powered up by a rechargable NI-MH 7.2V battery.|



## Sound System <a name="Sound"></a>


|**MP3 module**|**Speaker**||
|:--|:--:|--:|
|<img src="https://github.com/dadit97/Razorback-Project/blob/master/Images/DF_Mini.jpg" width="400"> |<img src="https://github.com/dadit97/Razorback-Project/blob/master/Images/Speaker.jpg" width="400" heigth="400">|To create the illusion of the engine rumble, a MP3 module has been added to the control system, connected to a tiny 3W speaker, which is the maximum power that the module can supply without an amplifier.|


The module will be instructed by the Arduino using Serial communication, which will change the sound to be reproduced to suit the state of the model.
The storage of the module is a microSD card, in which the looped sounds were loaded.

## Motors System <a name="Motors"></a>

At the beginning of the project, the propulsion was based on two small 3-6V DC motors, but during the construction, the model's weigth showed clearly that this solution was unacceptable.


|**Motors used**||
|:--:|--|
|<img src="https://github.com/dadit97/Razorback-Project/blob/master/Images/9V_Motor.jpg" width="400">|According to this requirement, the little motors had been replaced by two 9V DC motors, which can provide the torque required to move the tank at a reasonable speed.|


After the replacing another problem arose, the motor driver.
The driver chosen was the classic L298N, basically the standard for<br> Arduino-powered moving projects.


|**L298N**||
|:--:|--|
|<img src="https://github.com/dadit97/Razorback-Project/blob/master/Images/L298N.jpg" width="400">|This double motor driver, unfortunately, can provide just 2 Ampere for channel, which is enough for 3-6V motors, but too low for something bigger.According to a multimeter test, the motors can use up to 5.6 Ampere at stall load, so even using one driver per motor by bridging the outputs (a solution that creates a lot of redundancy in the circuit), was not enough.|


|**IRF3205 Double Motor Driver**||
|:--:|--|
|<img src="https://github.com/dadit97/Razorback-Project/blob/master/Images/Double_Motor_Driver.jpg" width="400">|The driver chosen the replace the inadequate one is a 15 Ampere double motor driver, equipped with MOSFET bridges, wich allowed the motor to use the full power provided by the battery without the danger of overheating the chip.|

## Compass module <a name="Compass"></a>

|**GY-271 Electronic Compass**||
|:--:|--|
|<img src="https://github.com/dadit97/Razorback-Project/blob/master/Images/GY-271.jpg" width="400">|To implement the autonomus movement along a path, the tank has been equipped with an electronic compass. Via I2C communication it will provide the heading information to the Arduino, so the model will be able to control its direction with a good amount of accuracy.

## Microcontroller <a name="Microcontroller"></a>

|**Arduino Uno R3**||
|:--:|--|
|<img src="https://github.com/dadit97/Razorback-Project/blob/master/Images/Prototyping_Module.jpg" width="600">|In the initial stages of the project, the idea was to use a normal Arduino Uno supported by a mini breadboard, but, the small amount of space remained for it, suggested that something smaller was necessary. So the microcontroller was fitted with a Prototyping Module, which provides more Ground and Power connections, along with making the unit more compact.

|**HC-05 Bluetooth Module**||
|:--:|--|
|<img src="https://github.com/dadit97/Razorback-Project/blob/master/Images/HC-05.jpg" width="400">|The various commands will be sent to the tank via Bluetooth, using an Android application, this module will collect them and, using Serial communication, send the instructions to the microcontroller.

## Other components <a name="Other"></a>

To create the complete circuit some basic electronic components were necessary, like LEDs, jumper wires, electric cables, resistors and NPN transistors(for the lights circuit).

||
|:--:|
|<img src="https://github.com/dadit97/Razorback-Project/blob/master/Images/Resistors.jpg" width="200">  <img src="https://github.com/dadit97/Razorback-Project/blob/master/Images/2222A_Transistor.jpg" width="200"> <img src="https://github.com/dadit97/Razorback-Project/blob/master/Images/Cables.jpg" width="200"> <img src="https://github.com/dadit97/Razorback-Project/blob/master/Images/Jumpers.jpg" width="200">|

