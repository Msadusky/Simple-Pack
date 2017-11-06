/*****************************************************
 * sunpack - Simple Unpack
 *
 * This program unpacks files created
 * with spack (simple pack).
 *
 * Author: Michael Sadusky <msadusky@kent.edu>
 * Version: 1.0
 ******************************************************/

/*********************************
 * "spack" is a simplified
 * variable record lenth binary
 * file specification. It's sole
 * purpose is as a demonstration
 * of how any binary data can be
 * packed into a file.
 *********************************/
#include <stdio.h>
#include <stdint.h>

#ifndef _SPACK_H
#define _SPACK_H

#define SPACK_FILE 0x1337 /* File Identifier */
#define SPACK_VERSION 1

#define MAX_FILE_BUFF 4096

typedef struct sunpack_args {
    unsigned char list;
    unsigned char extract;
    unsigned char extract_single;
    char *dest;
    char *file_name;
    char *packfile;
} sunpack_args;

typedef struct spack_record {
    unsigned short name_len;
    char *filename;
    uint64_t filesize;
    char *file_data1;
	char *file_data2;
    long file_pos;
} spack_record;


/*
 * Writes the intial 4 bytes to the file to
 * specify that this is a spack file.
 */
int write_spack_header(FILE *);

int read_spack_header(FILE *);

int pack_dir(char *, FILE *);

void listFiles(char *);

void extractFile(char *, char *, char *);

void extractAll(char *, char *);

void usage(char **);

void parse_args(int, char **, struct sunpack_args *);

void print_file_record(char *, uint64_t);

#endif
