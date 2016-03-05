# raspberrypi

Herein lies Raspberry Pi related code.

###License

Unless otherwise noted all code in this repository is under the MIT license.

## What's available

RTC support library. Currently supports the DS3231 RTC device.

There is also three implementations of I2C bus wrappers. One is based on the Linux native SMBUS support. This is currently the only implementation that has been tested.

The other two are experimental and based on the [pgpio library](http://abyz.co.uk/rpi/pigpio/). pigpio has no license (it is in the public domain.) These implementations have not been tested *at all* so I suggest not using them.

If the pgpio library turns out to be useful I intend to copyright it myself and release it under MIT. I am not a big fan of public domain code from a licensing perspective. Code without copyright is often frowned upon by corporate attorneys. It's ironic that having no license is actually more restrictive than a very liberal (MIT/BSD, etc) license.

If I do assign copyright to myself I will of course not claim authorship.
