#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <assert.h>
//#include <intrin.h>
#include <stdlib.h>

#include "db_error.h"
#include "db_malloc.h"

#define MAX_key		4096	// maximum key size in bytes

//	types of handles/arenas

typedef enum {
	Hndl_newarena = 0,
	Hndl_catalog,
	Hndl_database,
	Hndl_docStore,
	Hndl_artIndex,
	Hndl_btree1Index,
	Hndl_btree2Index,
	Hndl_colIndex,
	Hndl_iterator,
	Hndl_cursor,
	Hndl_txns
} HandleType;

//	general object pointer

typedef union {
	struct {
		uint32_t offset;	// offset in the segment
		uint16_t segment;	// arena segment number
		union {
			struct {
				uint8_t mutex :1;	// mutex bit
				uint8_t kill  :1;	// kill entry
				uint8_t type  :6;	// object type
			};
			volatile uint8_t latch[1];
		};
		union {
			uint8_t nbyte;		// number of bytes in a span node
			uint8_t nslot;		// number of frame slots in use
			uint8_t maxidx;		// maximum slot index in use
			uint8_t firstx;		// first array inUse to chk
			uint8_t ttype;		// index transaction type
			int8_t rbcmp;		// red/black comparison
		};
	};
	uint64_t addr:48;			// segment/offset
	uint64_t bits;
} DbAddr;

#define TYPE_SHIFT (6*8 + 2)	// number of bits to shift type left and zero all bits
#define BYTE_SHIFT (2)			// number of bits to shift type left and zero latch
#define MUTEX_BIT  0x01
#define KILL_BIT   0x02
#define TYPE_BITS  0xFC

#define ADDR_MUTEX_SET	0x0001000000000000ULL
#define ADDR_KILL_SET	0x0002000000000000ULL
#define ADDR_BITS		0x0000ffffffffffffULL

typedef union {
	struct {
		uint32_t idx;	// record ID in the segment
		uint16_t seg;	// arena segment number
		uint16_t xtra;	// xtra bits (available for txn)
	};
	uint64_t addr:48;	// address part of struct above
	uint64_t bits;
} ObjId;

typedef struct SkipHead_ SkipHead;
typedef struct RedBlack_ RedBlack;
typedef union Handle_ Handle;
typedef struct DbMap_ DbMap;

//	param slots

typedef enum {
	Size = 0,		// total Params structure size
	OnDisk,			// Arena resides on disk
	InitSize,		// initial arena size
	HndlXtra,		// extra bytes in handle struct
	ObjIdSize,		// size of arena ObjId array element
	MapXtra,		// local process extra map storage
	ArenaXtra,		// extra bytes in arena
	ResetVersion,	// reset arena version

	IdxKeyUnique = 10,
	IdxKeyDeferred,		// uniqueness constraints deferred to commit
	IdxKeyAddr,			// index key definition address
	IdxKeySparse,
	IdxKeyPartial,		// offset of partial document
	IdxKeyFlds,			// store field lengths in keys
	IdxType,			// 0 for artree, 1 & 2 for btree
	IdxNoDocs,			// stand-alone index file

	Btree1Bits = 20,	// Btree1 set
	Btree1Xtra,

	Btree2Bits = 23,	// Btree2 set
	Btree2Xtra,

	CursorDeDup = 25,	// de-duplicate cursor results

	UserParams = 30,
	MaxParam = 64		// count of param slots defined
} ParamSlot;

typedef union {
	uint64_t intVal;
	uint32_t offset;
	double dblVal;
	char charVal;
	bool boolVal;
	DbAddr addr;
	void *obj;
} Params;

// cursor move/positioning operations

typedef enum {
	OpLeft		= 'l',
	OpRight 	= 'r',
	OpNext		= 'n',
	OpPrev		= 'p',
	OpFind		= 'f',
	OpOne		= 'o',
	OpBefore	= 'b',
	OpAfter		= 'a'
} CursorOp;

// user's DbHandle
//	contains the Handle ObjId bits

typedef struct {
	uint64_t hndlBits;
} DbHandle;

uint32_t store64(uint8_t *key, uint32_t keylen, int64_t what, bool binaryFlds);
uint64_t get64(uint8_t *key, uint32_t len, bool binaryFlds);
uint32_t size64(uint8_t *key, uint32_t len);
uint32_t calc64(int64_t value, bool binaryFlds);

#define db_abort(expr, msg, val) (fprintf(stderr, "db_abort: line:%d file:%s\nexpr:(%s) is false: %s\n", __LINE__, __FILE__, #expr, msg), abort(), val)
