# Scroll-Wheel
A big high resolution Scroll wheel inspired by [Engineer Bo](https://www.patreon.com/c/engineerbo/posts)'s [Youtube Video](https://www.youtube.com/watch?v=FSy9G6bNuKA)

# This Project is still in an Early Development Phase, use with caution

## Hardware

The main controller is a ESP32 (For now)

As Magnetic encoder I am using a [AS5600](https://ams-osram.com/products/sensor-solutions/position-sensors/ams-as5600-position-sensor)

The Battery Pack is a NiMh S3P2 Pack of Eneloops

## Software

The Software is made of two parts, the Firmware on the ESP and a Driver for the pc (Linux only) written in Python

The communication is done via a BLE uart connection (I hope I will be able to get a BLE HID interface running soon)


## Progress

Here you can see the First PCB-prototype, with a bunch of Bugs

<img src="images/pcb-prototype.jpg" alt="First PCB prototype" width="300" />

Here is the Prototype PCB in the Case

<img src="images/PCB-in-case.jpg" alt="PCB Prototype in case" width="400" />

Here I am testing the First Firmware version with an external ESP32

<img src="images/first-firmware-test.jpg" alt="Firmware test" width="400" />

Here is a Picture of the Fully asmblied PCB

<img src="images/pcb-fully-asembled.jpg" alt="PCB Assembled" width="400" />