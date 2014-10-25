/*
 * Thread Synchronization: Boating Oahu to Molokai
 * Yuen Hsi Chang
 *
 * The algorithm works as follows: 
 * All the children get across to Molokai first by alternating between two children rowing across, 
 * and one child rowing back. When no children are left in Oahu, the child that rows back to Oahu 
 * sleeps and an adult rows to Molokai. He then wakes up a child, who rows back to Oahu, and 
 * re-checks whether there are other children in Oahu. If so, the process repeats, until all the
 * adults are transferred to Molokai. The last trip would be two children rowing from Oahu to 
 * Molokai, regardless of the number of starting children or adults. 
 * As a side note:
 * Rowing from Oahu to Molokai will only ever be either 2 children or 1 adult.
 * Rowing from Molokai to Oahu will only ever be 1 child.
 */
 
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sched.h>

int start = 0;
int CinOahu = 0;
int AinOahu = 0;
int everyoneSafe = 0;
int CinBoat = 0;
int AinBoat = 0;
int childrenAcross = 0;
int CinMolokai = 0;
int AinMolokai = 0;

pthread_mutex_t lock;
pthread_cond_t ready;
pthread_cond_t openChildSpot;
pthread_cond_t openAdultSpot;
pthread_cond_t boatFull;
pthread_cond_t everyoneInOahu;
pthread_cond_t adultInMolokai;
pthread_cond_t bothChildrenAcross;
pthread_cond_t oneChildGotOff;
pthread_cond_t everyoneAcross;
pthread_cond_t sleeping;

void* beginTransfer(int nChildren, int nAdults);
void* child(void *);
void* adult(void *);

int main(int argc, char *argv[])
{
    // get the number of children / adults as command line arguments. 
    if (argc < 3) {
        printf("Please enter the number of children and adults (between 0 and 9) in Oahu," 
                "separated by whitespace. \n");
        exit(1);
    }
    if (argv[1][1] != '\0' || argv[2][1] != '\0') {
        printf("Error: The number of children and adults in Oahu must be between 0 and 9.\n");
        exit(1);
    }
    
    if (argv[1][0]-'0' < 2) {
        printf("Error: There must be at least 2 children in Oahu. \n");
        exit(1);
    }
    int numC = argv[1][0]-'0';
    int numA = argv[2][0]-'0';
    beginTransfer(numC, numA);
}

void* beginTransfer(int nChildren, int nAdults) {   
    // declare array of threads representing children and adults in Oahu
    pthread_t threads[nChildren + nAdults];
    
    // initialize mutex and condition variables
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&ready, NULL);
    pthread_cond_init(&openChildSpot, NULL);
    pthread_cond_init(&openAdultSpot, NULL);
    pthread_cond_init(&boatFull, NULL);
    pthread_cond_init(&everyoneInOahu, NULL);
    pthread_cond_init(&adultInMolokai, NULL);
    pthread_cond_init(&bothChildrenAcross, NULL);
    pthread_cond_init(&oneChildGotOff, NULL);
    pthread_cond_init(&everyoneAcross, NULL);
    pthread_cond_init(&sleeping, NULL);

    int i, j;
    for (i = 0; i < nChildren; i++) {
        pthread_create(&threads[i], NULL, child, NULL);
    }
    for (j = i; j < nAdults + nChildren; j++) {
        pthread_create(&threads[j], NULL, adult, NULL);
    }
    
    // Move everyone to Oahu first
    pthread_mutex_lock(&lock);
    while (CinOahu != nChildren || AinOahu != nAdults) {
        pthread_cond_wait(&ready, &lock); 
    }
    
    // Wake up all waiting threads
    start = 1; 
    int error = pthread_cond_broadcast(&everyoneInOahu);
    if (error == -1) {
        printf("error on broadcast of everyoneInOahu.\n");
    }
    
    printf("\nEveryone is safely on Oahu! \nAlgorithm for crossing from Oahu to Molokai begins here.\n\n"); 
    fflush(stdout);
    
    while (everyoneSafe == 0) {
        pthread_cond_wait(&everyoneAcross, &lock);
        everyoneSafe++;
    }
    printf("\nEveryone's across in Molokai! The simulation ends here. \n\n");
    pthread_mutex_unlock(&lock);
}

