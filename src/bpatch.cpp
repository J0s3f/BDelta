/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <string.h>
#include "file.h"
#include "compatibility.h"

typedef int64_t pos;

bool copy_bytes_to_file(FILE *infile, FILE *outfile, unsigned numleft) {
	size_t numread;
	do {
		char buf[1024];
		numread = fread(buf, 1, numleft > 1024 ? 1024 : numleft, infile);
		if (fwrite(buf, 1, numread, outfile) != numread) {
			printf("Could not write temporary data.  Possibly out of space\n");
			return false;
		}
		numleft -= numread;
	} while (numleft && !(numread < 1024 && numleft));
	return (numleft == 0);
}

int main(int argc, char **argv) {
	try {
		if (argc != 4) {
			printf("usage: bpatch <oldfile> <newfile> <patchfile>\n");
			printf("needs a reference file, file to output, and patchfile:\n");
			return 1;
		}

		if (!fileExists(argv[1]) || !fileExists(argv[3])) {
			printf("one of the input files does not exist\n");
			return 1;
		}

		FILE *patchfile = fopen(argv[3], "rb");
		char magic[3];
		fread_fixed(patchfile, magic, 3);
		if (strncmp(magic, "BDT", 3)) {
			printf("Given file is not a recognized patchfile\n");
			return 1;
		}
		unsigned short version = read_varint(patchfile);
		if (version != 1 && version != 2) {
			printf("unsupported patch version\n");
			return 1;
		}
		pos size1 = read_varint(patchfile),
		    size2 = read_varint(patchfile);

		unsigned nummatches = read_varint(patchfile);

		pos * copyloc1 = new pos[nummatches + 1]; // locations in ref file
		pos * copynump = new pos[nummatches + 1]; // num from patch file
		pos * copynumr = new pos[nummatches + 1]; // num frum ref file

		for (unsigned i = 0; i < nummatches; ++i) {

		  copyloc1[i] = read_varint(patchfile);
		  copynump[i] = read_varint(patchfile);
		  copynumr[i] = read_varint(patchfile);

			size2 -= copynump[i] + copynumr[i];
		}
		if (size2) {
			copyloc1[nummatches] = 0; copynumr[nummatches] = 0;
			copynump[nummatches] = size2;
			++nummatches;
		}

		FILE *ref = fopen(argv[1], "rb");
		FILE *outfile = fopen(argv[2], "wb");

		for (unsigned i = 0; i < nummatches; ++i) {
			if (!copy_bytes_to_file(patchfile, outfile, copynump[i])) {
				printf("Error.  patchfile is truncated\n");
				return -1;
			}

			pos copyloc = copyloc1[i];
			fseeko(ref, copyloc, SEEK_CUR);

			if (!copy_bytes_to_file(ref, outfile, copynumr[i])) {
			  printf("Error while copying from reference file (ofs %ld, %u bytes)\n", ftello(ref), copynumr[i]);
			  return -1;
			}
		}

		delete [] copynumr;
		delete [] copynump;
		delete [] copyloc1;

	} catch (const char * desc){
		fprintf (stderr, "FATAL: %s\n", desc);
		return -1;
	}

	return 0;
}
