
                             NMEA Generator Utility

                               NMEAGEN.exe Ver2.02

                               Nov. 2016 (C)4river
                           http://4river.a.la9.jp/gps/

Summary.

  NMEA Generator generates the NMEA sentence of the GPS receiver and outputs it in a serial port.
  Because the output cycle and the output sentence and the course, etc. can be arbitrarily set, 
  A necessary sentence and response speed for the digital map for GPS, etc. can be confirmed.
  Confirming the operation of the digital map used in the travel destination of the foreign country
  can be done domestically.


  Operating condition.
     Operational OS:      Window 10(Desktop), Window 8(Desktop), Windows 7, Windows Vista, Windows XP.
     RS-232C baud rate:   300 to 460,800bps.


Feature.
  1) Each sentence of GGA, RMC, GSA, GSV, GLL, VTG, ZDA, GNS and HDT can be output at an arbitrary cycle.
  2) The course can specify the position, the altitude, and the speed more than 50 points
     (It can be extended in the configuration file).
  3) The decimal digit in the latitude and longitude can be specified.
  4) The presence of the output at less than second of the UTC time can be specified.
  5) The presence of the output of "Mode" indicator since NMEA0183 Ver2.3 can be specified.
  6) The height of the geoid can be arbitrarily set.
  7) The order of sending GGA,GNS and RMC can be specified.
  8) "Fix quality" of GGA(GNS) can be arbitrarily set.
  9) GSA and GSV sentence beforehand register and when executing it, can switch four kinds.
 10) When executing it, the options can be in real time changed (except the course setting).
 11) The option setting is preserved in configuration file NMEAGEN.ini, and when reactivating, used.
 12) The configuration file is renamed, several kinds are prepared, and the setting can be switched
     instantaneously by the command line option or drag & drop.


Install.
  In installation just copies executable file NMEAGEN.exe in the suitable folder.
  Because registry is not used, it can Un-install with only the deletion of NMEAGEN.exe and NMEAGEN.ini


Connection with digital map.
  The following method is used to connect the digital map with NMEAGEN.exe.
  1) When the personal computer has two serial ports.
     It is possible to use it with one PC by connecting between serial ports with the RS-232C cross cable.
  2) When the personal computer has only one serial port.
     The serial port between personal computers is connected with the RS-232C cross cable with two personal computers.
  3) When the personal computer doesn't have the serial port or there is only one port.
     It connects it by using the virtual serial port software such as GpsGate.