void* child(void *args) {
    // upon arrival, increment # of children on Oahu, signal main thread in case this was
    // the last to arrive.
    // wait until main updates start to indicate that all threads have arrived
        pthread_mutex_lock(&lock);
        CinOahu++;    
        pthread_cond_signal(&ready);
        
        while(start == 0) {
            pthread_cond_wait(&everyoneInOahu, &lock);
        }
        int error = pthread_mutex_unlock(&lock);
        if (error == -1) {
            printf("Error: Child was not unlocked\n");
            fflush(stdout);
        }
    while (1) {
        // Transfer finally begins! 
        pthread_mutex_lock(&lock); 
        
        // If there's a child awake on Oahu, that means he is about to row to Molokai with another
        // child. This implies three conditions: boatEmpty, openChildSpot, and boatFull
        while (CinBoat >= 2) { 
            pthread_cond_wait(&openChildSpot, &lock);
        }
        
        CinBoat++; //gets in boat, and increases cinboat
        printf("Child gets in the boat on Oahu. \n");
        fflush(stdout);
        
        // This child checks whether he made the boat full. If it gets in the else statement, it means 
        // the boat is full. 
        if (CinBoat == 1) {  // one child in boat
            pthread_cond_signal(&openChildSpot);
            pthread_cond_wait(&boatFull, &lock);
        }
        else {
            if (CinBoat == 2) {
                pthread_cond_signal(&boatFull);
            }
        }

        CinOahu--;
        pthread_mutex_unlock(&lock);
        // The boat now crosses the river!
        printf("Child travels from Oahu to Molokai.\n");
        fflush(stdout);
        
        pthread_mutex_lock(&lock);
        childrenAcross++;  // increasing so know when both children arrive
        
        // Wait for the other child to get here
        if (childrenAcross == 2) {
            childrenAcross = 0;
            pthread_cond_signal(&bothChildrenAcross);
        }
        else {
            pthread_cond_wait(&bothChildrenAcross, &lock);
        }
    
        // Both children are now across! 
        // If child is the last one in the boat (and people are left on Oahu), 
        // it stays in the boat and rows back to Oahu
        // so it does not increase count for CinMolokai
        if (CinBoat == 2) {
            // Jump off the boat
            printf("Child gets out of boat on Molokai.\n");
            fflush(stdout);
            CinBoat--;
            CinMolokai++;
            pthread_cond_wait(&adultInMolokai, &lock);
            CinMolokai--;
        }
        else {           
            if (CinOahu == 0 && AinOahu == 0) {
                printf("Child gets out of boat on Molokai.\n");
                fflush(stdout);
                CinMolokai++;
                pthread_cond_signal(&everyoneAcross);
                pthread_cond_wait(&sleeping, &lock);
            }
        }        
        // Row back to Oahu
        printf("\nChild travels from Molokai back to Oahu.\n");
        fflush(stdout);
        CinBoat = 0;
        printf("Child gets out of boat on Oahu.\n");
        fflush(stdout);
        if (CinOahu == 0) {
            pthread_cond_signal(&openAdultSpot);
            CinOahu++;
            pthread_cond_wait(&openChildSpot, &lock);
        }
        else {
            pthread_cond_signal(&openChildSpot);
            CinOahu++;
        }
        pthread_mutex_unlock(&lock);
    }
}

void* adult(void *args) {
    pthread_mutex_lock(&lock);
    AinOahu++;    
    pthread_cond_signal(&ready);
    
    while(start == 0) {
        pthread_cond_wait(&everyoneInOahu, &lock);
    }
    int error = pthread_mutex_unlock(&lock);
    if (error == -1) {
        printf("Error: Adult was not unlocked. \n");
        fflush(stdout);
    }
    
    pthread_mutex_lock(&lock);
    
    // Adult goes to sleeping immediately and waits for a child to wake it up.
    pthread_cond_wait(&openAdultSpot, &lock);
    
    if (AinBoat == 0) {
        AinBoat++;
        printf("Adult gets in boat on Oahu.\n");
        fflush(stdout);
        
        AinOahu--;
        AinMolokai++;
        printf("Adult travels from Oahu to Molokai!\n");
        fflush(stdout);
        AinBoat = 0;
        pthread_cond_signal(&adultInMolokai);    // Adult wakes up a child on Molokai 
        pthread_cond_wait(&sleeping, &lock);       // Adult is now done
    }
    pthread_mutex_unlock(&lock);
}