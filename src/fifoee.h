/* .+

.context    : FIFOEE, FIFO of variable size data blocks over EEPROM
.title      : FIFOEE class definition
.kind       : c++ source
.author     : Fabrizio Pollastri <mxgbot@gmail.com>
.site       : Revello - Italy
.creation   : 28-Sep-2021
.copyright  : (c) 2021-2022 Fabrizio Pollastri
.license    : GNU Lesser General Public License

.description
  FIFOEE realizes an EEPROM ring buffer for blocks of data with variable size,
  managed in FIFO mode. FIFOEE can work also with a RAM ring buffer.

.compile_options
  1. debugging printout methods, to include them define symbol FIFOEE_DEBUG.
  2. use RAM instead of EEPROM, to activate define symbol FIFOEE_RAM.
  3. multiple instances needs explicit EEPROM begin, tell it to FIFOEE defining
  symbol EEPROM_PROGRAM_BEGIN.
  These options must be defined before including fifoee.h .

.- */

#ifndef FIFOEE_H
#define FIFOEE_H

#include "EEPROM.h"


/**** constants ****/

#define BUFFER_SIZE_MIN 5
#define FIFOEE_DATA_SIZE_MAX 127
#define BLOCK_SIZE_MAX FIFOEE_DATA_SIZE_MAX + 1

// block status codes
#define FREE_BLOCK 0x80          // never pushed or pushed and then popped
#define USED_BLOCK 0x00          // pushed and not yet popped

// block header masks
#define BLOCK_STATUS_BIT 0x80
#define BLOCK_SIZE_BITS 0x7f


/**** macros ****/

#ifdef FIFOEE_RAM
  #define EE_WRITE( addr , val ) (buffer[(int)( addr )] = val)
  #define EE_READ( addr ) (buffer[(int)( addr )])
#else
  #ifdef __AVR__
    #define EE_WRITE( addr , val ) (EEPROM[ (int)( addr )  ].update( val ))
  #elif defined(ESP8266) || defined(ESP32)
    #define EE_WRITE( addr , val ) (EEPROM.write((int)( addr ),( val )))
  #else
    #error ERROR: unsupported architecture 
  #endif
  #ifdef ESP32
    #define EE_READ( addr ) (EEPROM.read((int)( addr )))
  #else
    #define EE_READ( addr ) (EEPROM[(int)( addr )])
  #endif
  #include "EEPROM.h"
#endif


/**** class ****/

struct FIFOEE {

  /**** class constants ****/

  public:

  enum returnStatus: uint8_t {

    SUCCESS = 0,
    FIFO_EMPTY,
    FIFO_FULL,
    INVALID_FIFO_BUFFER_SIZE,
    INVALID_BLOCK_HEADER,
    DATA_BUFFER_SMALL,
    PUSH_BLOCK_NOT_FREE,
    UNCLOSED_BLOCK_LIST,
    WRONG_RBUFFER_SIZE

  };

  private:

  /**** class control vars ****/

  uint8_t *pBlock;
  uint8_t blockHeader;
  uint8_t blockStatus;
  size_t blockSize;
  uint8_t *pBotBlock;
  uint8_t *pBotBlockOffset;
  size_t size;
  size_t rBufSize;
  uint8_t *pRBufStart;
  uint8_t *pRBufEnd;

  uint8_t *pPush;
  uint8_t *pPop;
  uint8_t *pRead;

  #if (defined(ESP8266) || defined(ESP32)) && !defined FIFOEE_RAM
  uint32_t nextCommit = 0;
  uint32_t commitPeriod;
  bool eepromBegin = true;
  #endif

  #ifdef FIFOEE_RAM
  uint8_t *buffer;
  #endif


  /**** class member functions ****/

  public:

