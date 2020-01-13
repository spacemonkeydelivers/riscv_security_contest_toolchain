/* VxWorks provides its own version of malloc, and we can't use this
   one because VxWorks does not provide sbrk.  So we have a hook to
   not compile this code.  */

/* The routines here are simple cover fns to the routines that do the real
   work (the reentrant versions).  */
/* FIXME: Does the warning below (see WARNINGS) about non-reentrancy still
   apply?  A first guess would be "no", but how about reentrancy in the *same*
   thread?  */

#ifdef MALLOC_PROVIDED

int _dummy_malloc = 1;

#else

/*
FUNCTION
<<malloc>>, <<realloc>>, <<free>>---manage memory

INDEX
	malloc
INDEX
	realloc
INDEX
	reallocf
INDEX
	free
INDEX
	memalign
INDEX
	malloc_usable_size
INDEX
	_malloc_r
INDEX
	_realloc_r
INDEX
	_reallocf_r
INDEX
	_free_r
INDEX
	_memalign_r
INDEX
	_malloc_usable_size_r

SYNOPSIS
	#include <stdlib.h>
	void *malloc(size_t <[nbytes]>);
	void *realloc(void *<[aptr]>, size_t <[nbytes]>);
	void *reallocf(void *<[aptr]>, size_t <[nbytes]>);
	void free(void *<[aptr]>);

	void *memalign(size_t <[align]>, size_t <[nbytes]>);

	size_t malloc_usable_size(void *<[aptr]>);

	void *_malloc_r(void *<[reent]>, size_t <[nbytes]>);
	void *_realloc_r(void *<[reent]>, 
                         void *<[aptr]>, size_t <[nbytes]>);
	void *_reallocf_r(void *<[reent]>, 
                         void *<[aptr]>, size_t <[nbytes]>);
	void _free_r(void *<[reent]>, void *<[aptr]>);

	void *_memalign_r(void *<[reent]>,
			  size_t <[align]>, size_t <[nbytes]>);

	size_t _malloc_usable_size_r(void *<[reent]>, void *<[aptr]>);

DESCRIPTION
These functions manage a pool of system memory.

Use <<malloc>> to request allocation of an object with at least
<[nbytes]> bytes of storage available.  If the space is available,
<<malloc>> returns a pointer to a newly allocated block as its result.

If you already have a block of storage allocated by <<malloc>>, but
you no longer need all the space allocated to it, you can make it
smaller by calling <<realloc>> with both the object pointer and the
new desired size as arguments.  <<realloc>> guarantees that the
contents of the smaller object match the beginning of the original object.

Similarly, if you need more space for an object, use <<realloc>> to
request the larger size; again, <<realloc>> guarantees that the
beginning of the new, larger object matches the contents of the
original object.

When you no longer need an object originally allocated by <<malloc>>
or <<realloc>> (or the related function <<calloc>>), return it to the
memory storage pool by calling <<free>> with the address of the object
as the argument.  You can also use <<realloc>> for this purpose by
calling it with <<0>> as the <[nbytes]> argument.

The <<reallocf>> function behaves just like <<realloc>> except if the
function is required to allocate new storage and this fails.  In this
case <<reallocf>> will free the original object passed in whereas
<<realloc>> will not.

The <<memalign>> function returns a block of size <[nbytes]> aligned
to a <[align]> boundary.  The <[align]> argument must be a power of
two.

The <<malloc_usable_size>> function takes a pointer to a block
allocated by <<malloc>>.  It returns the amount of space that is
available in the block.  This may or may not be more than the size
requested from <<malloc>>, due to alignment or minimum size
constraints.

The alternate functions <<_malloc_r>>, <<_realloc_r>>, <<_reallocf_r>>, 
<<_free_r>>, <<_memalign_r>>, and <<_malloc_usable_size_r>> are reentrant
versions.  The extra argument <[reent]> is a pointer to a reentrancy structure.

If you have multiple threads of execution which may call any of these
routines, or if any of these routines may be called reentrantly, then
you must provide implementations of the <<__malloc_lock>> and
<<__malloc_unlock>> functions for your system.  See the documentation
for those functions.

These functions operate by calling the function <<_sbrk_r>> or
<<sbrk>>, which allocates space.  You may need to provide one of these
functions for your system.  <<_sbrk_r>> is called with a positive
value to allocate more space, and with a negative value to release
previously allocated space if it is no longer required.
@xref{Stubs}.

RETURNS
<<malloc>> returns a pointer to the newly allocated space, if
successful; otherwise it returns <<NULL>>.  If your application needs
to generate empty objects, you may use <<malloc(0)>> for this purpose.

<<realloc>> returns a pointer to the new block of memory, or <<NULL>>
if a new block could not be allocated.  <<NULL>> is also the result
when you use `<<realloc(<[aptr]>,0)>>' (which has the same effect as
`<<free(<[aptr]>)>>').  You should always check the result of
<<realloc>>; successful reallocation is not guaranteed even when
you request a smaller object.

<<free>> does not return a result.

<<memalign>> returns a pointer to the newly allocated space.

<<malloc_usable_size>> returns the usable size.

PORTABILITY
<<malloc>>, <<realloc>>, and <<free>> are specified by the ANSI C
standard, but other conforming implementations of <<malloc>> may
behave differently when <[nbytes]> is zero.

<<memalign>> is part of SVR4.

<<malloc_usable_size>> is not portable.

Supporting OS subroutines required: <<sbrk>>.  */

