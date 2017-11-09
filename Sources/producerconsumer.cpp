#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include<iostream>
#include<queue>
using namespace std;

#define LOOP 20

void *producer (void *args);
void *consumer (void *args);

struct Jqueue {
	queue<int> cont;
	int size;
	pthread_mutex_t mut;
	pthread_cond_t notFull, notEmpty;
public:
	Jqueue(int n) {
		size=n;		
		pthread_mutex_init (&mut, NULL);
		pthread_cond_init (&notFull, NULL);
		pthread_cond_init (&notEmpty, NULL);
	}
	bool empty() { return cont.empty();}
	int pop() { int tmp=cont.front(); cont.pop() ; return tmp;}
	void push(int i) { cont.push(i);}
	bool full() { cont.size() >= size;}
};


int main ()
{
	Jqueue fifo(10);
	pthread_t pro, con;

	pthread_create (&pro, NULL, producer, &fifo);
	pthread_create (&con, NULL, consumer, &fifo);
	pthread_join (pro, NULL);
	pthread_join (con, NULL);

	return 0;
}

void *producer (void *q)
{
	Jqueue *fifo;
	int i;

	fifo = (Jqueue *)q;

	for (i = 0; i < LOOP; i++) {
		pthread_mutex_lock (&fifo->mut);
		while (fifo->full()) {
			cout << "producer: queue FULL.\n";
			pthread_cond_wait (&fifo->notFull, &fifo->mut);
		}
		fifo->push(i);
		pthread_mutex_unlock (&fifo->mut);
		pthread_cond_signal (&fifo->notEmpty);
		usleep (100000);
	}
	for (i = 0; i < LOOP; i++) {
		pthread_mutex_lock (&fifo->mut);
		while (fifo->full()) {
			printf ("producer: queue FULL.\n");
			pthread_cond_wait (&fifo->notFull, &fifo->mut);
		}
		fifo->push(i);
		pthread_mutex_unlock (&fifo->mut);
		pthread_cond_signal (&fifo->notEmpty);
		usleep (200000);
	}
	return (NULL);
}

void *consumer (void *q)
{
	Jqueue *fifo;
	int i, d;

	fifo = (Jqueue *)q;

	for (i = 0; i < LOOP; i++) {
		pthread_mutex_lock (&fifo->mut);
		while (fifo->empty()) {
			cout << "consumer: queue EMPTY.\n";
			pthread_cond_wait (&fifo->notEmpty, &fifo->mut);
		}
		d=fifo->pop();
		pthread_mutex_unlock (&fifo->mut);
		pthread_cond_signal (&fifo->notFull);
		cout << "consumer: recieved " << d << '\n';
		usleep(200000);
	}
	for (i = 0; i < LOOP; i++) {
		pthread_mutex_lock (&fifo->mut);
		while (fifo->empty()) {
			cout << "consumer: queue EMPTY.\n";
			pthread_cond_wait (&fifo->notEmpty, &fifo->mut);
		}
		d=fifo->pop();
		pthread_mutex_unlock (&fifo->mut);
		pthread_cond_signal (&fifo->notFull);
		cout << "consumer: recieved " << d << '\n';
		usleep (50000);
	}
	return (NULL);
}
