# Code for the Adafruit USB Power Gauge mini kit

<a href="http://www.adafruit.com/products/1549"><img src="assets/image.jpg?raw=true" width="500px"></a>

This little USB port go-between is like a speed gauge for your USB devices. Instead of hauling out a multimeter and splicing cables, plug this in between for a quick reading on how much current is being drawn from the port. Great for seeing the charge rate of your phone or tablet, checking your battery chargers, or other USB powered projects.

There are a few USB power meters out there, The [Practical Meter](http://www.kickstarter.com/projects/david-toledo/the-practical-meter-know-your-power) and the [USB Spypow](http://dangerousprototypes.com/2013/07/19/pcb-a-week-17-usb-spypow/). We wanted something that was made for makers: Reprogrammable micro-controller, analog output, TTL serial output for debugging / datalogging and of course, [open source](https://github.com/adafruit/USB-Power-Gauge).

Data is passed through transparently from end to end, so you can use it with any USB device at any speed. The power line has a 0.1 ohm current sense resistor and an INA169 high-side current sensor that is tracked by a little ATtiny85 chip. The microcontroller is programmed to read the current draw as well as the bus voltage and light up the strip of LEDs on the side.

The blue LEDs will light up, one for each Watt of power draw (which is ~200mA at 5V nominal), with a couple levels of PWM dimming for increasing current. You can measure up to 1A of current draw - most USB ports are rated for 500mA. It's safe to use this with something that draws up to 2 A of current, it'll just 'max out' the LEDs.

The green LED is helpful to tell if you have too much droop on your power line. It stays lit as long as the voltage is higher than 4.5V, most devices won't charge effectively once it goes below that so if the green LED goes out, you know you should check your port, shorten the USB cable, or reduce the current draw.

As an awesome extra, we also print out the voltage, current and wattage data as readable text on the TX pin at 9600 baud. Connect an FTDI friend, USB console cable, microcontroller, XBee, whatever you want that can read 9600 baud TTL serial data for datalogging, plotting or display.

Comes as a mini kit with an assembled & tested PCB plus a separate USB jack and plug as shown above. Before use, solder the jack and plug. It'll only take you a few minutes and can be done with any soldering iron. Or, advanced users can splice it between a USB extension cable.

__Please note__: this is a handy gadget but it isn't a multimeter! We do some basic calibration during test, but the serial output readings are not precise and should be used as a basic guide rather than lab-grade data plots. Assume a variance of at least 0.1V and 50mA due to noise, thermal changes, etc.

## License

Adafruit invests time and resources providing this open source design, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Designed by Adafruit Industries.  
Creative Commons Attribution, Share-Alike license, check license.txt for more information
All text above must be included in any redistribution
