# Arduino board LED driver

This repo contains the code for an arduino board driving an individually addressable LED strip. It uses the [FastLED](https://github.com/FastLED/FastLED) library to interface with the LEDs. This repo mostly contains code related to the creating of certain modes. For example the "solid" mode sets a solid color, the "flash" mode blinks a color, the "dot" mode makes a given number of dots of given colors go around at a given speed. They can all be found in `src/modes.cpp`.

This project uses [PlatformIO](https://platformio.org/) for simple uploading and interfacing with the board itself.

This project is part of the larger [home-automation](https://github.com/SanderRonde/home-automation) project, where this is part of the RGB module.

## Modules

### Modes

As mentioned before, there are a lot of individual modes. The current mode is set, along with a bunch of configuration values for the mode, through the serial module. It then runs until told to stop. 

### Power

The power module makes sure power consumption does not go above the set amount. A problem with LED strips is that they tend to use quite a bit of power. Around 10 watts per meter. This may not seem like a lot but when you have 15 meters of LED strips it adds up, especially since the cables are not capable of transferring all of the delivered power. Hooking up a regular PSU will still not deliver all of the required power.

The result of insufficient power is LEDs not turning on, turning what is supposed to be white light (R+G+B) into yellowish light. Of course it's better to have a less bright but accurate color than a very bright but washed out color. To achieve this, the power module calculates the power usage of the current paint job and scales accordingly.

### Serial control

Serial control from the server needs to be possible. This becomes a bit complex since the modes and their parameters can be quite numerous. The serial control module exists to parse all incoming data.

Because data is sometimes lost in serial messages (probably having to do with the arduino never waiting and constantly painting), a way of checksumming and acknowledging messages has to be created. It's not necessary for this to be very complicates since messages can just be re-sent. As a result, messages just have the format `/ message \`, where the slashes' presences are checked. If this was correct, the arduino sends back a bunch of "ack" messages to the server.

## License

```text
Copyright 2019 Sander Ronde

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```