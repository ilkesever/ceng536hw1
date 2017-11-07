#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

void synchronizeFolders(char* dir1, char* dir2);
void intersectFolders(char* dir1, char* dir2);

char *dir1root, *dir2root;

int main(int argc, char *argv[])
{

    if (argc < 3 || argc > 4) {
	    printf( "Wrong usage\n");
    }

    if (argc == 3){
    	dir1root = argv[1];
    	dir2root = argv[2];

    	synchronizeFolders(dir1root, dir2root);
    
    }

    else if (argc == 4 ) {
    	if (strcmp(argv[1], "-i") != 0)
    		printf("Wrong usage use -i\n");
    	else {
	    	dir1root = argv[2];
	    	dir2root = argv[3];

	    	intersectFolders(dir1root, dir2root);
	    }
    }


    return 0;
}

int isDirectory(char* root, char* path) {

	char* newd1 = malloc (strlen(root) + strlen(path) + 1);
	newd1[0] = '\0';
	strcat(newd1, root);
	strcat(newd1, "/");
	strcat(newd1, path);

	struct stat buf;
	lstat(newd1, &buf);
	
	free(newd1);

	if (S_ISDIR(buf.st_mode))
		return 1;
	if (S_ISREG(buf.st_mode))
		return 2;
	else 
		return 3;

}

time_t getModificationTime(char* root, char* path) {
	char* newd1 = malloc (strlen(root) + strlen(path) + 1);
	newd1[0] = '\0';
	strcat(newd1, root);
	strcat(newd1, "/");
	strcat(newd1, path);

	struct stat buf;
	lstat(newd1, &buf);
	
	return buf.st_mtime;
}

off_t getSize(char* root, char* path) {
	char* newd1 = malloc (strlen(root) + strlen(path) + 1);
	newd1[0] = '\0';
	strcat(newd1, root);
	strcat(newd1, "/");
	strcat(newd1, path);

	struct stat buf;
	lstat(newd1, &buf);

	return buf.st_size;
}

char* readLink(char* root, char* path){
	char* newd1 = malloc (strlen(root) + strlen(path) + 1);
	newd1[0] = '\0';
	strcat(newd1, root);
	strcat(newd1, "/");
	strcat(newd1, path);

	char* buff = (char*)malloc(1024);
	buff[0] = '\0';
	size_t bufsiz = 1024;
	ssize_t last = readlink(newd1, buff, bufsiz);
	if (last > 0)
		buff[last] = '\0';
	return buff;
}

