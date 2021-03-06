/* ============================================================================
*  list_t.c
* ============================================================================

*  Author:         (c) 2015 Sergio Pedri and Andrea Salvati
*/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include "list_t.h"
#include "Introsort\introsort.h"

/* ================== list_t internal types ================== */

// Single list_t node
struct listElem
{
	struct listElem* previous;
	T info;
	struct listElem* next;
};

// Type declarations for the list_t node and a node pointer
typedef struct listElem listNode;
typedef listNode* nodePointer;

/* ---------------------------------------------------------------------
*  listBase
*  ---------------------------------------------------------------------
*  Description:
*    The head of a list_t: it stores a pointer to the first node, one
*    to the last node (so that functions like get_first(), get_last() and
*    add() have a O(1) cost), the current length of the list (this way
*    getting the size has a O(1) cost as well) and a sync variable used
*    to check if a list iterator is valid for the current list. */
struct listBase
{
	nodePointer head;
	nodePointer tail;
	int length;
	unsigned int sync;
};

/* ---------------------------------------------------------------------
*  listIterator
*  ---------------------------------------------------------------------
*  Description:
*    An iterator for a list_t list. It stores a pointer to the target node,
*    one to the target list, the current position inside the list, a sync
*    variable that checks if the iterator is still valid and a started
*    variable used to calculate the next node to return. */
struct listIterator
{
	nodePointer pointer;
	list_t list;
	int position;
	unsigned int sync;
	bool_t started;
};

// Type declaration for the list_t iterator
typedef struct listIterator iteratorInstance;

/* ============================================================================
*  Generic functions
*  ========================================================================= */

// Create
list_t create()
{
	list_t outList = (list_t)malloc(sizeof(struct listBase));
	outList->head = NULL;
	outList->tail = NULL;
	outList->length = 0;
	outList->sync = 0;
	return outList;
}

#define GET_ITERATOR(target) nodePointer iterator = target
#define GET_HEAD_ITERATOR GET_ITERATOR(list->head)
#define GET_TAIL_ITERATOR GET_ITERATOR(list->tail)
#define MOVE_NEXT iterator = iterator->next
#define MOVE_NEXT_W_INDEX(index) MOVE_NEXT; index++
#define MOVE_BACK iterator = iterator->previous
#define MOVE_BACK_W_INDEX(index) MOVE_BACK; index--
#define SYNC_PLUS list->sync++;

#define CLEAR_LIST   \
list->length = 0;    \
list->head = NULL;   \
list->tail = NULL

// Clear
bool_t clear(list_t list)
{
	if (list == NULL) return FALSE;
	if (list->length == 0) return TRUE;
	SYNC_PLUS;
	if (list->length == 1)
	{
		free(list->head);
		CLEAR_LIST;
		return TRUE;
	}
	GET_ITERATOR(list->head->next);
	while (TRUE)
	{
		free(iterator->previous);
		if (iterator->next == NULL)
		{
			free(iterator);
			break;
		}
		MOVE_NEXT;
	}
	CLEAR_LIST;
	return TRUE;
}

// Destroy
bool_t destroy(list_t* list)
{
	if (clear(*list))
	{
		free(*list);
		*list = NULL;
		return TRUE;
	}
	return FALSE;
}

#define ASSIGN_IF_NOT_NULL(value)    \
if (result != NULL) *result = value

// DestroySequence
void destroy_sequence(bool_t* result, list_t** pending)
{
	if (pending == NULL)
	{
		ASSIGN_IF_NOT_NULL(FALSE);
		return;
	}
	if (*pending == NULL)
	{
		ASSIGN_IF_NOT_NULL(FALSE);
		return;
	}
	bool_t destroyed = FALSE;
	while (*pending)
	{
		destroy(*pending);
		destroyed = TRUE;
		pending++;
	}
	ASSIGN_IF_NOT_NULL(destroyed);
}

// Copy
list_t copy(const list_t source)
{
	if (source == NULL) return NULL;
	list_t outList = create();
	if (source->length == 0) return outList;
	GET_ITERATOR(source->head);
	while (iterator != NULL)
	{
		add(iterator->info, outList);
		MOVE_NEXT;
	}
	return outList;
}

// Create random
list_t create_random(int length, int min, int max)
{
	if (min >= max) return NULL;
	if (length == 0) return create();
	list_t outList = create();
	srand((unsigned)time(NULL));
	while (length != 0)
	{
		add((T)((rand() % (max - min)) + min), outList);
		length--;
	}
	return outList;
}

// CreateFrom
list_t create_from(T* array, int size)
{
	if (array == NULL || size <= 0) return NULL;
	list_t outList = create();
	int i;
	for (i = 0; i < size; i++)
	{
		add(array[i], outList);
	}
	return outList;
}

// ToArray
T* to_array(list_t list, int* size)
{
	if (list == NULL || list->length == 0)
	{
		*size = -1;
		return NULL;
	}
	*size = list->length;
	T* array = (T*)malloc(sizeof(T) * (*size));
	int i = 0;
	GET_HEAD_ITERATOR;
	while (iterator != NULL)
	{
		array[i] = iterator->info;
		i++;
		MOVE_NEXT;
	}
	return array;
}

