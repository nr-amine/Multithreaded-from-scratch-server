#include "list.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

/* for test with ints on the stack, we need a print_int function */
void print_int(const void *n);

/* a string_value struct to test lists with other types */
struct string_value {
	char *string; /* on the heap */
	int value;
};

/* return 1 if sv->string equals ch and sv->value equals val, 0 otherwise. */
int string_value_eq(const struct string_value *sv, const char *ch, int val);

/* create a string_value on the heap with a malloc'ed copy of string */
struct string_value *create_string_value(const char *string, int value);

/* frees the value field, then frees the string_value struct */
void free_string_value(void *string_value);

void print_string_value(const void *string_value);

int main(void)
{
	/* first tests with a list of ints, ints are on the stack */
	/* empty list */
	struct list *l = list_create();
	list_print(l, print_int); printf("\n");
	assert(list_is_empty(l));

	int nb[] = { 10, 20, 30, 40, 50 };
	/* l = [10, 20, 30] */
	list_add(list_add(list_add(l, nb), &nb[1]), &nb[2]);
	assert(list_length(l) == 3);
	assert(*((int *) list_get(l, 0)) == 10);
	assert(*((int *) list_get(l, 1)) == 20);
	assert(*((int *) list_get(l, 2)) == 30);
	list_print(l, print_int); printf("\n");

	/* insert 40 at position 1, l should be [10, 40, 20, 30] */
	list_add_index(l, &nb[3], 1);
	assert(list_length(l) == 4);
	assert(*((int *) list_get(l, 0)) == 10);
	assert(*((int *) list_get(l, 1)) == 40);
	assert(*((int *) list_get(l, 2)) == 20);
	assert(*((int *) list_get(l, 3)) == 30);
	list_print(l, print_int); printf("\n");

	/* remove value at index 1 */
	/* l should be [10, 20, 30] */
	int *val = list_remove_index(l, 1);
	assert(*val == 40);
	assert(list_length(l) == 3);
	assert(*((int *) list_get(l, 0)) == 10);
	assert(*((int *) list_get(l, 1)) == 20);
	assert(*((int *) list_get(l, 2)) == 30);
	list_print(l, print_int); printf("\n");

	/* remove the head of the list
	 * l should be [20, 30] */
	val = list_remove_first(l);
	assert(*val == 10);
	assert(list_length(l) == 2);
	assert(*((int *) list_get(l, 0)) == 20);
	assert(*((int *) list_get(l, 1)) == 30);
	list_print(l, print_int); printf("\n");

	/* remove the last value of the list
	 * l should be [20] */
	val = list_remove(l);
	assert(*val == 30);
	list_print(l, print_int); printf("\n");
	assert(list_length(l) == 1);
	assert(*((int *) list_get(l, 0)) == 20);

	/* remove the last head of the list
	 * l should be [] */
	val = list_remove_first(l);
	assert(*val == 20);
	assert(list_length(l) == 0);
	assert(list_is_empty(l));
	list_print(l, print_int); printf("\n");

	list_free(l, NULL);

	/* Next test with (string, int) couples on the heap */
	l = list_create();
	assert(list_is_empty(l));

	/* l = [("plop", 42)] */
	list_add(l, create_string_value("plop", 42));
	assert(list_length(l) == 1);
	assert(string_value_eq(list_get(l, 0), "plop", 42));
	list_print(l, print_string_value); printf("\n");

	/* l = [("plop", 42), ("hop", -1)] */
	list_add(l, create_string_value("hop", -1));
	assert(list_length(l) == 2);
	assert(string_value_eq(list_get(l, 1), "hop", -1));
	list_print(l, print_string_value); printf("\n");
	list_add_index(l, create_string_value("lol", 1), 1);

	/* l = [("plop", 42), ("lol", 1), ("hop", -1)] */
	assert(list_length(l) == 3);
	assert(string_value_eq(list_get(l, 1), "lol", 1));
	list_print(l, print_string_value); printf("\n");

	/* l = [("plop", 42), ("lol", 1), ("hop", -1), ("rofl", 10)] */
	list_add_index(l, create_string_value("rofl", 10), 3);
	assert(list_length(l) == 4);
	assert(string_value_eq(list_get(l, 3), "rofl", 10));
	list_print(l, print_string_value); printf("\n");

	/* this is how to remove and recover a value in the list
	 * if need be, sv1 could be freed afterwards */
	struct string_value *sv1 = list_remove_first(l);
	assert(string_value_eq(sv1, "plop", 42));
	list_print(l, print_string_value); printf("\n");

	struct string_value *sv2 = list_remove_first(l);
	assert(string_value_eq(sv2, "lol", 1));
	list_print(l, print_string_value); printf("\n");

	struct string_value *sv3 = list_remove(l);
	assert(string_value_eq(sv3, "rofl", 10));
	list_print(l, print_string_value); printf("\n");

	/* test low level functions */
	list_insert_after_node(l, sv3, l->first);
	list_print(l, print_string_value); printf("\n");
	list_insert_before_node(l, sv2, l->last);
	list_print(l, print_string_value); printf("\n");
	list_insert_before_node(l, sv1, l->first);
	list_print(l, print_string_value); printf("\n");
	assert(list_length(l) == 4);
	assert(string_value_eq(list_get(l, 0), "plop", 42));
	assert(string_value_eq(list_get(l, 1), "hop", -1));
	assert(string_value_eq(list_get(l, 2), "lol", 1));
	assert(string_value_eq(list_get(l, 3), "rofl", 10));

	/* this is how we free and remove an element with
	 * list_remove_node */
	free_string_value(list_remove_node(l, l->first->next));
	assert(list_length(l) == 3);
	assert(string_value_eq(list_get(l, 0), "plop", 42));
	assert(string_value_eq(list_get(l, 1), "lol", 1));
	assert(string_value_eq(list_get(l, 2), "rofl", 10));

	/* testing remove_element */
	free_string_value(list_remove_element(l, sv3));
	assert(list_length(l) == 2);
	assert(string_value_eq(list_get(l, 0), "plop", 42));
	assert(string_value_eq(list_get(l, 1), "lol", 1));

	free_string_value(list_remove_element(l, sv2));
	assert(list_length(l) == 1);
	assert(string_value_eq(list_get(l, 0), "plop", 42));

	list_free(l, free_string_value);

	return 0;
}

void print_int(const void *n)
{
	const int *i = n;
	printf("%d", *i);
}

int string_value_eq(const struct string_value *sv, const char *ch, int val)
{
	return !strcmp(sv->string, ch) && sv->value == val;
}

struct string_value *create_string_value(const char *string, int value)
{
	struct string_value *res = malloc(sizeof(*res));
	res->string = strdup(string);
	res->value = value;
	return res;
}

void free_string_value(void *string_value)
{
	struct string_value *sv = string_value;
	free(sv->string);
	free(sv);
}

void print_string_value(const void *string_value)
{
	const struct string_value *sv = string_value;
	printf("(%s, %d)", sv->string, sv->value);
}
