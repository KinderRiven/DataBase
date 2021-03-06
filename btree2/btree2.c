#include <inttypes.h>

#include "btree2.h"
#include "btree2_slot.h"

//	create an empty page

uint64_t btree2NewPage (Handle *index, uint8_t lvl) {
Btree2Index *btree2 = btree2index(index->map);
Btree2PageType type;
Btree2Page *page;
uint32_t size;
DbAddr addr;

	size = btree2->pageSize;
	type = Btree2_interior;

	if (!lvl) {
		size <<= btree2->leafXtra;
		type = Btree2_leafPage;;
	}
  
	//  allocate page
  
	if ((addr.bits = allocObj(index->map, listFree(index,type), NULL, type, size, true)))
		page = getObj(index->map, addr);
	else
		return 0;
  
	page->alloc->nxt = (size >> btree2->skipBits) - 1;
	page->alloc->state = Btree2_pageactive;
	page->pageBits = btree2->pageBits;
	page->leafXtra = btree2->leafXtra;
	page->skipBits = btree2->skipBits;
	page->pageType = type;
    page->size = size;
	page->lvl = lvl;

 	return addr.bits;
}

//	initialize btree2 root page

DbStatus btree2Init(Handle *index, Params *params) {
Btree2Index *btree2 = btree2index(index->map);
ObjId pageNo, *pageSlot;
Btree2Page *page;
DbAddr addr;

	if (params[Btree2Bits].intVal > Btree2_maxbits || params[Btree2Bits].intVal < Btree2_minbits ) {
		fprintf(stderr, "createIndex: bits = %" PRIu64 " > max = %d\n", params[Btree2Bits].intVal, Btree2_maxbits);
		exit(1);
	}

	if (params[Btree2Bits].intVal + params[Btree2Xtra].intVal > Btree2_maxbits || params[Btree2Bits].intVal < Btree2_minbits ) {
		fprintf(stderr, "createIndex: bits = %" PRIu64 " + xtra = %" PRIu64 " > max = %d\n", params[Btree2Bits].intVal, params[Btree2Xtra].intVal, Btree2_maxbits);
		exit(1);
	}

	btree2->pageSize = 1 << params[Btree2Bits].intVal;
	btree2->pageBits = (uint32_t)params[Btree2Bits].intVal;
	btree2->leafXtra = (uint32_t)params[Btree2Xtra].intVal;

	//	initial btree2 root/leaf page

	if ((addr.bits = btree2NewPage(index, 0)))
		page = getObj(index->map, addr);
	else
		return DB_ERROR_outofmemory;

	if ((pageNo.bits = btree2AllocPageNo(index)))
		pageSlot = fetchIdSlot(index->map, pageNo);
	else
		return DB_ERROR_outofmemory;

	page->pageNo.bits = pageNo.bits;
	page->attributes = Btree2_rootPage;
	pageSlot->bits = addr.bits;

	btree2->root.bits = pageNo.bits;
	btree2->right.bits = pageNo.bits;
	btree2->left.bits = pageNo.bits;

	// release arena

	index->map->arena->type[0] = Hndl_btree2Index;
	return DB_OK;
}

//	allocate btree2 pageNo

uint64_t btree2AllocPageNo(Handle *index) {
	return allocObjId(index->map, listFree(index, ObjIdType), listWait(index, ObjIdType));
}

bool btree2RecyclePageNo(Handle *index, ObjId pageNo) {
	return addSlotToFrame(index->map, listHead(index, ObjIdType), listWait(index, ObjIdType), pageNo.bits);
}

bool btree2RecyclePage(Handle *index, int type, DbAddr addr) {
	return addSlotToFrame(index->map, listHead(index, type), listWait(index, type), addr.bits);
}

