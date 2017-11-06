/*****************************************************
 * sunpack - Simple Unpack
 *
 * This program unpacks files created
 * with spack (simple pack).
 *
 * Author: Michael Sadusky <msadusky@kent.edu>
 * Version: 1.0
 ******************************************************/

#include "spack.h"
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>

int main(int argc, char **argv)
{

	struct sunpack_args flags;

    parse_args(argc, argv, &flags);

    if( flags.list )
	{
        listFiles(flags.packfile);
    }
    else if(flags.extract && flags.extract_single)
	{
		extractFile(flags.file_name, flags.packfile, flags.dest);
    }
    else if( flags.extract )
    {
        extractAll(flags.packfile, flags.dest);
    }
    return 0;
}

//=============================================================================
//=============================================================================
//=============================================================================

/**********************************************
 * listFiles
 *
 * Finds all files packed within .spack file
 * and prints the filenames and sizes. Does not
 * extract the file from the .spack
 ***********************************************/
void listFiles(char * openFile)
{
	// Declaring stream
	FILE *pFile;

	// Struct variable to hold record information
	spack_record sr;

	// Set the memory of sr to 0
	memset( &sr, 0, sizeof(spack_record) );

	// Open the compressed file
    pFile = fopen(openFile, "rb");

	// Error checking
	if( pFile == NULL )
	{
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}
	else
	{
		// Checking to see that it's a .spack file
		unsigned short identifier = 0;
		fread( &identifier, sizeof(unsigned short), 1, pFile );
		if(identifier != 4919)
		{
			perror("The following error occurred");
			exit(EXIT_FAILURE);
		}
		identifier = 0;
		fread( &identifier, sizeof(unsigned short), 1, pFile );
		if(identifier != 1)
		{
			perror("The following error occurred");
			exit(EXIT_FAILURE);
		}

		// While a file exists, read the file
	   	while(fread( &sr.name_len, sizeof(unsigned short), 1, pFile ) == 1)
	   	{
			// Reads in the packed filename
			sr.filename = (char *)malloc(sizeof(char) * sr.name_len);
			sr.filename[0] = 0;
			fread( sr.filename, sizeof(char), sr.name_len, pFile );

			// Reads in the filesize
			fread( &sr.filesize, sizeof(uint64_t), 1, pFile );

			// Initializes empty var of type long long
			uint64_t size = 0;

			// Sets value of size to filesize
			size = sr.filesize;

			// Grabs the first half of size, 32 bits of it. Long long is 64 bits.
			unsigned int upper32 = (size >> 32);

			// Grabs the other half
			unsigned int lower32 = size & 0xffFFffFF;

			// Reads in the first half of the file data
			sr.file_data1 = (char *)malloc(sizeof(char) * lower32);
			fread( sr.file_data1, sizeof(char), lower32, pFile );

			// If the split variable was big enough to use both integers, use the second half
			if(upper32 > 0)
			{
				// Reads in second half of the data
				sr.file_data2 = (char *)malloc(sizeof(char) * upper32);
				fread( sr.file_data2, sizeof(char), upper32, pFile );
			}

			// Prints the filename, and filesize
			print_file_record(sr.filename, sr.filesize);

		}

		// Close the file
		fclose(pFile);
		free(sr.filename);
		free(sr.file_data1);
		free(sr.file_data2);
	}

}

//=============================================================================
//=============================================================================
//=============================================================================

/**********************************************
 * extractFile
 *
 * Finds specified file packed within .spack
 * file and extracts it by finding the correct
 * beginning and end of the requested file, and
 * writes the file to the specified directory.
 ***********************************************/
