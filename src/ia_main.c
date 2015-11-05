
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

#include <ioarena.h>

ia ioarena;

int
main(int argc, char *argv[])
{
	memset(&ioarena, 0, sizeof(ioarena));
	int rc = ia_init(&ioarena, argc, argv);
	if (rc < 0 || rc)
		goto done;
	rc = ia_run(&ioarena);
done:
	ia_free(&ioarena);
	switch (rc) {
	case -1: rc = 1;
		break;
	case  1: rc = 0; /* usage */
		break;
	}
	return rc;
}
