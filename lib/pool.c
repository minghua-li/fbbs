/*
 * Code taken from nginx src/core/ngx_palloc.c with modifications.
 *
 * Copyright (C) 2002-2010 Igor Sysoev
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdlib.h>
#include <unistd.h>

#include "fbbs/pool.h"
#include "fbbs/util.h"
#include <string.h>

/**
 * @defgroup pool Memory pool
 * @{
 */

enum {
	ALIGNMENT = sizeof(unsigned long),
};

typedef struct pool_large_t {
	void *ptr;
	struct pool_large_t *next;
} pool_large_t;

typedef struct pool_block_t {
	uchar_t *last;
	uchar_t *end;
	struct pool_block_t *next;
} pool_block_t;

struct pool_t {
	struct pool_block_t *head;
	pool_large_t *large;
};

static int _max_pool_alloc = 0;

/**
 * Align pointer.
 * @param ptr The pointer to be aligned.
 * @return The aligned pointer.
 */
static inline void *_align_ptr(void *ptr)
{
	return (void *)(((uintptr_t)ptr + ALIGNMENT - 1) & ~(ALIGNMENT - 1));
}

/**
 * Create a memory pool.
 * @param size Size of each memory block. Use default value if 0.
 * @return A pointer to created pool, NULL on error.
 */
pool_t *pool_create(size_t size)
{
	if (!_max_pool_alloc)
		_max_pool_alloc = sysconf(_SC_PAGESIZE);

	if (!size)
		size = _max_pool_alloc * 2;

	if (size <= sizeof(pool_block_t))
		return NULL;

	pool_block_t *b = malloc(size);
	if (!b)
		return NULL;

	b->last = _align_ptr((uchar_t *)b + sizeof(*b));
	b->end = (uchar_t *)b + size;
	b->next = NULL;

	pool_t *p = (pool_t *) b->last;
	b->last = (uchar_t *)p + sizeof(*p);
	p->head = b;
	p->large = NULL;

	return p;
}

/**
 * Clear memory pool.
 * Large memories are freed. Blocks are cleared.
 * @param p The memory pool.
 */
void pool_clear(pool_t *p)
{
	for (pool_large_t *l = p->large; l; l = l->next) {
		if (l->ptr)
			free(l->ptr);
	}
	p->large = NULL;

	for (pool_block_t *b = p->head->next; b; b = b->next) {
		b->last = (uchar_t *)b + sizeof(*b);
	}
	p->head->last = _align_ptr((uchar_t *)(p->head) + sizeof(pool_block_t));
	p->head->last += sizeof(*p);
}

/**
 * Destroy memory pool.
 * @param p The memory pool.
 */
void pool_destroy(pool_t *p)
{
	for (pool_large_t *l = p->large; l; l = l->next) {
		if (l->ptr)
			free(l->ptr);
	}
	p->large = NULL;

	pool_block_t *b = p->head, *next;
	while (b) {
		next = b->next;
		free(b);
		b = next;
	}
}

/**
 * Allocate memory from pool by creating a new block.
 * @param p The pool to allocate from.
 * @param size Required memory size.
 * @return On success, a pointer to allocated space, NULL otherwise.
 */
static void *_pool_alloc_block(pool_t *p, size_t size)
{
	size_t bsize = p->head->end - (uchar_t *)(p->head);
	pool_block_t *b = malloc(bsize);
	if (!b)
		return NULL;

	b->last = (uchar_t *)b + sizeof(*b);
	b->end = (uchar_t *)b + bsize;
	b->next = p->head->next;
	p->head->next = b;

	uchar_t *ret = _align_ptr(b->last);
	b->last = ret + size;
	return ret;
}

/**
 * Allocate a large memory directly.
 * @param p The memory pool.
 * @param size Required memory size.
 * @return On success, a pointer to allocated space, NULL otherwise.
 */
static void *_pool_alloc_large(pool_t *p, size_t size)
{
	pool_large_t *l;
	l = pool_alloc(p, sizeof(*l));
	if (!l)
		return NULL;

	l->next = p->large;
	p->large = l;

	l->ptr = malloc(size);
	return l->ptr;
}

/**
 * Allocate memory from pool.
 * @param p The pool to allocate from.
 * @param size Required memory size.
 * @return On success, a pointer to allocated space, NULL otherwise.
 */
void *pool_alloc(pool_t *p, size_t size)
{
	uchar_t *ret;
	pool_block_t *b;
	if (size < _max_pool_alloc) {
		b = p->head;
		do {
			ret = _align_ptr(b->last);
			if (b->end - ret >= size) {
				b->last = (uchar_t *)ret + size;
				return ret;
			}
			b = b->next;
		} while (b);
		return _pool_alloc_block(p, size);
	}
	return _pool_alloc_large(p, size);
}

/**
 * Duplicate a string using memory allocated from a pool.
 * @param p The pool to allocate from.
 * @param str The string to duplicate.
 * @param size Length of the string. If 0, the length will be calculated.
 * @return Pointer to the newly allocated string.
 */
char *pool_strdup(pool_t *p, const char *str, size_t size)
{
	if (!str)
		return NULL;

	if (!size)
		size = strlen(str) + 1;

	char *dst = pool_alloc(p, size);
	if (dst)
		memcpy(dst, str, size);
	return dst;
}

/** @} */
