/* .+

.context    : FIFOEE, FIFO of variable size data blocks over EEPROM
.title      : FIFOEE class definition
.kind       : c++ source
.author     : Fabrizio Pollastri <mxgbot@gmail.com>
.site       : Revello - Italy
.creation   : 28-Sep-2021
.copyright  : (c) 2021 Fabrizio Pollastri
.license    : GNU Lesser General Public License

.description
  FIFOEE realizes an EEPROM ring buffer for blocks of data with variable size,
  managed in FIFO mode. FIFOEE can work also with a RAM ring buffer.

.compile_options
  1. debugging printout methods, to include them define symbol FIFOEE_DEBUG.
  2. use RAM instead of EEPROM, to activate define symbol FIFOEE_RAM.

.- */

#ifndef FIFOEE_H
#define FIFOEE_H


/**** macros ****/

#ifdef FIFOEE_RAM
  #define EE_WRITE( addr , val ) (buffer[(int)( addr )] = val)
  #define EE_READ( addr ) (buffer[(int)( addr )])
#else
  #ifdef __AVR__
    #define EE_WRITE( addr , val ) (EEPROM[ (int)( addr )  ].update( val ))
  #elif ESP8266
    #define EE_WRITE( addr , val ) (EEPROM.write((int)( addr ),( val )))
  #else
    #error ERROR: unsupported architecture 
  #endif
  #define EE_READ( addr ) (EEPROM[(int)( addr )])
  #include <EEPROM.h>
#endif


/**** class ****/

struct FIFOEE {

  /**** class constants ****/

  public:

  static const size_t DATA_SIZE_MAX = 64;

  enum returnStatus: uint8_t {

    SUCCESS = 0,
    FIFO_EMPTY,
    FIFO_FULL,
    DATA_BUFFER_SMALL,
    PUSH_BLOCK_NOT_FREE,
    INVALID_BLOCK_STATUS,
    INVALID_BLOCK_STATUS_CHANGE

  };


  private:

  static const size_t BLOCK_SIZE_MAX = DATA_SIZE_MAX + 1;
  static const size_t BLOCK_STATUS_BITS = 0xc0;
  static const size_t BLOCK_SIZE_BITS = 0x3f;

  // block status codes
  static const size_t FREE_BLOCK = 0xc0;  // never pushed or pushed and then popped
  static const size_t NEW_BLOCK = 0x40;   // pushed and not yet read or popped
  static const size_t READ_BLOCK = 0x00;  // pushed, read at least once and not popped


  /**** class control vars ****/

  uint8_t *pBlock;
  uint8_t blockHeader;
  uint8_t blockStatus;
  uint8_t blockSize;

  uint8_t *pBotBlockOffset;
  size_t size;
  size_t RBufSize;
  uint8_t *pRBufStart;
  uint8_t *pRBufEnd;

  uint8_t *pPush;
  uint8_t *pPop;
  uint8_t *pRead;

  #if defined ESP8266 && !defined FIFOEE_RAM
  uint32_t nextCommit = 0;
  uint32_t commitPeriod;
  bool noEepromBegin = true;
  #endif

  #ifdef FIFOEE_RAM
    uint8_t *buffer;
  #endif


  /**** class member functions ****/

  public:

