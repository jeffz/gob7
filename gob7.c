/* 
 gob7.c 
 Jedi Knight Dark Forces II - gob
 Mysteries of the Sith - goo
 Pathways to the Force - gob
 Indiana Jones and the Infernal Machine - gob
 Droid Works
 unpacker

 Copyright (C) 2008 Jeff Zaroyko
 
 2008-01-16 v0.01 - initial release
 2008-01-17 v0.02 - handle zero sized entries
 2008-01-30 v0.03 - update comments to mention Indiana Jones
 2008-02-01 v0.04 - update banner and fix fopen modes to use binary mode.
 2008-12-16 v0.05 - update comments to mention Droid Works
 2010-01-15 v0.05 - update comments to mention Pathways to the Force

;    This program is free software: you can redistribute it and/or modify
;    it under the terms of the GNU General Public License as published by
;    the Free Software Foundation, either version 3 of the License, or
;    (at your option) any later version.

;    This program is distributed in the hope that it will be useful,
;    but WITHOUT ANY WARRANTY; without even the implied warranty of
;    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;    GNU General Public License for more details.

;    You should have received a copy of the GNU General Public License
;    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#define MAGIC 4
struct toc_entry
{
  int length;
  int offset;
  char filename[128];
};
struct toc_entry* read_toc(FILE* gobfile, int toc_begin, int item_count)
{
  int current_entry;
  struct toc_entry* t = malloc(sizeof(struct toc_entry)*item_count);
  if(!t)
    {
      printf("%m\n");
      exit(EXIT_FAILURE);
    }
  printf("allocated %d bytes for toc\n",sizeof(struct toc_entry)*item_count);
  fseek(gobfile, toc_begin, SEEK_SET);
  for(current_entry = 0; current_entry < item_count; current_entry++)
    {
      printf("reading entry %d:\n",
	     current_entry);
      assert(fread(
		   &(t+(current_entry))->offset,
		   sizeof(int),
		   1,
		   gobfile)); /* offset */
      assert(fread(
		   &(t+(current_entry))->length,
		    sizeof(int),
		    1,
		    gobfile)); /* length */
      assert(fread(
		   &(t+(current_entry))->filename,
		   128,
		   1,
		   gobfile)); /* filename */
    }
  return t;
}
void create_path(char *path)
{
  char *t = path;
  while(*path)
    {
      t = strstr(t,"/");
      if(!t)
        {
          mkdir(path,0777);
          return;
        }
      t = strstr(t, "/");
      *t = '\0';
      mkdir(path,0777);
      *t = '/';
      t = t+1;
    }
}
char* fix_path_sep(char *filename, char *path)
{
  char *t = path;
  strcpy(path, filename);
  while(*t++)
    {
      if(*t == '\\')
	*t = '/';
    }
  return path;
}
char* file_to_path(char* filename, char *path)
{
  char *t;
  t = path;
  strcpy(path, filename);
  if(!strstr(path, "\\"))
    {
      sprintf(path, "./");
      return path;
    }
  while(*t++)
    if(*t == '\\')
      *t = '/';
  while(*--t != '/');
  *t = '\0';
  return path;
}
char* path_to_file(char *filename, char *path)
{
  char *t;
  t = path;
  strcpy(path, filename);
  while(*t++);
  while(*--t != '/');
  return *++t ? t : path;
}
void unpack_gob(struct toc_entry *toc, int length, FILE* gobfile)
{
  int current_file;
  char path[128];
  printf("unpacking...\n");
  for(current_file = 0; length > current_file;current_file++)
    {
      FILE* outfile;
      char* outdata;
      fseek(gobfile, (toc+current_file)->offset, SEEK_SET);
      printf("%s\n", (toc+current_file)->filename); 
      create_path(fix_path_sep(file_to_path((toc+current_file)->filename,path),path));
      outdata = malloc((toc+current_file)->length);
      if(!outdata)
	{
	  printf("%m\n");
	  exit(EXIT_FAILURE);
	}
      if((toc+current_file)->length)
	assert(fread(outdata, (toc+current_file)->length, 1, gobfile));
      outfile = fopen(fix_path_sep((toc+current_file)->filename,path), "wb");
      if(!outfile)
	{
	  printf("%s: %m\n", (toc+current_file)->filename);
	  exit(EXIT_FAILURE);
	}
      if((toc+current_file)->length)
	assert(fwrite(outdata, (toc+current_file)->length, 1, outfile));
      fclose(outfile);
      free(outdata);
    }  
  puts("unpacked!");
}
void prnt(char *foo, int len)
{
  int i;
  for(i = 0; i < len; i++)
    {
      putchar(*(foo+i));
    }
  printf("done: %d\n",i);
}
void skip_magic(FILE* gobfile)
{
  char header[MAGIC] = {'0','0','0','0'}; 
  assert(fread(header,MAGIC,1, gobfile));
}
int get_toc_location(FILE* gobfile)
{
  int reposition_to;
  assert(fread(&reposition_to, 4, 1, gobfile));
  return reposition_to-sizeof(int); /* ?? :) */
}
int get_item_count(FILE* gobfile)
{
  int gobitem_count;
  assert(fread(&gobitem_count, 4, 1, gobfile));
  fseek(gobfile, gobitem_count, SEEK_SET);
  assert(fread(&gobitem_count, 4, 1, gobfile));
  return gobitem_count;
}
void init(int argc, char *argv[])
{
  if(sizeof(int) != 4)
    {
      printf("This program is non-portable, and only designed to run where" \
	     "sizeof(int) == 4), try recompiling with -m32");
      exit(EXIT_FAILURE);
    }
  if(argc != 2)
    {
      puts("gob7 - a gob/goo unpacker by Jeff Zaroyko, licensed under the GPLv3\n"\
	   "usage: ./gob7 [gob|goo file]");
      exit(EXIT_FAILURE);
    }
}
int main(int argc, char *argv[])
{
  int toc_begin= 0;
  int item_count=0;
  struct toc_entry *toc;
  init(argc, argv);
  FILE* gobfile = fopen(argv[1],"rb");
  if(!gobfile)
    {
      printf("fopen input: %m\n");
      return EXIT_FAILURE;
    }
  skip_magic(gobfile);
  toc_begin = get_toc_location(gobfile);
  item_count = get_item_count(gobfile);
  printf("%d objects\n"\
	 "building table of contents list\n\n",item_count);
  toc = read_toc(gobfile, toc_begin, item_count);
  unpack_gob(toc, item_count, gobfile);
  free(toc);
  fclose(gobfile);
  return 0;
}