#define CHECK_EMPTY(list) list == NULL || list->length == 0
#define NULL_IF_EMPTY(list) if (CHECK_EMPTY(list)) return NULL
#define RETURN_IF_EMPTY(list, value) if (CHECK_EMPTY(list)) return value

// IsEmpty
bool_t is_empty(list_t list)
{
	return CHECK_EMPTY(list);
}

// IsElement
bool_t is_element(const T item, list_t list)
{
	RETURN_IF_EMPTY(list, FALSE);
	GET_HEAD_ITERATOR;
	while (iterator != NULL)
	{
		if (iterator->info == item) return TRUE;
		MOVE_NEXT;
	}
	return FALSE;
}

// Inline function used inside the get_first and peek functions
static inline bool_t GetFirst(list_t list, T* result)
{
	RETURN_IF_EMPTY(list, FALSE);
	*result = list->head->info;
	return TRUE;
}

// GetFirst
bool_t get_first(list_t list, T* result)
{
	return GetFirst(list, result);
}

// GetLast
bool_t get_last(list_t list, T* result)
{
	RETURN_IF_EMPTY(list, FALSE);
	*result = list->tail->info;
	return TRUE;
}

// Get
bool_t get(list_t list, int index, T* result)
{
	if (index < 0 || index >= list->length) return FALSE;
	bool_t fromHead = index <= list->length / 2;
	if (fromHead)
	{
		int position = 0;
		GET_HEAD_ITERATOR;;
		while (iterator != NULL)
		{
			if (position == index)
			{
				*result = iterator->info;
				return TRUE;
			}
			MOVE_NEXT_W_INDEX(position);
		}
	}
	else
	{
		int position = list->length - 1;
		GET_TAIL_ITERATOR;
		while (iterator != NULL)
		{
			if (position == index)
			{
				*result = iterator->info;
				return TRUE;
			}
			MOVE_BACK_W_INDEX(position);
		}
	}
	return FALSE;
}

// IndexOf
int index_of(const T item, list_t list)
{
	RETURN_IF_EMPTY(list, -1);
	int index = 0;
	GET_HEAD_ITERATOR;
	while (iterator != NULL)
	{
		if (iterator->info == item) return index;
		MOVE_NEXT_W_INDEX(index);
	}
	return -1;
}

// LastIndexOf
int last_index_of(const T item, list_t list)
{
	RETURN_IF_EMPTY(list, -1);
	int index = list->length - 1;
	GET_TAIL_ITERATOR;
	while (iterator != NULL)
	{
		if (iterator->info == item) return index;
		MOVE_BACK_W_INDEX(index);
	}
	return -1;
}

// Add
bool_t add(const T item, list_t list)
{
	if (list == NULL) return FALSE;
	nodePointer newNode = (nodePointer)malloc(sizeof(listNode));
	newNode->info = item;
	newNode->next = NULL;
	if (list->length == 0)
	{		
		newNode->previous = NULL;
		list->head = newNode;
		list->tail = newNode;
	}
	else
	{
		newNode->previous = list->tail;
		list->tail->next = newNode;
		list->tail = newNode;
	}
	list->length++;
	SYNC_PLUS;
	return TRUE;
}

// AddAt
bool_t add_at(const T item, list_t list, int index)
{
	if (CHECK_EMPTY(list) || index < 0 || index >= list->length) return FALSE;
	if (index == 0) return add(item, list);
	bool_t fromHead = index <= list->length / 2;
	nodePointer newNode = (nodePointer)malloc(sizeof(listNode));
	newNode->info = item;
	if (fromHead)
	{
		int position = 0;
		GET_HEAD_ITERATOR;
		while (iterator != NULL)
		{
			if (position == index)
			{
				newNode->previous = iterator->previous;
				newNode->previous->next = newNode;
				newNode->next = iterator;
				iterator->previous = newNode;
				break;
			}
			MOVE_NEXT_W_INDEX(position);
		}
	}
	else
	{
		int position = list->length - 1;
		GET_TAIL_ITERATOR;
		while (iterator != NULL)
		{
			if (position == index)
			{
				newNode->previous = iterator->previous;
				newNode->previous->next = newNode;
				newNode->next = iterator;
				iterator->previous = newNode;
				break;
			}
			MOVE_BACK_W_INDEX(position);
		}
	}
	SYNC_PLUS;
	list->length++;
	return TRUE;
}

// AddAll
bool_t add_all(list_t target, const list_t source)
{
	if (target == NULL) return FALSE;
	RETURN_IF_EMPTY(source, FALSE);
	GET_ITERATOR(source->head);
	while (iterator != NULL)
	{
		add(iterator->info, target);
		MOVE_NEXT;
	}
	return TRUE;
}

#define SIZE(list) list == NULL ? -1 : list->length

// Size
int size(list_t list)
{
	return SIZE(list);
}

