======================
FIFOEE Arduino Library
======================

FIFOEE, FIFO of variable size data blocks over EEPROM.

The FIFOEE library allow to build and use a FIFO persistent
EEPROM storage for any kind of data on systems with small resources.
The elements that can be stored are
data blocks with a size in the range 1-64 byte (unsigned chars).
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
* Each block can have a variable size in the range 1-64 byte
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
set to 258 byte and a buffer start address set to 10 (hex), just after
buffer formatting.

Print out of **dumpControl** method: all FIFO control constans and variables.
::

  pRBufStart:     11
  pRBufEnd:       112
  RBufSize:       101
  BotBlockOffset: 0
  pPush:          11
  pPop:           11
  pRead:          11


Print out of **dumpBuffer** method: the content of the FIFO ring buffer.
::

  11: FF FF 69 AD ED FE 8A 6D 3F 3E F6 FE 2A DE 97 CF
  21: CB DD CF 73 E7 DD F7 7D BF BB B2 BC 3F F4 F1 D5
  31: A8 F3 3F AF EB EF BF EA 01 1A F7 FF 5E 2E F3 E7
  41: C0 F3 EB FB 7D 0C EF DD A7 6F 37 A7 F9 B7 37 D5
  51: FE FF A7 7E 19 ED F7 7E 0D EF EC 5F EB B1 E8 AD
  61: 36 E7 5F F7 AD F7 EF BF 8A FB DE FF FF D7 53 F1
  71: 3E 4E FB FB CD C6 6F 24 AD 39 7D FD 9A E3 7D F7
  81: DF 5F CF FF BF 25 AD BB DE D7 FA D6 77 57 AF 7A
  91: CB 6B FF D4 FA E6 38 BF 21 F3 FB 57 DE DA 2F CF
  A1: BE F8 F6 8E E1 07 FF E7 8B ED EF ED DE EF 17 BD
  B1: D5 F6 2B B0 ED 37 74 56 7B B5 F8 DE 35 FB FC DF
  C1: C2 69 5A 2B BA 9D 68 E8 F7 ED C7 DD CE E5 3B CE
  D1: AD D3 FF FC F3 F2 5F FE 6D BF 4F 67 F4 DB 87 BD
  E1: 67 DE 5D 8A FD F4 E7 5C 39 F3 CE C7 58 DA B1 04
  F1: 79 FC 7F BD 7D FB F4 6C 31 FF 99 56 9D DB BE F5
  101: D7 96 DD 16 6E F7 BF B6 63 BB B4 78 FF FE EE 7E
  111: BE


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

                                         buffer_size - 3
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


ESP8266 commit parameter
------------------------

NodeMCU boards with ESP8266 microprocessor have no EEPROM. The functionality
of such memory is emulated using the flash memory. In this process, since
the flash memory is significantly slower than an EEPROM, the data is
first read and written from/to a cache buffer into RAM memory and then 
stored really into the flash memory only upon request by calling the
**commit** method.

To control the frequency of data committing into flash memory, FIFOEE allows
to set a **commitPeriod** argument that specifies the minimum time
period between two consecutive commits. **commitPeriod** is expressed in 
milliseconds. A zero value disables committing.


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


void **format** (void);

  Initialize the essential metadata of the FIFO buffer. The FIFO is initialized
  as completely empty. Format is required to be run at least one time before
  the first call to push/pop/read. Can be called to clear the whole circular
  buffer.

 
int **begin** (void);

  Analyze the FIFO content and restore the proper status and values of the
  FIFO control variables. To be called at power up before any other FIFO
  operation.
  
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
