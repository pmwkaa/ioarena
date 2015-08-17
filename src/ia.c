
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

#include <ioarena.h>

int ia_init(ia *a, int argc, char **argv)
{
	ia_log("IOARENA (embedded storage benchmarking)");
	ia_log("");
	int rc;
	rc = ia_configinit(&a->conf);
	if (rc == -1)
		return -1;
	rc = ia_configparse(&a->conf, argc, argv);
	if (rc == -1 || rc == 1)
		return rc;
	rc = ia_kvinit(&a->kv, a->conf.ksize, a->conf.vsize);
	if (rc == -1)
		return -1;
	ia_configprint(&a->conf);
	ia_log("");
	ia_histogram_init(&a->hg);
	a->driver = a->conf.driver_if;
	rc = a->driver->open();
	if (rc == -1)
		return -1;
	return 0;
}

void ia_free(ia *a)
{
	if (a->driver)
		a->driver->close();
	ia_kvfree(&a->kv);
	ia_configfree(&a->conf);
}

int ia_run(ia *a)
{
	iabenchmark bench = IA_SET;
	for (; bench < IA_MAX; bench++) {
		if (! a->conf.benchmark_list[bench])
			continue;
		char *name = ia_benchmarkof(bench);
		ia_log("<<>> %s.%s", a->conf.driver, name);
		ia_histogram_init(&a->hg);
		if (a->conf.csv)
			ia_histogram_open(&a->hg, a->conf.driver, name);
		ia_kvreset(&a->kv);
		int rc = a->driver->run(bench);
		if (rc == -1)
			return -1;
		ia_histogram_print(&a->hg);
		if (a->conf.csv)
			ia_histogram_close(&a->hg);
		ia_log("");
	}
	ia_log("complete.");
	return 0;
}