// RemoveItem
bool_t remove_item(const T item, list_t list)
{
	RETURN_IF_EMPTY(list, FALSE);
	if (list->length == 1)
	{
		if (list->head->info == item)
		{
			free(list->head);
			list->head = NULL;
			list->tail = NULL;
			list->length = 0;
			SYNC_PLUS;
			return TRUE;
		}
		else return FALSE;
	}
	GET_HEAD_ITERATOR;
	while (iterator != NULL)
	{
		if (iterator->info == item)
		{
			// First node inside the list_t
			if (iterator->previous == NULL)
			{
				iterator->next->previous = NULL;
				list->head = iterator->next;
			}
			else if (iterator->next == NULL)
			{
				//Last node of the list_t
				iterator->previous->next = NULL;
				list->tail = iterator->previous;
			}
			else 
			{
				// Random position inside the list_t
				iterator->previous->next = iterator->next;
				iterator->next->previous = iterator->previous;
			}			
			free(iterator);
			list->length--;
			SYNC_PLUS;
			return TRUE;
		}
		MOVE_NEXT;
	}
	return FALSE;
}

// RemoveAt
bool_t remove_at(list_t list, int index)
{
	RETURN_IF_EMPTY(list, FALSE);
	if (index < 0 || index >= list->length) return FALSE;
	if (index == 0 && list->length == 1)
	{
		free(list->head);
		list->head = NULL;
		list->tail = NULL;
		list->length = 0;
		SYNC_PLUS;
		return TRUE;
	}
	else if (index == 0 && list->length > 1)
	{
		nodePointer temp = list->head;
		list->head = temp->next;
		list->head->previous = NULL;
		free(temp);
		list->length--;
		SYNC_PLUS;
		return TRUE;
	}
	else if (index == list->length - 1)
	{
		nodePointer temp = list->tail;
		list->tail = list->tail->previous;
		list->tail->next = NULL;
		free(temp);
		list->length--;
		SYNC_PLUS;
		return TRUE;
	}
	bool_t fromHead = index <= list->length / 2;
	if (fromHead)
	{
		int position = 0;
		GET_HEAD_ITERATOR;
		while (iterator != NULL)
		{
			if (position == index)
			{
				iterator->previous->next = iterator->next;
				iterator->next->previous = iterator->previous;
				free(iterator);
				break;
			}
			MOVE_NEXT_W_INDEX(position);
		}
	}
	else
	{
		int position = list->length - 1;
		GET_TAIL_ITERATOR;
		while (iterator != NULL)
		{
			if (position == index)
			{
				iterator->previous->next = iterator->next;
				iterator->next->previous = iterator->previous;
				free(iterator);
				break;
			}
			MOVE_BACK_W_INDEX(position);
		}
	}
	SYNC_PLUS;
	list->length--;
	return TRUE;
}

// RemoveAllItems
int remove_all_items(const T item, list_t list)
{
	RETURN_IF_EMPTY(list, -1);
	if (list->length == 1)
	{
		if (list->head->info == item)
		{
			clear(list);
			return 1;
		}
		else return -1;
	}
	GET_HEAD_ITERATOR;
	int total = 0;
	while (iterator != NULL)
	{
		if (iterator->info == item)
		{
			// Last element inside the list_t
			if (list->length == 1)
			{
				clear(list);
				return total + 1;
			}

			// Item in last position
			if (iterator->next == NULL)
			{
				// Set previous item as the last one
				iterator->previous->next = NULL;
				list->tail = iterator->previous;
				list->length--;
				free(iterator);
				SYNC_PLUS;
				return total + 1;
			}

			// First element inside the list_t
			if (iterator->previous == NULL)
			{
				list->head = iterator->next;
				iterator->next->previous = NULL;			
			}
			else 
			{
				// Element in a random position
				iterator->previous->next = iterator->next;
				iterator->next->previous = iterator->previous;		
			}		
			list->length--;
			nodePointer temp = iterator;
			MOVE_NEXT;
			free(temp);
			total++;
			SYNC_PLUS;
		}
		else MOVE_NEXT;
	}
	return total == 0 ? -1 : total;
}

// ReplaceItem
bool_t replace_item(const T target, const T replacement, list_t list)
{
	RETURN_IF_EMPTY(list, FALSE);
	GET_HEAD_ITERATOR;
	while (iterator != NULL)
	{
		if (iterator->info == target)
		{
			iterator->info = replacement;
			SYNC_PLUS;
			return TRUE;
		}
		MOVE_NEXT;
	}
	return FALSE;
}

// ReplaceAt
bool_t replace_at(const T item, list_t list, int index)
{
	if (list == NULL || index < 0 || index >= list->length) return FALSE;
	bool_t fromHead = index <= list->length / 2;
	if (fromHead)
	{
		int position = 0;
		GET_HEAD_ITERATOR;
		while (iterator != NULL)
		{
			if (position == index)
			{
				iterator->info = item;
				SYNC_PLUS;
				return TRUE;
			}
			MOVE_NEXT_W_INDEX(position);
		}
	}
	else
	{
		int position = list->length - 1;
		GET_TAIL_ITERATOR;
		while (iterator != NULL)
		{
			if (position == index) return iterator->info;
			{
				iterator->info = item;
				SYNC_PLUS;
				return TRUE;
			}
			MOVE_BACK_W_INDEX(position);
		}
	}
	return FALSE;
}

