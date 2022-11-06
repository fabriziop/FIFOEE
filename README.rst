======================
FIFOEE Arduino Library
======================

FIFOEE, FIFO of variable size data blocks over EEPROM.

The FIFOEE library allow to build and use a FIFO persistent
EEPROM storage for any kind of data on systems with small resources.
The elements that can be stored are
data blocks with a data size in the range 0-127 byte (unsigned chars).
The size can change dynamically from block to block.
The data blocks can be stored into the FIFO (push method), read out
and deleted from the FIFO (pop method) or simply read out without
deleting them (read method). The block read can be restarted from
the oldest data block at any time and for any number of times.

Particular attention was paid to minimize EEPROM wearing. The internal
structure of the FIFO was designed to distribute evenly the writing for all
the part of EEPROM used by the FIFO.

Since Arduino systems have small EEPROMs (i.e 1KB for AVR, 4KB for NodeMCU),
the metadata overhead for each data block is kept to a minimum of 1 byte.

FIFOEE is designed for use on EEPROM, but it works equally on RAM memory
with the same API. This is very useful during application developement
to avoid an unecessary wear of EEPROM.

Typical applications that can take advantage from FIFOEE are data
logging systems as used for IOT sensors, weather stations.


Features
========

* FIFO stores binary data blocks
* Each block can have a variable data size in the range 0-127 byte
* Allocated EEPROM range from 5 byte (minimum) to the whole available space
* Push, pop (read and delete), read (read and keep) API interface
* Low EEPROM data overhead: 1 byte for each data block plus one byte
* Designed to minimize EEPROM wearing
* Works also with RAM with the same API
* Supports AVR328p, ESP8266, ESP32 processors.
* Tested on arduino nano, nodeMCU 8266, nodeMCU 32S boards.


Quick start
===========

Here is a skeletal structure of an application for an AVR 328p
arduino board (e.g. Arduino nano) using the FIFOEE library.

.. code:: cpp

  ...

  #include <fifoee.h>

  // EEPROM FIFO buffer configuration
  #define BUFFER_START 0x4      // buffer start address
  #define BUFFER_SIZE 128       // buffer size

  // define FIFO buffer (e.g. Arduino nano, AVR 328p microprocessor)
  FIFOEE fifo((uint8_t *)BUFFER_START_ADDR,BUFFER_SIZE);

  // define a data buffer and its size for push and pop
  #define DATA_BUFFER_SIZE_MAX 16
  uint8_t data[DATA_BUFFER_SIZE_MAX];
  size_t dataSize;
  ...
  

  void setup ()
  {

    ...

    // try to begin FIFO buffer, if it fails, format FIFO
    if (fifo.begin())
      fifo.format();

    ...

  }


  void loop()
  {

    ...

    // let suppose that the application wants to store a block of data into
    // the FIFO at this point. Given a uint8_t *data the source of data and
    // the size of this data as size_t dataSize, the statement is ...
    fifo.push(data, dataSize); 

    ...

    // let suppose that the application wants to read, delete and save the
    // oldest data block from the FIFO to the given buffer uint8_t *data with
    // size_t dataSize, the statement is ...  
    fifo.pop(data, &dataSize);
    // pop returns the size of data read from the FIFO into dataSize argument

    ...

  }


Examples
========
 
Two example programs are provided with this library. All of them are tested
on both Arduino nano and NodeMCU (ESP8266) boards.

The "testRingBuffer" example is a deep test for the consistence of the FIFO
internal structure. Since it makes many write cycles, it runs in RAM mode
to avoid an heavy EEPROM wear.

The "upTime" example prints out at power up several information about the
FIFO content and the cumulated up time in the last 3 power up/down cycles.
Every 3 power cycles, the FIFO is formatted. Obviously, this example runs
using the EEPROM to demonstrate the FIFO persistence.

 
Programming options and parameters
==================================

EEPROM/RAM selection
--------------------

By default, the FIFOEE library stores the FIFO into EEPROM. To store the
FIFO into RAM write the following definition at the beginning of the
program source and before the include of the FIFOEE library.

.. code:: cpp

  ...
  #define FIFOEE_RAM
  #include <fifoee.h>
  ...


Debug facility
--------------

The FIFOEE library comes with a couple of optional debug methods that
print out all the internal control variables of the FIFO and the whole
content of the FIFO ring buffer in hexadecimal format. By default these
methods are not included at compile time from the library source.
If they are needed,
write the following definition at the beginning of the
program source and before the include of the FIFOEE library.

.. code:: cpp

  ...
  #define FIFOEE_DEBUG
  #include <fifoee.h>
  ...

