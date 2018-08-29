#include "RefBase.h"

#define INITIAL_STRONG_VALUE (1<<28)

class RefBase::weakref_impl : public RefBase::weakref_type
{
public:
	volatile int32_t    mStrong;
	volatile int32_t    mWeak;
	RefBase* const      mBase;
	volatile int32_t    mFlags;

#if !DEBUG_REFS

	weakref_impl(RefBase* base)
		: mStrong(INITIAL_STRONG_VALUE)
		, mWeak(0)
		, mBase(base)
		, mFlags(0)
	{
	}

	void addStrongRef(const void* /*id*/) { }
	void removeStrongRef(const void* /*id*/) { }
	void renameStrongRefId(const void* /*old_id*/, const void* /*new_id*/) { }
	void addWeakRef(const void* /*id*/) { }
	void removeWeakRef(const void* /*id*/) { }
	void renameWeakRefId(const void* /*old_id*/, const void* /*new_id*/) { }
	void printRefs() const { }
	void trackMe(bool, bool) { }

#else

	weakref_impl(RefBase* base)
		: mStrong(INITIAL_STRONG_VALUE)
		, mWeak(0)
		, mBase(base)
		, mFlags(0)
		, mStrongRefs(NULL)
		, mWeakRefs(NULL)
		, mTrackEnabled(!!DEBUG_REFS_ENABLED_BY_DEFAULT)
		, mRetain(false)
	{
	}

	~weakref_impl()
	{
		bool dumpStack = false;
		if (!mRetain && mStrongRefs != NULL) {
			dumpStack = true;
			ALOGE("Strong references remain:");
			ref_entry* refs = mStrongRefs;
			while (refs) {
				char inc = refs->ref >= 0 ? '+' : '-';
				ALOGD("\t%c ID %p (ref %d):", inc, refs->id, refs->ref);
#if DEBUG_REFS_CALLSTACK_ENABLED
				refs->stack.log(LOG_TAG);
#endif
				refs = refs->next;
			}
		}

		if (!mRetain && mWeakRefs != NULL) {
			dumpStack = true;
			ALOGE("Weak references remain!");
			ref_entry* refs = mWeakRefs;
			while (refs) {
				char inc = refs->ref >= 0 ? '+' : '-';
				ALOGD("\t%c ID %p (ref %d):", inc, refs->id, refs->ref);
#if DEBUG_REFS_CALLSTACK_ENABLED
				refs->stack.log(LOG_TAG);
#endif
				refs = refs->next;
			}
		}
		if (dumpStack) {
			ALOGE("above errors at:");
			CallStack stack(LOG_TAG);
		}
	}

	void addStrongRef(const void* id) {
		//ALOGD_IF(mTrackEnabled,
		//        "addStrongRef: RefBase=%p, id=%p", mBase, id);
		addRef(&mStrongRefs, id, mStrong);
	}

	void removeStrongRef(const void* id) {
		//ALOGD_IF(mTrackEnabled,
		//        "removeStrongRef: RefBase=%p, id=%p", mBase, id);
		if (!mRetain) {
			removeRef(&mStrongRefs, id);
		}
		else {
			addRef(&mStrongRefs, id, -mStrong);
		}
	}

	void renameStrongRefId(const void* old_id, const void* new_id) {
		//ALOGD_IF(mTrackEnabled,
		//        "renameStrongRefId: RefBase=%p, oid=%p, nid=%p",
		//        mBase, old_id, new_id);
		renameRefsId(mStrongRefs, old_id, new_id);
	}

	void addWeakRef(const void* id) {
		addRef(&mWeakRefs, id, mWeak);
	}

	void removeWeakRef(const void* id) {
		if (!mRetain) {
			removeRef(&mWeakRefs, id);
		}
		else {
			addRef(&mWeakRefs, id, -mWeak);
		}
	}

	void renameWeakRefId(const void* old_id, const void* new_id) {
		renameRefsId(mWeakRefs, old_id, new_id);
	}

