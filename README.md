# arduino_cripplingly_slow_oscope
arduino single channel graphing voltmeter using 320x200 TFT screen

This uses an analog input, measures at some frequency (currently set by the ADC divider) and plots the result.
The points on the time axis are separated by a user-specified (compile time) distance, currently set so
it plots a data point every five pixels.  When it fills the screen it discards the oldest measurement
and adds the newest at the left-hand side, shifting the plot over.  As it measures, it logs the measurement to 
the SDcard on the board.
Intent is multiple channels with a high impedance voltage divider front end so it can handle inputs well above 
the 5V arduino limitation.
