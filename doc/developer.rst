============================
FIFOEE Developer information
============================

Information about FIFOEE internals, useful for FIFOEE developement.


FIFO structure
==============

The FIFO internal structure is designed to accomodate a sequence of 
ordered data blocks, each of one with a different data size in the
range 0-127 byte. The sequence is realized as a forward linked list
of blocks and managed for insertions and deletions as a ring buffer.
FIFOEE mantains three pointers to this list:

  - pPush, pointer to the first free block, the FIFO queue tail.
  - pPop, pointer to the older block, the FIFO queue head.
  - pRead, pointer to next block to read, always between pPush and pPop.

Each pointer tells to the homonymous FIFO method which block to read or
write. These pointers change at each FIFO operation, for this reason
they are stored into RAM, not into EEPROM, so they are lost at power down.

The pointers recovery at power up is performed by the **begin** method.
The method scans the whole FIFO, examines each block status to detect
the queue head, tail and the read positions.

The block content can be any kind of binary data and the blocks,
in general, are not aligned to the start of the FIFO ring buffer,
for these reasons there is no
FIFO content that can be used by **begin** to establish a start point
for its FIFO scan. This problem is solved by storing into EEPROM the
variable **botBlockOffset** that tells to **begin** the offset in byte
between the start of the FIFO ring buffer and the start of the first
block just above the start of the FIFO ring buffer. **botBlockOffset**,
unlike pPush
pPop pRead pointers, changes more slowly, with the same frequency of
a full round of the ring buffer. So, it is not a problem to store it
into EEPROM, just before the FIFO ring buffer.

Fig. 1 shows a byte breakdown of the whole structure of the FIFO.
::

 byte   0          1     2     3     4          N-4   N-3   N-2   N-1
 +--------------+-----+-----+-----+-----+-~ ~-+-----+-----+-----+-----+
 |botBlockOffset| rb0 | rb1 | rb2 | rb3 |.~ ~.|rbN-5|rbN-4|rbN-3|rbN-2|
 +--------------+-----+-----+-----+-----+-~ ~-+-----+-----+-----+-----+
 |   variable   |<---- FIFO ring buffer --~ ~------------------------>|
 |<------------------------ FIFO buffer --~ ~------------------------>|



Block structure
---------------

Fig. 2 shows a byte breakdown of the structure of the blocks stored by
the FIFO.
::

 byte 0      1       2       3     4      N-3     N-2    N-1
 +-------+-------+-------+-------+-~ ~-+-------+------+-------+
 | header| data0 | data1 | data2 |.~ ~.|dataN-4|data-3|dataN-2|
 +-------+-------+-------+-------+-~ ~-+-------+------+-------+
         |<---- data payload ------~ ~----------------------->|

The block starts with a one byte header followed by the user data bytes.
To reduce the header overhead, all block status and list linking
information is crammed into a single byte.

Fig. 3 show a bit breakdown of a block header.
::

  bit 7    6     5     4     3     2     1     0
  +-----+-----+-----+-----+-----+-----+-----+-----+
  |stat |                data size                |
  +-----+-----+-----+-----+-----+-----+-----+-----+
     |
     |\_ 1: free block, a block available for storing data.
      \_ 0: used block, a block with new data, not popped out. 

The 7 bits of the data size field allows to specify a size range from
0 to 127 byte.

The pointer to the next block is computed as the current block header
address plus the current block data size plus 1.

This pointer chains all blocks, both free and used, in a single forward
linked list that fills completely the ring buffer of the FIFO.

If the next block address goes beyond the end of the FIFO ring buffer, it is
wrapped from the start of the ring buffer with the formula
::

  next_block_address = next_block_address_beyond - ring_buffer_size

The blocks that do not fit completely at FIFO ring buffer end are splitted
into two parts: the first fits exactly the free space at ring buffer end,
the second completes all the remaining data at ring buffer start.

Block allocation
----------------

The most tricky part of FIFOEE is the block memory allocation required
by push operation. In fact, pop and read operations do not modify blocks
memory allocation, they change only the block status.

FIFOEE implements a block split and merge algorithm for the proper memory
allocation of new data blocks.

When the **push** method is called, the push pointer points to the
first available free block. If the size of this block is greater
than the size of the data to be pushed, this block is splitted, the
first part becomes a new block with the new data, the residual second
part becomes a new free block. Otherwise, if the size of the free block
is smaller than the size of the data to be pushed, the free block is
merged with the following free block. This process is repeated until
a free block big enough is created. This new free block is splitted
into two parts and used as explained above.

The split and merge algorithm takes care to preserve at least one
free block in the FIFO, even with a minimum size (1 byte).
This free block or a sequence of free blocks is taken as the separation
mark between the FIFO queue head and tail.


Copyright
---------

FIFOEE is authored by Fabrizio Pollastri <mxgbot_a_t_gmail.com>,
years 2021-2022, under the GNU Lesser General Public License version 3.

.. ==== END ====
