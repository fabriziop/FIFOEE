/* .+

.context    : FIFOEE, FIFO of variable size data blocks over EEPROM
.title      : test push, pop, read, begin methods
.kind       : c++ source
.author     : Fabrizio Pollastri <mxgbot@gmail.com>
.site       : Revello - Italy
.creation   : 6-Oct-2021
.copyright  : (c) 2021-2022 Fabrizio Pollastri
.license    : GNU Lesser General Public License

.description
  This application performs several test cycles, each with a sequence of
  push with a random length and random size data blocks to the FIFO,
  followed by a sequence of pop with a random length from the same buffer.
  Pushed and popped data is compared for equality. At each cycle, between
  push sequence and pop sequence, a FIFO begin is done. If begin performs
  correctly, it must be completely transparent.
  
.- */

#define FIFOEE_RAM      // WARNING: do not remove if you want a healthy EEPROM.
                        // This test can completely wear out your EEPROM,
                        // so let it run into RAM.
#define FIFOEE_DEBUG    // include debug printout methods

#define TEST_CYCLES 1000
#define BUFFER_START_ADDR  (int)0x10
#define BUFFER_SIZE 258      // make ring buffer size a prime number (257)
#define MAX_DATA_SIZE 127
#define BLOCK_SEQ_LEN_MAX 20 // max number of data block written to buffer
#define COMMIT_PERIOD 0      // never commit

#include <fifoee.h>


// a single data block
uint8_t data[FIFOEE_DATA_SIZE_MAX];

// random parameters for test data
size_t dataSize;
size_t random_seq_len;

// define ring buffer
#ifdef __AVR__
  FIFOEE rFIFOEE((uint8_t *)BUFFER_START_ADDR,BUFFER_SIZE);
#elif defined(ESP8266) || defined(ESP32)
  FIFOEE rFIFOEE((uint8_t *)BUFFER_START_ADDR,BUFFER_SIZE,COMMIT_PERIOD);
#else
  #error ERROR: unsupported architecture.
#endif

// general vars
char line[80];
int error;
int errCount = 0;


void setup()
{
  // initialize serial
  Serial.begin(9600);
  delay(1000);

  rFIFOEE.format();
 
  // begin fifo buffer and check for errors
  error = rFIFOEE.begin();
  if (!error)
    return;

  snprintf(line,sizeof(line),"ERROR #%d at FIFO buffer begin.",error);
  Serial.println(line);
  errCount++;
  rFIFOEE.dumpControl();
  rFIFOEE.dumpBuffer();
  
  // format FIFO, retry begin and check for errors
  Serial.print("Formatting FIFO ...");
  rFIFOEE.format();
  Serial.println("Done");
  error = rFIFOEE.begin();

  if (!error)
    return;
  snprintf(line,sizeof(line),"ERROR #%d at FIFO begin.",error);
  Serial.println(line);
  rFIFOEE.dumpControl();
  rFIFOEE.dumpBuffer();

  // stop here
  while (1) {delay(1000);};
}


void loop()
{
  
  // data payload generator, checker
  uint8_t pushData = 1;
  uint8_t pushDataStart = 1;
  uint8_t readData = 1;
  uint8_t popData = 1;
  uint8_t popDataStart = 1;
  
  // repeat the test, each time changing data
  
  for (int k=0; k < TEST_CYCLES; k++) {
  
    // push a sequence of blocks filled with progressive numbering data,
    // for each block numbering starts with the block size,
    // with a random sequence length in the range 1-SEQ_LEN_MAX and with
    // blocks of random data size in the rage 1-64 byte.

    int blockSeqLen = rand() % BLOCK_SEQ_LEN_MAX + 1;
    for (int j=0; j < blockSeqLen; j++) {

      // generate data payload: unsigned 8 bit integer numbering sequence
      size_t dataSize = rand() % MAX_DATA_SIZE + 1;
      pushDataStart = pushData;
      for (int i=0; i < dataSize; i++)
        data[i] = (uint8_t)(pushData++);

      // push generated data into FIFO and check for errors
      error = rFIFOEE.push(data,dataSize);
      
      if (error) {
        if (error == FIFOEE::FIFO_FULL) {
          pushData = pushDataStart;
          break;
        }
        snprintf(line,sizeof(line),
          "ERROR #%d pushing into FIFO. cycle = %d, block = %d",error,k,j);
        Serial.println(line);
        errCount++;
        rFIFOEE.dumpControl();
        rFIFOEE.dumpBuffer();
      }

    }

    // read all pushed blocks
    blockSeqLen <<= 1;
    for (int j=0; j < blockSeqLen; j++) {

      // read a block of data and check for errors
      dataSize = FIFOEE_DATA_SIZE_MAX;
      error = rFIFOEE.read(data,&dataSize);
      if (error) {
        if (error == FIFOEE::FIFO_EMPTY)
          break;
        snprintf(line,sizeof(line),
          "ERROR #%d reading from FIFO. cycle = %d, block = %d",error,k,j);
        Serial.println(line);
        errCount++;
        rFIFOEE.dumpControl();
        rFIFOEE.dumpBuffer();
      }

      // check read data for correct sequence
      for (size_t i=0; i < dataSize; i++) {
        if (data[i] == readData++)
          continue;
        snprintf(line,sizeof(line),"Read error. cycle = %d, block/byte ="
          " %d/%d, pushData = %x, readData = %x",k,j,i,readData,data[i]);
        Serial.println(line);
        errCount++;
      }
    }

    // pop a random number of blocks from FIFO and check their data versus
    // a continuos numbering sequence
    
    blockSeqLen = rand() % BLOCK_SEQ_LEN_MAX + 1;
    for (int j=0; j < blockSeqLen; j++) {

      // pop a block of data and check for errors
      dataSize = FIFOEE_DATA_SIZE_MAX;
      error = rFIFOEE.pop(data,&dataSize);
      if (error) {
        if (error == FIFOEE::FIFO_EMPTY)
          continue;
        snprintf(line,sizeof(line),
          "ERROR #%d popping from FIFO. cycle = %d, block = %d\n",error,k,j);
        Serial.println(line);
        errCount++;
        rFIFOEE.dumpControl();
        rFIFOEE.dumpBuffer();
      }

      // compare pushed data with popped data
      for (size_t i=0; i < dataSize; i++) {
        if (data[i] == (uint8_t)(popData++))
          continue;
        snprintf(line,sizeof(line),"Pop error. cycle = %d, block/byte = %d/%di"
          ", pushData = %x, popData = %x",k,j,i,popData - 1,data[i]);
        Serial.println(line);
        errCount++;
      }
    }

    // keep software WDT quiet
    delay(1);

  }

  if (errCount)
    snprintf(line,sizeof(line),"TEST FAIL: %d errors",errCount);
  else
    snprintf(line,sizeof(line),"TEST OK");
  Serial.println(line);
    
  // stop here
  while (1) { delay(1000); }
  
}

/**** END ****/
