#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "install.h"
#include "lib/rpmlib.h"
#include "messages.h"

static int hashesPrinted = 0;

static void printHash(const unsigned long amount, const unsigned long total);
static void printPercent(const unsigned long amount, const unsigned long total);

static void printHash(const unsigned long amount, const unsigned long total) {
    int hashesNeeded;

    if (hashesPrinted != 50) {
	hashesNeeded = 50 * (((float) amount) / total);
	while (hashesNeeded > hashesPrinted) {
	    printf("#");
	    hashesPrinted++;
	}
	fflush(stdout);
	hashesPrinted = hashesNeeded;

	if (hashesPrinted == 50)
	    printf("\n");
    }
}

static void printPercent(const unsigned long amount, const unsigned long total) 
{
    printf("%%%% %f\n", (float) (((float) amount) / total) * 100);
}

void doInstall(char * prefix, char * arg, int installFlags, int interfaceFlags) {
    rpmdb db;
    int fd;
    int mode, rc;
    char * chptr;
    notifyFunction fn;

    hashesPrinted = 0;

    if (installFlags & INSTALL_TEST) 
	mode = O_RDONLY;
    else
	mode = O_RDWR | O_EXCL;

    if (interfaceFlags & RPMINSTALL_PERCENT)
	fn = printPercent;
    else if (interfaceFlags & RPMINSTALL_HASH)
	fn = printHash;
    else
	fn = NULL;
	
    if (!rpmdbOpen(prefix, &db, mode, 0644)) {
	/* try opening it O_CREAT */
	mode |= O_CREAT;
	if (!rpmdbOpen(prefix, &db, mode, 0644)) {
	    fprintf(stderr, "error: cannot open %s/var/lib/rpm/packages.rpm\n", 
			prefix);
	    exit(1);
	}
    }

    message(MESS_DEBUG, "installing %s\n", arg);
    fd = open(arg, O_RDONLY);
    if (fd < 0) {
	rpmdbClose(db);
	fprintf(stderr, "error: cannot open %s\n", arg);
	return;
    }

    if (interfaceFlags & RPMINSTALL_PERCENT) 
	printf("%%f %s\n", arg);
    else if (isVerbose() && (interfaceFlags & RPMINSTALL_HASH)) {
	chptr = strrchr(arg, '/');
	if (!chptr)
	    chptr = arg;
	else
	    chptr++;

	printf("%-28s", chptr);
    } else if (isVerbose())
	printf("Installing %s\n", arg);

    rc = rpmInstallPackage(prefix, db, fd, installFlags, fn);
    if (rc == 1) {
	fprintf(stderr, "error: %s cannot be installed\n", arg);
    }

    close(fd);
    rpmdbClose(db);
}

void doUninstall(char * prefix, char * arg, int test, int uninstallFlags) {
    rpmdb db;
    dbIndexSet matches;
    int i;
    int mode;

    if (test) 
	mode = O_RDONLY;
    else
	mode = O_RDWR | O_EXCL;
	
    if (!rpmdbOpen(prefix, &db, mode, 0644)) {
	fprintf(stderr, "cannot open %s/var/lib/rpm/packages.rpm\n", prefix);
	exit(1);
    }
   
    if (rpmdbFindPackage(db, arg, &matches)) {
	fprintf(stderr, "package %s is not installed\n", arg);
    } else {
	if (matches.count > 1) {
	    fprintf(stderr, "\"%s\" specifies multiple packages\n", arg);
	}
	else { 
	    for (i = 0; i < matches.count; i++) {
		rpmRemovePackage(prefix, db, matches.recs[i].recOffset, test);
	    }
	}

	freeDBIndexRecord(matches);
    }

    rpmdbClose(db);
}
