#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define MAX_MEALS 200
#define MAX_HUNGRY_CYCLES 10  // Maximum cycles a philosopher can starve before dying

int NUMBER_OF_PHILOSOPHERS;

// Simple spinlock implementation
typedef struct {
    volatile int locked;  // Lock state: 0 - unlocked, 1 - locked
} SimpleLock;

// Initialize the lock
void init_lock(SimpleLock *lock) {
    lock->locked = 0;
}

// Acquire the lock (busy-waiting)
void acquire_lock(SimpleLock *lock) {
    while (__sync_lock_test_and_set(&(lock->locked), 1)) {
        // Active waiting (spinlock)
    }
}

// Release the lock
void release_lock(SimpleLock *lock) {
    __sync_lock_release(&(lock->locked));
}

SimpleLock *forks;           // Array of locks representing the forks
int *meals_eaten;            // Array to track how many meals each philosopher has eaten
int *hungry_cycles;          // Array to track how many cycles a philosopher has been hungry
pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for synchronizing console output

// Function representing the philosopher's behavior
void *philosopher(void *arg) {
    int id = *(int *)arg;

    // Identify left and right forks (adjusting order to avoid deadlock)
    int left_fork = id;
    int right_fork = (id + 1) % NUMBER_OF_PHILOSOPHERS;

    // Odd philosophers pick up forks in reverse order to prevent circular wait (deadlock)
    if (id % 2 != 0) {
        left_fork = (id + 1) % NUMBER_OF_PHILOSOPHERS;
        right_fork = id;
    }

    while (meals_eaten[id] < MAX_MEALS) {
        // Thinking phase
        printf("Philosopher %d is thinking\n", id);
        sleep(rand() % 2 + 1);  // Simulate thinking time

        // Check if the philosopher has starved
        if (hungry_cycles[id] >= MAX_HUNGRY_CYCLES) {
            pthread_mutex_lock(&output_mutex);
            printf("Philosopher %d died of hunger after %d cycles!\n", id, MAX_HUNGRY_CYCLES);
            pthread_mutex_unlock(&output_mutex);
            exit(0);  // Terminate the program if any philosopher dies
        }

        // Try to pick up the left fork
        acquire_lock(&forks[left_fork]);
        printf("Philosopher %d picked up left fork\n", id);

        // Try to pick up the right fork
        acquire_lock(&forks[right_fork]);
        printf("Philosopher %d picked up right fork\n", id);

        // Eating phase
        pthread_mutex_lock(&output_mutex);
        printf("Philosopher %d is eating\n", id);
        pthread_mutex_unlock(&output_mutex);

        meals_eaten[id]++;      // Increase the meal counter
        hungry_cycles[id] = 0;  // Reset the hunger counter
        sleep(rand() % 2 + 1);  // Simulate eating time

        // Release the left fork
        release_lock(&forks[left_fork]);
        printf("Philosopher %d put down left fork\n", id);

        // Release the right fork
        release_lock(&forks[right_fork]);
        printf("Philosopher %d put down right fork\n", id);

        // Increment the hunger counter (philosopher didn't eat this cycle)
        hungry_cycles[id]++;
    }

    printf("Philosopher %d has finished eating\n", id);
    return NULL;
}

int main() {
    // Read the number of philosophers from user input
    printf("Enter the number of philosophers: ");
    scanf("%d", &NUMBER_OF_PHILOSOPHERS);

    // Allocate memory for philosopher threads, locks, and tracking arrays
    pthread_t *threads = (pthread_t *)malloc(NUMBER_OF_PHILOSOPHERS * sizeof(pthread_t));
    forks = (SimpleLock *)malloc(NUMBER_OF_PHILOSOPHERS * sizeof(SimpleLock));
    meals_eaten = (int *)malloc(NUMBER_OF_PHILOSOPHERS * sizeof(int));
    hungry_cycles = (int *)malloc(NUMBER_OF_PHILOSOPHERS * sizeof(int));
    int *ids = (int *)malloc(NUMBER_OF_PHILOSOPHERS * sizeof(int));

    // Initialize locks and counters
    for (int i = 0; i < NUMBER_OF_PHILOSOPHERS; i++) {
        init_lock(&forks[i]);
        meals_eaten[i] = 0;
        hungry_cycles[i] = 0;
        ids[i] = i;

        // Create a thread for each philosopher
        pthread_create(&threads[i], NULL, philosopher, &ids[i]);
    }

    // Wait for all philosopher threads to complete
    for (int i = 0; i < NUMBER_OF_PHILOSOPHERS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Free allocated memory
    free(threads);
    free(forks);
    free(meals_eaten);
    free(hungry_cycles);
    free(ids);

    printf("Simulation finished successfully.\n");
    return 0;
}
