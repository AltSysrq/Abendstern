/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/config/libconfig.cxx
 */

/*
 * libconfig.cxx
 *
 *  Created on: 01.08.2011
 *      Author: jason
 */

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cerrno>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>
#include <queue>

#include "libconfig.hxx"

#define EXIT_THE_SKY_IS_FALLING 255

using namespace std;

/* GENERAL DESIGN
 * First of, why we don't use memory-mapped files:
 * 1. The mechanism is different on different platforms
 * 2. Not necessarily supported on all platforms
 * 3. It generally does not work as expected, especially on Linux.
 *    On Linux, namely, it is impossible to map a file larger than
 *    RAM (but not than address space) and have it guaranteed to work
 *    because the kernel seems to think it needs to allocate that much
 *    cache space for it.
 * Since the swap-file is used for lesser-used settings, the performance
 * hit of using normal FILE*s should not be big.
 *
 * Each Setting* encodes data as follows:
 * Bits 0--2 encode the type; bits 3--15 encode the lower index, and the rest
 * are the upper index plus one.
 *
 * The indices are used to locate the actual data for the setting.
 * The upper index is a table of pointers to lower tables; bit 0 of
 * each pointer is zero to indicate an in-RAM pointer and 1 to indicate
 * an in-file offset. This type of pointer will be referred to as a
 * SwapPointer.
 * The lower index is an array of pointer-sized data. The interpretation of
 * each datum depends on the type of the setting:
 * + TypeBoolean: Contains a pointer-sized bool
 * + TypeInt: Contains union { unsigned; signed; };
 * + TypeInt64:
 *   When sizeof(iptr)==sizeof(long long),
 *     union { unsigned long long; signed long long };
 *   Otherwise, a SwapPointer to the long long.
 * + TypeFloat: Contains the float
 * + TypeString: SwapPointer to string data
 * + TypeList, TypeArray, TypeGroup: SwapPointer to data
 *
 * The lower index also tracks free slots. Any unused Setting slot contains
 * a Setting* (with undefined type) pointing to the next free lower index
 * slot, or NULL for no more free. The first free Setting is tracked in-RAM.
 *
 * A secondary upper/lower index pair exists for tracking parenthood; the
 * lower index in this case holds Setting*s.
 *
 * All in-memory out-of-index structures are subtypes of the Swappable
 * structure. This structure tracks the size of the contained data (including
 * the structure itself), the one SwapPointer or Setting* that tracks it (there
 * may never be more than one), as well as next/previous links for a
 * most-recently-used list. Whenever a Swappable is accessed, it is unlinked
 * from this list and reinserted to the beginning. When the total memory used
 * exceeds the requested amount and a new Swappable is to be allocated,
 * Swappables from the tail of this list are moved to swap, their SwapPointer
 * updated, and then deallocated.
 *
 * In order to prevent a lower index to be swapped out before a Setting it
 * contains, each lower index object is retraversed after an access to a Setting
 * so it will be earlier in the recently-used list.
 *
 * In-file structures are stored naked in whatever format is necessary; since
 * the type is already known when they must be loaded back, the appropriate
 * decoder can run which can deallocate appropriately.
 *
 * Space in the file is allocated in 32-byte blocks, enough for four pointers.
 * Block 0 is never used, so that a NULL pointer is never valid. Free blocks
 * begin with a pointer to the next free block, or 0 for end. The address of
 * the first free block is held in RAM.
 * The following steps are taken to allocate a block:
 * 1. If the head pointer to free segments is NULL, append new segments to the
 *    file and adjust head accordingly
 * 2. Use the block the head points to as the return value
 * 3. Copy the block's next into the head
 * To deallocate a block:
 * 1. Copy head into the block's next
 * 2. Set head to the block
 *
 * Since file offsets are signed integers in cstdio, and since the logical
 * offsets are multiplied by block size over two, data is split into up to 16
 * files, each limited in size to (1<<(8*sizeof(long)-1))-BLOCK_SZ bytes.
 */

namespace libconfig {
  #define TYPE_BITS 3
  #define TYPE_OFF 0
  #define TYPE_MASK (((1<<TYPE_BITS)-1) << TYPE_OFF)
  #define LOWER_INDEX_SZ 8192
  #define LOWER_INDEX_BITS 13
  #define LOWER_INDEX_OFF 3
  #define LOWER_INDEX_MASK (((1<<LOWER_INDEX_BITS)-1) << LOWER_INDEX_OFF)
  #define UPPER_INDEX_OFF 16
  #define NUM_TYPES 8
  #define BLOCK_BITS 5
  #define BLOCK_SZ (1<<BLOCK_BITS)


  //Shortcut for reinterpret_cast<iptr>(x)
  #define RS(x) reinterpret_cast<iptr>(x)
  //Shortcut for reinterpret_cast<Setting*>(x)
  #define SR(x) reinterpret_cast<Setting*>(x)

  //Evaluate Or Die
  //Evaluates the given condition; if it is false, print
  //a diagnosis and exit
  static inline void eod(bool cond, const char* msg) {
    if (!cond) perror(msg), exit(254);
  }

  //Returns whether the given character is an identifier character
  static inline bool isIDChar(char ch) {
    return (ch >= '0' && ch <= '9')
        || (ch >= 'a' && ch <= 'z')
        || (ch >= 'A' && ch <= 'Z')
        || ch == '-'
        || ch == '_';
  }
  //Returns whether the given character is a "whitespace" character
  //For compatibility with the original libconfig, the characters :=,;
  //are included. , is not included if the second argument is false.
  static inline bool isWhite(char ch, bool includeComma = true) {
    return ch == ' '
        || ch == '\t'
        || ch == '\n'
        || ch == '\r'
        || ch == '\f'
        || ch == '='
        || ch == ':'
        || (ch == ',' && includeComma)
        || ch == ';'
        || ch == '/'
        || ch == '#';
  }

  struct Swappable;
  union SwapPointer {
    Swappable* ptr;//In-memory case, bit 0 is 0
    iptr dsk; //In-file case, bit 0 is 1
  };

  template<unsigned ptrsz> struct SettingEncoder_tmpl {
    char pointer_size_not_supported[((signed)ptrsz)-0xDEAD];
  };
  template<>
  struct SettingEncoder_tmpl<4> {
    static Setting::Type type(iptr i) {
      return (Setting::Type)((i & TYPE_MASK) >> TYPE_OFF);

      //This code is dead, but will still stop compilation if
      //assumptions this encoder makes are invalid
      switch(true) {
        case (sizeof(void*) != 4):
        case (sizeof(float)==sizeof(unsigned)
           && sizeof(unsigned)==sizeof(iptr)):;
      }
    }

    union LowerIndex {
      bool b;
      unsigned ui;
      signed si;
      float f;
      SwapPointer ptr;
      Setting* nxt; //next free setting
      Setting* parent; //For parental lower index
      iptr ipt;
      //Dummy items
      int ul;
      int sl;
    };

    static bool canEmbedInt64() { return false; }

    static void indices(iptr i, unsigned& uix, unsigned& lix) {
      uix = i >> UPPER_INDEX_OFF;
      --uix;
      lix = (i & LOWER_INDEX_MASK) >> LOWER_INDEX_OFF;
    }

    static void indices(const Setting* s, unsigned& uix, unsigned& lix) {
      indices(RS(s), uix, lix);
    }

    static iptr create(Setting::Type t, unsigned u, unsigned l) {
      iptr i = (iptr)t;
      i |= l << LOWER_INDEX_OFF;
      i |= ((u+1) << UPPER_INDEX_OFF);
      return i;
    }
  };
  template<>
  struct SettingEncoder_tmpl<8> {
    static Setting::Type type(iptr i) {
      return (Setting::Type)((i & TYPE_MASK) >> TYPE_OFF);

      //This code is dead, but will still cause a compilation error
      //if any assumption we make is invalid
      switch(true) {
        case (sizeof(void*)!=8):
        case (sizeof(float)==sizeof(unsigned)
              && sizeof(unsigned) < sizeof(iptr)):;
      }
    }

    union LowerIndex {
      bool b;
      unsigned ui;
      signed si;
      float f;
      unsigned long long ul;
      signed long long sl;
      SwapPointer ptr;
      Setting* nxt; //next free setting
      Setting* parent; //For parental lower index
      iptr ipt;
    };

    static bool canEmbedInt64() { return true; }

    static void indices(iptr i, unsigned& uix, unsigned& lix) {
      uix = i >> UPPER_INDEX_OFF;
      --uix;
      lix = (i & LOWER_INDEX_MASK) >> LOWER_INDEX_OFF;
    }

    static void indices(const Setting* s, unsigned& uix, unsigned& lix) {
      indices(RS(s), uix, lix);
    }

    static iptr create(Setting::Type t, unsigned u, unsigned l) {
      iptr i = (iptr)t;
      i |= l << LOWER_INDEX_OFF;
      i |= ((u+1) << UPPER_INDEX_OFF);
      return i;
    }
  };
  typedef SettingEncoder_tmpl<sizeof(void*)> SettingEncoder;

  extern struct Swappable ruhead, rutail; //Predeclare so we can use below
  struct Swappable {
    SwapPointer* parent; //Always-in-memory pointer, bit 0 is 0
    Swappable* nxt,* prv;
    unsigned size;
    //Swapout is responsible for allocating and writing the
    //structure to file, and deallocating everything other than
    //the Swappable itself.
    //It is to return the index within the file that the structure
    //is rooted in.
    iptr (*swapout)(Swappable*);
  } ruhead = { NULL, &rutail, NULL },
    rutail = { NULL, NULL, &ruhead }; //Recently-used list

  static iptr primaryUsed=0, maxPrimary=1024*1024;
  //Number of blocks (block 0 is always "used")
  static iptr secondaryUsed=1, secondarySize=1;
  static FILE* swapfile[16] = {0};
  //Index of next free block in the swapfile, or zero for none
  static iptr nextFreeBlock=0;
  //Location of next free Setting, or NULL for none
  static Setting* nextFreeSetting=0;
  /* The upper index.
   * This has an extra pointer so that any SwapPointer* will survive
   * resizing of the vector.
   */
  static vector<SwapPointer*> upperIndex, parentUpperIndex;

  static queue<Setting*> toDelete;

  /* Set to false when swap files are closed.
   * If false, the Config destructor will do nothing (and it is
   * unsafe for the library to do anything else).
   */
  static bool enableConfigDestructor = true;

