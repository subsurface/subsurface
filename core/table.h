// SPDX-License-Identifier: GPL-2.0
/* This header defines a number of macros that generate table-manipulation functions.
 * There is no design behind these macros - the functions were take as-is and transformed
 * into macros. Thus, this whole header could do with some redesign. */
#ifndef CORE_TABLE_H
#define CORE_TABLE_H

#define MAKE_GROW_TABLE(table_type, item_type, array_name) \
	item_type *grow_##table_type(struct table_type *table)					\
	{											\
		int nr = table->nr, allocated = table->allocated;				\
		item_type *items = table->array_name;						\
												\
		if (nr >= allocated) {								\
			allocated = (nr + 32) * 3 / 2;						\
			items = (item_type *)realloc(items, allocated * sizeof(item_type));	\
			if (!items)								\
				exit(1);							\
			table->array_name = items;						\
			table->allocated = allocated;						\
		}										\
		return items;									\
	}

/* get the index where we want to insert an object so that everything stays
 * ordered according to a comparison function() */
#define MAKE_GET_INSERTION_INDEX(table_type, item_type, array_name, fun)		\
	int table_type##_get_insertion_index(struct table_type *table, item_type item)	\
	{										\
		/* we might want to use binary search here */				\
		for (int i = 0; i < table->nr; i++) {					\
			if (fun(item, table->array_name[i]))				\
				return i;						\
		}									\
		return table->nr;							\
	}

/* add object at the given index to a table. */
#define MAKE_ADD_TO(table_type, item_type, array_name)					\
	void add_to_##table_type(struct table_type *table, int idx, item_type item)	\
	{										\
		int i;									\
		grow_##table_type(table);						\
		table->nr++;								\
											\
		for (i = idx; i < table->nr; i++) {					\
			item_type tmp = table->array_name[i];				\
			table->array_name[i] = item;					\
			item = tmp;							\
		}									\
	}

#define MAKE_REMOVE_FROM(table_type, array_name)						\
	void remove_from_##table_type(struct table_type *table, int idx)			\
	{											\
		int i;										\
		for (i = idx; i < table->nr - 1; i++)						\
			table->array_name[i] = table->array_name[i + 1];			\
		memset(&table->array_name[--table->nr], 0, sizeof(table->array_name[0]));	\
	}

#define MAKE_GET_IDX(table_type, item_type, array_name)						\
	int get_idx_in_##table_type(const struct table_type *table, const item_type item)	\
	{											\
		for (int i = 0; i < table->nr; ++i) {						\
			if (table->array_name[i] == item)					\
				return i;							\
		}										\
		return -1;									\
	}

#define MAKE_SORT(table_type, item_type, array_name, fun)					\
	static int sortfn_##table_type(const void *_a, const void *_b)				\
	{											\
		const item_type a = *(const item_type *)_a;					\
		const item_type b = *(const item_type *)_b;					\
		return fun(a, b);								\
	}											\
												\
	void sort_##table_type(struct table_type *table)					\
	{											\
		qsort(table->array_name, table->nr, sizeof(item_type), sortfn_##table_type);	\
	}

#define MAKE_REMOVE(table_type, item_type, item_name)				\
	int remove_##item_name(const item_type item, struct table_type *table)	\
	{									\
		int idx = get_idx_in_##table_type(table, item);			\
		if (idx >= 0)							\
			remove_from_##table_type(table, idx);			\
		return idx;							\
	}

#define MAKE_CLEAR_TABLE(table_type, array_name, item_name)			\
	void clear_##table_type(struct table_type *table)			\
	{									\
		for (int i = 0; i < table->nr; i++)				\
			free_##item_name(table->array_name[i]);			\
		free(table->array_name);					\
		table->array_name = NULL;					\
		table->allocated = 0;						\
		table->nr = 0;							\
	}

/* Move data of one table to the other - source table is empty after call. */
#define MAKE_MOVE_TABLE(table_type, array_name)					\
	void move_##table_type(struct table_type *src, struct table_type *dst)	\
	{									\
		clear_##table_type(dst);					\
		free(dst->array_name);						\
		*dst = *src;							\
		src->nr = src->allocated = 0;					\
		src->array_name = NULL;						\
	}

#endif
