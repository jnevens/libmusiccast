/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2015-2026 Jasper Nevens
 */
#include <check.h>
#include <stdlib.h>
#include <unistd.h>

#include <musiccast.h>

START_TEST(test_discover_list_free_null)
{
	musiccast_discover_list_free(NULL);
}
END_TEST

START_TEST(test_discover_short_timeout)
{
	musiccast_discover_list_t *found;

	found = musiccast_discover(200);
	ck_assert_ptr_nonnull(found);
	/* Zero or more devices on the LAN; either is fine. */
	musiccast_discover_list_free(found);
}
END_TEST

START_TEST(test_init_unreachable_host)
{
	musiccast_conn_t *mc;

	/* Loopback address with no listener on port 80 (not 127.0.0.1 — may run a web server). */
	mc = musiccast_init("127.0.0.99");
	ck_assert_ptr_null(mc);
}
END_TEST

START_TEST(test_init_invalid_hostname)
{
	musiccast_conn_t *mc;

	/* Unresolvable name must not abort the process. */
	mc = musiccast_init("libmusiccast.invalid.");
	ck_assert_ptr_null(mc);
}
END_TEST

Suite *libmusiccast_suite(void)
{
	Suite *s;
	TCase *tc_core;
	TCase *tc_discover;

	s = suite_create("musiccast");

	tc_discover = tcase_create("discover");
	tcase_add_test(tc_discover, test_discover_list_free_null);
	tcase_add_test(tc_discover, test_discover_short_timeout);
	suite_add_tcase(s, tc_discover);

	tc_core = tcase_create("connection");
	tcase_add_test(tc_core, test_init_unreachable_host);
	tcase_add_test(tc_core, test_init_invalid_hostname);
	suite_add_tcase(s, tc_core);

	return s;
}

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = libmusiccast_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
