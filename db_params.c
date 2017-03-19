#include "db.h"
#include "db_object.h"
#include "db_redblack.h"
#include "db_arena.h"
#include "db_map.h"

extern DbAddr openMaps[1];
extern DbMap *hndlMap;

//	if this is a new map file, copy param
//	structure to a new ArenaDef in parent
//	otherwise, return existing arenaDef
//	from the parent.

RedBlack *procParam(DbMap *parent, char *name, int nameLen, Params *params) {
SkipEntry *skipPayLoad;
PathStk pathStk[1];
ArenaDef *arenaDef;
RedBlack *rbEntry;
Catalog *catalog;

	//	see if ArenaDef already exists as a child in the parent

	while (true) {
	  lockLatch (parent->arena->arenaDef->nameTree->latch);

	  if ((rbEntry = rbFind(parent->db, parent->arena->arenaDef->nameTree, name, nameLen, pathStk))) {
		arenaDef = (ArenaDef *)(rbEntry + 1);

		if (*arenaDef->dead & KILL_BIT) {
		  unlockLatch (parent->arena->arenaDef->nameTree->latch);
		  yield ();
		  continue;
		}

		unlockLatch (parent->arena->arenaDef->nameTree->latch);
		return rbEntry;
	  }

	  break;
	}

	// create new rbEntry in parent
	// with an arenaDef payload

	catalog = (Catalog *)(hndlMap->arena + 1);

	if ((rbEntry = rbNew(parent->db, name, nameLen, sizeof(ArenaDef)))) {
		arenaDef = (ArenaDef *)(rbEntry + 1);
	} else {
		unlockLatch(parent->arena->arenaDef->nameTree->latch);
		return NULL;
	}

	//	fill in new arenaDef r/b entry

	time (&arenaDef->creation);
	memcpy (arenaDef->params, params, sizeof(arenaDef->params));
	initLock(arenaDef->idList->lock);

	arenaDef->mapIdx = arrayAlloc(hndlMap, catalog->openMap, sizeof(void *));
	arenaDef->id = atomicAdd64(&parent->arena->arenaDef->childId, 1);

	//	add arenaDef to parent's child arenaDef by name tree

	rbAdd(parent->db, parent->arena->arenaDef->nameTree, rbEntry, pathStk);

	//	add new rbEntry to parent's child id array

	writeLock(parent->arena->arenaDef->idList->lock);
	skipPayLoad = skipAdd (parent->db, parent->arena->arenaDef->idList->head, arenaDef->id);
	*skipPayLoad->val = rbEntry->addr.bits;
	writeUnlock(parent->arena->arenaDef->idList->lock);

	unlockLatch(parent->arena->arenaDef->nameTree->latch);
	return rbEntry;
}

