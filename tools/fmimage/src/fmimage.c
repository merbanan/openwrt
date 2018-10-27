#include <stdio.h>
#include <strings.h>
#include <stdlib.h>

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

#define BUF	0x10000

/*
 * Fludmesh Image Format
 * 8 bytes fixed header, plus a 8 byte header for every partition
 *
 * +--------+---+
 * |FRESCUEX| 8 | "FRESCUE" string + the number of partitions (one byte, 1-255)
 * +--------+---+
 * |LENGTH1 | 4 | the length of the partition
 * +--------+---+
 * |ADDRESS1| 4 | the address where to write the partition to
 * +--------+---+
 * | DATA1  | X | the partition data
 * +--------+---+
 * |LENGTHX.|...| and so on of all partitions
 * +--------+---+
 * | CRC32  | 8 | the CRC32 of the full image file
 * +--------+---+
 *
 * usage: fmimage output partition1@address ...
 * example: fmimage firmware.bin kernel@50000 rootfs@150000 conf@df0000
 */

int main(int argc, char *argv[])
{
	int i;
	if(argc < 3) {
		fprintf(stderr, "usage: %s image [file1@address]...\n", argv[0]);
		return 1;
	}

	FILE *image = fopen(argv[1], "w");
	if(!image) {
		fprintf(stderr, "cannot create '%s': %m\n", argv[1]);
		return 2;
	}

	unsigned char header[8] = "FRESCUE";
	header[7] = (argc - 2) & 0xf;
	fwrite(header, 1, sizeof(header), image);
	uLong crc = crc32(0L, (const Bytef *)header, sizeof(header));

	for(i = 2; i < argc; i++) {
		FILE *file;
		char *at = index(argv[i], '@');

		if(!at) {
			fprintf(stderr, "invalid argument %s\n", argv[i]);
			return 3;
		}
		*at = 0;
		at++;
		long addr = strtol(at, NULL, 16);
		if(addr <= 0 || addr > 0x1000000) {
			fprintf(stderr, "invalid address %s\n", at);
			return 4;
		}

		struct stat filestat;
		if(stat(argv[i], &filestat) == -1) {
			fprintf(stderr, "stat failed on '%s': %m\n", argv[i]);
			return 5;
		}

		file = fopen(argv[i], "r");
		if(!file) {
			fprintf(stderr, "cannot open '%s': %m\n", argv[i]);
			return 6;
		}

		uint32_t header[2] = {htobe32(filestat.st_size), htobe32(addr)};
		if(fwrite(header, 4, 2, image) != 2) {
			fprintf(stderr, "writing failed: %m\n");
			return 7;
		}
		printf("partition %d:\n	size:		%lu\n	address:	%lx\n", i - 1, filestat.st_size, addr);
		crc = crc32(crc, (const Bytef *)header, sizeof(header));

		char buf[BUF];
		size_t readed;
		while((readed = fread(buf, 1, BUF, file))) {
			if(fwrite(buf, 1, readed, image) != readed) {
				fprintf(stderr, "writing failed: %m\n");
				return 7;
			} else
				crc = crc32(crc, (const Bytef *)buf, readed);
		}
		// data MUST be aligned or the bootloader freezes
		if(filestat.st_size & 3) {
			char zero[3] = {0};
			fwrite(zero, 1, 4 - (filestat.st_size & 3), image);
			crc = crc32(crc, (const Bytef *)zero, 4 - (filestat.st_size & 3));
		}

		fclose(file);
	}

	printf("crc32: %lx\n", crc);
	crc = htobe32(crc);
	fwrite(&crc, 1, 4, image);
	fclose(image);
	return 0;
}
