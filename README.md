# soundlevel_esp32
Provide SPL meter meaures via html and telnet

This project for platformio and Arduino uses the serial output of a Tondaj SL-814 to publish sound pressure levels on your network. The meter can be read by html or a telnet connection.

Add https://dl.espressif.com/dl/package_esp32_index.json in Arduino boards manager and install ESP32 v1.0.2 or better.

Download and install https://github.com/me-no-dev/ESPAsyncWebServer and https://github.com/me-no-dev/AsyncTCP to your libraries directory
