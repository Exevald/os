#include "ThreadSafeQueue.h"

#include <iostream>
#include <thread>

int main()
{
	ThreadSafeQueue<int> q(2);

	std::jthread producer([&] {
		for (int i = 0; i < 10; ++i)
		{
			q.Push(i);
		}
	});
	std::jthread consumer([&] {
		for (int i = 0; i < 10; ++i)
		{
			const int x = q.WaitAndPop();
			std::cout << x << ' ';
		}
	});
	producer.join();
	consumer.join();

	std::cout << std::endl;
}