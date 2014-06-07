#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <check.h>
#include "mm.h"
#include "mm_macros.h"
#define __LIFO_ORDERING__
START_TEST (test_macros) {
  ck_assert(ALIGNMENT == 8);
  ck_assert(MIN_BK_SIZE == 16);
  ck_assert(WSIZE == 4);
  ck_assert(DSIZE == 8);
  ck_assert(CHUNKSIZE == 4096);

  ck_assert_int_eq(ALIGN(11) % 8, 0);
  ck_assert(IS_ALIGN(ALIGN(13)));
  ck_assert_int_eq(ALIGN_CHUNKSIZE(34) % (CHUNKSIZE), 0);
  ck_assert(IS_ALIGN_WITH_MIN_BK_SIZE(16));
  ck_assert(!IS_ALIGN_WITH_MIN_BK_SIZE(33));

  size_t val = 32;
  ck_assert(READ_WORD(&val) == val);
  WRITE_WORD(&val, 34);
  ck_assert(READ_WORD(&val) == 34);


  val = 0;
  SET_CURR_ALLOC_BIT(&val);
  ck_assert(READ_WORD(&val) == 0x1);
  CLR_CURR_ALLOC_BIT(&val);
  ck_assert(READ_WORD(&val) == 0x0);

  SET_PREV_ALLOC_BIT(&val);
  ck_assert(READ_WORD(&val) == 0x2);
  CLR_PREV_ALLOC_BIT(&val);
  ck_assert(READ_WORD(&val) == 0x0);

  SET_CURR_ALLOC_BIT(&val);
  SET_PREV_ALLOC_BIT(&val);
  ck_assert(READ_WORD(&val) == 0x3);

  WRITE_WORD(&val, PACK(0xF0, CURR_ALLOC | PREV_ALLOC));
  ck_assert(READ_WORD(&val) == 0xF3);

  ck_assert(SIZE_MASK == 0xFFFFFFF8);

  uint32_t val_arr[] = {0x3, 0x00, 0x00, 0xF3}; // header, prev_ptr, next_ptr, footer
  SET_SIZE(&val_arr[0], 0xF0);
  ck_assert(READ_WORD(&val_arr[0]) == 0xF3);
  ck_assert(GET_SIZE(&val_arr[0]) == 0xF0);
  ck_assert(GET_ALLOC(&val_arr[0]) == 0x1);
  ck_assert(GET_PREV_ALLOC(&val_arr[0]) == 0x2);
  ck_assert(HDRP_USE_PLDP(&val_arr[1]) == ((char*)&val_arr[1] - WSIZE));
  ck_assert(FTRP_USE_PLDP(&val_arr[1]) == (char*)&val_arr[1] + 0xF0 - DSIZE);

  ck_assert(PREV_PTR(&val_arr[0]) == (char*)&val_arr[0] + WSIZE);
  ck_assert(NEXT_PTR(&val_arr[0]) == (char*)&val_arr[0] + DSIZE);
  ck_assert(GET_PREV_PTR(&val_arr[0]) == 0x00);
  ck_assert(GET_NEXT_PTR(&val_arr[0]) == 0x00);
  SET_PREV_PTR(&val_arr[0], 0xFFFFFFF8);
  SET_NEXT_PTR(&val_arr[0], 0x12345);
  ck_assert(GET_PREV_PTR(&val_arr[0]) == 0xFFFFFFF8);
  ck_assert(GET_NEXT_PTR(&val_arr[0]) == 0x12345);

}
END_TEST


extern int index_arr_size;
extern size_t index_array[];
extern int find_index(size_t size);
extern int index_arr_size;
START_TEST(find_index_test) {
  int i = 0;
  for (; i < index_arr_size; ++i) {
    ck_assert(find_index(index_array[i] + 3) == i); }
}
END_TEST

extern void init_free_block(void *hdrp, size_t bk_size);
START_TEST(init_free_block_test) {
  size_t bk[4];
  init_free_block(bk, 4 * WSIZE);
  ck_assert(bk[0] == PACK(4 * WSIZE, 0));
  ck_assert(bk[3] == PACK(4 * WSIZE, 0));
}
END_TEST