	void trackMe(bool track, bool retain)
	{
		mTrackEnabled = track;
		mRetain = retain;
	}

	void printRefs() const
	{
		String8 text;

		{
			Mutex::Autolock _l(mMutex);
			char buf[128];
			sprintf(buf, "Strong references on RefBase %p (weakref_type %p):\n", mBase, this);
			text.append(buf);
			printRefsLocked(&text, mStrongRefs);
			sprintf(buf, "Weak references on RefBase %p (weakref_type %p):\n", mBase, this);
			text.append(buf);
			printRefsLocked(&text, mWeakRefs);
		}

		{
			char name[100];
			snprintf(name, 100, DEBUG_REFS_CALLSTACK_PATH "/%p.stack", this);
			int rc = open(name, O_RDWR | O_CREAT | O_APPEND, 644);
			if (rc >= 0) {
				write(rc, text.string(), text.length());
				close(rc);
				ALOGD("STACK TRACE for %p saved in %s", this, name);
			}
			else ALOGE("FAILED TO PRINT STACK TRACE for %p in %s: %s", this,
				name, strerror(errno));
		}
	}

private:
	struct ref_entry
	{
		ref_entry* next;
		const void* id;
#if DEBUG_REFS_CALLSTACK_ENABLED
		CallStack stack;
#endif
		int32_t ref;
	};

	void addRef(ref_entry** refs, const void* id, int32_t mRef)
	{
		if (mTrackEnabled) {
			AutoMutex _l(mMutex);

			ref_entry* ref = new ref_entry;
			// Reference count at the time of the snapshot, but before the
			// update.  Positive value means we increment, negative--we
			// decrement the reference count.
			ref->ref = mRef;
			ref->id = id;
#if DEBUG_REFS_CALLSTACK_ENABLED
			ref->stack.update(2);
#endif
			ref->next = *refs;
			*refs = ref;
		}
	}

	void removeRef(ref_entry** refs, const void* id)
	{
		if (mTrackEnabled) {
			AutoMutex _l(mMutex);

			ref_entry* const head = *refs;
			ref_entry* ref = head;
			while (ref != NULL) {
				if (ref->id == id) {
					*refs = ref->next;
					delete ref;
					return;
				}
				refs = &ref->next;
				ref = *refs;
			}

			ALOGE("RefBase: removing id %p on RefBase %p"
				"(weakref_type %p) that doesn't exist!",
				id, mBase, this);

			ref = head;
			while (ref) {
				char inc = ref->ref >= 0 ? '+' : '-';
				ALOGD("\t%c ID %p (ref %d):", inc, ref->id, ref->ref);
				ref = ref->next;
			}

			CallStack stack(LOG_TAG);
		}
	}

	void renameRefsId(ref_entry* r, const void* old_id, const void* new_id)
	{
		if (mTrackEnabled) {
			AutoMutex _l(mMutex);
			ref_entry* ref = r;
			while (ref != NULL) {
				if (ref->id == old_id) {
					ref->id = new_id;
				}
				ref = ref->next;
			}
		}
	}

	void printRefsLocked(String8* out, const ref_entry* refs) const
	{
		char buf[128];
		while (refs) {
			char inc = refs->ref >= 0 ? '+' : '-';
			sprintf(buf, "\t%c ID %p (ref %d):\n",
				inc, refs->id, refs->ref);
			out->append(buf);
#if DEBUG_REFS_CALLSTACK_ENABLED
			out->append(refs->stack.toString("\t\t"));
#else
			out->append("\t\t(call stacks disabled)");
#endif
			refs = refs->next;
		}
	}

	mutable Mutex mMutex;
	ref_entry* mStrongRefs;
	ref_entry* mWeakRefs;

	bool mTrackEnabled;
	// Collect stack traces on addref and removeref, instead of deleting the stack references
	// on removeref that match the address ones.
	bool mRetain;

#endif
};

// ---------------------------------------------------------------------------