#include <_ansi.h>
#include <reent.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>

#ifndef _REENT_ONLY

/*
static inline unsigned soc_hwrand(void) {
    unsigned result = 0;
    __asm__ __volatile__(
            "csrr %[result], rnd\n\t"
            : [result]"=r"(result)
            :
            : );
    return result;
}
*/

#define GRANULE_SIZE 16
int sec_tag = 1; // it would be better to have an initialization routine
static unsigned  __sec_generate_tag() {
    // tag #15 is reserved for OS code
    do {
        // NOTE: instead of incrementing, we could call soc_hwrand function
        // but for demonstration purposes we decided to use a simpler approach
        sec_tag = (sec_tag + 1) & 0xff;
        // sec_tag = soc_hwrand();
    } while (sec_tag == 0 || sec_tag == 15);
    return sec_tag;
}

static unsigned __sec_protect_ptr(void* ptr, unsigned size) {
    unsigned tag = __sec_generate_tag();
    unsigned raw_ptr = (unsigned)ptr;

    unsigned untagged_ptr = (raw_ptr & (~(0xf << 28)));
    unsigned granules_to_tag = size / GRANULE_SIZE + ((size % GRANULE_SIZE) ? 1 : 0);
    unsigned address_to_tag = untagged_ptr;
    for (unsigned i = 0; i < granules_to_tag; ++i) {
        __asm__ __volatile__(
                "st %[tag], 0(%[address])"
                :
                : [tag]"r"(tag), [address]"r"(address_to_tag)
                : "t0", "memory");
        address_to_tag += 16;
    }
    unsigned result =  untagged_ptr | (tag << 28);
    return result;
}
unsigned __sec_untag_ptr(void* ptr) {
    return ((unsigned)ptr) & ((1 << 28) - 1);
}

#define alignto(p, bits)      (((p) >> bits) << bits)
#define aligntonext(p, bits)  alignto(((p) + (1 << bits) - 1), bits)
void *
malloc (size_t nbytes)		/* get a block */
{
  size_t alloc_size = aligntonext(nbytes, 4);
  // printf("requesting nbytes = %d, allocating = %d\n", nbytes, alloc_size);
  void * raw_ptr =  _malloc_r (_REENT, alloc_size);
  if (!raw_ptr) {
      return (void*)0;
  }
  void * protected_ptr = (void*)__sec_protect_ptr(raw_ptr, alloc_size); // yes, we tag the whole alloc size
  // printf("s_malloc: ptr before %p, ptr after %p; [%d]?\n", raw_ptr, protected_ptr, nbytes);
  return protected_ptr;
}

void
free (void *aptr)
{
    void* raw_ptr = (void*)__sec_untag_ptr(aptr);
    // printf("s_free: ptr before %p, ptr_after %p\n", aptr, raw_ptr);
    _free_r (_REENT, raw_ptr);
}

#endif

#endif /* ! defined (MALLOC_PROVIDED) */
