#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <check.h>
#include "mm.h"
#include "mm_macros.h"

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

Suite *macros_test_suit(void) {
  Suite *s = suite_create("macros_test");
  TCase *tc_core = tcase_create("Core");
  tcase_add_test(tc_core, test_macros);
  suite_add_tcase(s, tc_core);
  return s;
}

int main() {
  int num_failed;
  Suite *s = macros_test_suit();
  SRunner *sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  num_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return 0;
}