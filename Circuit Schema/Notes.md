## Notes

The specific motor driver used in the project was not present in the part list, so it has been substituted by a VNH25P30.
The main difference is that the IRF3205 requires only a pin to select the direction of the motor, instead of the two required by the VNH25P30.
The motor direction pins of the Arduino have been connected to the first direction pins of the driver in the schema.

The HC-05 Bluetooth module has been relaced by the Bluetooth Mate Silver, which is an equivalent one. The voltage divider is required to reduce the transmission voltage from the Arduino under 3.3 Volts, without that there is a risk of damaging the module.