  #if defined ESP8266
  FIFOEE(uint8_t *aBuffer,size_t aBufSize,uint32_t aCommitPeriod) {
  #else
  FIFOEE(uint8_t *aBuffer,size_t aBufSize) {
  #endif
  /* class constructor
   * aBuffer: FIFO buffer start address, area for control var and ring buffer.
   * aBufSize: buffer size (bytes).
   * aCommitPeriod: (ESP8266 only) real write to EEPROM happens every period.
   */

    #ifdef FIFOEE_RAM
      buffer = (uint8_t *)malloc((int)aBuffer + aBufSize);
    #endif

    // allocate and init control vars
    pBotBlockOffset = aBuffer;
    RBufSize = aBufSize - 1;
    pRBufStart = aBuffer + 1;
    pRBufEnd = pRBufStart + RBufSize;

    #if defined ESP8266 && !defined FIFOEE_RAM
    commitPeriod = aCommitPeriod;
    nextCommit = millis() + commitPeriod;
    #endif

  }


  void format(void) {
  /* format essential metadata of ring buffer, buffer is logically cleared.
   * Fill the ring buffer with a chain of forward linked free blocks with
   * a prefixed data size of DATA_SIZE_MAX and one or two final blocks with
   * a proper size to fill all ring buffer.
   */

    #if defined ESP8266 && !defined FIFOEE_RAM
    if (noEepromBegin) {
      EEPROM.begin((size_t)pRBufStart + RBufSize);
      noEepromBegin = false;
    }
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
    size_t sizeToFill = RBufSize;
    pBlock = pRBufStart;

    while(sizeToFill >= BLOCK_SIZE_MAX) {

      sizeToFill -= BLOCK_SIZE_MAX;

      // adjust for residual space of only one byte: reduce block size by 1
      // to increase residual space to 2 byte, the required minimun.
      size_t dataSize;
      if (sizeToFill == 1) {
	sizeToFill++;
	dataSize = DATA_SIZE_MAX - 1;
      }
      else
	dataSize = DATA_SIZE_MAX;

      EE_WRITE(pBlock,FREE_BLOCK | dataSize - 1);
      pBlock += dataSize + 1;

    }

    // fill residual space with a last block of proper size
    EE_WRITE(pBlock,FREE_BLOCK | sizeToFill - 2);

  }


  int begin(void) {
  /* check if the ring buffer contains a valid data structure. If yes,
   * scan it and gather all relevant data for its management. If not,
   * return an error code.
   */

    #if defined ESP8266 && !defined FIFOEE_RAM
    if (noEepromBegin) {
      EEPROM.begin((size_t)pRBufStart + RBufSize);
      noEepromBegin = false;
    }
    #endif

    // scan the blocks sequence in the ring buffer for changes of status
    // to find the position of pointers pPush, pPop, pRead.
    pBlock = pRBufStart + EE_READ(pBotBlockOffset);
    blockHeader = EE_READ(pBlock);

    // check for valid block status
    blockStatus = blockHeader & BLOCK_STATUS_BITS;
    if (blockStatus == 0x80)
      return INVALID_BLOCK_STATUS;

    // init pointers and control variables for an empty ring buffer
    pPush = pBlock;
    pPop = pBlock;
    pRead = pBlock;

    // scan blocks into ring buffer
    uint8_t oldChangeStatus = blockStatus;
    while (1) {

      // point to next block
      pBlock += (blockHeader & BLOCK_SIZE_BITS) + 2;

      // if end of block chain (reached the ring buffer end) ...
      if (pBlock >= pRBufEnd)
	return SUCCESS;

      // get next block status and check it for validity
      blockHeader = EE_READ(pBlock);
      blockStatus = blockHeader & BLOCK_STATUS_BITS;
      if (blockStatus == 0x80)
        return INVALID_BLOCK_STATUS;

      // go on untill there is a change of status
      if (oldChangeStatus == blockStatus)
	continue;

      // there is a change of status in block sequence, process it.
      uint8_t oldChangeStatusSaved = oldChangeStatus;
      oldChangeStatus = blockStatus;
      switch (oldChangeStatusSaved) {

	// previous blocks have free status: next block is the head of
	// the FIFO queue.
	case FREE_BLOCK:

          pPop = pBlock;
	  if (blockStatus == NEW_BLOCK)
	    pRead = pBlock;
	  continue;

	// previous blocks have read status: if next block has free
	// status, it is the FIFO queue tail, otherwise next block has
	// new status and it is the middle of the FIFO queue. 
	case READ_BLOCK:

	  pRead = pBlock;
	  if (blockStatus == FREE_BLOCK)
	    pPush = pBlock;
	  continue;
 
	// previous blocks have new status: next block must nave a free
	// status and it is the tail of the FIFO queue.
	case NEW_BLOCK:

	  pPush = pBlock;
	  if (blockStatus == READ_BLOCK)
	    return INVALID_BLOCK_STATUS_CHANGE;
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
    blockStatus = blockHeader & BLOCK_STATUS_BITS;
    if (blockStatus != FREE_BLOCK)
      return PUSH_BLOCK_NOT_FREE;
    
    //// allocate required data size. If current pPush block is smaller
    // than the requested size, merge the following free blocks until
    // the requested size + 3 is satisfied or exceeded.
    size_t blockSize = (blockHeader & BLOCK_SIZE_BITS) + 2;

    while (size + 3 > blockSize) {

      pBlock = pPush + blockSize;

      if (pBlock >= pRBufEnd)
	pBlock -= RBufSize;

      blockHeader = EE_READ(pBlock);
      blockStatus = blockHeader & BLOCK_STATUS_BITS;

      if (blockStatus != FREE_BLOCK)
        return FIFO_FULL;

      blockSize += (blockHeader & BLOCK_SIZE_BITS) + 2;

    }

    //// copy given data to eeprom data block
    uint8_t *pData;

    // if the block is splitted ...
    if (pPush + size + 1 > pRBufEnd) {

      //  copy first data part
      for (pData=pPush+1; pData < pRBufEnd; pData++)
        EE_WRITE(pData,*data++);
 
      //  copy second data part
      for (pData=pRBufStart; pData < pPush + size + 1 - RBufSize;  pData++)
        EE_WRITE(pData,*data++);

      // update offset of bottom block
      EE_WRITE(pBotBlockOffset,pData - pRBufStart);

    }

    // if the block is not splitted ...
    else {

      //  copy data
      for (pData=pPush+1; pData < pPush + size + 1; pData++)
        EE_WRITE(pData,*data++);

    }

    // set size and status for copied data block
    EE_WRITE(pPush,NEW_BLOCK | size - 1);

    // update push pointer to next block and bottommost block offset
    // from pRBufStart 
    if (pData < pRBufEnd)
      pPush = pData;
    else {
      pPush = pRBufStart;
      // update offset of bottom block
      EE_WRITE(pBotBlockOffset,0);
    }

    // ran out what remains of the allocated block creating a free block
    EE_WRITE(pPush,FREE_BLOCK | blockSize - size - 3);

    #if defined ESP8266 && !defined FIFOEE_RAM
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
    EE_WRITE(pPop,FREE_BLOCK | *size - 1);

    #if defined ESP8266 && !defined FIFOEE_RAM
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

    // mark block just read as read
    EE_WRITE(pRead,READ_BLOCK | *size - 1);

    #if defined ESP8266 && !defined FIFOEE_RAM
    commitRequest();
    #endif

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
   * return success (0). Also return the size of copied datai. The pBlock
   * pointer is set to point to the next block.
   */

    // check for sufficient data size
    blockSize = (EE_READ(pBlock) & BLOCK_SIZE_BITS) + 2;
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
      for (pData=pRBufStart; pData < pBlock + blockSize - RBufSize; pData++)
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


  #if defined ESP8266 && !defined FIFOEE_RAM
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
    Serial.print("RBufSize:       ");
    Serial.println((int)RBufSize,HEX);
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