Below there is an example of these print outs for a FIFO buffer with size
set to 258 byte and a buffer start address set to F (hex), just after
buffer formatting.

Print out of **dumpControl** method: all FIFO control constans and variables.
::

  pRBufStart:     10
  pRBufEnd:       111
  rBufSize:       101
  BotBlockOffset: 0
  pPush:          10
  pPop:           10
  pRead:          10


Print out of **dumpBuffer** method: the content of the FIFO ring buffer.
::

  10: FF FF 6F 64 61 70 61 73 00 4D 6F 62 69 6C 65 57
  20: 69 46 69 2D 34 33 38 36 31 31 00 00 00 FF 00 00
  30: 00 FF 61 73 70 65 69 6C 75 00 30 31 32 33 00 FF
  40: FF FF 76 6F 64 61 70 73 00 00 4D 6F 62 69 6C 65
  50: 57 69 46 69 2D 00 FF 38 36 31 31 00 00 00 FF 00
  60: 00 00 70 61 73 70 65 69 6C 75 00 30 31 32 33 00
  70: FF FF F7 76 6F 64 61 70 61 73 00 4D 6F 62 69 6C
  80: 65 FF 69 46 69 2D 34 00 FF 36 31 31 00 00 00 FF
  90: FF FF 00 00 FF 73 00 FF FF 6C 75 00 30 31 32 33
  A0: 00 FF FF FF 76 6F 64 61 70 61 73 00 4D 6F 62 69
  B0: 6C FF 57 69 46 69 2D 34 33 38 36 31 31 00 00 00
  C0: FF 00 00 00 70 61 73 70 65 69 6C 75 00 30 31 32
  D0: 33 00 FF FF FF 56 FF 64 61 66 6F 6E 65 4D 6F 62
  E0: 69 6C 65 57 69 46 69 2D 34 33 38 36 31 31 00 00
  F0: 00 FF 00 00 00 00 00 63 00 35 36 37 38 39 30 31
  100: 00 80 FF 00 FF FF FF FF FF FF FF FF FF FF FF FF
  110: 80


EEPROM buffer sizing
--------------------

The four main factors influencing the choice of EEPROM buffer size are:

  1. data writing period
  2. data size of each written block
  3. duration of data storage before overwrite by new coming data
  4. EEPROM wearing

All the parameter above comes from program specifications, but generally,
the wanted result is to have a guaranteed minimum duration of data
storage. This duration is the time taken by a sequence of push operations
to run out the FIFO ring buffer, in the absence of pop operations.
To determine this parameter, the formula below can be used
::

                                         buffer_size - 2
  storage_duration_in_hours = -----------------------------------------
                               (block_data_size + 1) * writes_per_hour

If the data size of each block is variable, a mean value can be used.

Another fundamental aspect is the EEPROM wearing. Since, this kind of
memory is generally rated for about 100,000 erase cycles, it comes
straightforward to compute the EEPROM life using the result of the
above formula as follows
::

  EEPROM_life_in_hours = 100,000 * storage_duration_in_hours

A last limit to these factors is imposed by the EEPROM memory sizes that are
1KB for Arduino nano and 4KB for NodeMCU.


ESP8266 and ESP32 commit parameter
----------------------------------

NodeMCU boards with ESP8266 or ESP32 microprocessor have no EEPROM.
The functionality of such memory is emulated using the flash memory.
In this process, since the flash memory is significantly slower than
an EEPROM, the data is first read and written from/to a cache buffer
into RAM memory and then stored really into the flash memory only
upon request by calling the **commit** method.

To control the frequency of data committing into flash memory, FIFOEE allows
to set a **commitPeriod** argument that specifies the minimum time
period between two consecutive commits. **commitPeriod** is expressed in 
milliseconds. A zero value disables committing.


ESP8266 and ESP32 caveat
========================

Since ESP8266 and ESP32 processor boards simulate EEPROM using a RAM
buffer and FLASH EPROM, they needs same sort of begin call before
reading or writing the EEPROM. This code is embedded into the **begin**
and **format** methods of FIFOEE. This means that one of these methods
must be called before any other FIFOEE method.

Moreover, if multiple instances of FIFOEE are used and/or if other program
parts needs their own EEPROM buffer, an explicit EEPROM begin call must
be put in the application before any access to the EEPROM. This must be
done defining the symbol **EEPROM_PROGRAM_BEGIN** before any include
involving the EEPROM and with the call
**EEPROM.begin(<required_eeprom_size>)** in the arduino **setup**
function.

AVR processor boards have a true EEPROM, so they do not need any EEPROM
begin and multiple instances of FIFOEE and/or other program parts using
EEPROM do not need any special provision to coexist in the same program.


