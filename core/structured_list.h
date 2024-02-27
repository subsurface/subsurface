// SPDX-License-Identifier: GPL-2.0
#ifndef STRUCTURED_LIST_H
#define STRUCTURED_LIST_H

/* Clear whole list; this works for taglist and dive computers */
#define STRUCTURED_LIST_FREE(_type, _start, _free) \
	{                                          \
		_type *_ptr = _start;              \
		while (_ptr) {                     \
			_type *_next = _ptr->next; \
			_free(_ptr);               \
			_ptr = _next;              \
		}                                  \
	}

#define STRUCTURED_LIST_COPY(_type, _first, _dest, _cpy)          \
	{                                                         \
		_type *_sptr = _first;                            \
		_type **_dptr = &_dest;                           \
		while (_sptr) {                                   \
			*_dptr = (_type *)malloc(sizeof(_type));  \
			_cpy(_sptr, *_dptr);                      \
			_sptr = _sptr->next;                      \
			_dptr = &(*_dptr)->next;                  \
		}                                                 \
		*_dptr = 0;                                       \
	}

#endif