// ReplaceAllItems
int replace_all_items(const T target, const T replacement, list_t list)
{
	RETURN_IF_EMPTY(list, -1);
	GET_HEAD_ITERATOR;
	int total = 0;
	while (iterator != NULL)
	{
		if (iterator->info == target)
		{
			iterator->info = replacement;
			total++;
		}
		MOVE_NEXT;
	}
	if (total != 0) SYNC_PLUS;
	return total == 0 ? -1 : total;
}

// Swap
bool_t swap(list_t list, int index1, int index2)
{
	RETURN_IF_EMPTY(list, FALSE);
	if (index1 < 0 || index1 >= list->length || index2 < 0
		|| index2 >= list->length || index1==index2) return FALSE;
	T temp1, temp2;
	get(list, index1, &temp1);
	get(list, index2, &temp2);
	replace_at(temp2, list, index1);
	replace_at(temp1, list, index2);
	return TRUE;
}

// FormattedPrint
bool_t formatted_print(char* pattern, list_t list)
{
	if (list == NULL)
	{
		printf("NULL list");
		return FALSE;
	}
	if (list->length == 0)
	{
		printf("Empty list");
		return FALSE;
	}
	GET_HEAD_ITERATOR;
	while (iterator != NULL)
	{
		printf(pattern, iterator->info);
		if (iterator->next != NULL) printf(", ");
		MOVE_NEXT;
	}
	return TRUE;
}

// Print
bool_t print(char* pattern, list_t list)
{
	if (list == NULL) return FALSE;
	GET_HEAD_ITERATOR;
	while (iterator != NULL)
	{
		printf(pattern, iterator->info);
		MOVE_NEXT;
	}
	return TRUE;
}

/* ============================================================================
*  stack_t
*  ========================================================================= */

// Push
bool_t push(const T item, stack_t stack)
{
	if (stack == NULL) return FALSE;
	if (stack->length == 0)
	{
		return add(item, stack);
	}
	nodePointer newNode = (nodePointer)malloc(sizeof(listNode));
	newNode->info = item;
	newNode->previous = NULL;
	stack->head->previous = newNode;
	newNode->next = stack->head;
	stack->head = newNode;
	stack->length++;
	stack->sync++;
	return TRUE;
}

// Pop
bool_t pop(stack_t stack, T* result)
{
	RETURN_IF_EMPTY(stack, FALSE);
	*result = stack->head->info;
	if (stack->length == 1)
	{
		free(stack->head);
		stack->head = NULL;
		stack->tail = NULL;
	}
	else
	{
		nodePointer temp = stack->head;
		stack->head = temp->next;
		stack->head->previous = NULL;
		free(temp);
	}
	stack->length--;
	stack->sync++;
	return TRUE;
}

// Peek
bool_t peek(stack_t stack, T* result)
{
	return GetFirst(stack, result);
}

/* ============================================================================
*  LINQ
*  ========================================================================= */

// FirstOrDefault
bool_t first_or_default(list_t list, T* result, bool_t(*expression)(T))
{
	RETURN_IF_EMPTY(list, FALSE);
	GET_HEAD_ITERATOR;
	while (iterator != NULL)
	{
		if (expression(iterator->info))
		{
			*result = iterator->info;
			return TRUE;
		}
		MOVE_NEXT;
	}
	return FALSE;
}

// LastOrDefault
bool_t last_or_default(list_t list, T* result, bool_t(*expression)(T))
{
	RETURN_IF_EMPTY(list, FALSE);
	GET_TAIL_ITERATOR;
	while (iterator != NULL)
	{
		if (expression(iterator->info))
		{
			*result = iterator->info;
			return TRUE;
		}
		MOVE_BACK;
	}
	return FALSE;
}

// Count
int count(list_t list, bool_t(*expression)(T))
{
	RETURN_IF_EMPTY(list, -1);
	int total = 0;
	GET_HEAD_ITERATOR;
	while (iterator != NULL)
	{
		if (expression(iterator->info)) total++;
		MOVE_NEXT;
	}
	return total;
}

// FirstIndexWhere
int first_index_where(list_t list, bool_t(*expression)(T))
{
	RETURN_IF_EMPTY(list, -1);
	int position = 0;
	GET_HEAD_ITERATOR;
	while (iterator != NULL)
	{
		if (expression(iterator->info)) return position;
		MOVE_NEXT_W_INDEX(position);
	}
	return -1;
}

// LastIndexWhere
int last_index_where(list_t list, bool_t(*expression)(T))
{
	RETURN_IF_EMPTY(list, -1);
	int position = list->length - 1;
	GET_TAIL_ITERATOR;
	while (iterator != NULL)
	{
		if (expression(iterator->info)) return position;
		MOVE_BACK_W_INDEX(position);
	}
	return -1;
}

// Where
list_t where(list_t list, bool_t(*expression)(T))
{
	NULL_IF_EMPTY(list);
	list_t outList = create();
	GET_HEAD_ITERATOR;
	while (iterator != NULL)
	{
		if (expression(iterator->info)) add(iterator->info, outList);
		MOVE_NEXT;
	}
	return outList;
}

