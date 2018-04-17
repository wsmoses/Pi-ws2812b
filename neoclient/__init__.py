import atexit

def Color(red, green, blue, white = 0):
    """Convert the provided red, green, blue color to a 24-bit color value.
    Each color component should be a value 0-255 where 0 is the lowest intensity
    and 255 is the highest intensity.
    """
    return (white << 24) | (red << 16)| (green << 8) | blue

class Adafruit_NeoPixel(object):
    def __init__(self, num, ip, port):
        from remotearray import RemoteLED
        self._led_data = RemoteLED(ip, port, num)
        atexit.register(self._cleanup)

    def _cleanup(self):
        # Clean up memory used by the library when not needed anymore.
        if self._led_data is not None:
            self._led_data.close()
            self._led_data = None
            pass

    def begin(self):
        pass

    def show(self):
        self._led_data.flush()

    def setPixelColor(self, n, color):
        """Set LED at position n to the provided 24-bit color value (in RGB order).
        """
        self._led_data[n] = color

    def setPixelColorRGB(self, n, red, green, blue, white = 0):
        """Set LED at position n to the provided red, green, and blue color.
        Each color component should be a value from 0 to 255 (where 0 is the
        lowest intensity and 255 is the highest intensity).
        """
        self.setPixelColor(n, Color(red, green, blue, white))

    def getPixels(self):
        """Return an object which allows access to the LED display data as if
        it were a sequence of 24-bit RGB values.
        """
        return self._led_data

    def numPixels(self):
        """Return the number of pixels in the display."""
        return len(self._led_data)

    def getPixelColor(self, n):
        """Get the 24-bit RGB color value for the LED at position n."""
        return self._led_data[n]
