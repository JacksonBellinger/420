#define _XOPEN_SOURCE
#define _GNU_SOURCE
#define __STDC_LIB_EXT1__
#include <unistd.h>
#include <stdlib.h> // rand
#include <stdio.h> // puts, printf, etc
#include <time.h> // time
#include <string.h>
#include <math.h>
#include <crypt.h>
#include <errno.h>
#include <mpi.h> // mpi stuff

int num_words = 235886;

void print_char_vals(char* x)
{
	printf("printing: %s\n", x);
	int i;
	for(i = 0; i < strlen(x); i++)
		printf("%d ", x[i]);
	printf("\n");
}
MPI_Offset find_offset(int rank, int num_nodes, MPI_File* file)
{//sets the offset of file to where the node should start
//and returns the length of how many bytes it should read
	MPI_File_seek(*file, 0, MPI_SEEK_END);
	MPI_Offset file_len;
	MPI_File_get_size(*file, &file_len);
	MPI_Offset starting_point = (file_len / num_nodes) * rank;
	MPI_Offset ending_point = (file_len / num_nodes) * (rank + 1) -1;

	MPI_Status status;
	char c = ' ';
	//printf("Node#%d starting at %d\n", rank, starting_point);
	MPI_File_seek(*file, starting_point, MPI_SEEK_SET);
	if(rank != 0)
	{
		while( c != '\n' && c != 0)
		{
			//MPI_File_seek(file, -2, MPI_SEEK_CUR);
			MPI_File_read(*file, &c, 1, MPI_CHAR, &status);
			//printf("\tNode #%d s reading char %c\n", rank, c);
		}
		MPI_File_get_position(*file, &starting_point);
	//	printf("\n\noffset before seek is %d\n", starting_point);
		MPI_File_get_position(*file, &starting_point);
		//printf("offset after seek is %d\n\n", starting_point);
	}
	//printf("Node#%d starting at %d\n", rank, starting_point);

	c = ' ';
	MPI_File_seek(*file, ending_point, MPI_SEEK_SET);
	while( c != '\n'  && c != 0)
	{
		MPI_File_read(*file, &c, 1, MPI_CHAR, &status);
		//printf("\tNode #%d e reading char %c\n", rank, c);
	}
	MPI_File_seek(*file, -1, MPI_SEEK_CUR);

	MPI_File_get_position(*file, &ending_point);
	//printf("Node #%d ended at %d\n", rank, ending_point);//minus 2 so it goes back past the newline

	MPI_File_seek(*file, starting_point, MPI_SEEK_SET);
	return ending_point;
}

