# AppVolumeController
This project is a volume controller that uses an ESP32 microcontroller, a KY-040 rotary encoder, and a GC9A01 round TFT display to control and visualize the audio levels of selected applications on a Windows PC. A python desktop client is communicated through UART to dynamically adjusts volume and application icon for the selected Windows processes in real time.

The active app icons are also cached for fast use.

Will be adding a digital clock for when the computer is turned off/no app connection with different themes. The themes will be able to be chosen through the desektop app, but will not be required after configuring once.