void extractFile(char * fName, char * openFile, char * dest)
{
	// Declaring stream
	FILE *pFile;

	// Struct variable to hold record information
	spack_record sr;

	// Set the memory of sr to 0
	memset( &sr, 0, sizeof(spack_record) );

	// Open the compressed file
    pFile = fopen(openFile, "rb");

	// Error checking
	if( pFile == NULL )
	{
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}
	else
	{
		// Checking to see that it's a .spack file
		unsigned short identifier = 0;
		fread( &identifier, sizeof(unsigned short), 1, pFile );
		if(identifier != 4919)
		{
			perror("The following error occurred");
			exit(EXIT_FAILURE);
		}
		identifier = 0;
		fread( &identifier, sizeof(unsigned short), 1, pFile );
		if(identifier != 1)
		{
			perror("The following error occurred");
			exit(EXIT_FAILURE);
		}

		int found = 0;

		// While a file exists, read the file
	   	while(fread( &sr.name_len, sizeof(unsigned short), 1, pFile ) == 1)
	   	{
			// Reads in the packed filename
			sr.filename = (char *)malloc(sizeof(char) * sr.name_len);
			sr.filename[0] = 0;
			fread( sr.filename, sizeof(char), sr.name_len, pFile );

			// Reads in the filesize
			fread( &sr.filesize, sizeof(uint64_t), 1, pFile );

			// Initializes empty var of type long long
			uint64_t size = 0;

			// Sets value of size to filesize
			size = sr.filesize;

			// Grabs the first half of size, 32 bits of it. Long long is 64 bits.
			unsigned int upper32 = (size >> 32);

			// Grabs the other half
			unsigned int lower32 = size & 0xffFFffFF;

			// Reads in the first half of the file data
			sr.file_data1 = (char *)malloc(sizeof(char) * lower32);
			sr.file_data1[0] = 0;
			fread( sr.file_data1, sizeof(char), lower32, pFile );

			// If the split variable was big enough to use both integers, use the second half
			if(upper32 > 0)
			{
				// Reads in second half of the data
				sr.file_data2 = (char *)malloc(sizeof(char) * upper32);
				sr.file_data2[0] = 0;
				fread( sr.file_data2, sizeof(char), upper32, pFile );
			}

			// If requested filename matches filename in pack
			if(strcmp (sr.filename, fName) == 0)
			{
				//  Declare one has been found
				found = 1;

				// Allocate enough memory for the filename, requested dir, and extra "\"
				char * dirname = (char *)malloc(strlen(sr.filename)+strlen(dest)+4);
				dirname[0] = 0;

				// Initialize slash for concatenation
				char * slash = "/";

				// Add dest to string dirname
				strcat(dirname, dest);

				// Add the slash
				strcat(dirname, slash);

				// Add the name of the requested file
				strcat(dirname, sr.filename);

				// Declare the stream
				FILE * wFile;

				// Create the file to be written too
				wFile = fopen(dirname, "wb");

				// Write all the data from the packed file to the new file
				fwrite( sr.file_data1, sizeof(char), lower32, wFile );

				// If the split variable was big enough to use both integers, use the second half
				if(upper32 > 0)
				{
					// Writes in second half of the data
					fwrite( sr.file_data2, sizeof(char), upper32, wFile );
				}

				// Close the file
				fclose(wFile);
				free (dirname);
			}
		}

		// If the file wasn't found
		if(found == 0)
		{
			// Print the file wasn't found
			printf("Your file was not found!\n");
		}

		// Close the file
		fclose(pFile);
		free(sr.filename);
		free(sr.file_data1);
		free(sr.file_data2);
	}
}

//=============================================================================
//=============================================================================
//=============================================================================

/**********************************************
 * extractAll
 *
 * Finds all files packed within .spack file
 * and extracts by finding the correct
 * beginning and end to each to packed file.
 ***********************************************/
void extractAll(char * openFile, char * dest)
{

	// Declaring stream
	FILE *pFile;

	// Struct variable to hold record information
	spack_record sr;

	// Set the memory of sr to 0
	memset( &sr, 0, sizeof(spack_record) );

	// Open the compressed file
    pFile = fopen(openFile, "rb");

	// Error checking
	if( pFile == NULL )
	{
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}
	else
	{
		// Checking to see that it's a .spack file
		unsigned short identifier = 0;
		fread( &identifier, sizeof(unsigned short), 1, pFile );
		if(identifier != 4919)
		{
			perror("The following error occurred");
			exit(EXIT_FAILURE);
		}
		identifier = 0;
		fread( &identifier, sizeof(unsigned short), 1, pFile );
		if(identifier != 1)
		{
			perror("The following error occurred");
			exit(EXIT_FAILURE);
		}

		// While a file exists, read the file
	   	while(fread( &sr.name_len, sizeof(unsigned short), 1, pFile ) == 1)
	   	{
			// Reads in the packed filename
			sr.filename = (char *)malloc(sizeof(char) * sr.name_len);
			sr.filename[0] = 0;
			fread( sr.filename, sizeof(char), sr.name_len, pFile );

			// Reads in the filesize
			fread( &sr.filesize, sizeof(uint64_t), 1, pFile );

			// Initializes empty var of type long long
			uint64_t size = 0;

			// Sets value of size to filesize
			size = sr.filesize;

			// Grabs the first half of size, 32 bits of it. Long long is 64 bits.
			unsigned int upper32 = (size >> 32);

			// Grabs the other half
			unsigned int lower32 = size & 0xffFFffFF;

			// Reads in the first half of the file data
			sr.file_data1 = (char *)malloc(sizeof(char) * lower32);
			sr.file_data1[0] = 0;
			fread( sr.file_data1, sizeof(char), lower32, pFile );

			// If the split variable was big enough to use both integers, use the second half
			if(upper32 > 0)
			{
				// Reads in second half of the data
				sr.file_data2 = (char *)malloc(sizeof(char) * upper32);
				sr.file_data2[0] = 0;
				fread( sr.file_data2, sizeof(char), upper32, pFile );
			}

			// Allocate enough memory for the filename, requested dir, \n, and extra "\"
			char * dirname = (char *)malloc(strlen(sr.filename)+strlen(dest) + 4);
			dirname[0] = 0;

			// Initialize slash for concatenation
			char * slash = "/";

			// Add dest to string dirname
			strcat(dirname, dest);

			// Add the slash
			strcat(dirname, slash);

			// Add the name of the file
			strcat(dirname, sr.filename);

			// Declare the stream
			FILE *wFile;

			// Create the file to be written too
			wFile = fopen(dirname, "wb");

			// Write all the data from the packed file to the new file
			fwrite( sr.file_data1, sizeof(char), lower32, wFile );

			// If the split variable was big enough to use both integers, use the second half
			if(upper32 > 0)
			{
				// Writes in second half of the data
				fwrite( sr.file_data2, sizeof(char), upper32, wFile );
			}

			// Close the file
			fclose(wFile);
			free (dirname);
		}

		// Close the file
		fclose(pFile);
		free(sr.filename);
		free(sr.file_data1);
		free(sr.file_data2);
	}

}

