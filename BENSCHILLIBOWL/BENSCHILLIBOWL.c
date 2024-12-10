#include "BENSCHILLIBOWL.h"
#include <assert.h>
#include <stdlib.h>
#include <time.h>

bool IsEmpty(BENSCHILLIBOWL* bcb);
bool IsFull(BENSCHILLIBOWL* bcb);
void AddOrderToBack(Order **orders, Order *order);

MenuItem BENSCHILLIBOWLMenu[] = { 
    "BensChilli", 
    "BensHalfSmoke", 
    "BensHotDog", 
    "BensChilliCheeseFries", 
    "BensShake",
    "BensHotCakes",
    "BensCake",
    "BensHamburger",
    "BensVeggieBurger",
    "BensOnionRings",
};
int BENSCHILLIBOWLMenuLength = 10;

/* Select a random item from the Menu and return it */
MenuItem PickRandomMenuItem() {
    int index = rand() % BENSCHILLIBOWLMenuLength;
    return BENSCHILLIBOWLMenu[index];
}

/* Allocate memory for the Restaurant, then create the mutex and condition variables needed to instantiate the Restaurant */
BENSCHILLIBOWL* OpenRestaurant(int max_size, int expected_num_orders) {
    BENSCHILLIBOWL *bcb = malloc(sizeof(BENSCHILLIBOWL));
    bcb->max_size = max_size;
    bcb->expected_num_orders = expected_num_orders;
    bcb->current_size = 0;
    bcb->orders_handled = 0;
    bcb->orders_remaining = expected_num_orders;
    bcb->orders = NULL;

    pthread_mutex_init(&(bcb->mutex), NULL);
    pthread_cond_init(&(bcb->can_add_orders), NULL);
    pthread_cond_init(&(bcb->can_get_orders), NULL);

    srand(time(NULL));  // Seed the random number generator

    printf("Restaurant is open!\n");
    return bcb;
}

/* check that the number of orders received is equal to the number handled (ie.fullfilled). Remember to deallocate your resources */
void CloseRestaurant(BENSCHILLIBOWL* bcb) {
    assert(bcb->orders_handled == bcb->expected_num_orders);

    pthread_mutex_destroy(&(bcb->mutex));
    pthread_cond_destroy(&(bcb->can_add_orders));
    pthread_cond_destroy(&(bcb->can_get_orders));
    free(bcb);
    printf("Restaurant is closed!\n");
}

/* add an order to the back of queue */
int AddOrder(BENSCHILLIBOWL* bcb, Order* order) {
    pthread_mutex_lock(&(bcb->mutex));

    while (IsFull(bcb)) {
        pthread_cond_wait(&(bcb->can_add_orders), &(bcb->mutex));
    }

    order->order_number = bcb->next_order_number++;  // Assign order number
    AddOrderToBack(&(bcb->orders), order);
    bcb->current_size++;
    bcb->orders_remaining--;  // Update orders remaining
    pthread_cond_signal(&(bcb->can_get_orders));  // Signal cooks

    pthread_mutex_unlock(&(bcb->mutex));
    return order->order_number;
}

/* remove an order from the queue */
Order *GetOrder(BENSCHILLIBOWL* bcb) {
    pthread_mutex_lock(&(bcb->mutex));

    while (IsEmpty(bcb) && bcb->orders_remaining > 0) {
        pthread_cond_wait(&(bcb->can_get_orders), &(bcb->mutex));
    }

    if (IsEmpty(bcb) && bcb->orders_remaining == 0) {
        pthread_cond_broadcast(&(bcb->can_get_orders));  // Notify other cooks
        pthread_mutex_unlock(&(bcb->mutex));
        return NULL;  // No more orders to process
    }

    Order *order = bcb->orders;
    bcb->orders = bcb->orders->next;  // Move to the next order
    bcb->current_size--;
    bcb->orders_handled++;
    pthread_cond_signal(&(bcb->can_add_orders));  // Signal customers

    pthread_mutex_unlock(&(bcb->mutex));
    return order;
}

bool IsFull(BENSCHILLIBOWL* bcb) {
    return bcb->current_size >= bcb->max_size;
}

bool IsEmpty(BENSCHILLIBOWL* bcb) {
    return bcb->current_size == 0;
}

void AddOrderToBack(Order **orders, Order *order) {
    if (*orders == NULL) {
        *orders = order;  // First order in the queue
    } else {
        Order *current = *orders;
        while (current->next != NULL) {
            current = current->next;  // Traverse to the end
        }
        current->next = order;  // Add the new order at the end
    }
    order->next = NULL;  // Ensure the new order is the last in the queue
}