int main(int argc, char** argv)
{
	MPI_Init(NULL, NULL);

	MPI_Comm world = MPI_COMM_WORLD;
	int rank, world_size;
	MPI_Comm_rank(world, &rank);
	MPI_Comm_size(world, &world_size);
	MPI_File mpi_words_file;


	MPI_File_open(
		world,
		"newoutputwords",
		MPI_MODE_RDONLY,
		MPI_INFO_NULL,
		&mpi_words_file
	);

	//FILE* test = fopen("test_offset.txt", "r");
	int endpoint = find_offset(rank, world_size, &mpi_words_file);
	//printf("Node#%d should start at %d and end at ")

	FILE* shadow_fp;

	shadow_fp = fopen("shadow2", "r");
	if (shadow_fp == NULL)
		exit(EXIT_FAILURE);

	int num_users = 0;
	char c;
	// Extract characters from file and store in character c
	for (c = getc(shadow_fp); c != EOF; c = getc(shadow_fp))
		if (c == '\n') // Increment count if this character is newline
			num_users++;

	char** user_names = (char**) calloc(num_users, sizeof(char*));
	char** user_pass_hashes = (char**) calloc(num_users, sizeof(char*));
	char** user_passwords = (char**) calloc(num_users, sizeof(char*));

	int i;
	for(i = 0; i < num_users; i++) {
		user_names[i] = calloc(255,sizeof(char*));
		user_pass_hashes[i] =calloc(255,sizeof(char*));
		user_passwords[i] = calloc(255,sizeof(char*));
	}
	char* shadow_line = NULL;
	size_t len = 0;

	int pass_num = 0;
	rewind(shadow_fp);
	while (getline(&shadow_line, &len, shadow_fp) != -1)
	{
		//printf("password line: %s\n", line);
		char* pass_hash = strchr(shadow_line, ':') + 1;
		//pass_hash[strlen(pass_hash)-1] = 0;
		int colon_index = strchr(shadow_line, ':') - shadow_line;

		user_names[pass_num] = calloc(10, sizeof(char));
		user_pass_hashes[pass_num] = calloc(100, sizeof(char));
		user_passwords[pass_num] = calloc(40, sizeof(char));
		strncpy(user_names[pass_num], shadow_line, colon_index);
		strncpy(user_pass_hashes[pass_num], pass_hash, strlen(pass_hash)-1);
		//int id_salt_len = strrchr(line, '$') - pass_hash;
		// char* "$1$ab$" =  malloc("$1$ab$"_len * sizeof(char) + 1);//for some reason only this one needs a null terminating char
		// memset("$1$ab$", 0, "$1$ab$"_len+1);//zero out the array
		// strncpy("$1$ab$", pass_hash, "$1$ab$"_len);
		// "$1$ab$" = "$1$ab$";
		if(rank ==0)
			printf("\nusername: %s\n$id$salt: %s\npassword hash: %s\n", user_names[pass_num], "$1$ab$", user_pass_hashes[pass_num]);
		pass_num++;

	}

	int word_num = 0;
	char* word = calloc(50, sizeof(char));
	char* word2 = calloc(50, sizeof(char));

	for(i = 0; i < 10; i++)
		word[i] = '0';
	len = 0;

	MPI_Status status;

	MPI_Offset words_pointer_pos = 0;
	//rewind(words_fp);

	pass_num = 0;
	while (words_pointer_pos <= endpoint)
	{
		MPI_File_get_position(mpi_words_file, &words_pointer_pos);
		c = ' ';
		i = 10;
		while(c != '\n' && c != 0)
		{
			MPI_File_read(mpi_words_file, &c, 1, MPI_CHAR, &status);
			word[i++] = c;
		}
		//MPI_File_seek(mpi_words_file, 1, MPI_SEEK_CUR);
		word[i] = 0;
		int word_len = strlen(word+ 10) - 1;
		word[word_len+ 10] = 0;
		//printf("checking %s\n", word+10);
		// print_char_vals(word+10);
		strcpy(word2, word+10);
		//int word_len = strlen(word2);

		word_num++;

		//skip checking the word testpassword if it's not on the first password
		if(pass_num != 0 && word_num == 1)
			continue;

		int ffix;
		for(ffix = -1; ffix < 1000; ffix++)//10000000000 = max
		{
			//-1 edge case
			int fix_len = (int)(ffix ? log10(abs(ffix)) + 1 : 1);
			if(ffix == -1)
				fix_len = 0;
			if(ffix == 0)
			{
				word2[word_len] = '0';
				word2[word_len+1] = 0;
			}
			if(ffix > 0)
			{
				int i = 9;
				while(word[i] == '9' && i >= 7)
						word[i--] = '0';
				if( word[i] == 0)
					word[i] = '0';
				word[i]++;
				i = word_len + fix_len - 1;
				while(word2[i] == '9' && i > word_len)
						word2[i--] = '0';
				if(word2[i] == 0)// && word2[i] == '9'+1)
				{
					word2[i] = '0';
					word2[i+1] = 0;
					while( i > word_len - 1)
						word2[i--] = '0';
					i++;
				}
				word2[i]++;
			}

			// print_char_vals(word+10-fix_len);
			// printf("\n");
			// print_char_vals(word2);
			// printf("\n");

			char* pre_result = crypt(word+10-fix_len, "$1$ab$");

			int u;
			for(u = 0; u < num_users; u++)
				if(pre_result)
					if(strcmp(user_pass_hashes[u], pre_result) == 0)
						printf("Found! %s's password is %s\nhash: %s\n", user_names[u], word+10-fix_len, pre_result);

			char* suf_result = crypt(word2, "$1$ab$");
			for(u = 0; u < num_users; u++)
				if(suf_result)
					if(strcmp(user_pass_hashes[u], suf_result) == 0)//{
						printf("Found! %s's password is %s\nhash: %s\n", user_names[u], word2, suf_result);
						// print_char_vals(word+10-fix_len);
						// printf("\n");
						// print_char_vals(word2);
						// printf("\n");}
		}

		//break;//if you want to check only one word
	}
	
	for(i = 0; i < num_users; i++) {
		free(user_names[i]);
		free(user_pass_hashes[i]);
		free(user_passwords[i]);
	}
	free(user_names);
	free(user_pass_hashes);
	free(user_passwords);
	free(word);
	free(word2);
	
	exit(EXIT_SUCCESS);

	MPI_Finalize();
	return 0;
}