void intersectFolders(char* dir1, char* dir2) {
	struct dirent **ent1;
	struct dirent **ent2;
	int l1, l2;
	char **dir1e, **dir2e;
	// dir1 traverse
	DIR *dir;
	if ((dir = opendir (dir1)) != NULL) {

		struct dirent *tmpent;
		l1=0;
		while ((tmpent = readdir (dir)) != NULL) {

			if (strcmp(tmpent->d_name, ".") != 0 && strcmp(tmpent->d_name, "..") != 0 ){
				if (l1==0) {
					ent1 = (struct dirent**) malloc(8);
					dir1e = (char**) malloc(8);
				}
				else{
					ent1 = (struct dirent**) realloc(ent1, 8 * (l1+1));
					dir1e = (char**) realloc(dir1e, 8 * (l1+1));
				}
				// printf("%d\n", strlen(tmpent->d_name));
				dir1e[l1] = (char*) malloc(strlen(tmpent->d_name));
				strcpy(dir1e[l1], tmpent->d_name);
				ent1[l1] = tmpent;
				//printf ("%s/%s %d\n", dir1, ent1[l1]->d_name, isDirectory(dir1, ent1[l1]->d_name));
				l1++;
			}
			
		}
		closedir (dir);
	}

	//printf("\n end of \n\n");
	// dir2 traverse
	DIR *dirt;
	if ((dirt = opendir (dir2)) != NULL) {

		struct dirent *tmpent;
		l2=0;
		while ((tmpent = readdir (dirt)) != NULL) {

			if (strcmp(tmpent->d_name, ".") != 0 && strcmp(tmpent->d_name, "..") != 0 ){
				if (l2==0){
					dir2e = (char**) malloc(8);
					ent2 = (struct dirent**) malloc(8);
				}
				else{
					dir2e = (char**) realloc(dir2e, 8*(l2+1));
					ent2 = (struct dirent**) realloc(ent2, 8 * (l2+1));
				}
				dir2e[l2] = (char*) malloc(strlen(tmpent->d_name));
				strcpy(dir2e[l2], tmpent->d_name);
				ent2[l2] = tmpent;
				//printf ("%s/%s %d\n", dir2, ent2[l2]->d_name, isDirectory(dir2, ent2[l2]->d_name));
				l2++;
			}
			
		}
		closedir (dirt);
	}

	//printf("%d %d\n\n", l1, l2);

	/* BEGIN INTERSECTION */

	// Directory Intersect
	int i = 0, j = 0;
	while (i < l1) {

		if (isDirectory(dir1, dir1e[i]) != 1) {
			i++;
			continue;
		}
		j=0;
		while (j < l2){

			if (strcmp(dir1e[i], dir2e[j]) == 0) {
				// They have same folders sync their inside
				if (isDirectory(dir2, dir2e[j]) != 1){
					break;
				}
				char* newd1 = malloc (strlen(dir1) + strlen(dir1e[i]) + 1);
				newd1[0] = '\0';
				strcat(newd1, dir1);
				strcat(newd1, "/");
				strcat(newd1, dir1e[i]);

				char* newd2 = malloc (strlen(dir2) + strlen(dir1e[i]) + 1);
				newd2[0] = '\0';
				strcat(newd2, dir2);
				strcat(newd2, "/");
				strcat(newd2, dir1e[i]);
				// printf("same: %s|%s|\n", newd1, newd2);
				intersectFolders(newd1, newd2);
				break;
			}
			if (j == l2 - 1) {
				printf("rm -rf %s/%s\n", dir1, dir1e[i]);
			}
			j++;
		}
		if (l2 == 0)
			printf("rm -rf %s/%s\n", dir1, dir1e[i]);
		i++;
	}

	// Other Directory Intersect
	i = 0; j=0;
	while (i < l2) {
		
		if (isDirectory(dir2, dir2e[i]) != 1) {
			i++;
			continue;
		}
		j=0;
		while (j < l1){

			if (strcmp(dir2e[i], dir1e[j]) == 0) {
				// They have same folders sync their inside
				break;
			}
			if (j == l1 - 1) {
				printf("rm -rf %s/%s\n", dir2, dir2e[i]);
			}
			j++;
		}
		if (l1 == 0)
			printf("rm -rf %s/%s\n", dir2, dir2e[i]);
		i++;
	}

	// File Intersect
	i = 0; j = 0;
	while (i < l1) {

		if (isDirectory(dir1, dir1e[i]) != 2) {
			i++;
			continue;
		}
		j=0;
		while (j < l2){

			if (strcmp(dir1e[i], dir2e[j]) == 0) {
				// They have same files sync the most recent one
				if (isDirectory(dir2, dir2e[j]) != 2) {
					break;
				}
				time_t d1t = getModificationTime(dir1, dir1e[i]);
				time_t d2t = getModificationTime(dir2, dir2e[j]);
				if (d2t > d1t)
					printf("cp -p %s/%s %s\n", dir1, dir1e[i], dir2);
				else if (d1t > d2t)
					printf("cp -p %s/%s %s\n", dir2, dir2e[j], dir1);
				else {
					// Same modification time

					if (getSize(dir1, dir1e[i]) < getSize(dir2, dir2e[j]))
						printf("cp -p %s/%s %s\n", dir1, dir1e[i], dir2);
					else if (getSize(dir1, dir1e[i]) > getSize(dir2, dir2e[j]))
						printf("cp -p %s/%s %s\n", dir2, dir2e[j], dir1);
				}
				break;
			}
			if (j == l2 - 1) {
				printf("rm %s/%s\n", dir1, dir1e[i]);
			}
			j++;
		}
		if (l2 == 0)
			printf("rm %s/%s\n", dir1, dir1e[i]);
		i++;
	}

	// Other File Intersect
	i = 0; j = 0;
	while (i < l2) {
		
		if (isDirectory(dir2, dir2e[i]) != 2) {
			i++;
			continue;
		}
		j=0;
		while (j < l1){

			if (strcmp(dir2e[i], dir1e[j]) == 0) {
				// They have same files sync their inside
				break;
			}
			if (j == l1 - 1) {
				printf("rm %s/%s\n", dir2, dir2e[i]);
			}
			j++;
		}
		if (l1 == 0)
			printf("rm %s/%s\n", dir2, dir2e[i]);
		i++;
	}

	// Link Intersect
	i=0; j=0;
	while (i < l1) {

		if (isDirectory(dir1, dir1e[i]) != 3) {
			i++;
			continue;
		}
		j=0;
		while (j < l2){

			if (strcmp(dir1e[i], dir2e[j]) == 0) {
				// Both are Link
				if (isDirectory(dir2, dir2e[j]) == 3){
					time_t d1t = getModificationTime(dir1, dir1e[i]);
					time_t d2t = getModificationTime(dir2, dir2e[j]);
					if (d1t < d2t)
						printf("ln -sf %s %s/%s\n", readLink(dir1, dir1e[i]), dir2, dir1e[i]);
					else if (d2t < d1t)
						printf("ln -sf %s %s/%s\n", readLink(dir2, dir2e[j]), dir1, dir1e[i]);
					else {
						// Same modification time

						if (getSize(dir1, dir1e[i]) < getSize(dir2, dir2e[j]))
							printf("ln -sf %s %s/%s\n", readLink(dir1, dir1e[i]), dir2, dir1e[i]);
						else
							printf("ln -sf %s %s/%s\n", readLink(dir2, dir2e[j]), dir1, dir1e[i]);
					}

				} // Link and File
				else if (isDirectory(dir2, dir2e[j]) == 2){
					time_t d1t = getModificationTime(dir1, dir1e[i]); // Link
					time_t d2t = getModificationTime(dir2, dir2e[j]); // File
					if (d1t < d2t)
						printf("ln -sf %s %s/%s\n", readLink(dir1, dir1e[i]), dir2, dir1e[i]);
					else if (d2t < d1t){
						printf("rm %s/%s\n", dir1, dir2e[j]);
						printf("cp -p %s/%s %s\n", dir2, dir2e[j], dir1);
					}
					else {
						// Same modification time
						printf("ln -sf %s %s/%s\n", readLink(dir1, dir1e[i]), dir2, dir1e[i]);
					}
				}
				
				break;
			}
			if (j == l2 - 1) {
				printf("rm %s/%s\n", dir1, dir1e[i]);
			}
			j++;
		}
		if (l2 == 0)
			printf("rm %s/%s\n", dir1, dir1e[i]);
		i++;
	}

	// Other Link Intersect
	i = 0; j = 0;
	while (i < l2) {
		
		if (isDirectory(dir2, dir2e[i]) != 3) {
			i++;
			continue;
		}
		j=0;
		while (j < l1){

			if (strcmp(dir2e[i], dir1e[j]) == 0) {
				// They have same folders sync their inside
				if (isDirectory(dir1, dir1e[j]) == 2){
					time_t d1t = getModificationTime(dir1, dir1e[j]); // File
					time_t d2t = getModificationTime(dir2, dir2e[i]); // Link
					if (d1t < d2t){
						printf("rm %s/%s\n", dir2, dir1e[j]);
						printf("cp -p %s/%s %s\n", dir1, dir1e[j], dir2);
					}
					else if (d2t < d1t)
						printf("ln -sf %s %s/%s\n", readLink(dir2, dir2e[i]), dir1, dir1e[j]);
					else {
						// Same modification time
						printf("ln -sf %s %s/%s\n", readLink(dir2, dir2e[i]), dir1, dir1e[j]);
					}
				}
				break;
			}
			if (j == l1 - 1) {
				printf("rm %s/%s\n", dir2, dir2e[i]);
			}
			j++;
		}
		if (l1 == 0)
			printf("rm %s/%s\n", dir2, dir2e[i]);
		i++;
	}
}


