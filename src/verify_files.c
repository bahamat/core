/* 

        Copyright (C) 1994-
        Free Software Foundation, Inc.

   This file is part of GNU cfengine - written and maintained 
   by Mark Burgess, Dept of Computing and Engineering, Oslo College,
   Dept. of Theoretical physics, University of Oslo
 
   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 3, or (at your option) any
   later version. 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

*/

/*****************************************************************************/
/*                                                                           */
/* File: verify_files.c                                                      */
/*                                                                           */
/*****************************************************************************/

#include "cf3.defs.h"
#include "cf3.extern.h"


/*****************************************************************************/

void FindAndVerifyFilesPromises(struct Promise *pp)

{
PromiseBanner(pp); 

/*
exclude_dirs
include_dirs
*/

FindFilePromiserObjects(pp);
}

/*****************************************************************************/

void FindFilePromiserObjects(struct Promise *pp)

{ int literal = false;
 
if (literal)
   {
   VerifyFilePromise(pp->promiser,pp);
   return;
   }
else
   {
   SearchForFilePromisers(pp->promiser,pp,VerifyFilePromise);
   }
}

/*****************************************************************************/

void SearchForFilePromisers(char *wildpath,struct Promise *pp,void (*fnptr)(char *path, struct Promise *ptr))

{ struct Item *path,*ip,*remainder = NULL;
  char pbuffer[CF_BUFSIZE];
  struct stat statbuf;
  int count = 0,lastnode = false, expandregex = false;
  uid_t agentuid = getuid();

Debug("SearchForFilePromisers(%s)\n",wildpath);

if (!IsPathRegex(wildpath))
   {
   (*fnptr)(wildpath,pp);
   return;
   }

pbuffer[0] = '\0';
path = SplitString(wildpath,FILE_SEPARATOR);

for (ip = path; ip != NULL; ip=ip->next)
   {
   if (strlen(ip->name) == 0)
      {
      continue;
      }

   if (ip->next == NULL)
      {
      lastnode = true;
      }

   /* No need to chdir as in recursive descent, since we know about the path here */
   
   if (IsRegex(ip->name))
      {
      // if (lastnode & policy is create) error - can't create
      remainder = ip->next;
      expandregex = true;
      break;
      }

   if (BufferOverflow(pbuffer,ip->name))
      {
      CfLog(cferror,"Buffer overflow in SearchForFilePromisers\n","");
      return;
      }

   AddSlash(pbuffer);
   strcat(pbuffer,ip->name);
   
   if (stat(pbuffer,&statbuf) == -1)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Link %s in search path %s does not exist - cannot continue under policy match",pbuffer,wildpath);
      CfLog(cferror,OUTPUT,"");
      return;
      }
   else
      {
      if (statbuf.st_uid != agentuid)
         {
         Verbose("Link %s in search path %s is not owned by the agent uid - operation is potentially risky\n",pbuffer,path);
         }
      }
   }

if (expandregex) /* Expand one regex link and hand down */
   {
   char nextbuffer[CF_BUFSIZE],regex[CF_BUFSIZE];
   struct dirent *dirp;
   DIR *dirh;
 
   strncpy(regex,ip->name,CF_BUFSIZE-1);
   strncpy(nextbuffer,pbuffer,CF_BUFSIZE-1);
   
   if ((dirh=opendir(pbuffer)) == NULL)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Can't open dir: %s\n",pbuffer);
      CfLog(cferror,OUTPUT,"opendir");
      return;
      }
   
   count = 0;
   
   for (dirp = readdir(dirh); dirp != 0; dirp = readdir(dirh))
      {
      if (!SensibleFile(dirp->d_name,pbuffer,NULL))
         {
         continue;
         }

      if (!lastnode && !S_ISDIR(statbuf.st_mode))
         {
         Debug("Skipping non-directory %s\n",dirp->d_name);
         continue;
         }

      if (FullTextMatch(regex,dirp->d_name))
         {
         Debug("Link %s matched regex %s\n",dirp->d_name,regex);
         }
      else
         {
         continue;
         }

      count++;
      
      AddSlash(nextbuffer);
      strcat(nextbuffer,dirp->d_name);
      
      for (ip = remainder; ip != NULL; ip=ip->next)
         {
         AddSlash(pbuffer);
         strcat(pbuffer,ip->name);
         }

      /* The next level might still contain regexs, so go again*/

      SearchForFilePromisers(nextbuffer,pp,fnptr);
      ChopLastNode(nextbuffer);
      }
   
   closedir(dirh);
   }
else
   {
   (*fnptr)(wildpath,pp);
   }

if (count == 0)
   {
   Verbose("No promiser file objects matched this regular expression %s\n",wildpath);
   }
}

/*******************************************************************/
/* Level                                                           */
/*******************************************************************/

void VerifyFilePromise(char *path,struct Promise *pp)

{
 printf("VerifyPromiseon .. %s\n",path);
}