// TakeWhile
list_t take_while(list_t list, bool_t(*expression)(T))
{
	NULL_IF_EMPTY(list);
	list_t outList = create();
	GET_HEAD_ITERATOR;
	while (iterator != NULL)
	{
		if (expression(iterator->info))
		{
			add(iterator->info, outList);
			MOVE_NEXT;
		}
		else break;
	}
	return outList;
}

// TakeRange
list_t take_range(list_t list, int start, int end)
{
	NULL_IF_EMPTY(list);
	if (start < 0 || end < 0 || start >= list->length
		|| end >= list->length || start >= end) return NULL;
	list_t outList = create();	
	bool_t fromHead = start <= list->length / 2;
	nodePointer iterator;
	if (fromHead)
	{
		int position = 0;
		iterator = list->head;
		while (iterator != NULL)
		{
			if (position == start) break;
			MOVE_NEXT_W_INDEX(position);
		}
	}
	else
	{
		int position = list->length - 1;
		iterator = list->tail;
		while (iterator != NULL)
		{
			if (position == start) break;
			MOVE_BACK_W_INDEX(position);
		}
	}
	int elements = end + 1 - start;
	while (elements > 0)
	{
		add(iterator->info, outList);
		iterator = iterator->next;
		elements--;
	}
	return outList;
}

// Concat
list_t concat(list_t list1, list_t list2)
{
	if (list1 == NULL || list2 == NULL) return NULL;
	if (list1->length == 0) return copy(list2);
	list_t outList = copy(list1);
	if (list2->length == 0) return outList;
	GET_ITERATOR(list2->head);
	while (iterator != NULL)
	{
		add(iterator->info, outList);
		MOVE_NEXT;
	}
	return outList;
}

#define GET_COUPLE_ITERATORS                                   \
nodePointer iterator1 = list1->head, iterator2 = list2->head

// Zip
list_t zip(list_t list1, list_t list2, T(*expression)(T, T))
{
	if (CHECK_EMPTY(list1) || CHECK_EMPTY(list2)) return NULL;
	GET_COUPLE_ITERATORS;
	list_t outList = create();
	while (iterator1 != NULL && iterator2 != NULL)
	{
		add(expression(iterator1->info, iterator2->info), outList);
		iterator1 = iterator1->next;
		iterator2 = iterator2->next;
	}
	return outList;
}

// Any
bool_t any(list_t list, bool_t(*expression)(T))
{
	RETURN_IF_EMPTY(list, FALSE);
	GET_HEAD_ITERATOR;
	while (iterator != NULL)
	{
		if (expression(iterator->info)) return TRUE;
		MOVE_NEXT;
	}
	return FALSE;
}

// All
bool_t all(list_t list, bool_t(*expression)(T))
{
	RETURN_IF_EMPTY(list, FALSE);
	GET_HEAD_ITERATOR;
	while (iterator != NULL)
	{
		if (!expression(iterator->info)) return FALSE;
		MOVE_NEXT;
	}
	return TRUE;
}

// Skip
list_t skip(list_t list, int count)
{
	NULL_IF_EMPTY(list);
	if (count >= list->length) return NULL;
	list_t outList = create();
	GET_HEAD_ITERATOR;
	while (iterator != NULL)
	{
		if (count) count--;
		else add(iterator->info, outList);
		MOVE_NEXT;
	}
	return outList;
}

// SkipWhile
list_t skip_while(list_t list, bool_t(*expression)(T))
{
	NULL_IF_EMPTY(list);
	list_t outList = create();
	bool_t triggered = FALSE;
	GET_HEAD_ITERATOR;
	while (iterator != NULL)
	{
		if (!triggered)
		{
			if (expression(iterator->info))
			{
				MOVE_NEXT;
				continue;
			}
			else triggered = TRUE;
		}
		add(iterator->info, outList);
		MOVE_NEXT;
	}
	return outList;
}

// ForEach
bool_t for_each(list_t list, void(*expression)(T))
{
	RETURN_IF_EMPTY(list, FALSE);
	GET_HEAD_ITERATOR;
	while (iterator != NULL)
	{
		expression(iterator->info);
		MOVE_NEXT;
	}
	return TRUE;
}

// InverseForEach
bool_t inverse_for_each(list_t list, void(*expression)(T))
{
	RETURN_IF_EMPTY(list, FALSE);
	GET_TAIL_ITERATOR;
	while (iterator != NULL)
	{
		expression(iterator->info);
		MOVE_BACK;
	}
	return TRUE;
}

#define LIST_CONTAINS(list)                                 \
nodePointer backupIterator = list->head;                    \
bool_t found = FALSE;                                       \
while (backupIterator != NULL)                              \
{                                                           \
	if (expression(iterator->info, backupIterator->info))   \
	{                                                       \
		found = TRUE;                                       \
		break;                                              \
	}                                                       \
	backupIterator = backupIterator->next;                  \
}

#define NULL_IF_EITHER_ONE_NULL                  \
if (list1 == NULL || list2 == NULL) return NULL