void synchronizeFolders(char* dir1, char* dir2){
	struct dirent **ent1;
	struct dirent **ent2;
	int l1, l2;
	char **dir1e, **dir2e;
	// dir1 traverse
	DIR *dir;
	if ((dir = opendir (dir1)) != NULL) {

		struct dirent *tmpent;
		l1=0;
		while ((tmpent = readdir (dir)) != NULL) {

			if (strcmp(tmpent->d_name, ".") != 0 && strcmp(tmpent->d_name, "..") != 0 ){
				if (l1==0) {
					ent1 = (struct dirent**) malloc(8);
					dir1e = (char**) malloc(8);
				}
				else{
					ent1 = (struct dirent**) realloc(ent1, 8 * (l1+1));
					dir1e = (char**) realloc(dir1e, 8 * (l1+1));
				}
				// printf("%d\n", strlen(tmpent->d_name));
				dir1e[l1] = (char*) malloc(strlen(tmpent->d_name));
				strcpy(dir1e[l1], tmpent->d_name);
				ent1[l1] = tmpent;
				//printf ("%s/%s %d\n", dir1, ent1[l1]->d_name, isDirectory(dir1, ent1[l1]->d_name));
				l1++;
			}
			
		}
		closedir (dir);
	}

	//printf("\n end of \n\n");
	// dir2 traverse
	DIR *dirt;
	if ((dirt = opendir (dir2)) != NULL) {

		struct dirent *tmpent;
		l2=0;
		while ((tmpent = readdir (dirt)) != NULL) {

			if (strcmp(tmpent->d_name, ".") != 0 && strcmp(tmpent->d_name, "..") != 0 ){
				if (l2==0){
					dir2e = (char**) malloc(8);
					ent2 = (struct dirent**) malloc(8);
				}
				else{
					dir2e = (char**) realloc(dir2e, 8*(l2+1));
					ent2 = (struct dirent**) realloc(ent2, 8 * (l2+1));
				}
				dir2e[l2] = (char*) malloc(strlen(tmpent->d_name));
				strcpy(dir2e[l2], tmpent->d_name);
				ent2[l2] = tmpent;
				//printf ("%s/%s %d\n", dir2, ent2[l2]->d_name, isDirectory(dir2, ent2[l2]->d_name));
				l2++;
			}
			
		}
		closedir (dirt);
	}

	//printf("%d %d\n\n", l1, l2);


	/** BEGIN SYNCHRONIZATION **/

	//Directory Sync
	int i = 0, j = 0;
	while (i < l1) {

		if (isDirectory(dir1, dir1e[i]) != 1) {
			i++;
			continue;
		}
		j=0;
		while (j < l2){

			if (strcmp(dir1e[i], dir2e[j]) == 0) {
				// They have same folders sync their inside
				// synchronizeFolders();
				if (isDirectory(dir2, dir2e[j]) != 1){
					break;
				}

				char* newd1 = malloc (strlen(dir1) + strlen(dir1e[i]) + 1);
				newd1[0] = '\0';
				strcat(newd1, dir1);
				strcat(newd1, "/");
				strcat(newd1, dir1e[i]);

				char* newd2 = malloc (strlen(dir2) + strlen(dir1e[i]) + 1);
				newd2[0] = '\0';
				strcat(newd2, dir2);
				strcat(newd2, "/");
				strcat(newd2, dir1e[i]);
				// printf("same: %s|%s|\n", newd1, newd2);
				synchronizeFolders(newd1, newd2);
				break;
			}
			if (j == l2 - 1) {
				printf("cp -rp %s/%s %s\n", dir1, dir1e[i], dir2);
			}
			j++;
		}
		if (l2 == 0)
			printf("cp -rp %s/%s %s\n", dir1, dir1e[i], dir2);
		i++;
	}

	// Other Directory Sync
	i = 0; j=0;
	while (i < l2) {
		
		if (isDirectory(dir2, dir2e[i]) != 1) {
			i++;
			continue;
		}
		j=0;
		while (j < l1){

			if (strcmp(dir2e[i], dir1e[j]) == 0) {
				// They have same folders sync their inside
				break;
			}
			if (j == l1 - 1) {
				printf("cp -rp %s/%s %s\n", dir2, dir2e[i], dir1);
			}
			j++;
		}
		if (l1 == 0)
			printf("cp -rp %s/%s %s\n", dir2, dir2e[i], dir1);
		i++;
	}

	// File Sync
	i = 0; j = 0;
	while (i < l1) {

		if (isDirectory(dir1, dir1e[i]) != 2) {
			i++;
			continue;
		}
		j=0;
		while (j < l2){

			if (strcmp(dir1e[i], dir2e[j]) == 0) {
				// They have same files sync the most recent one
				if (isDirectory(dir2, dir2e[j]) != 2) {
					break;
				}
				time_t d1t = getModificationTime(dir1, dir1e[i]);
				time_t d2t = getModificationTime(dir2, dir2e[j]);
				if (d1t > d2t)
					printf("cp -p %s/%s %s\n", dir1, dir1e[i], dir2);
				else if (d2t > d1t)
					printf("cp -p %s/%s %s\n", dir2, dir2e[j], dir1);
				else {
					// Same modification time

					if (getSize(dir1, dir1e[i]) > getSize(dir2, dir2e[j]))
						printf("cp -p %s/%s %s\n", dir1, dir1e[i], dir2);
					else if (getSize(dir1, dir1e[i]) < getSize(dir2, dir2e[j]))
						printf("cp -p %s/%s %s\n", dir2, dir2e[j], dir1);
				}
				break;
			}
			if (j == l2 - 1) {
				printf("cp -p %s/%s %s\n", dir1, dir1e[i], dir2);
			}
			j++;
		}
		if (l2 == 0)
			printf("cp -p %s/%s %s\n", dir1, dir1e[i], dir2);
		i++;
	}

	// Other File Sync
	i = 0; j = 0;
	while (i < l2) {
		
		if (isDirectory(dir2, dir2e[i]) != 2) {
			i++;
			continue;
		}
		j=0;
		while (j < l1){

			if (strcmp(dir2e[i], dir1e[j]) == 0) {
				// They have same folders sync their inside
				break;
			}
			if (j == l1 - 1) {
				printf("cp -p %s/%s %s\n", dir2, dir2e[i], dir1);
			}
			j++;
		}
		if (l1 == 0)
			printf("cp -p %s/%s %s\n", dir2, dir2e[i], dir1);
		i++;
	}

	// Link Sync
	i=0; j=0;
	while (i < l1) {

		if (isDirectory(dir1, dir1e[i]) != 3) {
			i++;
			continue;
		}
		j=0;
		while (j < l2){

			if (strcmp(dir1e[i], dir2e[j]) == 0) {
				// Both are Link
				if (isDirectory(dir2, dir2e[j]) == 3){
					time_t d1t = getModificationTime(dir1, dir1e[i]);
					time_t d2t = getModificationTime(dir2, dir2e[j]);
					if (d1t > d2t)
						printf("ln -sf %s %s/%s\n", readLink(dir1, dir1e[i]), dir2, dir1e[i]);
					else if (d2t > d1t)
						printf("ln -sf %s %s/%s\n", readLink(dir2, dir2e[j]), dir1, dir1e[i]);
					else {
						// Same modification time

						if (getSize(dir1, dir1e[i]) > getSize(dir2, dir2e[j]))
							printf("ln -sf %s %s/%s\n", readLink(dir1, dir1e[i]), dir2, dir1e[i]);
						else
							printf("ln -sf %s %s/%s\n", readLink(dir2, dir2e[j]), dir1, dir1e[i]);
					}

				} // Link and File
				else if (isDirectory(dir2, dir2e[j]) == 2){
					time_t d1t = getModificationTime(dir1, dir1e[i]); // Link
					time_t d2t = getModificationTime(dir2, dir2e[j]); // File
					if (d1t > d2t)
						printf("ln -sf %s %s/%s\n", readLink(dir1, dir1e[i]), dir2, dir1e[i]);
					else if (d2t > d1t){
						printf("rm %s/%s\n", dir1, dir2e[j]);
						printf("cp -p %s/%s %s\n", dir2, dir2e[j], dir1);
					}
					else {
						// Same modification time
						printf("rm %s/%s\n", dir1, dir2e[j]);
						printf("cp -p %s/%s %s\n", dir2, dir2e[j], dir1);
					}
				}
				
				break;
			}
			if (j == l2 - 1) {
				printf("ln -sf %s %s/%s\n", readLink(dir1, dir1e[i]), dir2, dir1e[i]);
			}
			j++;
		}
		if (l2 == 0)
			printf("ln -sf %s %s/%s\n", readLink(dir1, dir1e[i]), dir2, dir1e[i]);
		i++;
	}

	// Other Link Sync
	i = 0; j = 0;
	while (i < l2) {
		
		if (isDirectory(dir2, dir2e[i]) != 3) {
			i++;
			continue;
		}
		j=0;
		while (j < l1){

			if (strcmp(dir2e[i], dir1e[j]) == 0) {
				// They have same folders sync their inside
				if (isDirectory(dir1, dir1e[j]) == 2){
					time_t d1t = getModificationTime(dir1, dir1e[j]); // File
					time_t d2t = getModificationTime(dir2, dir2e[i]); // Link
					if (d1t > d2t){
						printf("rm %s/%s\n", dir2, dir1e[j]);
						printf("cp -p %s/%s %s\n", dir1, dir1e[j], dir2);
					}
					else if (d2t > d1t)
						printf("ln -sf %s %s/%s\n", readLink(dir2, dir2e[i]), dir1, dir1e[j]);
					else {
						// Same modification time
						printf("rm %s/%s\n", dir2, dir1e[j]);
						printf("cp -p %s/%s %s\n", dir1, dir1e[j], dir2);
					}
				}
				break;
			}
			if (j == l1 - 1) {
				printf("ln -sf %s %s/%s\n", readLink(dir2, dir2e[i]), dir1, dir2e[i]);
			}
			j++;
		}
		if (l1 == 0)
			printf("ln -sf %s %s/%s\n", readLink(dir2, dir2e[i]), dir1, dir2e[i]);
		i++;
	}

}