  /* BEGIN: Structs for on-disk */
  /* Free block; contains pointer to next free block */
  union BFree { iptr nxt; char pad[BLOCK_SZ]; };
  /* Int64 storage */
  union BInt64 { unsigned long long val; char pad[BLOCK_SZ]; };
  /* String storage. (this is just a union to ensure it is packed)
   * dat, starting at index sizeof(iptr) contains actual characters
   * (up to 24 or 28), up to a term NUL or the end of the array.
   * If nxt is non-zero, it contains another BString which is appended
   * to this one.
   */
  union BString { iptr nxt; unsigned char dat[BLOCK_SZ]; };
  /* Lower index storage, part 2
   * Contains three data (0..2) and a pointer to the next BLindex, or 0
   * for none.
   */
  union BLIndex { char pad[BLOCK_SZ]; iptr dat[4]; };
  /* Group storage.
   * nxt: Next group item, or NULL for end
   * name: BString containing full name
   * value: Setting* which is the target
   * dat[0]: Hash of string
   * dat[1]: Length of string
   */
  union BGroup {
    char pad[BLOCK_SZ];
    struct { iptr nxt, name; Setting* value; unsigned dat[2]; };
  };
  /* Array and list storage.
   * nxt: Next block of the composite, or 0 for end
   * dat[0..n-1]: Contents (NULL for end of contents)
   */
  struct BArray { iptr nxt; Setting* dat[BLOCK_SZ/sizeof(iptr) - 1]; };

  //Make sure they're all the right size
  //(inline so the compiler doesn't complain about this not used)
  static inline void btestAssertions() {
    switch(true) {
      case false:
      case (sizeof(BFree)==BLOCK_SZ):;
    }
    switch(true) {
      case false:
      case (sizeof(BInt64)==BLOCK_SZ):;
    }
    switch(true) {
      case false:
      case (sizeof(BString)==BLOCK_SZ):;
    }
    switch(true) {
      case false:
      case (sizeof(BLIndex)==BLOCK_SZ):;
    }
    switch(true) {
      case false:
      case (sizeof(BGroup)==BLOCK_SZ):;
    }
    switch(true) {
      case false:
      case (sizeof(BArray)==BLOCK_SZ):;
    }
  }

  /* END: Structures for on-disk */

  /* BEGIN: Structures for in-RAM */
  /* Lower index storage
   * Size: sizeof(RLIndex)
   */
  struct RLIndex: public Swappable {
    SettingEncoder::LowerIndex dat[LOWER_INDEX_SZ];
  };
  /* 64-bit integer storage.
   * Size: sizeof(RInt64)
   */
  struct RInt64: public Swappable {
    union { unsigned long long ul; signed long long sl; };
  };
  /* String storage.
   * Size: sizeof(RString) + strlen(str) + 1
   */
  struct RString: public Swappable {
    const char* str;
  };
  /* List and array storage.
   * Size: sizeof(RArray) + length*sizeof(Setting*)
   */
  struct RArray: public Swappable {
    Setting** dat;
    unsigned length, capacity;
  };
  /* Group storage.
   * Size: sizeof(RGroup) + entriesCap*sizeof(OrderedEntry) +
   *       tableSize*sizeof(HashEntry) +
   *       sigma(i, numEntries, strlen(entries[i].name)) +
   *       sigma(i, tableSize, numLinks(table[i].nxt))
   */
  struct RGroup: public Swappable {
    struct OrderedEntry {
      const char* name;
      unsigned hash;
      Setting* dat;
    }* entries;
    unsigned numEntries, entriesCap;
    struct HashEntry {
      const char* name;
      Setting* dat;
      HashEntry* nxt; //For duplicate hashing
    }* table;
    //The table size MUST always be a power of two
    //This allows the hashtable functions to assume that
    //  x & (tableSize-1)
    //is equivalent to
    //  x % tableSize
    //which will give a meaningful speed boost in some situations
    //without meaningfully complicating the code
    unsigned tableSize;
  };
  /* END: Structures for in-RAM */

  const char* openSwapFile() {
    for (unsigned i=0; i<sizeof(swapfile)/sizeof(swapfile[0]); ++i) {
      //tmpfile() sometimes has issues on WIN32 (not sure why/where);
      //just use the debugging files.
      #if !defined(DEBUG) && !defined(WIN32)
      swapfile[i] = tmpfile();
      #else /* DEBUG */
      //In debug mode, use deterministic filenames in the current dir
      //These will not be deleted
      char filename[32];
      sprintf(filename, "swapfile.%d", i);
      swapfile[i] = fopen(filename, "wb+");
      #endif /* DEBUG */
      if (!swapfile[i]) {
        const char* ret = strerror(errno);
        for (unsigned j=0; j<i; ++j) {
          fclose(swapfile[j]);
          swapfile[j] = NULL;
        }
        return ret;
      }
    }
    char zero[BLOCK_SZ] = {0};
    //Force the zeroth block to be unused
    eod(fwrite(zero, BLOCK_SZ, 1, swapfile[0]), "fwrite");
    return NULL;
  }

  static void bread(iptr,void*);
  static void bwrite(iptr,const void*);
  void destroySwapFile() {
    //For debugging
    #if 0
    BFree blk;
    char fill[BLOCK_SZ];
    memset(fill, 0xFF, sizeof(fill));
    for (iptr i = nextFreeBlock; i; i = blk.nxt) {
      bread(i,&blk);
      bwrite(i,fill);
    }
    #endif /* disabled */
    enableConfigDestructor=false;
    for (unsigned i=0; i<sizeof(swapfile)/sizeof(swapfile[0]); ++i)
      if (swapfile[i]) fclose(swapfile[i]);
  }

  /* Writes the given data to the given block. The data are assumed to
   * be exactly BLOCK_SZ bytes long; the block number must be even.
   *
   * If the index points to the end of the current file, it is transparently
   * expanded (data is written to the end).
   *
   * Calls exit(254) if the write fails.
   */
  static void bwrite(iptr ix, const void* dat) {
    ix >>= 1; //Drop redundant bit 0
    unsigned fix = (ix >> (sizeof(long)*8-1-BLOCK_BITS));
    long off = ix << BLOCK_BITS;
    off &= ~(1L << (sizeof(long)*8 - 1)); //Clear the sign bit
    eod(!fseek(swapfile[fix], off, SEEK_SET),"fseek");
    eod(1 == fwrite(dat, BLOCK_SZ, 1, swapfile[fix]),"fwrite");

    if (ix == secondarySize) ++secondarySize;
  }

  /* Reads the given data from the given block. The data are assumed
   * to be exactly BLOCK_SZ bytes long; the block number must be even.
   *
   * Calls exit(254) if the read fails.
   */
  static void bread(iptr ix, void* dat) {
    ix >>= 1; //Drop redundant bit 0
    unsigned fix = (ix >> (sizeof(long)*8-1-BLOCK_BITS));
    long off = ix << BLOCK_BITS;
    off &= ~(1L << (sizeof(long)*8-1)); //Clear the sign bit
    eod(!fseek(swapfile[fix], off, SEEK_SET), "fseek");
    eod(1 == fread(dat, BLOCK_SZ, 1, swapfile[fix]), "fread");
  }

  /* Extends the last file with 64k blocks and adds them to the
   * free list.
   */
  static void bextend() {
    BFree blk;
    iptr base = secondarySize*2;
    nextFreeBlock = base;
    for (iptr i=0; i<65536; ++i) {
      if (i < 65535) blk.nxt = base + i*2 + 2;
      else           blk.nxt = 0;
      bwrite(base+i*2, &blk);
    }
  }

  /* Allocates a new block and returns its index. */
  static iptr balloc() {
    if (!nextFreeBlock) bextend();
    iptr ret = nextFreeBlock;
    BFree blk;
    bread(ret, &blk);
    nextFreeBlock = blk.nxt;
    ++secondaryUsed;
    return ret;
  }

  /* Frees a block. */
  static void bfree(iptr i) {
    BFree blk;
    blk.nxt = nextFreeBlock;
    nextFreeBlock=i;
    bwrite(i, &blk);
    --secondaryUsed;
  }

  //This will be used quite frequently below, so put it above so the compiler
  //can inline it
  Setting::Type Setting::getType() const {
    return SettingEncoder::type(RS(this));
  }

  /* BEGIN: Data management functions.
   * There are four for each type:
   * + ramfree  Frees the subdata of a Swappable*.
   *            This recurses to other Setting*s contained.
   *            Does not actually free the Swappable.
   * + diskfree Frees all on-disk data of a block structure.
   *            This recurses to other Setting*s contained
   * + swapout  Allocates space on disk and copies a Swappable there,
   *            then frees strongly-associated data (ie, not Settings)
   * + swapin   Allocates a Swappable and initialises it with data from
   *            disk, then frees strongly-ossociated data (ie, not Settings)
   */
  static void ramfree_int64(Swappable* swp) { }
  static void diskfree_int64(iptr i) {
    bfree(i);
  }
  static iptr swapout_int64(Swappable* swp) {
    BInt64 blk;
    iptr i = balloc();
    blk.val = ((RInt64*)swp)->ul;
    bwrite(i, &blk);
    ramfree_int64(swp);
    return i;
  }
  static Swappable* swapin_int64(iptr i) {
    BInt64 blk;
    RInt64* r = new RInt64;
    r->size = sizeof(RInt64);
    r->swapout = swapout_int64;
    bread(i, &blk);
    r->ul = blk.val;
    diskfree_int64(i);
    return r;
  }

  static void ramfree_string(Swappable* swp) {
    delete[] ((RString*)swp)->str;
  }
  static void diskfree_string(iptr i) {
    BString blk;
    do {
      bread(i, &blk);
      bfree(i);
      i = blk.nxt;
    } while (i);
  };
  static iptr swapout_string(Swappable* swp) {
    const char* str = ((RString*)swp)->str;
    BString blk;
    iptr base = balloc();
    iptr curr = base;
    do {
      for (unsigned i=sizeof(iptr); i<BLOCK_SZ; ++i, ++str) {
        blk.dat[i] = *str;
        //End of string (after the copy since we do want to copy the NUL)
        if (!*str) break;
      }

      blk.nxt = (*str? balloc() : 0);
      bwrite(curr, &blk);
      curr = blk.nxt;
    } while (curr);
    ramfree_string(swp);

    return base;
  }
  static Swappable* swapin_string(iptr base) {
    BString blk;
    RString* r = new RString;
    //Find the length to allocate
    unsigned length = 0;
    for (iptr i = base; i; i = blk.nxt) {
      bread(i, &blk);
      if (blk.nxt) length += BLOCK_SZ-sizeof(iptr);
      //strlen won't work for this case, since the block might not be
      //NUL-terminated
      else for (unsigned j=sizeof(iptr); j<BLOCK_SZ && blk.dat[j]; ++j)
        ++length;
      ++length; //terminating NUL
    }

    r->size=sizeof(RString)+length;
    r->swapout = swapout_string;
    char* dst = new char[length];
    r->str = dst;

    //Copy in and free simultaneously
    bread(base, &blk);
    for (iptr i = base; i; i = blk.nxt) {
      bread(i, &blk);
      if (blk.nxt) {
        memcpy(dst, blk.dat+sizeof(iptr), BLOCK_SZ-sizeof(iptr));
        dst += BLOCK_SZ-sizeof(iptr);
      } else {
        for (unsigned j=sizeof(iptr); j<BLOCK_SZ && blk.dat[j]; ++j)
          *dst++ = blk.dat[j];
        *dst = 0; //term NUL
      }
      bfree(i);
    }
    return r;
  }