//=============================================================================
//=============================================================================
//=============================================================================

/**********************************************
 * usage
 *
 * Takes argv from main, and prints out
 * command line usage for the program
 * (i.e. expected command arguments )
 * This function terminates the program.
 * It's meant to be run when the user supplied
 * wrong command line arguments, and therefore
 * the program has to stop.
 ***********************************************/
void usage(char **argv){
    printf("Usage: \n");
    printf("%s -h \n", argv[0]);
    printf("or\n");
    printf("%s [ -l | -x DIR ] [ -f FILE ] PACK_FILE\n",argv[0]);
    printf("Where:\n");
    printf("%-10s - Print this help message and exit.\n", "-h");
    printf("%-10s - Print filename and size of all files in PACK_FILE.\n", "-l");
    printf("%-10s - Extract files into directory DIR.\n", "-x DIR");
    printf("%-10s - Extract only file named FILE.\n", "-f FILE");
    printf("%-10s - The name of the packed file to operate on (required!).\n","PACK_FILE");
    printf("\nNote: -l and -x are mutually exclusive. Only one can be specified at a time.\n");
    exit(1);
}

/***************************************************
 * parse_args
 *
 * This function takes the args from the command
 * line and parses them using getopt, filling in
 * in the sunpack_args struct with the options
 * the user passed in.
 *
 * No return value. Function just returns on success,
 * terminates the program with usage info on error.
 ****************************************************/
void parse_args(int argc, char **argv, struct sunpack_args * flags){
    int c = 0;

    //make sure there ARE arguments
    if( argc < 2 ) usage(argv);

    //Initialize struct
    flags->list = 0;
    flags->extract = 0;
    flags->extract_single = 0;
    flags->dest = NULL;
    flags->file_name = NULL;


    while( ( c = getopt( argc, argv, "hx:f:l" ) ) != -1 ){
        fprintf(stderr,"%c\n",c);
        switch(c){
            case 'h':
                fprintf(stderr,"Asked for help!\n");
                usage(argv);
                break;
            case 'l':
                flags->list++;
                break;
            case 'x':
                flags->extract++;
                flags->dest = optarg;
                fprintf(stderr,"DEST: %s\n",optarg);
                break;
            case 'f':
                flags->file_name = optarg;
                flags->extract_single++;
                break;
            default:
                //unknown option
                fprintf(stderr,"Uknown option specified!\n");
                usage(argv);
        }
    }

    if( flags->list > 0 && flags->extract > 0 ){
        //both flags shouldn't be set, so print help
        fprintf(stderr,"You can't specify -l and -x at the same time!\n");
        usage(argv);
    } else if( flags->extract_single > 0 && flags->extract < 1 ){
        fprintf(stderr,"If you specify -f, you have to also specify -x!\n");
        usage(argv);
    }

    if( optind < argc ){
        flags->packfile = argv[optind];
    }else{
        fprintf(stderr,"No pack file specified!\n");
        usage(argv);
    }
}

/********************************************
 * print_file_record
 *
 * This function prints out a single file
 * record from a pack file.
 *
 * Having everyone use the same function,
 * ensures that everyone's code will have
 * a uniform and testable output in a
 * classroom setting.
 *******************************************/
void print_file_record(char *fname, uint64_t fsize){
    printf("%-20lld\t%s\n", fsize, fname);
}