// Join
list_t join(list_t list1, list_t list2, bool_t(*expression)(T, T))
{
	NULL_IF_EITHER_ONE_NULL;
	if (list1->length == 0) return copy(list2);
	list_t outList = copy(list1);
	if (list2->length == 0) return outList;
	GET_ITERATOR(list2->head);
	while (iterator != NULL)
	{
		LIST_CONTAINS(list1);
		if (!found) add(iterator->info, outList);
		MOVE_NEXT;
	}
	return outList;
}

// JoinWhere
list_t join_where(list_t list1, list_t list2, bool_t(*condition)(T), bool_t(*expression)(T, T))
{
	NULL_IF_EITHER_ONE_NULL;
	if (list1->length == 0 && list2->length == 0) return create();
	if (list1->length == 0) return where(list2, condition);
	if (list2->length == 0) return where(list1, condition);
	list_t outList = create();
	GET_ITERATOR(list1->head);
	while (iterator != NULL)
	{
		if (condition(iterator->info)) add(iterator->info, outList);
		MOVE_NEXT;
	}
	iterator = list2->head;
	while (iterator != NULL)
	{
		if (condition(iterator->info)) 
		{
			LIST_CONTAINS(outList);
			if (!found) add(iterator->info, outList);
		}
		MOVE_NEXT;
	}
	return outList;
}

// Intersect
list_t intersect(list_t list1, list_t list2, bool_t(*expression)(T, T))
{
	NULL_IF_EITHER_ONE_NULL;
	if (list1->length == 0 || list2->length == 0) return create();
	list_t outList = create();
	GET_ITERATOR(list1->head);
	while (iterator != NULL)
	{
		LIST_CONTAINS(list2);
		if (found) add(iterator->info, outList);
		MOVE_NEXT;
	}
	return outList;
}

// Except
list_t except(list_t list1, list_t list2, bool_t(*expression)(T, T))
{
	NULL_IF_EITHER_ONE_NULL;
	if (list1->length == 0) return create();
	if (list2->length == 0) return copy(list1);
	list_t outList = create();
	GET_ITERATOR(list1->head);
	while (iterator != NULL)
	{
		LIST_CONTAINS(list2);
		if (!found) add(iterator->info, outList);
		MOVE_NEXT;
	}
	return outList;
}

// Reverse
list_t reverse(list_t list)
{
	NULL_IF_EMPTY(list);
	list_t outList = create();
	GET_TAIL_ITERATOR;
	while (iterator != NULL)
	{
		add(iterator->info, outList);
		MOVE_BACK;
	}
	return outList;
}

// ReverseRange
list_t reverse_range(list_t list, int start, int end)
{
	NULL_IF_EMPTY(list);
	if (start < 0 || end < 0 || start >= list->length
		|| end >= list->length || start >= end) return NULL;
	list_t outList = copy(list);
	while (start <= end)
	{
		swap(outList, start, end);
		start++;
		end--;
	}
	return outList;
}

#define GET_LIST_SUM                         \
RETURN_IF_EMPTY(list, (T)NULL);              \
GET_HEAD_ITERATOR;                           \
int total = 0;                               \
while (iterator != NULL)                     \
{                                            \
	total += expression(iterator->info);     \
	MOVE_NEXT;                               \
}

// Sum
int sum(list_t list, int(*expression)(T))
{
	GET_LIST_SUM;
	return total;
}

// Average
int average(list_t list, int(*expression)(T))
{
	GET_LIST_SUM;
	return total / list->length;
}

// GetNumericMin
int get_numeric_min(list_t list, int(*expression)(T))
{
	RETURN_IF_EMPTY(list, (T)NULL);
	GET_HEAD_ITERATOR;
	int minimum = INT_MAX;
	while (iterator != NULL)
	{
		int temp = expression(iterator->info);
		if (temp < minimum) minimum = temp;
		MOVE_NEXT;
	}
	return minimum;
}

#define FIRST_IF_SINGLE_ITEM if (list->length == 1) return list->head->info

// GetMin
bool_t get_min(list_t list, T* result, comparation(*expression)(T, T))
{
	RETURN_IF_EMPTY(list, FALSE);
	FIRST_IF_SINGLE_ITEM;
	GET_HEAD_ITERATOR;
	*result = iterator->info;
	MOVE_NEXT;
	while (iterator != NULL)
	{
		if (expression(*result, iterator->info) == GREATER) *result = iterator->info;
		MOVE_NEXT;
	}
	return TRUE;
}

// GetNumericMax
int get_numeric_max(list_t list, int(*expression)(T))
{
	RETURN_IF_EMPTY(list, (T)NULL);
	GET_HEAD_ITERATOR;
	int maximum = INT_MIN;
	while (iterator != NULL)
	{
		int temp = expression(iterator->info);
		if (temp > maximum) maximum = temp;
		MOVE_NEXT;
	}
	return maximum;
}

// GetMax
bool_t get_max(list_t list, T* result, comparation(*expression)(T, T))
{
	RETURN_IF_EMPTY(list, FALSE);
	FIRST_IF_SINGLE_ITEM;
	GET_HEAD_ITERATOR;
	*result = iterator->info;
	MOVE_NEXT;
	while (iterator != NULL)
	{
		if (expression(*result, iterator->info) == LOWER) *result = iterator->info;
		MOVE_NEXT;
	}
	return TRUE;
}