Usage.

   Note) The keyboard can input directly the numerical value that doesn't exist in the pull-down list.

 1. NMEA Output.
   Start:   The output of the NMEA sentence begins when clicking.

   Idling:  The NMEA sentence is output fixing to a present position when clicking after it starts and repeatedly.
            When "Start" is clicked, it continues from a present position.
            It jumps to the set point by clicking the "Jump" button while idling, and it moves from
            the point with the "Start" button.

   Stop:    The output of the NMEA sentence is stopped.

   Speed(Km/h): The unit at the speed is changed in order of Km/h -> mph -> knot every time it clicks.
                The unit of the altitude and the geoid height changes into m -> feet -> feet at the same time, too.

   Monitor:     The NMEA sentence can be monitored by clicking (toggle).

   When the display of the "Sending: xxx bytes" becomes yellow, it is necessary to raise the baud
   rate or to slow down the output cycle because the transmission rate is insufficient.


   Point display
     It displays by the following format.
     Current Span - Current Point / Total points of current span


 2. Track setting.
   Latitude, longitude, altitude and speed of each point are input.
   The latitude and longitude can be specified by following, various formats.

   Case of north latitude 35°40' 49.448"
     1) 35°40' 49.448" or 35°40' 49.448"N or N35°40' 49.448"
     2) 35 40 49.448 or 35 40 49.448N or N35 40 49.448
     3) 35.68040222 or 35.68040222N or N35.68040222
     Can be specified by above-mentioned.

   It returns to the starting point at the speed when the speed is specified for the last data.

   Minus sign or "S" or "W" are added for the south latitude or the west longitude.
     Ex) -35 40  49.448 or 35.68040222S or S35.68040222

   It returns to the starting point at the speed when the speed is specified for the last data.
     Note) A speed zero can't be designated (It's regarded as the end of data).

   It outputs it repeating the NMEA data when the "Rep." check box is checked.

   When the "Jump" button is clicked when stopping, the NMEA sentence that corresponds to the
   position is output one shot.
   When the "Jump" button is clicked while idling, the NMEA sentence that corresponds to the
   position is continuously output.
   It is possible to use it to confirm the setting data by connecting it with the digital map.


 3. GSA,GSV.
   The selection can switch four kinds of sentences set beforehand.
   Because the sentence text is displayed when "Edit" is clicked, it is possible to edit it.
   Check-sum is automatically added.
   The content corrected to close the text is applied.
   HDOP and the "Number of satellites in view" of GGA sentences are extracted from the GSA sentence.
     Note) It is also possible to describe arbitrary fixed sentences other than GSA or GSV and to send it.


 4. Output condition.
   Period:       The output cycle when the interval is one is specified with Hz.
   NMEA:         Specifies the version of the NMEA.
                   2.3 : A GNS sentence and MODE are supported.
                   4.1 : A GNS sentence, MODE and Navigation Status are supported.
   Nav. Status:  Navigation Status を設定します。
   GGA,RMC,GLL,VTG,ZDA,GNS,HDT,GSA,GSV:
                 It is specified whether to output the sentence with "Output" check box.
                 What each count at the output cycle it outputs it by "Interval" is specified.
                 It specifies the first two characters of the sentence in the "Prefix".
   Lat/Lon dec.: The number of digits of decimal parts in the latitude and longitude is specified(1 to 8).
   Second dec.:  It specifies the number of digits in less than a second of time (0-3).
   Geoid:        The height of the geoid is specified.
   GGA quality:  The quality status of the GGA sentence is specified.

                Allocation to set value and each sentence status.
                                       GGA   RMC   RM,GNS
                    Set value         Status Status Mode
                  Invalid                0     V     N
                  GPS fix (SPS)          1     A     A
                  DGPS fix               2     A     D
                  PPS fix                3     A     P
                  Real Time Kinematic    4     A     R
                  Float RTK              5     A     F
                  Estimated              6     V     E
                  Manual input mode      7     V     M
                  Simulation mode        8     V     S

                RMC Status
                  V: Invalid
                  A: Valid

   RMC presed:  The RMC sentence is previously output when checking it.

 5. Serial port.
   Port number: The serial port number is specified.
   Speed(bps):  The transmission rate is specified.
   Open:        The serial port is opened when clicking.
   Close:       The serial port is closed when clicking.

   When the port number and the speed are changed, the port is closed once and it does.

   Exit:  NMEAGEN.exe is ended.


 6. Commandline option.
   An arbitrary option file is applicable by the specification of the configuration file.
   The extension of the configuration file is limited to ".ini".
     Ex) NMEAGEN tokyo.ini

   When the blank is included in path or the file name, it is necessary to enclose it with double quotes(").
     Ex) NMEAGEN "C:\Documents and Settings\User\tokyo.ini"


 7. Drag & drop.
   An arbitrary setting condition is instantaneously applicable the execution file icon or the
   form of the configuration file when dragging it.
   However, drag & drop is disregarded while outputting the NMEA sentence.


 8. Configuration file (NMEAGEN.ini)
   When the folder with the configuration file is read-only, it copies onto folder %APPDATA%\NMEAGEN\ and it uses it.
   %APPDATA% is a folder displayed to input by the DOS prompt as "echo %APPDATA%" <Enter>.

  1) [Output] Section
     The output condition of the NMEA sentence is preserved.

  2) [Track] Section
     The track data is preserved.
       Format: Dn=Latitude, Longitude, Altitude, Speed

  3) [Serial] Section
     The serial port number and the transmission rate are preserved.

  4) [Option] Section
     The unit and the display position at the font and the speed are preserved.
     The font specification is effective only to the main form.

     Initial values of font specification other than Japanese.
       EngCharset=0
       EngFontName=Arial
       EngFontSize=8

     Initial value of Japanese font.
       JpCharset=1
       JpFontName=Tahoma
       JpFontSize=8

     The maximum number of tracks is specified with "TrackMax".
       Ex) TrackMax=50

     Local zone hours & minutes of the ZDA sentence is specified with "TimeDiff".
       Ex) TimeDiff=-9:0

     Magnetic variation of the RMC sentence is specified with "MagVari" (Apply to the VTG sentence).
       Ex) MagVari=-7.0


  5) [DGPS] Section
     Initial value of "DGPA age" and "DGPS-ID" of the GGA sentence.
       Invalid=,
       SPS=,
       DGPS=30,0137
       PPS=,
       RTK=,
       FloatRTK=,
       Estimated=,
       Manual=,
       Simulation=,

  6) [GSAGSV] Section
     GSA and the GSV sentence are specified.


 9.Trouble shoot

  1)The error occurs when GPS of the digital map is connected.
    It terminates abnormally according to the digital map when it is a no signal when the serial port is connected.
    Please do connected operation of GPS after clicking "Idling" button.

  2)The trace by "Start" and "Jump" is different.
    The purpose is to draw in the straight line on the map projected to the plane.
    The trace is corresponding in the map where drawing on the globe is projected to the plane.

  3)The delay is not caused in yellow "Sending:xxx bytes" either.
    When the interval between GSA and the GSV sentence is two or more, the delay might be canceled
    for the period when GSA and GSV stop.
    To transmit data regardless of the baud rate, the delay is not caused for a virtual serial port.


Limitations and notes.

  1) The speed is fine-tuned to match the starting point and the terminal point in the section.
  2) The error margin grows for the large area or the high latitude so that the middle point in
     the section may do the proportion distribution on the orthogonalization coordinates.
  3) The middle point does the dividing equally, and the acceleration control when the start stops is not done.
  4) To use the preset data, neither GSA nor the GSV sentence are automatically changed.
  5) The transmission speed corresponding to the output cycle is necessary.
     There is a possibility of the data delay about the transmission speed dissatisfying it when
     the display "Sending: xxx byts" is yellow.
  6) The response of the digital map depends on the ability of the PC to use.