  static void ramfree_lindex(Swappable* swp) { }
  static inline void diskfree_lindex(iptr i) {
    BLIndex blk;
    do {
      bread(i, &blk);
      bfree(i);
      i = blk.dat[3];
    } while (i);
  }
  static iptr swapout_lindex(Swappable* swp) {
    RLIndex* rli = (RLIndex*)swp;
    BLIndex blk;

    iptr base = balloc();

    unsigned i=0;
    for (iptr curr = base; curr; curr = blk.dat[3]) {
      for (unsigned j=0; j<3 && i<LOWER_INDEX_SZ; ++i, ++j)
        blk.dat[j] = rli->dat[i].ipt;

      blk.dat[3] = (i < LOWER_INDEX_SZ? balloc() : 0);
      bwrite(curr, &blk);
    }

    ramfree_lindex(swp);

    return base;
  }
  static Swappable* swapin_lindex(iptr base) {
    RLIndex* rli = new RLIndex;
    rli->size = sizeof(RLIndex);
    rli->swapout = swapout_lindex;
    BLIndex blk;

    //Read in and free simultaneously
    for (unsigned i=0; i<LOWER_INDEX_SZ; /* i updated in inner loop */) {
      bread(base, &blk);
      for (unsigned j=0; j<3 && i<LOWER_INDEX_SZ; ++i, ++j)
        rli->dat[i].ipt = blk.dat[j];
      bfree(base);
      base = blk.dat[3];
    }

    return rli;
  }

  static void ramfree_array(Swappable* swp) {
    RArray* arr = (RArray*)swp;
    for (unsigned i=0; i<arr->length; ++i)
      toDelete.push(arr->dat[i]);
    delete[] arr->dat;
  }
  static void diskfree_array(iptr base) {
    BArray blk;
    do {
      bread(base, &blk);
      for (unsigned i=0; i<sizeof(blk.dat)/sizeof(Setting*) && blk.dat[i]; ++i)
        toDelete.push(blk.dat[i]);
      bfree(base);
      base = blk.nxt;
    } while (base);
  }
  static iptr swapout_array(Swappable* swp) {
    RArray* arr = (RArray*)swp;
    BArray blk;
    unsigned ix = 0;
    iptr base = balloc();
    for (iptr curr = base; curr; curr = blk.nxt) {
      //Test ix <= arr->length so we insert a final NULL if needed
      for (unsigned i=0; i<sizeof(blk.dat)/sizeof(Setting*)
                      && ix <= arr->length; ++i, ++ix)
        blk.dat[i] = (ix < arr->length? arr->dat[ix] : NULL);
      blk.nxt = (ix < arr->length? balloc() : 0);
      bwrite(curr, &blk);
    }

    delete[] arr->dat;
    return base;
  }
  static Swappable* swapin_array(iptr base) {
    RArray* arr = new RArray;
    BArray blk;
    //Start by finding the initial capacity
    unsigned initcap = 0;
    for (iptr curr = base; curr; curr = blk.nxt) {
      bread(curr, &blk);
      initcap += sizeof(blk.dat)/sizeof(Setting*);
    }

    //Initial setup
    arr->capacity = initcap;
    arr->dat = new Setting*[initcap];
    arr->size = sizeof(RArray) + initcap*sizeof(Setting*);
    arr->swapout = swapout_array;
    arr->length = 0;
    //Copy data in and delete old
    for (iptr curr = base; curr; curr = blk.nxt) {
      bread(curr, &blk);
      bfree(curr);
      for (unsigned i=0; i<sizeof(blk.dat)/sizeof(Setting*) && blk.dat[i]; ++i)
        arr->dat[arr->length++] = blk.dat[i];
    }

    return arr;
  }

  /* Inserts a hash/name/value tuble into a group's hashtable.
   * Automatically manages the size of the group (due to adding links).
   * This will NOT resize the table -- the caller must ensure that it
   * is appropriate for the current load.
   */
  static void hash_insertTrippleDirect(RGroup*, unsigned hash,
                                       const char* name, Setting* value);
  //Since the deallocation without group deletion is a bit complex and shares
  //almost all of its code with recursive deletion, this function supports
  //both (setting deletion is optional)
  static void ramfree_group_common(Swappable* swp,
                                   bool deleteSettingsAndStrings) {
    RGroup* grp = (RGroup*)swp;
    for (unsigned i=0; i<grp->tableSize; ++i) {
      if (grp->table[i].dat) {
        //Queue Settings and dealloc strings
        for (RGroup::HashEntry* e = &grp->table[i];
             e && deleteSettingsAndStrings; e = e->nxt) {
          toDelete.push(e->dat);
          delete[] e->name;
        }
        //Deallocate links
        for (RGroup::HashEntry* e = grp->table[i].nxt; e;
             /* updated in body */) {
          RGroup::HashEntry* nxt = e->nxt;
          delete e;
          e = nxt;
        }
      }
    }
    delete[] grp->entries;
    delete[] grp->table;
  }
  static void ramfree_group(Swappable* swp) {
    ramfree_group_common(swp, true);
  }
  static void diskfree_group(iptr base) {
    BGroup blk;
    do {
      bread(base, &blk);
      bfree(base);
      if (blk.name)
        diskfree_string(blk.name);
      if (blk.value)
        toDelete.push(blk.value);
      base = blk.nxt;
    } while (base);
  }
  static iptr swapout_group(Swappable* swp) {
    RGroup* grp = (RGroup*)swp;
    BGroup blk;
    iptr base = balloc();
    iptr curr = base;
    for (unsigned i=0; i<grp->numEntries; ++i) {
      blk.dat[0] = grp->entries[i].hash;
      blk.dat[1] = strlen(grp->entries[i].name)+1;
      blk.value = grp->entries[i].dat;
      //Move the name into an RString so we can reuse the BString functionality
      //This also deallocates the string
      RString rstr;
      rstr.str = grp->entries[i].name;
      blk.name = swapout_string(&rstr);
      blk.nxt = (i < grp->numEntries-1? balloc() : 0);
      bwrite(curr, &blk);
      curr = blk.nxt;
    }

    //Special case for no entries
    if (!grp->numEntries) {
      blk.dat[0] = blk.dat[1] = 0;
      blk.value = NULL;
      blk.name = 0;
      blk.nxt = 0;
      bwrite(base, &blk);
    }

    //Free data
    ramfree_group_common(grp, false);

    return base;
  }
  Swappable* swapin_group(iptr base) {
    BGroup blk;
    RGroup* grp = new RGroup;
    grp->swapout = swapout_group;
    //First, determine the number of entries
    grp->numEntries = 0;
    for (iptr curr = base; curr; curr = blk.nxt) {
      bread(curr, &blk);
      if (blk.value) ++grp->numEntries;
    }

    grp->entries = new RGroup::OrderedEntry[grp->numEntries];
    grp->entriesCap = grp->numEntries;
    //Find the second power of two which is >= numEntries for the table size
    grp->tableSize = 1;
    while (grp->tableSize < grp->numEntries) grp->tableSize <<= 1;
    grp->tableSize <<= 1;
    grp->table = new RGroup::HashEntry[grp->tableSize];
    memset(grp->table, 0, grp->tableSize*sizeof(RGroup::HashEntry));
    //Base total size
    grp->size = sizeof(RGroup)
              + grp->entriesCap*sizeof(RGroup::OrderedEntry)
              + grp->tableSize *sizeof(RGroup::HashEntry);

    //Read entries in, add to hash table, and free disk data
    unsigned ix=0;
    for (iptr curr = base; curr; curr = blk.nxt) {
      bread(curr, &blk);
      bfree(curr);
      if (blk.value) {
        grp->entries[ix].dat = blk.value;
        grp->entries[ix].hash = blk.dat[0];
        RString* rs = (RString*)swapin_string(blk.name);
        //Transfer string ownership to the entry, delete RString manually
        grp->entries[ix].name = rs->str;
        delete rs;

        //Update size
        grp->size += blk.dat[1];

        //Add to hashtable
        hash_insertTrippleDirect(grp, blk.dat[0], grp->entries[ix].name,
                                 blk.value);

        ++ix;
      }
    }

    return grp;
  }
  /* END: Data management functions */

  /* BEGIN: Memory management function */
  /* Ensures that current memory usage is below the requested maximum. */
  static void checkMemoryLimit() {
    while (primaryUsed > maxPrimary) {
      Swappable* swp = rutail.prv;
      iptr loc = swp->swapout(swp);
      swp->parent->dsk = loc | 1;
      primaryUsed -= swp->size;
      rutail.prv = swp->prv;
      rutail.prv->nxt = &rutail;
      delete swp;
    }
  }

  /* Adds a new Swappable to the head of the RU list. */
  static void radd(Swappable* swp) {
    assert(swp->swapout);
    swp->nxt = ruhead.nxt;
    swp->prv = &ruhead;
    swp->nxt->prv = swp;
    swp->prv->nxt = swp;
    primaryUsed += swp->size;
    checkMemoryLimit();
  }

  /* Removes a Swappable from the RU list and reduces current
   * memory usage by that amount.
   */
  static void rremove(Swappable* swp) {
    swp->nxt->prv = swp->prv;
    swp->prv->nxt = swp->nxt;
    primaryUsed -= swp->size;
  }

  /* Removes and deletes a Swappable. */
  static void rfree(Swappable* swp) {
    rremove(swp);
    delete swp;
  }

  /* Moves a Swappable to the head of the RU list.
   * This assumes that the size has not changed; if it has,
   * rremove() and radd() should be called instead.
   */
  static void rtraverse(Swappable* swp) {
    swp->nxt->prv = swp->prv;
    swp->prv->nxt = swp->nxt;
    swp->prv = &ruhead;
    swp->nxt = ruhead.nxt;
    swp->prv->nxt = swp;
    swp->nxt->prv = swp;
  }

  /* Returns a freshly traversed Swappable* from the given SwapPointer.
   * The data is automatically loaded from disk if necessary, using the
   * given function pointer.
   * This function should only allocate and initialise the Swappable object;
   * it should not be added (as this function does that). Additionally, it
   * is responsible for freeing all the on-disk data.
   * The SwapPointer will be modified if the data must be fetched from disk.
   */
  static Swappable* rget(SwapPointer& ptr, Swappable* (*swapin)(iptr)) {
    if (ptr.dsk & 1) {
      //On-disk
      Swappable* ret = swapin(ptr.dsk ^ 1);
      ret->parent = &ptr;
      ptr.ptr = ret;
      radd(ret);
      return ret;
    } else {
      //In RAM
      rtraverse((Swappable*)ptr.ptr);
      return (Swappable*)ptr.ptr;
    }
  }