// OrderHelper
static inline list_t orderHelper(list_t list, comparation(*expression)(T, T), bool_t reverse)
{
	list_t outList = copy(list);
	if (list->length == 1) return outList;
	bool_t sorted = FALSE;
	GET_ITERATOR(outList->head);
	while (TRUE)
	{
		comparation result = expression(iterator->info, iterator->next->info);
		if (reverse && result != EQUAL)
		{
			result = result == GREATER ? LOWER : GREATER;
		}
		if (result == GREATER)
		{
			T backup = iterator->info;
			iterator->info = iterator->next->info;
			iterator->next->info = backup;
			sorted = TRUE;
		}
		bool_t loopEnd = iterator->next->next == NULL ? TRUE : FALSE;
		if (loopEnd && sorted)
		{
			iterator = outList->head;
			sorted = FALSE;
		}
		else if (loopEnd) break;
		else MOVE_NEXT;
	}
	return outList;
}

// InPlaceOrderBy
list_t in_place_order_by(list_t list, comparation(*expression)(T, T))
{
	NULL_IF_EMPTY(list);
	return orderHelper(list, expression, FALSE);
}

// InPlaceOrderByDescending
list_t in_place_order_by_descending(list_t list, comparation(*expression)(T, T))
{
	NULL_IF_EMPTY(list);
	return orderHelper(list, expression, TRUE);
}

// OrderBy
list_t order_by(list_t list, comparation(*expression)(T, T))
{
	NULL_IF_EMPTY(list);
	int len;
	T* temp_vector = to_array(list, &len);
	introsort(temp_vector, len, expression);
	list = create_from(temp_vector, len);
	free(temp_vector);
	return list;
}

// OrderByDescending
list_t order_by_descending(list_t list, comparation(*expression)(T, T))
{
	// Get the sorted array
	NULL_IF_EMPTY(list);
	int len;
	T* temp_vector = to_array(list, &len);
	introsort(temp_vector, len, expression);

	// Reverse the array and return a new list
	int i, target = len >> 1;
	for (i = 0; i < target; i++)
	{
		T temp = temp_vector[i];
		temp_vector[i] = temp_vector[target];
		temp_vector[target] = temp;
		target--;
	}
	list = create_from(temp_vector, len);
	free(temp_vector);
	return list;
}

/* ============== Other LINQ functions ============== */

#define GET_DISTINCT_LIST                                       \
list_t outList = create();                                    \
GET_HEAD_ITERATOR;                                              \
while (iterator != NULL)                                        \
{                                                               \
	nodePointer testIterator = outList->head;                 \
	bool_t found = FALSE;                                       \
	while (testIterator != NULL)                                \
	{                                                           \
		if (expression(iterator->info, testIterator->info))     \
		{                                                       \
			found = TRUE;                                       \
			break;                                              \
		}                                                       \
		testIterator = testIterator->next;                      \
	}                                                           \
	if (!found) add(iterator->info, outList);                 \
	MOVE_NEXT;                                                  \
}

// Distinct
list_t distinct(list_t list, bool_t(*expression)(T, T))
{
	NULL_IF_EMPTY(list);
	GET_DISTINCT_LIST;
	return outList;
}

// CountDistinct
int count_distinct(list_t list, bool_t(*expression)(T, T))
{
	if (list == NULL) return -1;
	if (list->length == 0) return 0;
	GET_DISTINCT_LIST;
	return outList->length;
}

// Single
bool_t single(list_t list, T* result, bool_t(*expression)(T))
{
	RETURN_IF_EMPTY(list, FALSE);
	bool_t found = FALSE;
	GET_HEAD_ITERATOR;
	while (iterator != NULL)
	{
		if (expression(iterator->info))
		{
			if (found) return FALSE;
			found = TRUE;
			*result = iterator->info;
		}
		MOVE_NEXT;
	}
	return found;
}

// RemoveWhere
list_t remove_where(list_t list, bool_t(*expression)(T))
{
	NULL_IF_EMPTY(list);
	list_t outList = create();
	GET_HEAD_ITERATOR;
	while (iterator != NULL)
	{
		if (!expression(iterator->info)) add(iterator->info, outList);
		MOVE_NEXT;
	}
	return outList;
}

// ReplaceWhere
list_t replace_where(list_t list, const T replacement, bool_t(*expression)(T))
{
	NULL_IF_EMPTY(list);
	list_t outList = create();
	GET_HEAD_ITERATOR;
	while (iterator != NULL)
	{
		if (expression(iterator->info)) add(replacement, outList);
		else add(iterator->info, outList);
		MOVE_NEXT;
	}
	return outList;
}

// Derive
list_t derive(list_t list, T(*expression)(T))
{
	NULL_IF_EMPTY(list);
	list_t outList = create();
	GET_HEAD_ITERATOR;
	while (iterator != NULL)
	{
		add(expression(iterator->info), outList);
		MOVE_NEXT;
	}
	return outList;
}

