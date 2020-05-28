
#include "refs-internal.h"

struct debug_ref_store {
	struct ref_store base;
	struct ref_store *refs;
};

extern struct ref_storage_be refs_be_debug;
struct ref_store *debug_wrap(struct ref_store *store);

struct ref_store *debug_wrap(struct ref_store *store)
{
	struct debug_ref_store *res = malloc(sizeof(struct debug_ref_store));
	res->refs = store;
	base_ref_store_init((struct ref_store *)res, &refs_be_debug);
	return (struct ref_store *)res;
}

static int debug_init_db(struct ref_store *refs, struct strbuf *err)
{
	struct debug_ref_store *drefs = (struct debug_ref_store *)refs;
	int res = drefs->refs->be->init_db(drefs->refs, err);
	return res;
}

static int debug_transaction_prepare(struct ref_store *refs,
				     struct ref_transaction *transaction,
				     struct strbuf *err)
{
	struct debug_ref_store *drefs = (struct debug_ref_store *)refs;
	int res;
	transaction->ref_store = drefs->refs;
	res = drefs->refs->be->transaction_prepare(drefs->refs, transaction,
						   err);
	return res;
}

static void print_update(int i, const char *refname,
			 const struct object_id *old_oid,
			 const struct object_id *new_oid, unsigned int flags,
			 unsigned int type)
{
	char o[200] = "null";
	char n[200] = "null";
	if (old_oid)
		oid_to_hex_r(o, old_oid);
	if (new_oid)
		oid_to_hex_r(n, new_oid);

	type &= 0xf; /* see refs.h REF_* */
	flags &= REF_HAVE_NEW | REF_HAVE_OLD | REF_NO_DEREF |
		 REF_FORCE_CREATE_REFLOG | REF_LOG_ONLY;
	printf("%d: %s %s -> %s (F=0x%x, T=0x%x)\n", i, refname, o, n, flags,
	       type);
}

static void print_transaction(struct ref_transaction *transaction)
{
	printf("transaction {\n");
	for (int i = 0; i < transaction->nr; i++) {
		struct ref_update *u = transaction->updates[i];
		print_update(i, u->refname, &u->old_oid, &u->new_oid, u->flags,
			     u->type);
	}
	printf("}\n");
}

static int debug_transaction_finish(struct ref_store *refs,
				    struct ref_transaction *transaction,
				    struct strbuf *err)
{
	struct debug_ref_store *drefs = (struct debug_ref_store *)refs;
	int res;
	transaction->ref_store = drefs->refs;
	res = drefs->refs->be->transaction_finish(drefs->refs, transaction,
						  err);
	print_transaction(transaction);
	printf("finish: %d\n", res);
	return res;
}

static int debug_transaction_abort(struct ref_store *refs,
				   struct ref_transaction *transaction,
				   struct strbuf *err)
{
	struct debug_ref_store *drefs = (struct debug_ref_store *)refs;
	int res;
	transaction->ref_store = drefs->refs;
	res = drefs->refs->be->transaction_abort(drefs->refs, transaction, err);
	return res;
}

static int debug_initial_transaction_commit(struct ref_store *refs,
					    struct ref_transaction *transaction,
					    struct strbuf *err)
{
	struct debug_ref_store *drefs = (struct debug_ref_store *)refs;
	int res;
	transaction->ref_store = drefs->refs;
	res = drefs->refs->be->initial_transaction_commit(drefs->refs,
							  transaction, err);
	return res;
}

static int debug_pack_refs(struct ref_store *ref_store, unsigned int flags)
{
	struct debug_ref_store *drefs = (struct debug_ref_store *)ref_store;
	int res = drefs->refs->be->pack_refs(drefs->refs, flags);
	return res;
}

static int debug_create_symref(struct ref_store *ref_store,
			       const char *ref_target,
			       const char *refs_heads_master,
			       const char *logmsg)
{
	struct debug_ref_store *drefs = (struct debug_ref_store *)ref_store;
	int res = drefs->refs->be->create_symref(drefs->refs, ref_target,
						 refs_heads_master, logmsg);
	return res;
}

static int debug_delete_refs(struct ref_store *ref_store, const char *msg,
			     struct string_list *refnames, unsigned int flags)
{
	struct debug_ref_store *drefs = (struct debug_ref_store *)ref_store;
	int res =
		drefs->refs->be->delete_refs(drefs->refs, msg, refnames, flags);
	return res;
}
static int debug_rename_ref(struct ref_store *ref_store, const char *oldref,
			    const char *newref, const char *logmsg)
{
	struct debug_ref_store *drefs = (struct debug_ref_store *)ref_store;
	int res = drefs->refs->be->rename_ref(drefs->refs, oldref, newref,
					      logmsg);
	return res;
}
static int debug_copy_ref(struct ref_store *ref_store, const char *oldref,
			  const char *newref, const char *logmsg)
{
	struct debug_ref_store *drefs = (struct debug_ref_store *)ref_store;
	int res =
		drefs->refs->be->copy_ref(drefs->refs, oldref, newref, logmsg);
	return res;
}