Used compiler and component.

    In compiler used
      Delphi 10.1 Berlin.

    In addition free component used
      CommX Ver1.06 X(KYY06770) person work. RS232C communication component COMMX106.LZH In the
      writer who the very useful component was offered we appreciate.


Release note.

 * This application is the free software.
 * It does not prohibit redistribution, but *.txt and NMEAGEN.exe and *.ini including the set of distribution.
 * The author takes no responsibility to any losses and obstacles which were produced by use or distribution of this application.


Version history.

  Ver2.02
    1. Increased the DropDownCount value of the "Period" and "Geoid" combo box.

  Ver2.01
    1. Supported the "Navigation Status" output of NMEA 4.1 (RMC, GNS).
    2. The character code of the configuration file was changed to Unicode (Does not change existing settings file). 
    3. The compiler was changed to Delphi 10.1 Berlin.

  Ver2.00 Jul. 2016
    1. The prefix of the NMEA sentence was to be able to select.
    2. It allowed the output of HDT sentence.
    3. Fixed the typo of configuration file.
    4. The compiler was changed to Delphi XE8.

  Ver1.18 Aug. 2013
    1. The digit number was able to be specified at less than second of the NMEA sentence of time.

  Ver1.17 Aug. 2013
    1. The font of the option was applied to "GSA, GSV" and "NMEA sentence" form.
    2. The position of the vertical direction of each Label was set to the center.

  Ver1.16 Jul. 2013
    1. When DPI on the display was changed, the layout was maintained (The adjustment of the fontsize is necessary).

  Ver1.15 May. 2013
    1. Change of geoid height was made to be outputted in real time.

  Ver1.14 Apr. 2013
    1. When GSA and the GSV sentence with checksum were given, checksum was not added.

  Ver1.13 Apr, 2013
    1. The mistake of the direction of deviation of "Course/Magnetic" of the GPVTG sentence was corrected.

  Ver1.12 Mar. 2013
    1. The output of GLL, VTG, and the ZDA sentence was added.
    2. "Magnetic variation" of the RMC sentence was able to be specified by the configuration file (Apply to the VTG sentence).
    3. "Local zone hours & minutes" of the ZDA sentence was able to be specified by the configuration file.
    4. The output under the second of UTC was changed into OFF by the default.

  Ver1.11 Mar. 2013
   1. The number of maximum tracks was able to be specified by the configuration file (Default is 50).
      The scrollbar was added to the string grid.
   2. The configuration file was arranged.
      The DGPS status was separated from the [Option] section to the [DGPS] section.
      The Data section was separated to [GSAGSV] and [Track].
      Track information was brought together in one line per point.

  Ver1.10 Feb. 2013
    1. The GNGNS sentence was added.
    2. The output of two GSA and ten GSV was enabled.
    3. $GNGGA and $GNRMC were able to be output instead of $GPGGA and $GPRMC.
    4. The course point has been increased to six points.
    5. The progress bar was added.

  Ver1.09 Dec. 2012
    1. The setting of "DGPS age" and "DGPS-ID" of the GGA sentence was enabled.
       You can set it in [Option] section of configuration file "NMEAGEN.ini".

  Ver1.08 Jan. 2008
    1. Measures when the configuration file was in a read-only folder were added.
    2. Kilo was corrected to the lower-case.

  Ver1.07 Oct. 2007
    1. The distance was precisely calculated.
    2. The NMEA sending was stopped for the "Jump" button click when idling.

  Ver1.06 Oct. 2007
    1. "Course/True north" = 0 when idling was abolished.
    2. Manual supplement.

  Ver1.05 Oct. 2007
    1. When the speed was filled in on the last course data, it was made to return to the
       starting point.
    2. The number of digits below the decimal point of the distance report was adjusted
       according to the value.
    3. The repeat-output function was added.
    4. Jump under idling was enabled.

  Ver1.04 Sep. 2007
    1. The display of the distance and the time required was added (present span/total).
    2. The mistake of the span number of partitions was corrected.

  Ver1.03 Sep. 2007
    1. Setting had made it succeed before when it was drag & drop of the configuration file,
       the port number, the baud rate, and the display position were blanks.
 
  Ver1.02 Sep. 2007
    1. The font can have been specified by the configuration file.
    2. After the serial port had been opened, an effective port was acquired again.
    3. It was corrected not to have preserved the RMC mode in the configuration file.
    4. The number of digits of integer part of latitude was fixed to four digits and
       the number of digits of integer part in the longitude was fixed to five digits.

  Ver1.01 Sep. 2007
    1. Calculation of "Track made good, degrees true" of a RMC sentence was re-calculated for
       every point(Course of the true north standard).

  Ver1.00 Sep. 2007
    First editions.