// SequenceEquals
bool_t sequence_equals(list_t list1, list_t list2, bool_t(*expression)(T, T))
{
	if (list1 == NULL || list2 == NULL)
	{
		return list1 == NULL && list2 == NULL;
	}
	if (list1->length != list2->length) return FALSE;
	GET_COUPLE_ITERATORS;
	while (iterator1 != NULL)
	{
		if (!expression(iterator1->info, iterator2->info)) return FALSE;
		iterator1 = iterator1->next;
		iterator2 = iterator2->next;
	}
	return TRUE;
}

// Trim
list_t trim(list_t list, int length)
{
	NULL_IF_EMPTY(list);
	if (list->length <= length) return copy(list);
	list_t outList = create();
	GET_HEAD_ITERATOR;
	int position = 0;
	while (position < length)
	{
		add(iterator->info, outList);
		MOVE_NEXT_W_INDEX(position);
	}
	return outList;
}

/* ============================================================================
*  Iterator
*  ========================================================================= */

// GetIterator
list_iterator_t get_iterator(list_t list)
{
	NULL_IF_EMPTY(list);
	list_iterator_t iterator = (list_iterator_t)malloc(sizeof(iteratorInstance));
	iterator->list = list;
	iterator->pointer = list->head;
	iterator->position = 0;
	iterator->sync = list->sync;
	iterator->started = FALSE;
	return iterator;
}

// DestroyIterator
bool_t destroy_iterator(list_iterator_t* iterator)
{
	if (*iterator == NULL) return FALSE;
	free(*iterator);
	*iterator = NULL;
	return TRUE;
}

// IsSynced
bool_t is_synced(list_iterator_t iterator)
{
	if (iterator == NULL) return FALSE;
	return iterator->sync == iterator->list->sync ? TRUE : FALSE;
}

#define RETURN_IF_OUT_OF_SYNC(value)                        \
if (iterator->sync != iterator->list->sync) return value;

// GetCurrent
bool_t get_current(list_iterator_t iterator, T* result)
{
	if (iterator == NULL) return FALSE;
	RETURN_IF_OUT_OF_SYNC(FALSE);
	*result = iterator->pointer->info;
	return TRUE;
}

// Next
bool_t next(list_iterator_t iterator, T* result)
{
	if (iterator == NULL) return FALSE;
	RETURN_IF_OUT_OF_SYNC(FALSE);
	if (iterator->started == FALSE)
	{
		iterator->started = TRUE;
		*result = iterator->pointer->info;
		return TRUE;
	}
	if (move_next(iterator) == FALSE) return FALSE;
	*result = iterator->pointer->info;
	return TRUE;
}

// CheckGoForward
static inline bool_t checkGoForward(list_iterator_t iterator)
{
	if (iterator == NULL) return FALSE;
	RETURN_IF_OUT_OF_SYNC(FALSE);
	return iterator->pointer->next != NULL ? TRUE : FALSE;
}

// CanGoForward
bool_t can_go_forward(list_iterator_t iterator)
{
	return checkGoForward(iterator);
}

// CheckGoBack
static inline bool_t checkGoBack(list_iterator_t iterator)
{
	if (iterator == NULL) return FALSE;
	RETURN_IF_OUT_OF_SYNC(FALSE);
	return iterator->pointer->previous != NULL ? TRUE : FALSE;
}

// CanGoBack
bool_t can_go_back(list_iterator_t iterator)
{
	return checkGoBack(iterator);
}

// MoveNext
bool_t move_next(list_iterator_t iterator)
{
	if (!checkGoForward(iterator)) return FALSE;
	iterator->pointer = iterator->pointer->next;
	iterator->position++;
	iterator->started = TRUE;
	return TRUE;
}

// MoveBack
bool_t move_back(list_iterator_t iterator)
{
	if (!checkGoBack(iterator)) return FALSE;
	iterator->pointer = iterator->pointer->previous;
	iterator->position--;
	return TRUE;
}

// ActualPosition
int actual_position(list_iterator_t iterator)
{
	if (iterator == NULL) return -1;
	RETURN_IF_OUT_OF_SYNC(-1);
	return iterator->position;
}

// ElementsLeft
int elements_left(list_iterator_t iterator)
{
	if (iterator == NULL) return -1;
	RETURN_IF_OUT_OF_SYNC(-1);
	return iterator->list->length - iterator->position;
}

// ForEachRemaining
int for_each_remaining(list_iterator_t iterator, void(*expression)(T))
{
	if (iterator == NULL) return -1;
	RETURN_IF_OUT_OF_SYNC(-1);
	iterator->started = TRUE;
	int start = iterator->position;
	while (TRUE)
	{
		expression(iterator->pointer->info);
		if (iterator->pointer->next == NULL) break;
		iterator->pointer = iterator->pointer->next;
		iterator->position++;
	}
	return iterator->position - start;
}

// Restart
bool_t restart(list_iterator_t iterator)
{
	if (iterator == NULL) return FALSE;
	iterator->position = 0;
	iterator->pointer = iterator->list->head;
	iterator->sync = iterator->list->sync;
	iterator->started = FALSE;
	return TRUE;
}