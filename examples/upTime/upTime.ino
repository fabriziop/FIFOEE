/* .+

.context    : FIFOEE, FIFO of variable size data blocks over EEPROM
.title      : test FIFO persistence measuring system up time
.kind       : c++ source
.author     : Fabrizio Pollastri <mxgbot@gmail.com>
.site       : Revello - Italy
.creation   : 8-Dec-2021
.copyright  : (c) 2021 Fabrizio Pollastri
.license    : GNU Lesser General Public License

.description
  This application measures the system up time by writing into FIFO the
  seconds from the last power up, every minute. At each power up, the data
  already present into the FIFO is printed out. After 3 consecutive power
  up/down cycles, the FIFO is formatted again.
  
.- */

#define FIFOEE_DEBUG         // include debug printout methods

#define BUFFER_START_ADDR  (int)0x200
#define BUFFER_SIZE 258      // make ring buffer size a prime number (257)
#define FIFO_WRITE_PERIOD 6  // write current time to FIFO every 60 seconds
#define COMMIT_PERIOD 55     // comit every 55 milliseconds

#include <fifoee.h>

// define ring buffer
#ifdef __AVR__
  FIFOEE rFIFOEE((uint8_t *)BUFFER_START_ADDR,BUFFER_SIZE);
#elif defined(ESP8266) || defined(ESP32)
  FIFOEE rFIFOEE((uint8_t *)BUFFER_START_ADDR,BUFFER_SIZE,COMMIT_PERIOD);
#else
  #error ERROR: unsupported architecture.
#endif

// vars
int error;
char line[80];
union Data {
  uint32_t timeSample;
  uint8_t data[sizeof(timeSample)];
} data;


void setup()
{
  // initialize serial
  Serial.begin(9600);
  delay(1000);

  // try to begin fifo buffer and check for errors
  Serial.print("\nTrying to begin FIFO buffer ... ");
  error = rFIFOEE.begin();
  if (error) {

    Serial.println("FIFO begin failed, it is probably the first time.");

    // format FIFO, retry begin and check for errors
    Serial.print("Formatting FIFO ... ");
    rFIFOEE.format();
    Serial.println("Done");

  }
  else

    Serial.println("Done: FIFO already present.");

  // if present, gather all data from FIFO

  size_t dataSize = 4;
  uint8_t blocksNum = 0;
  uint8_t powerCycles = 0;
  uint32_t lastPowerUpTime = 0;
  uint32_t lastTimeSample = 0;
  uint32_t upTime = 0;

  rFIFOEE.restartRead();

  while (!rFIFOEE.read(data.data,&dataSize)) {

    snprintf(line,sizeof(line),"block %d, timeSample = %d",
      blocksNum,data.timeSample);
    Serial.println(line);
    
    // first loop init
    if (!blocksNum)
      lastPowerUpTime = lastTimeSample = data.timeSample;

    // detect down intervals and accumulate up times
    if (data.timeSample <= lastTimeSample) {
      upTime += lastTimeSample - lastPowerUpTime;
      lastPowerUpTime = data.timeSample;
      powerCycles++;
    }

    lastTimeSample = data.timeSample;
    blocksNum++;

  }

  // add last power cycle up time
  upTime += lastTimeSample - lastPowerUpTime;

  // print out all data  
  if (!blocksNum) {
    Serial.println("FIFO is empty, no data to print out.");
    return;
  }
  snprintf(line,sizeof(line),"Found a FIFO with %d blocks.",blocksNum);
  Serial.println(line);
  snprintf(line,sizeof(line),"Power cycles %d.",powerCycles);
  Serial.println(line);
  snprintf(line,sizeof(line),"Up time: %d sec.",upTime);
  Serial.println(line);

  // after 3 power cycles formmat FIFO
  if (powerCycles < 3)
    return;

  // format FIFO, retry begin and check for errors
  Serial.print("3 powerCycles reached: formatting FIFO ... ");
  rFIFOEE.format();
  Serial.println("Done");
  error = rFIFOEE.begin();
  if (!error)
    return;
  snprintf(line,sizeof(line),"FIFO BEGIN FATAL ERROR #%d",error);
  Serial.println(line);
  rFIFOEE.dumpControl();
  rFIFOEE.dumpBuffer();
  // stop here
  while (1) {delay(1000);};
  
}


void loop()
{

  data.timeSample = millis() / 1000.;
  error = rFIFOEE.push(data.data,sizeof(data.data));

  if (error) {

    snprintf(line,sizeof(line),"FIFO PUSH ERROR #%d",error);
    Serial.println(line);
    rFIFOEE.dumpControl();
    rFIFOEE.dumpBuffer();
    
    // format FIFO, retry begin and check for errors
    Serial.print("Formatting FIFO ... ");
    rFIFOEE.format();
    Serial.println("Done");

  }

  else {
    snprintf(line,sizeof(line),
      "Time from last power up %d sec written to FIFO.",data.timeSample);
    Serial.println(line);
  }
  delay(6000);

}
  
/**** END ****/