extern void *segregated_free_list[];
START_TEST(insert_into_size_class_test) {
  int i = 0;
  for (; i < index_arr_size; ++i) {
    segregated_free_list[i] = NULL;
  }

  size_t arr[16];
  init_free_block(&arr[0], 4 * WSIZE);
  init_free_block(&arr[4], 4 * WSIZE);
  init_free_block(&arr[8], 4 * WSIZE);
  init_free_block(&arr[12], 4 * WSIZE);
  insert_into_size_class(&arr[0], find_index(4 * WSIZE));
  insert_into_size_class(&arr[4], find_index(4 * WSIZE));
  insert_into_size_class(&arr[8], find_index(4 * WSIZE));
  insert_into_size_class(&arr[12], find_index(4 * WSIZE));

#ifdef __LIFO_ORDERING__
  void *ordering[] = {arr + 12, arr + 8, arr + 4, arr + 0};
#else
  void *ordering[] = {arr + 0, arr + 4, arr + 8, arr + 12};
#endif

  ck_assert(find_index(4 * WSIZE) == 0);
  void *head = segregated_free_list[find_index(4 * WSIZE)];
  for (i = 0; i < 16; i += 4) {
    ck_assert(head == ordering[i / 4]);
    head = GET_NEXT_PTR(head);
  }
  ck_assert(i == 16);
}
END_TEST

extern void delete_from_size_class(void *hdrp, int index);
START_TEST(delete_from_size_class_middle_node) {
  int i = 0;
  for (; i < index_arr_size; ++i) {
    segregated_free_list[i] = NULL;
  }

  size_t arr[16];
  init_free_block(&arr[0], 4 * WSIZE);
  init_free_block(&arr[4], 4 * WSIZE);
  init_free_block(&arr[8], 4 * WSIZE);
  init_free_block(&arr[12], 4 * WSIZE);
  insert_into_size_class(&arr[0], find_index(4 * WSIZE));
  insert_into_size_class(&arr[4], find_index(4 * WSIZE));
  insert_into_size_class(&arr[8], find_index(4 * WSIZE));
  insert_into_size_class(&arr[12], find_index(4 * WSIZE));

#ifdef __LIFO_ORDERING__
  void *ordering[] = {arr + 12, arr + 8, arr + 0};
#else
  void *ordering[] = {arr + 0, arr + 8, arr + 12};
#endif
  ck_assert(find_index(4 * WSIZE) == 0);
  delete_from_size_class(arr + 4, find_index(4 * WSIZE));
  ck_assert(!is_in_size_class(arr + 4));
  void *head = segregated_free_list[find_index(4 * WSIZE)];
  for (i = 0; i < 12; i += 4) {
    ck_assert(head == ordering[i / 4]);
    head = GET_NEXT_PTR(head);
  }
  ck_assert(i == 12);
  ck_assert(head == NULL);
}
END_TEST

START_TEST(delete_from_size_class_first_node) {
  int i = 0;
  for (; i < index_arr_size; ++i) {
    segregated_free_list[i] = NULL;
  }

  size_t arr[16];
  init_free_block(&arr[0], 4 * WSIZE);
  init_free_block(&arr[4], 4 * WSIZE);
  init_free_block(&arr[8], 4 * WSIZE);
  init_free_block(&arr[12], 4 * WSIZE);
  insert_into_size_class(&arr[0], find_index(4 * WSIZE));
  insert_into_size_class(&arr[4], find_index(4 * WSIZE));
  insert_into_size_class(&arr[8], find_index(4 * WSIZE));
  insert_into_size_class(&arr[12], find_index(4 * WSIZE));

#ifdef __LIFO_ORDERING__
  void *ordering[] = {arr + 8, arr + 4, arr + 0};
  void *del_head = arr + 12;
#else
  void *ordering[] = {arr + 4, arr + 8, arr + 12};
  void *del_head = arr + 0;
#endif
  ck_assert(find_index(4 * WSIZE) == 0);
  delete_from_size_class(del_head, find_index(4 * WSIZE));
  ck_assert(!is_in_size_class(del_head));
  void *head = segregated_free_list[find_index(4 * WSIZE)];
  for (i = 0; i < 12; i += 4) {
    ck_assert(head == ordering[i / 4]);
    head = GET_NEXT_PTR(head);
  }
  ck_assert(i == 12);
  ck_assert(head == NULL);
}
END_TEST

Suite *test_suit(void) {
  Suite *s = suite_create("test");
  TCase *tc_core = tcase_create("Core");
  tcase_add_test(tc_core, test_macros);
  tcase_add_test(tc_core, find_index_test);
  tcase_add_test(tc_core, init_free_block_test);
  tcase_add_test(tc_core, insert_into_size_class_test);
  tcase_add_test(tc_core, delete_from_size_class_middle_node);
  tcase_add_test(tc_core, delete_from_size_class_first_node);
  suite_add_tcase(s, tc_core);
  return s;
}

int main() {
  int num_failed;
  Suite *s = test_suit();
  SRunner *sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  num_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return 0;
}