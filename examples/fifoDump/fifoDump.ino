/* .+

.context    : FIFOEE, FIFO of variable size data blocks over EEPROM
.title      : show FIFO dump capabilitties
.kind       : c++ source
.author     : Fabrizio Pollastri <mxgbot@gmail.com>
.site       : Revello - Italy
.creation   : 30-Oct-2022
.copyright  : (c) 2022 Fabrizio Pollastri
.license    : GNU Lesser General Public License

.description
  This application show the dump capabilities of FIFOEE for an example FIFO
  configuration.
  
.- */

#define FIFOEE_DEBUG         // include debug printout methods

#define BUFFER_START_ADDR  (int)0xF
#define BUFFER_SIZE 258      // make ring buffer size a prime number (257)
#define COMMIT_PERIOD 0     // never commit

#include <fifoee.h>

// define ring buffer
#ifdef __AVR__
  FIFOEE rFIFOEE((uint8_t *)BUFFER_START_ADDR,BUFFER_SIZE);
#elif defined(ESP8266) || defined(ESP32)
  FIFOEE rFIFOEE((uint8_t *)BUFFER_START_ADDR,BUFFER_SIZE,COMMIT_PERIOD);
#else
  #error ERROR: unsupported architecture.
#endif


void setup()
{
  // initialize serial
  Serial.begin(9600);
  delay(1000);

  // try to begin fifo buffer and check for errors
  Serial.print("\n\nTrying to begin FIFO buffer ... ");
  int error = rFIFOEE.begin();
  if (error) {

    Serial.println("FIFO begin failed, it is probably the first time.");

    // format FIFO, retry begin and check for errors
    Serial.print("Formatting FIFO ... ");
    rFIFOEE.format();
    Serial.println("Done");

  }
  else
    Serial.println("Done: FIFO already present.");


  Serial.println("\nFIFO control variables");
  rFIFOEE.dumpControl();

  Serial.println("\nFIFO ring buffer");
  rFIFOEE.dumpBuffer();
 
}


void loop()
{
  // stop here
  while (1) {delay(1000);};
}
  
/**** END ****/
