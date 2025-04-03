This is a little monitor that displays data from nightscout! Made with a Wemos D1 Mini. The screen defaults to off, but pressing the button will turn on the display for 10 seconds. I did this so that the light wouldn't disturb my sleep!
It is largely based on Hypertoken's Desktop Nightscout project, which you can find [here](https://github.com/Hypertoken/Nightscout-Display). I tried to put my own spin on it!

# You Will Need...
## Supplies
- A Wemos D1 Mini board
- A 0.96in I2C OLED screen
- Wires and heatshrink (heatshrink is optional but recommended to protect soldered connections)
- A 6mm push button
- The 3D printed case (or, if you're feeling creative, you can make your own!)

Note: I found that the board struggled to connect to Wifi when on a breadboard.
## Software
- Arduino IDE 1.8 or above
### Boards Manager and Driver
You will probably need to install the driver for your board. For the Wemos D1 Mini, I used [this one](https://www.wemos.cc/en/latest/ch340_driver.html).

You will also need to install the Wemos board through the boards manager:
1. Go to File > Preferences and add this URL to the Additional Boards Manager Box: `https://arduino.esp8266.com/stable/package_esp8266com_index.json`
2. Confirm your changes, then go to Tools > Boards > Board Manager and install the ESP8266 board package.
3. Select your board from the Tools > Boards menu. `LOLIN(WEMOS) D1 R2 & mini` is what I use.
## Libraries
- Timelib
- Adafruit GFX
- The library that is compatible with your OLED screen (in my case, this was Adafruit_SSD1306. Change this in the code if you are using a different screen)
- ArduinoJson (7)
- ESP8266Wifi
- Wire

All of these can be installed through the Library Manager in Arduino IDE.
# Wiring
```
OLED -> Wemos

VCC -> 3.3V
GND -> GND
SCL -> D1
SDA -> D2

Button -> Wemos

Positive -> D6
GND -> GND

```
Note: I spliced the GND wires for the OLED and Button so that they both went to the singular GND pin on the Wemos. This might not be best practice.
# Setup
1. Load the files into the Arduino IDE
2. Change the defined values in `credentials.h` to match your wifi information, timezone and Nightscout site.
3. If necessary, change the screen declaration.
4. Upload to your board!