  /* Decodes the given Setting* and returns a SettingEncoder::LowerIndex*
   * that the Setting* points to.
   */
  static SettingEncoder::LowerIndex*
  sgetli(const Setting* s, const vector<SwapPointer*>& uIndex = upperIndex) {
    unsigned uix, lix;
    SettingEncoder::indices(RS(s), uix, lix);
    SwapPointer* uptr = uIndex[uix];
    RLIndex* li = (RLIndex*)rget(*uptr, swapin_lindex);
    return &li->dat[lix];
  }

  /* Extends the lower indices with another block and sets them up as
   * free Settings.
   */
  static void sextend() {
    RLIndex* rli = new RLIndex;
    RLIndex* rpli = new RLIndex;
    for (unsigned i=0; i<LOWER_INDEX_SZ; ++i)
      rli->dat[i].nxt = (i < LOWER_INDEX_SZ-1?
                         SR(SettingEncoder::create((Setting::Type)0,
                                                   upperIndex.size(), i+1))
                         : nextFreeSetting);
    nextFreeSetting = SR(SettingEncoder::create((Setting::Type)0,
                                                upperIndex.size(), 0));
    rli->size = sizeof(RLIndex);
    rpli->size = sizeof(RLIndex);
    rli->swapout = swapout_lindex;
    rpli->swapout = swapout_lindex;

    //Besides this, rpli can be left uninitialised (and besides the parent,
    //below)

    SwapPointer* rliptr = new SwapPointer, * rpliptr = new SwapPointer;
    rliptr->ptr = rli;
    rpliptr->ptr = rpli;
    rli->parent = rliptr;
    rpli->parent = rpliptr;
    upperIndex.push_back(rliptr);
    parentUpperIndex.push_back(rpliptr);
    radd(rli);
    radd(rpli);
  }

  /* Allocates and returns a new Setting* of the given type and parent.
   * The value is NOT initialised.
   *
   * This will return the most recently freed Setting if there is one;
   * the parsing functions, in particular, depend on this behaviour.
   */
  static Setting* salloc(Setting::Type t, Setting* parent) {
    //Allocate more if none free
    if (!nextFreeSetting) sextend();
    //Get information on the first free one
    unsigned uix, lix;
    SettingEncoder::indices(nextFreeSetting, uix, lix);
    SwapPointer* uptr = upperIndex[uix];
    RLIndex* rli = (RLIndex*)rget(*uptr, swapin_lindex);
    //Point free to next
    nextFreeSetting = rli->dat[lix].nxt;

    //Find parent pointer and set
    uptr = parentUpperIndex[uix];
    rli = (RLIndex*)rget(*uptr, swapin_lindex);
    rli->dat[lix].parent = parent;

    //Return new setting
    return SR(SettingEncoder::create(t, uix, lix));
  }

  /* Immediately frees a Setting. This should only be used from
   * garbageCollection() and parsing functions;
   * normally, one should add the Setting* to toDelete.
   */
  static void sfree(Setting* s) {
    Setting::Type type = s->getType();
    SettingEncoder::LowerIndex* sdat = sgetli(s);

    void (*diskfree)(iptr) = NULL;
    void (*ramfree)(Swappable*) = NULL;
    switch (type) {
      case Setting::TypeInt64:
        if (!SettingEncoder::canEmbedInt64()) {
          if (sdat->ptr.dsk & 1) diskfree = diskfree_int64;
          else                   ramfree  = ramfree_int64;
          break;
        } //else fallthrough and treat like other primitives
      case Setting::TypeInt:
      case Setting::TypeFloat:
      case Setting::TypeBoolean:
        //do nothing
        break;
      case Setting::TypeString:
        if (sdat->ptr.dsk & 1) diskfree = diskfree_string;
        else                   ramfree  = ramfree_string;
        break;
      case Setting::TypeList:
      case Setting::TypeArray:
        if (sdat->ptr.dsk & 1) diskfree = diskfree_array;
        else                   ramfree  = ramfree_array;
        break;
      case Setting::TypeGroup:
        if (sdat->ptr.dsk & 1) diskfree = diskfree_group;
        else                   ramfree  = ramfree_group;
        break;
    }

    if (diskfree) {
      diskfree(sdat->ptr.dsk ^ 1);
    } else if (ramfree) {
      ramfree(sdat->ptr.ptr);
      rfree(sdat->ptr.ptr);
    }

    //Add to head of list
    sdat->nxt = nextFreeSetting;
    unsigned uix, lix;
    SettingEncoder::indices(s, uix, lix);
    nextFreeSetting = SR(SettingEncoder::create((Setting::Type)0, uix, lix));
  }
  /* END: Memory management functions */

  //Setting initialisation functions
  static void sinit_bool(SettingEncoder::LowerIndex& li) {
    li.b = false;
  }
  static void sinit_int(SettingEncoder::LowerIndex& li) {
    li.ui = 0;
  }
  static void sinit_int64(SettingEncoder::LowerIndex& li) {
    if (SettingEncoder::canEmbedInt64())
      li.ul = 0;
    else {
      RInt64* ri = new RInt64;
      ri->size = sizeof(RInt64);
      ri->ul = 0;
      ri->parent = &li.ptr;
      ri->swapout = swapout_int64;
      li.ptr.ptr = ri;
      radd(ri);
    }
  }
  static void sinit_float(SettingEncoder::LowerIndex& li) {
    li.f = 0;
  }
  static void sinit_string(SettingEncoder::LowerIndex& li) {
    char* emptyString = new char[1];
    emptyString[0] = 0;
    RString* rs = new RString;
    rs->size = sizeof(RString)+1;
    rs->str = emptyString;
    rs->parent = &li.ptr;
    rs->swapout = swapout_string;
    li.ptr.ptr = rs;
    radd(rs);
  }
  static void sinit_array(SettingEncoder::LowerIndex& li) {
    RArray* ra = new RArray;
    ra->capacity = 4;
    ra->dat = new Setting*[ra->capacity];
    ra->length = 0;
    ra->size = sizeof(RArray) + sizeof(Setting*)*ra->capacity;
    ra->parent = &li.ptr;
    ra->swapout = swapout_array;
    li.ptr.ptr = ra;
    radd(ra);
  }
  static void sinit_group(SettingEncoder::LowerIndex& li) {
    RGroup* rg = new RGroup;
    rg->entriesCap = 4;
    rg->numEntries = 0;
    rg->entries = new RGroup::OrderedEntry[rg->entriesCap];
    rg->tableSize = 4;
    rg->table = new RGroup::HashEntry[rg->tableSize];
    memset(rg->table, 0, sizeof(RGroup::HashEntry)*rg->tableSize);
    rg->size = sizeof(RGroup)
             + sizeof(RGroup::OrderedEntry)*rg->entriesCap
             + sizeof(RGroup::HashEntry)*rg->tableSize;
    rg->parent = &li.ptr;
    rg->swapout = swapout_group;
    li.ptr.ptr = rg;
    radd(rg);
  }
  static void sinit(Setting* s) {
    SettingEncoder::LowerIndex* li = sgetli(s);
    switch (s->getType()) {
      case Setting::TypeBoolean:
        sinit_bool(*li);
        break;
      case Setting::TypeInt:
        sinit_int(*li);
        break;
      case Setting::TypeFloat:
        sinit_float(*li);
        break;
      case Setting::TypeInt64:
        sinit_int64(*li);
        //Retraverse lower index
        sgetli(s);
        break;
      case Setting::TypeString:
        sinit_string(*li);
        //Retraverse lower index
        sgetli(s);
        break;
      case Setting::TypeArray:
      case Setting::TypeList:
        sinit_array(*li);
        //Retraverse lower index
        sgetli(s);
        break;
      case Setting::TypeGroup:
        sinit_group(*li);
        //Retraverse lower index
        sgetli(s);
        break;
    }
  }

  //Functions for working with the group hashtable

