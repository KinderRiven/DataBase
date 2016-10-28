#include "db.h"
#include "db_txn.h"
#include "db_object.h"
#include "db_arena.h"
#include "db_map.h"

void addDocToTxn(DbMap *database, Txn *txn, ObjId docId, TxnCmd cmd) {

	docId.cmd = cmd;
	addSlotToFrame (database, txn->frame, NULL, docId.bits);
}

//  find appropriate document version per txn beginning timestamp

Doc *findDocVer(DbMap *docStore, ObjId docId, Txn *txn) {
DbAddr *addr = fetchIdSlot(docStore, docId);
DbMap *db = docStore->db;
Doc *doc = NULL;
uint64_t txnTs;
Txn *docTxn;

  //	examine prior versions

  while (addr->bits) {
	doc = getObj(docStore, *addr);

	// is this outside a txn? or
	// is version in same txn?

	if (!txn || doc->txnId.bits == txn->txnId.bits)
		return doc;

	// is the version permanent?

	if (!doc->txnId.bits)
		return doc;

	// is version committed before our txn began?

	if (doc->txnId.bits) {
		docTxn = fetchIdSlot(db, doc->txnId);

		if (isCommitted(docTxn->timestamp))
		  if (docTxn->timestamp < txn->timestamp)
			return doc;
	}

	//	advance txn ts past doc version ts
	//	and move onto next doc version

	while (isReader((txnTs = txn->timestamp)) && txnTs < docTxn->timestamp)
		compareAndSwap(&txn->timestamp, txnTs, docTxn->timestamp);

	addr = doc->prevDoc;
  }

  return NULL;
}