static int debug_write_pseudoref(struct ref_store *ref_store,
				 const char *pseudoref,
				 const struct object_id *oid,
				 const struct object_id *old_oid,
				 struct strbuf *err)
{
	struct debug_ref_store *drefs = (struct debug_ref_store *)ref_store;
	int res = drefs->refs->be->write_pseudoref(drefs->refs, pseudoref, oid,
						   old_oid, err);
	char o[100] = "null";
	char n[100] = "null";
	if (oid)
		oid_to_hex_r(o, oid);
	if (old_oid)
		oid_to_hex_r(n, old_oid);
	printf("write_pseudoref: %s, %s => %s, err %s: %d\n", pseudoref, o, n,
	       err->buf, res);
	return res;
}

static int debug_delete_pseudoref(struct ref_store *ref_store,
				  const char *pseudoref,
				  const struct object_id *old_oid)
{
	struct debug_ref_store *drefs = (struct debug_ref_store *)ref_store;
	int res = drefs->refs->be->delete_pseudoref(drefs->refs, pseudoref,
						    old_oid);
	char hex[100] = "null";
	if (old_oid)
		oid_to_hex_r(hex, old_oid);
	printf("delete_pseudoref: %s (%s): %d\n", pseudoref, hex, res);
	return res;
}

static struct ref_iterator *
debug_ref_iterator_begin(struct ref_store *ref_store, const char *prefix,
			 unsigned int flags)
{
	struct debug_ref_store *drefs = (struct debug_ref_store *)ref_store;
	struct ref_iterator *res =
		drefs->refs->be->iterator_begin(drefs->refs, prefix, flags);
	return res;
}

static int debug_read_raw_ref(struct ref_store *ref_store, const char *refname,
			      struct object_id *oid, struct strbuf *referent,
			      unsigned int *type)
{
	struct debug_ref_store *drefs = (struct debug_ref_store *)ref_store;
	int res = 0;

	oidcpy(oid, &null_oid);
	res = drefs->refs->be->read_raw_ref(drefs->refs, refname, oid, referent,
					    type);

	if (res == 0) {
		printf("read_raw_ref: %s: %s (=> %s) type %x: %d\n", refname,
		       oid_to_hex(oid), referent->buf, *type, res);
	} else {
		printf("read_raw_ref: %s err %d\n", refname, res);
	}
	return res;
}

static struct ref_iterator *
debug_reflog_iterator_begin(struct ref_store *ref_store)
{
	struct debug_ref_store *drefs = (struct debug_ref_store *)ref_store;
	struct ref_iterator *res =
		drefs->refs->be->reflog_iterator_begin(drefs->refs);
	return res;
}

static int debug_for_each_reflog_ent(struct ref_store *ref_store,
				     const char *refname, each_reflog_ent_fn fn,
				     void *cb_data)
{
	struct debug_ref_store *drefs = (struct debug_ref_store *)ref_store;
	int res = drefs->refs->be->for_each_reflog_ent(drefs->refs, refname, fn,
						       cb_data);
	return res;
}
static int debug_for_each_reflog_ent_reverse(struct ref_store *ref_store,
					     const char *refname,
					     each_reflog_ent_fn fn,
					     void *cb_data)
{
	struct debug_ref_store *drefs = (struct debug_ref_store *)ref_store;
	int res = drefs->refs->be->for_each_reflog_ent_reverse(
		drefs->refs, refname, fn, cb_data);
	return res;
}

static int debug_reflog_exists(struct ref_store *ref_store, const char *refname)
{
	struct debug_ref_store *drefs = (struct debug_ref_store *)ref_store;
	int res = drefs->refs->be->reflog_exists(drefs->refs, refname);
	return res;
}

static int debug_create_reflog(struct ref_store *ref_store, const char *refname,
			       int force_create, struct strbuf *err)
{
	struct debug_ref_store *drefs = (struct debug_ref_store *)ref_store;
	int res = drefs->refs->be->create_reflog(drefs->refs, refname,
						 force_create, err);
	return res;
}

static int debug_delete_reflog(struct ref_store *ref_store, const char *refname)
{
	struct debug_ref_store *drefs = (struct debug_ref_store *)ref_store;
	int res = drefs->refs->be->delete_reflog(drefs->refs, refname);
	return res;
}

static int debug_reflog_expire(struct ref_store *ref_store, const char *refname,
			       const struct object_id *oid, unsigned int flags,
			       reflog_expiry_prepare_fn prepare_fn,
			       reflog_expiry_should_prune_fn should_prune_fn,
			       reflog_expiry_cleanup_fn cleanup_fn,
			       void *policy_cb_data)
{
	struct debug_ref_store *drefs = (struct debug_ref_store *)ref_store;
	int res = drefs->refs->be->reflog_expire(drefs->refs, refname, oid,
						 flags, prepare_fn,
						 should_prune_fn, cleanup_fn,
						 policy_cb_data);
	return res;
}

struct ref_storage_be refs_be_debug = {
	NULL,
	"debug",
	NULL,
	debug_init_db,
	debug_transaction_prepare,
	debug_transaction_finish,
	debug_transaction_abort,
	debug_initial_transaction_commit,

	debug_pack_refs,
	debug_create_symref,
	debug_delete_refs,
	debug_rename_ref,
	debug_copy_ref,

	debug_write_pseudoref,
	debug_delete_pseudoref,

	debug_ref_iterator_begin,
	debug_read_raw_ref,

	debug_reflog_iterator_begin,
	debug_for_each_reflog_ent,
	debug_for_each_reflog_ent_reverse,
	debug_reflog_exists,
	debug_create_reflog,
	debug_delete_reflog,
	debug_reflog_expire,
};