  /* Calculates the Jenkins One-at-a-Time hash function for the
   * given string.
   * See: http://en.wikipedia.org/wiki/Jenkins_hash_function
   */
  static unsigned hash_calc(const char* str) {
    unsigned hash = 0;
    while (*str) {
      hash += *str++;
      hash += (hash << 10);
      hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
  }

  /* Inserts a hash/name/value tripple into a group's hashtable.
   * Automatically manages the size of the group (due to adding links).
   * This will NOT resize the table -- the caller must ensure that it
   * is appropriate for the current load.
   */
  void hash_insertTrippleDirect(RGroup* grp, unsigned hash, const char* name,
                                Setting* value) {
    hash &= (grp->tableSize - 1);
    if (!grp->table[hash].dat) {
      //Can be directly inserted into the table
      grp->table[hash].dat = value;
      grp->table[hash].name = name;
      //nxt is already NULL
    } else {
      //New link
      RGroup::HashEntry* entry = new RGroup::HashEntry;
      entry->dat = value;
      entry->name = name;
      entry->nxt = grp->table[hash].nxt;
      grp->table[hash].nxt = entry;
      grp->size += sizeof(RGroup::HashEntry);
    }
  }

  /* Ensures that the group's hashtable is sufficiently large to add
   * another value. If another value would make the number of values
   * greater than half the table size, the current table is destroyed
   * and a new one twice the size constructed.
   */
  void hash_ensureLargeEnough(RGroup* grp) {
    if (grp->numEntries+1 >= grp->tableSize/2) {
      //Deallocate current memory
      //First, find and free dupe links
      for (unsigned i=0; i<grp->tableSize; ++i) {
        for (RGroup::HashEntry* e = grp->table[i].nxt; e; /*updated within*/) {
          RGroup::HashEntry* nxt = e->nxt;
          delete e;
          e = nxt;
          grp->size -= sizeof(RGroup::HashEntry);
        }
      }

      //Reallocate (size difference is simply the current table size)
      grp->size += sizeof(RGroup::HashEntry)*grp->tableSize;
      delete[] grp->table;
      grp->tableSize *= 2;
      grp->table = new RGroup::HashEntry[grp->tableSize];
      memset(grp->table, 0, sizeof(RGroup::HashEntry)*grp->tableSize);

      //Rebuild
      for (unsigned i=0; i<grp->numEntries; ++i)
        hash_insertTrippleDirect(grp, grp->entries[i].hash,
                                 grp->entries[i].name, grp->entries[i].dat);
    }
  }

  /* Inserts a tripple into the table without checking to see if a duplicate
   * exists.
   * Automatically resizes the table if needed.
   */
  void hash_insert(RGroup* grp, unsigned hash, const char* name,
                   Setting* value) {
    hash_ensureLargeEnough(grp);
    hash_insertTrippleDirect(grp, hash, name, value);
  }

  /* Looks up the given hash/name pair and returns the associated datum.
   * Returns NULL if not found.
   */
  Setting* hash_lookup(const RGroup* grp, unsigned hash, const char* name) {
    hash &= (grp->tableSize-1);
    if (!grp->table[hash].dat) return NULL; //Nothing with this hash
    //Search links
    for (RGroup::HashEntry* e = &grp->table[hash]; e; e = e->nxt)
      if (0 == strcmp(name, e->name)) return e->dat;
    //Nothing found
    return NULL;
  }

  /* Removes the given entry from the hash table.
   * This will not reduce the size of the table; however, it may
   * alter the size by deallocating a dupe-link.
   * Returns whether the value was actually found.
   */
  bool hash_remove(RGroup* grp, unsigned hash, const char* name) {
    hash &= (grp->tableSize-1);
    if (!grp->table[hash].dat) return false; //Nothing with this hash
    if (0 == strcmp(name, grp->table[hash].name)) {
      //Removing the first match
      if (grp->table[hash].nxt) {
        //Another match follows
        //Move next link here, then remove it
        RGroup::HashEntry* nxt = grp->table[hash].nxt;
        grp->table[hash] = *nxt;
        delete nxt;
        grp->size -= sizeof(RGroup::HashEntry);
      } else {
        //Nothing follows, nullify
        memset(&grp->table[hash], 0, sizeof(RGroup::HashEntry));
      }
      return true;
    } else {
      //Search for a match in the list and remove it
      for (RGroup::HashEntry* prv = &grp->table[hash], *curr = prv->nxt;
           curr; prv = curr, curr = curr->nxt) {
        if (0 == strcmp(name, curr->name)) {
          //Match
          //Remove from list
          prv->nxt = curr->nxt;
          //Deallocate
          delete curr;
          grp->size -= sizeof(RGroup::HashEntry);
          //Finished
          return true;
        }
      }
      //No match found
      return false;
    }
  }

  //Setting implementation
  static const char* typeName(Setting::Type t) {
    switch (t) {
      case Setting::TypeBoolean:return "TypeBoolean";
      case Setting::TypeInt:    return "TypeInt";
      case Setting::TypeInt64:  return "TypeInt64";
      case Setting::TypeFloat:  return "TypeFloat";
      case Setting::TypeString: return "TypeString";
      case Setting::TypeArray:  return "TypeArray";
      case Setting::TypeList:   return "TypeList";
      case Setting::TypeGroup:  return "TypeGroup";
    }
    exit(EXIT_THE_SKY_IS_FALLING);
  }
  /* Swaps a setting's external data in and returns it after
   * retraversing the lower index.
   */
  template<typename T>
  T* sgetext(const Setting* s, Swappable* (*swapin)(iptr)) {
    T* ret = (T*)rget(sgetli(s)->ptr, swapin);
    sgetli(s);
    return ret;
  }
  /* Parses a [###] name and sets the second argument to
   * the contained number.
   * Returns whether this actually happened.
   */
  bool parseBracketIndex(const char* k, unsigned& ix) {
    if (*k != '[') return false;
    ++k;
    ix = 0;
    bool ok = false;
    while (*k && *k != ']') {
      if (*k >= '0' && *k <= '9') {
        ix *= 10;
        ix += (*k - '0');
        ok = true;
      } else return false;
      ++k;
    }

    //Ensure properly terminated
    return ok && *k == ']' && *(k+1) == 0;
  }
  #define TYPEXCPT(ltype,s) \
    if (ltype != s->getType()) { \
      string msg(#ltype " expected; "); \
      msg += typeName(s->getType()); \
      msg += " encountered"; \
      throw SettingTypeException(msg, s->getPath()); \
    }
  #define CTYPEXCPT(s) \
    if (Setting::TypeGroup != s->getType() \
    &&  Setting::TypeList  != s->getType() \
    &&  Setting::TypeArray != s->getType()) { \
      string msg("Composite expected; "); \
      msg += typeName(s->getType()); \
      msg += " encountered"; \
      throw SettingTypeException(msg, s->getPath()); \
    }

  Setting::operator bool() const {
    TYPEXCPT(TypeBoolean,this);
    return sgetli(this)->b;
  }
  Setting::operator unsigned() const {
    TYPEXCPT(TypeInt,this);
    return sgetli(this)->ui;
  }
  Setting::operator signed() const {
    TYPEXCPT(TypeInt,this);
    return sgetli(this)->si;
  }
  Setting::operator float() const {
    TYPEXCPT(TypeFloat,this);
    return sgetli(this)->f;
  }
  Setting::operator unsigned long long() const {
    TYPEXCPT(TypeInt64,this);
    if (SettingEncoder::canEmbedInt64())
      return sgetli(this)->ul;
    else
      return sgetext<RInt64>(this, swapin_int64)->ul;
  }
  Setting::operator signed long long() const {
    TYPEXCPT(TypeInt64,this);
    if (SettingEncoder::canEmbedInt64())
      return sgetli(this)->sl;
    else
      return sgetext<RInt64>(this, swapin_int64)->sl;
  }
  Setting::operator const char*() const {
    TYPEXCPT(TypeString,this);
    return sgetext<RString>(this, swapin_string)->str;
  }
  Setting& Setting::operator=(bool b) {
    TYPEXCPT(TypeBoolean,this);
    sgetli(this)->b = b;
    return *this;
  }
  Setting& Setting::operator=(unsigned i) {
    TYPEXCPT(TypeInt,this);
    sgetli(this)->ui = i;
    return *this;
  }
  Setting& Setting::operator=(signed i) {
    TYPEXCPT(TypeInt,this);
    sgetli(this)->si = i;
    return *this;
  }
  Setting& Setting::operator=(float f) {
    TYPEXCPT(TypeFloat,this);
    sgetli(this)->f = f;
    return *this;
  }
  Setting& Setting::operator=(unsigned long long l) {
    TYPEXCPT(TypeInt64,this);
    if (SettingEncoder::canEmbedInt64())
      sgetli(this)->ul = l;
    else
      sgetext<RInt64>(this,swapin_int64)->ul = l;
    return *this;
  }
  Setting& Setting::operator=(signed long long l) {
    TYPEXCPT(TypeInt64,this);
    if (SettingEncoder::canEmbedInt64())
      sgetli(this)->sl = l;
    else
      sgetext<RInt64>(this,swapin_int64)->sl = l;
    return *this;
  }
  Setting& Setting::operator=(const char* str) {
    TYPEXCPT(TypeString,this);
    RString* rstr = sgetext<RString>(this,swapin_string);
    rremove(rstr); //Remove since we'll update the size
    delete[] rstr->str;
    unsigned len = strlen(str);
    char* copy = new char[len+1];
    memcpy(copy,str,len+1);
    rstr->size = sizeof(RString)+len+1;
    rstr->str = copy;
    radd(rstr);
    //Retraverse lower index
    sgetli(this);

    return *this;
  }
  Setting& Setting::operator=(const string& str) {
    return operator=(str.c_str());
  }

  Setting& Setting::operator[](unsigned ix) {
    CTYPEXCPT(this);
    switch (getType()) {
      case TypeList:
      case TypeArray: {
        RArray* arr = sgetext<RArray>(this, swapin_array);
        if (ix >= arr->length) {
          ostringstream elt;
          elt << "[" << ix << "]";
          throw SettingNotFoundException("Index out of range",
                                         getPath() + "." + elt.str());
        }
        return *arr->dat[ix];
      } break;
      case TypeGroup: {
        RGroup* grp = sgetext<RGroup>(this, swapin_group);
        if (ix >= grp->numEntries) {
          ostringstream elt;
          elt << "[" << ix << "]";
          throw SettingNotFoundException("Index out of range",
                                         getPath() + "." + elt.str());
        }
        return *grp->entries[ix].dat;
      } break;
      default: ++*(int*)NULL; /* never reached, die */
      //Make the compiler happy, since it can't verify that this code-path
      //will never occur
      return *(Setting*)NULL;
    }
  }
  const Setting& Setting::operator[](unsigned ix) const {
    return const_cast<Setting*>(this)->operator[](ix);
  }
  Setting& Setting::operator[](const char* key) {
    //If key begins and ends with [ and ], parse and use
    //operator[](unsigned) instead
    unsigned tix;
    if (parseBracketIndex(key, tix)) return operator[](tix);

    TYPEXCPT(TypeGroup, this);
    RGroup* grp = sgetext<RGroup>(this, swapin_group);
    unsigned hash = hash_calc(key);
    Setting* ret = hash_lookup(grp, hash, key);
    if (!ret)
      throw SettingNotFoundException("No such key", getPath() + "." + key);
    return *ret;
  }
  const Setting& Setting::operator[](const char* key) const {
    return const_cast<Setting*>(this)->operator[](key);
  }
  Setting& Setting::operator[](const string& key) {
    return operator[](key.c_str());
  }
  const Setting& Setting::operator[](const string& key) const {
    return const_cast<Setting*>(this)->operator[](key.c_str());
  }

  #define LVFUN(ctype,ltype,access) \
    bool Setting::lookupValue(const char* key, ctype& dst) const { \
      if (getType() != TypeGroup) return false; \
      Setting* s = hash_lookup(sgetext<RGroup>(this,swapin_group), \
                               hash_calc(key), key); \
      if (!s) return false; \
      if (s->getType() != ltype) return false; \
      dst = access; \
      return true; \
    } bool Setting::lookupValue(const string& key, ctype& dst) const { \
      return lookupValue(key.c_str(), dst); \
    }
  LVFUN(bool,TypeBoolean,sgetli(s)->b)
  LVFUN(unsigned,TypeInt,sgetli(s)->ui)
  LVFUN(signed,TypeInt,sgetli(s)->si)
  LVFUN(float,TypeFloat,sgetli(s)->f)
  LVFUN(unsigned long long,TypeInt64,
        (SettingEncoder::canEmbedInt64()?
         sgetli(s)->ui :
         sgetext<RInt64>(s,swapin_int64)->ul))
  LVFUN(signed long long,TypeInt64,
        (SettingEncoder::canEmbedInt64()?
        sgetli(s)->si :
        sgetext<RInt64>(s,swapin_int64)->sl))
  LVFUN(const char*,TypeString,sgetext<RString>(s,swapin_string)->str)
  LVFUN(string,TypeString,sgetext<RString>(s,swapin_string)->str)
  #undef LVFUN

  Setting& Setting::add(const char* key, Setting::Type type) {
    TYPEXCPT(TypeGroup,this);
    if (!*key)
      throw SettingNameException("Empty key name", getPath());
    for (const char* k = key; *k; ++k) if (!isIDChar(*k))
      throw SettingNameException(string("Invalid key name ") + key, getPath());
    RGroup* grp = sgetext<RGroup>(this, swapin_group);
    unsigned hash = hash_calc(key);
    if (hash_lookup(grp, hash, key))
      throw SettingNameException("Given name already in use",
                                 getPath() + "." + key);

    //OK
    Setting* s = salloc(type, this);
    sinit(s);
    unsigned keylen = strlen(key)+1;
    char* name = new char[keylen];
    memcpy(name, key, keylen);

    //Remove from current allocations since we'll be modifying size
    rremove(grp);
    grp->size += keylen;

    //Add to hashtable
    hash_insert(grp, hash, name, s);

    //Add to ordered table
    if (grp->numEntries == grp->entriesCap) {
      //Need to resize
      grp->size += grp->entriesCap*sizeof(RGroup::OrderedEntry);
      grp->entriesCap *= 2;
      RGroup::OrderedEntry* newentries =
          new RGroup::OrderedEntry[grp->entriesCap];
      memcpy(newentries, grp->entries,
             sizeof(RGroup::OrderedEntry)*grp->numEntries);
      delete[] grp->entries;
      grp->entries = newentries;
    }
    grp->entries[grp->numEntries].dat = s;
    grp->entries[grp->numEntries].hash = hash;
    grp->entries[grp->numEntries].name = name;
    ++grp->numEntries;

    radd(grp);
    //Retraverse containing lower index
    sgetli(this);

    return *s;
  }
  Setting& Setting::add(const string& key, Setting::Type type) {
    return add(key.c_str(), type);
  }

  Setting& Setting::add(Setting::Type type) {
    if (getType() != TypeArray
    &&  getType() != TypeList) {
      string msg("TypeArray or TypeList expected; ");
      msg += typeName(getType());
      msg += " encountered";
      throw SettingTypeException(msg, getPath());
    }

    //We need the array object for further checks
    RArray* arr = sgetext<RArray>(this, swapin_array);
    if (getType() == TypeArray) {
      if (arr->length > 0 && type != arr->dat[0]->getType()) {
        string msg("Array type mismatch; insert ");
        msg = msg + typeName(type) + " into array containing "
            + typeName(arr->dat[0]->getType());
        throw SettingTypeException(msg, getPath());
      }

      if (type == TypeArray || type == TypeList || type == TypeGroup)
        throw SettingTypeException("Attempt to insert composite into array",
                                   getPath());
    }

    //OK
    Setting* s = salloc(type, this);
    sinit(s);

    if (arr->length == arr->capacity) {
      //Need to resize
      rremove(arr);
      arr->size += sizeof(Setting*)*arr->capacity;
      arr->capacity *= 2;
      Setting** ndat = new Setting*[arr->capacity];
      memcpy(ndat, arr->dat, sizeof(Setting*)*arr->length);
      delete[] arr->dat;
      arr->dat = ndat;

      radd(arr);
      //Retraverse containing lower index
      sgetli(this);
    }

    arr->dat[arr->length++] = s;
    return *s;
  }

  void Setting::remove(unsigned ix) {
    CTYPEXCPT(this);
    if (TypeGroup == getType()) {
      RGroup* grp = sgetext<RGroup>(this, swapin_group);
      if (ix >= grp->numEntries) {
        ostringstream subname;
        subname << "[" << ix << "]";
        throw SettingNotFoundException("Index out of range",
                                       getPath() + "." + subname.str());
      }

      //We'll be altering the size, so remove first
      rremove(grp);
      //Remove from hashtable
      hash_remove(grp, grp->entries[ix].hash, grp->entries[ix].name);
      //Update size and dealloc name
      grp->size -= 1+strlen(grp->entries[ix].name);
      delete[] grp->entries[ix].name;
      //Dealloc setting
      toDelete.push(grp->entries[ix].dat);
      //Remove from ordered list
      --grp->numEntries;
      memmove(grp->entries+ix, grp->entries+ix+1,
              (grp->numEntries-ix)*sizeof(RGroup::OrderedEntry));

      radd(grp);
      //Retraverse containing lower index
      sgetli(this);
    } else {
      //Else it is an array/list
      RArray* arr = sgetext<RArray>(this, swapin_array);
      if (ix >= arr->length) {
        ostringstream subname;
        subname << "[" << ix << "]";
        throw SettingNotFoundException("Index out of range",
                                       getPath() + "." + subname.str());
      }

      toDelete.push(arr->dat[ix]);
      --arr->length;
      memmove(arr->dat+ix, arr->dat+ix+1, (arr->length-ix)*sizeof(Setting*));
    }
  }

  void Setting::remove(const char* key) {
    unsigned tix;
    if (parseBracketIndex(key,tix)) {
      remove(tix);
      return;
    }

    TYPEXCPT(TypeGroup,this);
    RGroup* grp = sgetext<RGroup>(this, swapin_group);
    unsigned hash = hash_calc(key);
    Setting* s = hash_lookup(grp, hash, key);
    if (!s)
      throw SettingNotFoundException("No such key", getPath() + "." + key);

    //Since we'd need to search the array anyway, just use the functions
    //we already have
    remove(s->getIndex());
  }
  void Setting::remove(const string& key) {
    remove(key.c_str());
  }

  Setting* Setting::getParentNoThrow() {
    return sgetli(this, parentUpperIndex)->parent;
  }
  const Setting* Setting::getParentNoThrow() const {
    return sgetli(this, parentUpperIndex)->parent;
  }
  Setting& Setting::getParent() {
    Setting* par = getParentNoThrow();
    if (!par)
      throw SettingNotFoundException("Root has no parent", "");
    return *par;
  }
  const Setting& Setting::getParent() const {
    const Setting* par = getParentNoThrow();
    if (!par)
      throw SettingNotFoundException("Root has no parent", "");
    return *par;
  }

  const char* Setting::getName() const {
    const Setting* parent = getParentNoThrow();
    if (!parent) return NULL;

    if (parent->getType() == TypeGroup) {
      RGroup* grp = sgetext<RGroup>(parent, swapin_group);
      return grp->entries[getIndex()].name;
    } else {
      //Array/list
      ostringstream name;
      name << "[" << getIndex() << "]";
      static string ret;
      ret = name.str();
      return ret.c_str();
    }
  }
  unsigned Setting::getIndex() const {
    const Setting* parent = getParentNoThrow();
    if (!parent) return (unsigned)-1;

    if (parent->getType() == TypeGroup) {
      RGroup* grp = sgetext<RGroup>(parent, swapin_group);
      unsigned ix = 0;
      while (this != grp->entries[ix].dat) ++ix;
      return ix;
    } else {
      //Array/list
      RArray* arr = sgetext<RArray>(parent, swapin_array);
      unsigned ix = 0;
      while (this != arr->dat[ix]) ++ix;
      return ix;
    }
  }

  string Setting::getPath() const {
    const Setting* parent = getParentNoThrow();

    string p;
    if (parent) {
      p = parent->getPath();
      if (!p.empty()) p += ".";
      p += getName();
    }
    return p;
  }

  bool Setting::isRoot() const { return NULL == getParentNoThrow(); }

  bool Setting::exists(const char* key) const {
    if (TypeGroup != getType()) return false;
    return NULL != hash_lookup(sgetext<RGroup>(this, swapin_group),
                               hash_calc(key), key);
  }
  bool Setting::exists(const string& key) const {
    return exists(key.c_str());
  }

  unsigned Setting::getLength() const {
    switch (getType()) {
      case TypeGroup:
        return sgetext<RGroup>(this, swapin_group)->numEntries;
      case TypeList:
      case TypeArray:
        return sgetext<RArray>(this, swapin_array)->length;
      default:
        return 0;
    }
  }

  bool Setting::isGroup() const { return TypeGroup == getType(); }
  bool Setting::isArray() const { return TypeArray == getType(); }
  bool Setting::isList () const { return TypeList  == getType(); }
  bool Setting::isAggregate() const {
    return isGroup() || isArray() || isList();
  }
  bool Setting::isScalar() const { return !isAggregate(); }
  bool Setting::isNumber() const {
    return TypeInt == getType()
        || TypeInt64 == getType()
        || TypeFloat == getType();
  }
  /* END: Setting implementation */

  /* BEGIN: File parser and writer */
  /* Increments the given pointer until isWhite() returns
   * false for the pointee. Also updates line as necessary.
   * Also handles comments.
   */
  static void parser_skipWhitespace(const char*& s, unsigned& line,
                                    bool includeComma=true) {
    normalWhitespace:
    while (isWhite(*s, includeComma)) {
      if (*s == '\n') ++line;
      if (*s == '/') goto beginComment;
      if (*s == '#') goto lineComment;
      ++s;
    }
    return;

    beginComment:
    ++s;
    if (*s == '*') goto blockComment;
    if (*s == '/') goto lineComment;

    //Let the caller encounter and fail on the lone /
    --s;
    return;

    lineComment:
    while (*s && *s != '\n') ++s;
    goto normalWhitespace;

    blockComment:
    ++s;
    while (*s && *s != '*') if (*s++ == '\n') ++line;
    if (*s == '*') goto blockCommentEnd;
    //Encountered EOF, just return
    return;

    blockCommentEnd:
    ++s;
    if (*s == '/') {
      ++s;
      goto normalWhitespace;
    } else if (*s == 0) return;
    else if (*s == '\n') ++line;
    ++s;
    goto blockComment;
  }
  /* Reads characters into an identifier until a whitespace
   * character, or character in altEndChars or NUL is
   * encountered. If any other character that is not legal
   * in an identifier is encountered, a ParseException is
   * thrown.
   * The input pointer is left at the first non-identifier
   * character encountered.
   * If an empty identifier is read, a ParseException is thrown.
   */
  static string parser_readIdentifier(const char*& s, const string& file,
                                      unsigned line,
                                      const char* altEndChars = "([{}])") {
    string id;
    while (isIDChar(*s)) id += *s++;
    if (id.empty())
      throw ParseException("Expected identifier", file, line);
    if (isWhite(*s) || *s == 0 || strchr(altEndChars, *s))
      return id;
    throw ParseException(string("Invalid identifier; encountered character ") +
                                *s, file, line);
  }
  /* Reads in a string of characters until whitespace or an alternate
   * terminater is encountered.
   * Never throws an exception.
   */
  static string parser_readChunk(const char*& s, const char* altEndChars) {
    string str;
    while (!isWhite(*s) && !strchr(altEndChars, *s)) str += *s++;
    return str;
  }
  /* Determines the type of the next Setting by examining the current character.
   * For numeric types, further characters will be analysed.
   * Assumes that whitespace has been skipped to this point.
   * Throws ParseException if the type cannot be determined.
   */
  static Setting::Type parser_classify(const char* s, const string& file,
                                       unsigned line) {
    switch (*s) {
      case '{': return Setting::TypeGroup;
      case '[': return Setting::TypeArray;
      case '(': return Setting::TypeList;
      case '"': return Setting::TypeString;
      case 't':
      case 'f':
      case 'j':
      case 'n':
      case 'y': return Setting::TypeBoolean;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case '+':
      case '-':
      case '.': {
        string str = parser_readChunk(s, "]})"); //Doesn't destroy parent's s
        const char* cs = str.c_str();
        if (strchr(cs, '.')) return Setting::TypeFloat;
        if (strchr(cs, 'l') || strchr(cs, 'L')) return Setting::TypeInt64;
        return Setting::TypeInt;
      }
      default:
        throw ParseException("Unclassifiable type", file, line);
    }
  }

  static Setting* parser_readNumber(Setting* parent, const char*& s,
                                    const char* altEndChars,
                                    const string& file, unsigned& line);
  static Setting* parser_readBool(Setting* parent, const char*& s,
                                  const char* altEndChars,
                                  const string& file, unsigned& line);
  static Setting* parser_readString(Setting* parent, const char*& s,
                                    const char* altEndChars,
                                    const string& file, unsigned& line);
  static Setting* parser_readArray(Setting* parent, Setting::Type type,
                                   const char*& s,
                                   const string& file, unsigned& line);
  static Setting* parser_readGroup(Setting* parent, const char*& s,
                                   const string& file, unsigned& line);
  /* Reads a Setting of the given type in and returns it.
   * s is updated to the first non-whitespace character after the end of the
   * setting.
   * line is updated as necessary.
   */
  static Setting* parser_read(Setting::Type type, Setting* parent,
                              const char*& s, const char* altEndChars,
                              const string& file, unsigned& line) {
    switch (type) {
      case Setting::TypeInt:
      case Setting::TypeInt64:
      case Setting::TypeFloat:
        return parser_readNumber(parent, s, altEndChars, file, line);
      case Setting::TypeBoolean:
        return parser_readBool(parent, s, altEndChars, file, line);
      case Setting::TypeString:
        return parser_readString(parent, s, altEndChars, file, line);
      case Setting::TypeArray:
      case Setting::TypeList:
        return parser_readArray(parent, type, s, file, line);
      case Setting::TypeGroup:
        return parser_readGroup(parent, s, file, line);
    }

    exit(EXIT_THE_SKY_IS_FALLING);
  }

  static Setting* parser_readNumber(Setting* parent, const char*& s,
                                    const char* altEndChars,
                                    const string& file, unsigned& line) {
    string str = parser_readChunk(s, altEndChars);
    parser_skipWhitespace(s, line);

    Setting* ret;

    if (str[str.size()-1] == 'l' || str[str.size()-1] == 'L')
      //TypeInt64
      ret = salloc(Setting::TypeInt64, parent);
    else if (strchr(str.c_str(), '.'))
      //TypeFloat
      ret = salloc(Setting::TypeFloat, parent);
    else
      //TypeInt
      ret = salloc(Setting::TypeInt, parent);

    sinit(ret);

    istringstream in(str);

    if (ret->getType() == Setting::TypeFloat) {
      float f;
      in >> f;
      *ret = f;
    } else if (ret->getType() == Setting::TypeInt) {
      if (str[0] != '-') {
        unsigned i;
        unsigned a = (str[0] == '+'? 1 : 0);
        char ignore[2];
        if (str[0] == '+') in >> ignore[0];
        if (str.size() >= 3+a && str[a] == '0' &&
            (str[a+1] == 'x' || str[a+1] == 'X'))
          //Hexadecimal
          in >> hex >> ignore[0] >> ignore[1];
        in >> i;
        *ret = i;
      } else {
        signed i;
        char ignore[2];
        //Skip sign
        in >> ignore[0];
        if (str.size() >= 4 && str[1] == '0' &&
            (str[2] == 'x' || str[2] == 'X'))
          in >> hex >> ignore[0] >> ignore[1];
        in >> i;
        *ret = -i;
      }
    } else {
      if (str[0] != '-') {
        unsigned long long i;
        unsigned a = (str[0] == '+'? 1 : 0);
        char ignore[2];
        if (str[0] == '+') in >> ignore[0];
        if (str.size() >= 3+a && str[a] == '0' &&
            (str[a+1] == 'x' || str[a+1] == 'X'))
          //Hexadecimal
          in >> hex >> ignore[0] >> ignore[1];
        in >> i;
        *ret = i;
      } else {
        signed long long i;
        char ignore[2];
        in >> ignore[0]; //Skip sign
        if (str.size() >= 4 && str[1] == '0' &&
            (str[2] == 'x' || str[2] == 'X'))
          in >> hex >> ignore[0] >> ignore[1];
        in >> i;
        *ret = -i;
      }
    }

    if (!in || EOF != in.peek()) {
      //Error reading or not everything read
      sfree(ret);
      throw ParseException(string("Invalid numeric syntax: ") + str,
                           file, line);
    }

    parser_skipWhitespace(s, line);

    return ret;
  }

  static Setting* parser_readBool(Setting* parent, const char*& s,
                                  const char* altEndChars,
                                  const string& file, unsigned& line) {
    string id = parser_readIdentifier(s, file, line, altEndChars);
    parser_skipWhitespace(s, line);

    //We are only called if the id already starts with a valid item, so
    //we don't need to do any actual validation
    Setting* ret = salloc(Setting::TypeBoolean, parent);
    sinit(ret);
    *ret = id[0] == 't' || id[0] == 'j' || id[0] == 'y';
    return ret;
  }

  static Setting* parser_readString(Setting* parent, const char*& s,
                                    const char* altEndChars,
                                    const string& file, unsigned& line) {
    Setting* ret = salloc(Setting::TypeString, parent);
    sinit(ret);

    string str;
    const char* errorMessage;

    char escc, hex;

    //When reading a string, we have the following states:
    //  outside string  Switch to inside string on ", terminate otherwise
    //  inside string   Switch to outside and skip whitespace on ",
    //                  switch to escape on \, stay otherwise
    //  escape          Switch to inside string on valid sequence, to escapex0
    //                  on x, fail on invalid
    //  escapex0        Validate first hex digit and move to escapex1 or fail
    //  escapex1        Validate second hex digit and move to inside string or
    //                  fail
    outsideString:
    if (*s == '"') {
      ++s;
      goto insideString;
    }
    if (*s == ',') {
      ++s;
      parser_skipWhitespace(s, line);
    }
    //Done
    *ret = str;
    return ret;

    insideString:
    switch (*s++) {
      case '"': parser_skipWhitespace(s, line, false); goto outsideString;
      case '\\': goto escape;
      case '\r': goto insideString; //Drop CRs
      case '\0': errorMessage = "EOF within string"; goto fail;
      case '\n': ++line; //fall through
      default:
        str += *(s-1);
        goto insideString;
    }

    escape:
    switch (*s++) {
      case 'x': goto escapex0;
      case '\\': escc = '\\'; break;
      case '"': escc = '"'; break;
      case 'f': escc = '\f'; break;
      case 'n': escc = '\n'; break;
      case 'r': escc = '\r'; break;
      case 't': escc = '\t'; break;
      case 'a': escc = '\a'; break;
      default:
        errorMessage = "Invalid escape sequence";
        goto fail;
    }
    str += escc;
    goto insideString;

    escapex0:
    if (*s >= '0' && *s <= '9')
      hex = *s-'0';
    else if (*s >= 'a' && *s <= 'f')
      hex = *s-'a' + 0xA;
    else if (*s >= 'A' && *s <= 'F')
      hex = *s-'A' + 0xA;
    else {
      errorMessage = "Bad hex digit in \\x";
      goto fail;
    }

    ++s;

    //escapex1: //Only falling through
    escc = hex;
    escc <<= 4;
    if (*s >= '0' && *s <= '9')
      hex = *s-'0';
    else if (*s >= 'a' && *s <= 'f')
      hex = *s-'a' + 0xA;
    else if (*s >= 'A' && *s <= 'F')
      hex = *s-'A' + 0xA;
    else {
      errorMessage = "Bad hex digit in \\x";
      goto fail;
    }
    escc |= hex;

    ++s;
    str += escc;

    goto insideString;

    fail:
    sfree(ret);
    throw ParseException(errorMessage, file, line);
  }

  static Setting* parser_readArray(Setting* parent, Setting::Type type,
                                   const char*& s,
                                   const string& file, unsigned& line) {
    Setting* ret = salloc(type, parent);
    sinit(ret);

    //Skip opening bracket
    ++s;
    parser_skipWhitespace(s, line);

    const char* terminator = (type == Setting::TypeList? ")" : "]");

    try {
      while (*s != terminator[0]) {
        if (*s == 0) throw ParseException("EOF while reading array or list",
                                          file, line);

        Setting::Type subt = parser_classify(s, file, line);
        Setting* nxt = &ret->add(subt);
        //Immediately free the setting so that the lower parser will use the
        //same pointer
        sfree(nxt);
        Setting* sub = parser_read(subt, ret, s, terminator, file, line);
        assert(nxt == sub);
      }
    } catch (...) {
      toDelete.push(ret);
      throw;
    }

    ++s; //Skip closing bracket
    parser_skipWhitespace(s, line);

    return ret;
  }

  static Setting* parser_readGroup(Setting* parent, const char*& s,
                                   const string& file, unsigned& line) {
    char terminator;
    if (parent) {
      terminator = '}';
      //Skip initial brace
      ++s;
    } else {
      terminator = 0;
    }
    parser_skipWhitespace(s, line);

    Setting* ret = salloc(Setting::TypeGroup, parent);
    sinit(ret);

    try {
      while (*s != terminator && *s) {
        string name = parser_readIdentifier(s, file, line, "");
        parser_skipWhitespace(s, line);
        if (!*s || *s == terminator)
          throw ParseException("Group terminated after identifier", file, line);

        Setting::Type subt = parser_classify(s, file, line);
        Setting* nxt = &ret->add(name, subt);
        //Immediately_free the setting so that the lower parser will use
        //the same pointer
        sfree(nxt);
        Setting* sub = parser_read(subt, ret, s, (parent? "}" : ""), file,
                                   line);
        assert(nxt == sub);
      }

      if (*s != terminator)
        throw ParseException("Unterminated group", file, line);
    } catch (...) {
      toDelete.push(ret);
      throw;
    }

    if (parent) {
      //Skip closing brace
      ++s;
      parser_skipWhitespace(s, line);
    }

    return ret;
  }

  /* The written format for the types is (_ is space):
   *   Bool: t or f
   *   Int: %d
   *   Int64: %dL
   *   Float: %e
   *   String: Begin with tab stop; enclose characters with line breaks
   *     as necessary to not pass the 80th column. String terminated with
   *     comma after last block.
   *   Array: \t[...]   Each item in the array is separated by _
   *     if the column is less than 70, otherwise \n
   *   List: Like array with ()
   *   Group: \t{\n@\t%s#\n}
   *     @ = indentation level (spaces)
   *     # = setting format
   */
  static void writer_bool       (Setting*, ostream&, unsigned& col,
                                 unsigned indent);
  static void writer_int        (Setting*, ostream&, unsigned& col,
                                 unsigned indent);
  static void writer_int64      (Setting*, ostream&, unsigned& col,
                                 unsigned indent);
  static void writer_float      (Setting*, ostream&, unsigned& col,
                                 unsigned indent);
  static void writer_string     (Setting*, ostream&, unsigned& col,
                                 unsigned indent);
  static void writer_array      (Setting*, ostream&, unsigned& col,
                                 unsigned indent);
  static void writer_group      (Setting*, ostream&, unsigned& col,
                                 unsigned indent);
  static void addTab(unsigned& col) {
    col &= ~(8-1);
    col += 8;
  }

  static void writer_any(Setting* s, ostream& out, unsigned& col,
                         unsigned indent) {
    switch (s->getType()) {
      case Setting::TypeBoolean:writer_bool     (s,out,col,indent); break;
      case Setting::TypeInt:    writer_int      (s,out,col,indent); break;
      case Setting::TypeInt64:  writer_int64    (s,out,col,indent); break;
      case Setting::TypeFloat:  writer_float    (s,out,col,indent); break;
      case Setting::TypeString: writer_string   (s,out,col,indent); break;
      case Setting::TypeArray:
      case Setting::TypeList:   writer_array    (s,out,col,indent); break;
      case Setting::TypeGroup:  writer_group    (s,out,col,indent); break;
    }
  }

  static void writer_bool(Setting* s, ostream& out, unsigned& col,
                          unsigned indent) {
    if (s->operator bool())
      out << "t";
    else
      out << "f";
    ++col;
  }
  static void writer_int(Setting* s, ostream& out, unsigned& col,
                         unsigned indent) {
    streampos init = out.tellp();
    out << s->operator signed();
    col += out.tellp() - init;
  }
  static void writer_int64(Setting* s, ostream& out, unsigned& col,
                           unsigned indent) {
    streampos init = out.tellp();
    out << s->operator signed long long() << 'L';
    col += out.tellp() - init;
  }
  static void writer_float(Setting* s, ostream& out, unsigned& col,
                           unsigned indent) {
    //Unfortunately, there is no option to force at least one digit after
    //the decimal point to be printed (showpoint only appends a lone . if
    //needed).
    //To work around this, write to a separate string first, then, if it has no
    //decimal point, add ".0" to it
    ostringstream os;
    os << setprecision(10) << s->operator float();
    if (!strchr(os.str().c_str(), '.'))
      os << ".0";
    out << os.str();
    col += os.str().size();
  }
  static void writer_string(Setting* s, ostream& out, unsigned& col,
                            unsigned indent) {
    unsigned baseCol = (col < 32? col : 32);
    char sout[5]; //Maximum is \x## plus NUL
    unsigned slen;
    out << '"';
    ++col;
    for (const char* str = *s; *str; ++str) {
      unsigned char ch = *str;
      //We'll allow UTF-8 to pass through
      if (ch >= ' ' && ch != 127 && ch != '\\' && ch != '"') {
        sout[0] = ch;
        sout[1] = 0;
        slen = 1;
      } else switch (ch) {
        case '\n':
          sout[0] = '\\';
          sout[1] = 'n';
          sout[2] = 0;
          slen=2;
          break;
        case '\r':
          sout[0] = '\\';
          sout[1] = 'r';
          sout[2] = 0;
          slen=2;
          break;
        case '\f':
          sout[0] = '\\';
          sout[1] = 'f';
          sout[2] = 0;
          slen=2;
          break;
        case '\\':
          sout[0] = '\\';
          sout[1] = '\\';
          sout[2] = 0;
          slen=2;
          break;
        case '"':
          sout[0] = '\\';
          sout[1] = '"';
          sout[2] = 0;
          slen=2;
          break;
        case '\a':
          sout[0] = '\\';
          sout[1] = 'a';
          sout[2] = 0;
          slen=2;
          break;
        default:
          sout[0] = '\\';
          sout[1] = 'x';
          unsigned char high = (ch >> 4) & 0xF;
          sout[2] = (high >= 9? 'A'+high : '0'+high);
          unsigned char low  = ch & 0xF;
          sout[3] = (low  >= 9? 'A'+low  : '0'+low );
          sout[4] = 0;
          slen=4;
      }

      if (col+slen >= 80) {
        out << "\"\n";
        col = baseCol+1;
        for (unsigned i=0; i<baseCol/8; ++i)
          out << '\t';
        for (unsigned i=0; i<baseCol%8; ++i)
          out << ' ';
        out << '"';
      }
      out << sout;
      col += slen;
    }

    out << "\",";
    col += 2;
  }
  static void writer_array(Setting* s, ostream& out, unsigned& col,
                           unsigned indent) {
    unsigned baseCol = (col < 64? col : 64);
    out << (s->getType() == Setting::TypeArray? '[' : '(');
    unsigned len = s->getLength();
    for (unsigned i=0; i<len; ++i) {
      if (i>0) {
        if (col < 64) {
          out << '\t';
          addTab(col);
        } else {
          out << '\n';
          col = baseCol;
          for (unsigned j=0; j<baseCol/8; ++j)
            out << '\t';
          for (unsigned j=0; j<baseCol%8; ++j)
            out << ' ';
        }
      }
      writer_any(&s->operator[](i), out, col, indent+1);
    }
    out << (s->getType() == Setting::TypeArray? ']' : ')');
    ++col;
  }
  static void writer_group(Setting* s, ostream& out, unsigned& col,
                           unsigned indent) {
    //Since getName() is an O(n) operation, we'll run through the raw group
    //to avoid making this O(nn).

    if (!s->isRoot())
      out << "{\n";

    unsigned len = s->getLength();
    for (unsigned i=0; i<len; ++i) {
      for (unsigned j=0; j<indent; ++j)
        out << ' ';
      col = indent;

      //We need to reload this each time, since it could get swapped
      //out when writing the other settings
      RGroup* grp = sgetext<RGroup>(s, swapin_group);
      out << grp->entries[i].name << '\t';
      col += strlen(grp->entries[i].name);
      addTab(col);
      writer_any(grp->entries[i].dat, out, col, indent+1);

      out << '\n';
    }

    if (!s->isRoot()) {
      for (unsigned j=0; j<indent-1; ++j)
        out << ' ';
      out << "}";

      col = indent;
    } else col=0;
  }
  /* END: File parser and writer */

  /* BEGIN: Config implementation */
  static Setting* getInitRoot() {
    Setting* s = salloc(Setting::TypeGroup, NULL);
    sinit(s);
    return s;
  }
  Config::Config()
  : root(getInitRoot())
  { }
  Config::~Config() {
    if (root && enableConfigDestructor)
      toDelete.push(root);
  }

  void Config::readString(const char* input, const char* filename) {
    toDelete.push(root);

    unsigned line = 1;
    try {
      root = parser_read(Setting::TypeGroup, NULL, input, "", filename, line);
    } catch (...) {
      root = getInitRoot();
      throw;
    }
  }

  void Config::readFile(const char* filename) {
    string input;
    ifstream in(filename);
    if (!in)
      throw FileIOException(strerror(errno));
    getline(in, input, (char)0); //Read entire file
    readString(input.c_str(), filename);
  }

  string Config::writeString() const {
    ostringstream out;
    unsigned col = 0;
    writer_group(root, out, col, 0);
    return out.str();
  }
  void Config::writeFile(const char* filename) const {
    ofstream out(filename);
    if (!out)
      throw FileIOException(strerror(errno));
    unsigned col = 0;
    writer_group(root, out, col, 0);

    if (!out)
      throw FileIOException(strerror(errno));
  }

  const Setting& Config::getRoot() const {
    return *root;
  }
  Setting& Config::getRoot() {
    return *root;
  }

  Setting& Config::lookup(const char* origpath) {
    unsigned len = strlen(origpath)+1;
    char* path = new char[len];
    memcpy(path, origpath, len);

    try {
      Setting* curr = root;
      char* strtokarg = path;
      while (char* sub = strtok(strtokarg, ".")) {
        strtokarg = NULL;
        curr = &curr->operator[](sub);
      }

      delete[] path;
      return *curr;
    } catch (...) {
      delete[] path;
      throw;
    }
  }
  Setting& Config::lookup(const string& path) {
    return lookup(path.c_str());
  }
  const Setting& Config::lookup(const char* path) const {
    return const_cast<Config*>(this)->lookup(path);
  }
  const Setting& Config::lookup(const string& path) const {
    return const_cast<Config*>(this)->lookup(path.c_str());
  }

  bool Config::exists(const char* path) const {
    try {
      lookup(path);
      return true;
    } catch (...) {
      return false;
    }
  }
  bool Config::exists(const string& path) const {
    return exists(path.c_str());
  }

  #define LV(dstt, opt) \
    bool Config::lookupValue(const char* path, dstt& dst) const { \
      try { \
        dst = lookup(path).operator opt(); \
        return true; \
      } catch (...) { \
        return false; \
      } \
    } \
    bool Config::lookupValue(const string& path, dstt& dst) const { \
      try { \
        dst = lookup(path.c_str()).operator opt(); \
        return true; \
      } catch (...) { \
        return false; \
      } \
    }
  LV(bool,              bool              )
  LV(signed,            signed            )
  LV(unsigned,          unsigned          )
  LV(float,             float             )
  LV(signed long long,  signed long long  )
  LV(unsigned long long,unsigned long long)
  LV(const char*,       const char*       )
  LV(string,            const char*       )
  #undef LV

  /* END: Config implementation */
  /* Miscelaneous functions */
  std::size_t getMaxPriStorage() {
    return maxPrimary;
  }
  void setMaxPriStorage(std::size_t sz) {
    maxPrimary = sz;
  }
  std::size_t getCurrPriStorage() {
    return primaryUsed;
  }
  unsigned long long getCurrSecStorage() {
    unsigned long long sz = secondaryUsed;
    sz *= BLOCK_SZ;
    return sz;
  }
  unsigned long long getSwapFileSize() {
    unsigned long long sz = secondarySize;
    sz *= BLOCK_SZ;
    return sz;
  }
  void garbageCollection() {
    for (unsigned i=0; i<1024 && !toDelete.empty(); ++i) {
      sfree(toDelete.front());
      toDelete.pop();
    }
  }
}
