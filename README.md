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