Module reference
================

The FIFOEE library is implemented as a single C++ class. A FIFOEE object needs
to be instantiated with the proper parameters to manage the write/read
operations in the FIFO buffer.


Objects and methods
-------------------

**FIFOEE**

  This class embeds all FIFOEE object status info.


FIFOEE **FIFOEE** (uint8_t * **buffer**, size_t **bufSize**);

  The class constructor for AVR 328 microprocessor boards.

  **buffer**: start address of FIFO buffer.

  **bufSize**: FIFO buffer size (byte).

  Returns a **FIFOEE** object.


FIFOEE **FIFOEE** (uint8_t * **buffer**, size_t **bufSize**,
  uint32_t **commitPeriod**);

  The class constructor for ESP8266 microprocessor boards.

  **buffer** and **bufSize**: the same as above.

  **commitPeriod**: minimum period (ms) between two consecutive commits.
  If zero, disables committing.

  Returns a **FIFOEE** object.


int **format** (void);

  Initialize the essential metadata of the FIFO buffer. The FIFO is initialized
  as completely empty. Format is required to be run at least one time before
  the first call to push/pop/read. Can be called to clear the whole circular
  buffer.
 
  Returns the following **error** codes;

    **FIFOEE::SUCCESS** : FIFO is correctly formatted.

    **FIFOEE::INVALID_BUFFER_SIZE** : given FIFO buffer size too small (<5).

 
int **begin** (void);

  Analyze the FIFO content and restore the proper status and values of the
  FIFO control variables. To be called at power up before any other FIFO
  operation. *WARNING* this methods resets the read pointer, so block
  reading restarts from the older used block.
  
  Returns the following **error** codes;

    **FIFOEE::SUCCESS** : FIFO contains valid data. Note: also an empty FIFO
    is considered valid.

    **FIFOEE::INVALID_BLOCK_STATUS** : FIFO has not valid data, probably it
    is not formatted or may be corrupted.
  
 
int **push** (uint8_t * **data**, size_t **dataSize**);

  Push queues **data** at the FIFO queue tail.
  
    **data**: start address of data to be queued into the FIFO.
    
    **dataSize**: size of **data** in byte.
    
  Returns the following **error** codes;

    **FIFOEE::SUCCESS**: the data is successfully queued to the FIFO.

    **FIFOEE::FIFO_FULL**: data queuing failed, the FIFO has no enough
    room for pushing data.

    **FIFOEE::PUSH_BLOCK_NOT_FREE**: internal error, corrupted FIFO or
    unformatted FIFO.
 

int **pop** (uint8_t * **data**, size_t * **dataSize**);

  Pop out the data block at the head of the FIFO queue. The data from the FIFO
  is copied into **data** buffer. The size of copied data is stored into
  **dataSize**. The FIFO data block just copied is marked as free space, so
  the block is logically deleted and can be overwritten by a **push** .
 
    **data**: data buffer where to copy the popped out FIFO data.
  
    **dataSize**: a pointer to the size of data buffer in byte.

  Returns the following

    **dataSize**: the size in byte of the data block popped out from the FIFO.

  Returns the following **error** codes;

    **FIFOEE::SUCCESS**: the data is successfully popped out from the FIFO.

    **FIFOEE::FIFO_EMPTY**: no data into FIFO to pop out:

    **FIFOEE::DATA_BUFFER_SMALL**: the size of the data to be popped out
    is greater then the size of **data**, the given destination buffer.
 

int **read** (uint8_t * **data**, size_t * **dataSize**);

  The same functionality as **pop**, but the block read is not logically
  deleted and it is maked as read. The first read starts at the FIFO queue
  head. The following calls to **read** read the blocks in the order from
  the FIFO queue head toward the tail. If **pop** calls are faster then
  **read** calls, the next **read** will start again from the FIFO queue
  head.

  **read** has the same arguments and return values of **pop** method.


void **restartRead** (void);

  Set the next read pointing to the FIFO queue head. This allows to
  start again **read** from the oldest data or to read again data that was
  already read.


Installing
==========

By arduino IDE library manager or by unzipping FIFOEE.zip into
arduino libraries.


Contributing
============

Send wishes, comments, patches, etc. to mxgbot_a_t_gmail.com .

FIFOEE internals can be found at `Developer information`__ .

__ DEVINFO_


Copyright
=========

FIFOEE library is authored by Fabrizio Pollastri <mxgbot_a_t_gmail.com>,
years 2021-2022, under the GNU Lesser General Public License version 3.


.. _DEVINFO: doc/developer.rst

.. ==== END ====
