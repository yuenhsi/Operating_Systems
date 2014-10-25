/* Author: Yuen Hsi Chang
 * Thread Synchronization: Building H2SO4
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

// functions of the 3 types of threads, the ones that produce the hydrogen, sulfur, and oxygen
void* Hydrogen(void* args);
void* Sulfur(void* args);
void* Oxygen(void* args);

// declare our semaphores global
sem_t* hydro_sem;
sem_t* oxygen_sem;
sem_t* ready_sem;
sem_t* hdone_sem;
sem_t* sdone_sem;
sem_t* odone_sem;
sem_t* alldone_sem;

int main() {

    // create the semaphores
    hydro_sem = sem_open("/Accounts/hallsk/hydrsmphr", O_CREAT|O_RDWR, 0466, 0);
    while (hydro_sem==SEM_FAILED) {
        if (errno == EEXIST) {
          printf("semaphore hydrosmphr already exists, unlinking and reopening\n");
          fflush(stdout);
          sem_unlink("hydrsmphr");
          hydro_sem = sem_open("hydrsmphr", O_CREAT|O_EXCL, 0466, 0);
        }
        else {
          printf("semaphore could not be opened, error # %d\n", errno);
          fflush(stdout);
          exit(1);
        }
    }
    oxygen_sem = sem_open("/Accounts/hallsk/oxsmphr", O_CREAT|O_RDWR, 0466, 0);
    while (oxygen_sem==SEM_FAILED) {
        if (errno == EEXIST) {
          printf("semaphore oxsmphr already exists, unlinking and reopening\n");
          fflush(stdout);
          sem_unlink("oxsmphr");
          hydro_sem = sem_open("oxsmphr", O_CREAT|O_EXCL, 0466, 0);
        }
        else {
          printf("semaphore could not be opened, error # %d\n", errno);
          fflush(stdout);
          exit(1);
        }
    }
    ready_sem = sem_open("/Accounts/hallsk/rdsmphr", O_CREAT|O_RDWR, 0466, 0);
    while (ready_sem==SEM_FAILED) {
        if (errno == EEXIST) {
          printf("semaphore rdsmphr already exists, unlinking and reopening\n");
          fflush(stdout);
          sem_unlink("rdsmphr");
          hydro_sem = sem_open("rdsmphr", O_CREAT|O_EXCL, 0466, 0);
        }
        else {
          printf("semaphore could not be opened, error # %d\n", errno);
          fflush(stdout);
          exit(1);
        }
    }
    hdone_sem = sem_open("/Accounts/hallsk/hdonsmphr", O_CREAT|O_RDWR, 0466, 0);
    while (hdone_sem==SEM_FAILED) {
        if (errno == EEXIST) {
          printf("semaphore hdonsmphr already exists, unlinking and reopening\n");
          fflush(stdout);
          sem_unlink("hdonsmphr");
          hydro_sem = sem_open("hdonsmphr", O_CREAT|O_EXCL, 0466, 0);
        }
        else {
          printf("semaphore could not be opened, error # %d\n", errno);
          fflush(stdout);
          exit(1);
        }
    }
    sdone_sem = sem_open("/Accounts/hallsk/sdonsmphr", O_CREAT|O_RDWR, 0466, 0);
    while (sdone_sem==SEM_FAILED) {
        if (errno == EEXIST) {
          printf("semaphore sdonsmphr already exists, unlinking and reopening\n");
          fflush(stdout);
          sem_unlink("sdonsmphr");
          hydro_sem = sem_open("sdonsmphr", O_CREAT|O_EXCL, 0466, 0);
        }
        else {
          printf("semaphore could not be opened, error # %d\n", errno);
          fflush(stdout);
          exit(1);
        }
    }
    odone_sem = sem_open("/Accounts/hallsk/odnsmphr", O_CREAT|O_RDWR, 0466, 0);
    while (odone_sem==SEM_FAILED) {
        if (errno == EEXIST) {
          printf("semaphore odnsmphr already exists, unlinking and reopening\n");
          fflush(stdout);
          sem_unlink("odnsmphr");
          hydro_sem = sem_open("odnsmphr", O_CREAT|O_EXCL, 0466, 0);
        }
        else {
          printf("semaphore could not be opened, error # %d\n", errno);
          fflush(stdout);
          exit(1);
        }
    }
    alldone_sem = sem_open("/Accounts/hallsk/done", O_CREAT|O_RDWR, 0466, 1);
    while (alldone_sem==SEM_FAILED) {
        if (errno == EEXIST) {
          printf("semaphore done already exists, unlinking and reopening\n");
          fflush(stdout);
          sem_unlink("done");
          hydro_sem = sem_open("done", O_CREAT|O_EXCL, 0466, 0);
        }
        else {
          printf("semaphore could not be opened, error # %d\n", errno);
          fflush(stdout);
          exit(1);
        }
    }
    
    // testing water molecule creation, 2nd molecule should not be made until all 4 hydrogen
    // and 2 oxygen atoms have been produced
    pthread_t hydro1, hydro2, sulfur1, oxy1, oxy2, oxy3, oxy4;
    
    pthread_t ahydro1, ahydro2, asulfur1, aoxy1, aoxy2, aoxy3, aoxy4;
    
    pthread_create(&hydro1, NULL, Hydrogen, NULL);
    pthread_create(&hydro2, NULL, Hydrogen, NULL);
    pthread_create(&sulfur1, NULL, Sulfur, NULL);
    pthread_create(&oxy1, NULL, Oxygen, NULL);
    pthread_create(&oxy2, NULL, Oxygen, NULL);
    pthread_create(&oxy3, NULL, Oxygen, NULL);
    pthread_create(&oxy4, NULL, Oxygen, NULL);
    
    pthread_create(&ahydro1, NULL, Hydrogen, NULL);
    pthread_create(&ahydro2, NULL, Hydrogen, NULL);
    pthread_create(&asulfur1, NULL, Sulfur, NULL);
    pthread_create(&aoxy1, NULL, Oxygen, NULL);
    pthread_create(&aoxy2, NULL, Oxygen, NULL);
    pthread_create(&aoxy3, NULL, Oxygen, NULL);
    pthread_create(&aoxy4, NULL, Oxygen, NULL);
    
    pthread_join(hydro1, NULL);
    pthread_join(hydro2, NULL);
    pthread_join(sulfur1, NULL);
    pthread_join(oxy1, NULL);
    pthread_join(oxy2, NULL);
    pthread_join(oxy3, NULL);
    pthread_join(oxy4, NULL);   
    
    pthread_join(ahydro1, NULL);
    pthread_join(ahydro2, NULL);
    pthread_join(asulfur1, NULL);
    pthread_join(aoxy1, NULL);
    pthread_join(aoxy2, NULL);
    pthread_join(aoxy3, NULL);
    pthread_join(aoxy4, NULL);
    
    sem_close(hydro_sem);
    sem_close(oxygen_sem);
    sem_close(ready_sem);
    sem_close(hdone_sem);
    sem_close(sdone_sem);
    sem_close(odone_sem);
    sem_close(alldone_sem);
    return 0;
}

void* Hydrogen(void* args) {
    // produce a hydrogen molecule
    printf("Hydrogen atom produced\n");
    fflush(stdout);

    sem_post(hydro_sem);  // telling sulfur that hydrogen is available
    sem_wait(ready_sem);  // waiting to start releasing the atoms
    
    printf("Hydrogen atom leaving\n");
    fflush(stdout);
    
    sem_post(hdone_sem);  // telling sulfur atom has been released
    
}

void* Sulfur(void* args) {
    printf("Sulfur atom produced\n");
    fflush(stdout);
    
    sem_wait(alldone_sem);  // locks this whole section, so only one molecule
                            // created at a time
    sem_wait(hydro_sem);    // waiting for 2 hydrogens
    sem_wait(hydro_sem);
    
    sem_wait(oxygen_sem);   // waiting for 4 oxygens
    sem_wait(oxygen_sem);
    sem_wait(oxygen_sem);
    sem_wait(oxygen_sem);
    
    printf("\nH2SO4 molecule produced\n");
    
    sem_post(ready_sem);    // ready to start forming molecule now that
    sem_post(ready_sem);    // everyone is present
    sem_wait(hdone_sem);
    sem_wait(hdone_sem);

    printf("Sulfur atom leaving\n");
    fflush(stdout);
    
    sem_post(sdone_sem);
    sem_post(sdone_sem);
    sem_post(sdone_sem);
    sem_post(sdone_sem);

    sem_wait(odone_sem);
    sem_wait(odone_sem);
    sem_wait(odone_sem);
    sem_wait(odone_sem);
    
    sem_post(alldone_sem);  // molecule is done being produced
    
}

void* Oxygen(void* args) {
    // produce an oxygen molecule
    printf("Oxygen atom produced\n");
    fflush(stdout);
    
    sem_post(oxygen_sem);  // tell sulfur that oxygen is available
    sem_wait(sdone_sem);  // waiting for sulfur to be released
    
    printf("Oxygen atom leaving\n");
    fflush(stdout);
    
    sem_post(odone_sem);

}