  #if defined(ESP8266) || defined(ESP32)
  FIFOEE(uint8_t *aBuffer,size_t aBufSize,uint32_t aCommitPeriod) {
  #else
  FIFOEE(uint8_t *aBuffer,size_t aBufSize) {
  #endif
  /* class constructor
   * aBuffer: FIFO buffer start address, area for control var and ring buffer.
   * aBufSize: buffer size (bytes).
   * aCommitPeriod: (ESP8266 and ESP32 only) real write to EEPROM happens every
   *   period.
   */

    #ifdef FIFOEE_RAM
      buffer = (uint8_t *)malloc((int)aBuffer + aBufSize);
    #endif

    // allocate and init control vars
    pBotBlockOffset = aBuffer;
    rBufSize = aBufSize - 1;
    pRBufStart = aBuffer + 1;
    pRBufEnd = pRBufStart + rBufSize;

    #if (defined(ESP8266) || defined(ESP32)) && !defined FIFOEE_RAM
    commitPeriod = aCommitPeriod;
    nextCommit = millis() + commitPeriod;
    #endif

  }


  int format(void) {
  /* format essential metadata of ring buffer, buffer is logically cleared.
   * Fill the ring buffer with a chain of forward linked free blocks with
   * a prefixed data size of FIFOEE_DATA_SIZE_MAX and a final block with
   * a proper size to fill all ring buffer.
   */

    // check for valid buffer size: minimum size 5 bytes, a FIFO of one block
    // of one byte of data (not very useful :).
    if (rBufSize < BUFFER_SIZE_MIN - 1)
      return INVALID_FIFO_BUFFER_SIZE;

    // if required, begin EEPROM
    #ifndef EEPROM_PROGRAM_BEGIN
    #if !defined(FIFOEE_RAM) && !defined(__AVR__)
    if (eepromBegin) {
      #if defined ESP8266
      EEPROM.begin((size_t)pRBufStart + rBufSize);
      #elif defined(ESP32)
      if (!EEPROM.begin((size_t)pRBufStart + rBufSize)) {
        Serial.println("ERROR: EEPROM init failure");
        while(true) delay(1000);
      }
      delay(500);
      #endif
      eepromBegin = false;
    }
    #endif
    #endif

    // clear the offset of bottommost block
    EE_WRITE(pBotBlockOffset,0);

    // init pointers for an empty ring buffer
    pPush = pRBufStart;
    pPop = pPush;
    pRead = pPush;

    // insert the highest allowed number of free blocks with BLOCK_DATA_SIZE_MAX
    // byte fixed data size into the ring buffer 
    // fill all the remaining space with appropriately sized blocks
    size_t sizeToFill = rBufSize;
    pBlock = pRBufStart;

    while (sizeToFill > BLOCK_SIZE_MAX) {

      EE_WRITE(pBlock,FREE_BLOCK | FIFOEE_DATA_SIZE_MAX);
      pBlock += BLOCK_SIZE_MAX;
      sizeToFill -= BLOCK_SIZE_MAX;

    }

    // set residual space
    EE_WRITE(pBlock,FREE_BLOCK | sizeToFill - 1);

    #if (defined(ESP8266) || defined(ESP32)) && !defined FIFOEE_RAM
    EEPROM.commit();
    nextCommit = millis() + commitPeriod;
    #endif

  }


  // class initializer
  int begin(void) {
  /* Class instance init
   * Scan the ring buffer for a valid data structure. If yes,
   * gather all relevant data for its management. If not,
   * return an error code.
   */

    // if required, begin EEPROM
    #ifndef EEPROM_PROGRAM_BEGIN
    #if !defined(FIFOEE_RAM) && !defined(__AVR__)
    if (eepromBegin) {
      #if defined ESP8266
      EEPROM.begin((size_t)pRBufStart + rBufSize);
      #elif defined(ESP32)
      if (!EEPROM.begin((size_t)pRBufStart + rBufSize)) {
        Serial.println("ERROR: EEPROM init failure");
        while(true) delay(1000);
      }
      delay(500);
      #endif
      eepromBegin = false;
    }
    #endif
    #endif

    // scan the blocks sequence in the ring buffer for changes of status
    // to find the position of pointers pPush, pPop, pRead.
    pBotBlock = pRBufStart + EE_READ(pBotBlockOffset);
    blockHeader = EE_READ(pBotBlock);

    // check for invalid header (0x00: used block cannot have zero size)
    if (!blockHeader)
      return INVALID_BLOCK_HEADER;

    // init pointers and control variables for an empty ring buffer
    pBlock = pBotBlock;
    pPush = pBlock;
    pPop = pBlock;
    pRead = pBlock;

    // scan blocks into ring buffer for change of status and block size
    blockStatus = blockHeader & BLOCK_STATUS_BIT;
    uint8_t oldStatus = blockStatus;
    size_t detectedRBufSize = (blockHeader & BLOCK_SIZE_BITS) + 1;
    while (1) {

      // point to next block
      pBlock += (blockHeader & BLOCK_SIZE_BITS) + 1;

      // if end of block chain (reached the ring buffer end) ...
      if (pBlock >= pRBufEnd) {
	
	// check if block linked list closes over itself: topmost block
	// must link to bottommost block.
	if (pBotBlock != pBlock - rBufSize)
	  return UNCLOSED_BLOCK_LIST;

        // check if block sizes summation == given ring buffer size
	if (rBufSize != detectedRBufSize)
	  return WRONG_RBUFFER_SIZE;

	break;
      }

      // get next block status and check it for validity
      blockHeader = EE_READ(pBlock);
      if (!blockHeader)
        return INVALID_BLOCK_HEADER;

      blockStatus = blockHeader & BLOCK_STATUS_BIT;

      // accumulate block sizes
      detectedRBufSize += (blockHeader & BLOCK_SIZE_BITS) + 1;

      // go on untill there is a change of status
      if (oldStatus == blockStatus)
	continue;

      // there is a change of status in block sequence, process it.
      uint8_t oldStatusSaved = oldStatus;
      oldStatus = blockStatus;
      switch (oldStatusSaved) {

	// previous blocks have free status: next block is the head of
	// the FIFO queue.
	case FREE_BLOCK:

          pPop = pBlock;
	  pRead = pBlock;
	  continue;

	// previous blocks have used status: next block must nave a free
	// status and it is the tail of the FIFO queue.
	case USED_BLOCK:

	  pPush = pBlock;
	  continue;
      }
    }

    return SUCCESS;

  }
 

  int push(uint8_t *data, size_t size) {
  /* push data to EEPROM (write a new block)
   */

    // current push block must be free
    blockHeader = EE_READ(pPush);
    blockStatus = blockHeader & BLOCK_STATUS_BIT;
    if (blockStatus != FREE_BLOCK)
      return PUSH_BLOCK_NOT_FREE;
    
    //// allocate required data size. If current pPush block is smaller
    // than the requested size, merge the following free blocks until
    // the requested size + 3 is satisfied or exceeded.
    size_t blockSize = (blockHeader & BLOCK_SIZE_BITS) + 1;

    while (size + 1 > blockSize) {

      pBlock = pPush + blockSize;

      while (pBlock >= pRBufEnd)
	pBlock -= rBufSize;
	
      if (pBlock == pPush) 
        return FIFO_FULL;

      blockHeader = EE_READ(pBlock);
      blockStatus = blockHeader & BLOCK_STATUS_BIT;

      if (blockStatus != FREE_BLOCK)
        return FIFO_FULL;

      blockSize += (blockHeader & BLOCK_SIZE_BITS) + 1;
    }

    //// manage 2 relevant cases of required block size (size + 1) vs
    // the allowable free block size space found above (blockSize).

    // case #1: required size <= free size, allocate required size and
    // make a new free block to fill the residual space.
    if (size + 1 < blockSize) {

      pBlock = pPush + size + 1;

      while (pBlock >= pRBufEnd)
        pBlock -= rBufSize;

      EE_WRITE(pBlock,FREE_BLOCK | blockSize - size - 2);

    }

    // case #2: required size == free size, if next block is free go on.
    // Otherwise, return FIFO full.
    else {

      pBlock = pPush + blockSize;

      while (pBlock >= pRBufEnd)
        pBlock -= rBufSize;

      if (pBlock == pPush)
        return FIFO_FULL;

      blockHeader = EE_READ(pBlock);
      blockStatus = blockHeader & BLOCK_STATUS_BIT;

      if (blockStatus != FREE_BLOCK)
        return FIFO_FULL;

    }
 
    //// copy given data to eeprom data block
    uint8_t *pData;

    // if the block is splitted (block wraps at FIFO buffer end back to start)
    if (pPush + size + 1 > pRBufEnd) {

      //  copy first data part
      for (pData=pPush+1; pData < pRBufEnd; pData++)
        EE_WRITE(pData,*data++);
 
      //  copy second data part
      for (pData=pRBufStart; pData < pPush + size + 1 - rBufSize;  pData++)
        EE_WRITE(pData,*data++);

      // update offset of bottom block
      EE_WRITE(pBotBlockOffset,pData - pRBufStart);

    }

    // if the block is not splitted (block does not overflow FIFO buffer end)
    else {

      //  copy data
      for (pData=pPush+1; pData < pPush + size + 1; pData++)
        EE_WRITE(pData,*data++);

    }

    // set size and status for copied data block
    EE_WRITE(pPush,USED_BLOCK | size);

    // update push pointer to next block and bottommost block offset
    // from pRBufStart 
    if (pData < pRBufEnd)
      pPush = pData;
    else {
      pPush = pRBufStart;
      // update offset of bottom block
      EE_WRITE(pBotBlockOffset,0);
    }

    #if (defined(ESP8266) || defined(ESP32)) && !defined FIFOEE_RAM
    commitRequest();
    #endif

    return SUCCESS;

  }


  int pop(uint8_t *data,size_t *size) {
  /* pop out a block: copy data of the current pop block from the FIFO
   * ring buffer to a given data buffer and mark the popped block in the
   * FIFO ring buffer as "free", ready to be used to store another
   * incoming block. The pop pointer in the ring buffer is moved to
   * the next block.
   */

    // if ring buffer is empty
    if (pPop == pPush)
      return FIFO_EMPTY;

    // copy data from ring buffer to given data buffer
    pBlock = pPop;
    if (int rc = readData(data,size))
      return rc;

    // mark block just read as deleted
    EE_WRITE(pPop,FREE_BLOCK | blockSize - 1);

    #if (defined(ESP8266) || defined(ESP32)) && !defined FIFOEE_RAM
    commitRequest();
    #endif

    // read pointer must be always at or before pop pointer
    if (pRead == pPop)
      pRead = pBlock;

    // move pop pointer to next block
    pPop = pBlock;

    return SUCCESS;

  }


  int read(uint8_t *data,size_t *size) {
  /* read a block: copy data of the current read block from FIFO ring
   * buffer to a given data buffer and mark the read block in the FIFO
   * ring buffer as "read". The read pointer of the ring buffer is
   * moved to the next block.
   */

    // if ring buffer is empty
    if (pRead == pPush)
      return FIFO_EMPTY;

    // copy data from ring buffer to given data buffer
    pBlock = pRead;
    if (int rc = readData(data,size))
      return rc;

    // update read pointer
    pRead = pBlock;

    return SUCCESS;
    
  }
 

  void restartRead(void) {
  /* the read pointer is moved to the oldest data block, the FIFO queue head.
   * WARNING: already read block status is NOT changed. So, at FIFO begin,
   * all previous restarts are lost.
   */

    // restart reading
    pRead = pPop;

  }


  private:

  int readData(uint8_t *data,size_t *size) {
  /* copy all data of block pointed by pBlock from the FIFO ring buffer to
   * the given data buffer. If data block is splitted into a part at ring
   * buffer top and what remains at buffer bottom, copy both as if they
   * were a single block. If the data buffer size is too small for data
   * block size, do not copy it and return a proper error code. Otherwise,
   * return success (0). Also return the size of copied data. The pBlock
   * pointer is set to point to the next block.
   */

    // check for sufficient data size
    blockSize = (EE_READ(pBlock) & BLOCK_SIZE_BITS) + 1;
    if (blockSize - 1 > *size)
      return DATA_BUFFER_SMALL;

    //// copy data from eeprom to the given data buffer.
    uint8_t *pData;
    *size = blockSize - 1;

    // if the block is splitted ...
    if (pBlock + blockSize > pRBufEnd) {

      // copy first data part at end of ring buffer
      for (pData=pBlock+1; pData < pRBufEnd; pData++)
        *data++ = EE_READ(pData);

      // copy second data part at start of ring buffer
      for (pData=pRBufStart; pData < pBlock + blockSize - rBufSize; pData++)
        *data++ = EE_READ(pData);

    }

    // if the block is not splitted ...
    else {

      // copy data
      for (pData=pBlock+1; pData < pBlock + blockSize; pData++)
        *data++ = EE_READ(pData);

    }

    // next block pointer
    if (pData < pRBufEnd)
      pBlock = pData;
    else
      pBlock = pRBufStart;

    return SUCCESS;
    
  }


  #if (defined(ESP8266) || defined(ESP32)) && !defined FIFOEE_RAM
  void commitRequest(void) {
  /* commit changes to EEPROM not more frequently than a given period
   */

    if (!commitPeriod)
      return;

    uint32_t now = millis();
    if (now < nextCommit)
      return;

    EEPROM.commit();
    nextCommit = now + commitPeriod;

    return;

  }
  #endif


  /**** optional debug member functions ****/

  #ifdef FIFOEE_DEBUG

  public: 

  void dumpControl(void) {
  /* hexadecimal dump of control variables
   */

    Serial.print("pRBufStart:     ");
    Serial.println((int)pRBufStart,HEX);
    Serial.print("pRBufEnd:       ");
    Serial.println((int)pRBufEnd,HEX);
    Serial.print("rBufSize:       ");
    Serial.println((int)rBufSize,HEX);
    Serial.print("BotBlockOffset: ");
    Serial.println((int)EE_READ(pBotBlockOffset),HEX);
    Serial.print("pPush:          ");
    Serial.println((int)pPush,HEX);
    Serial.print("pPop:           ");
    Serial.println((int)pPop,HEX);
    Serial.print("pRead:          ");
    Serial.println((int)pRead,HEX);

  }


  void dumpBuffer(void) {
  /* hexadecimal dump of whole data buffer
   */

    for (uint8_t *addr = pRBufStart; addr < pRBufEnd;) {

      Serial.print((int)addr, HEX);
      Serial.print(":");
      uint8_t *lineEnd;
      lineEnd = addr + 16;
      if (lineEnd > pRBufEnd)
	lineEnd = pRBufEnd;

      for (addr; addr < lineEnd; addr++) {

        Serial.print(" ");
        uint8_t value = EE_READ(addr);
        if (value < 16)
	  Serial.print("0");
        Serial.print((int)value, HEX);
      }

      Serial.println();
    }

  }

  #endif

};

#endif

/**** end ****/
