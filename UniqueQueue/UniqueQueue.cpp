#include <condition_variable>
#include <mutex>
#include <iostream>
#include <queue>
#include <memory>

template <typename T>
class threadsafe_queue
{
private:
	mutable std::mutex mut;
	std::queue<T> data_queue;
	std::condition_variable data_cond;
public:
	explicit threadsafe_queue() {};
	
	explicit threadsafe_queue(const threadsafe_queue& other)
	{
		std::lock_guard<std::mutex> lock(mut);
		data_queue = other.data_queue;
	};
	
	threadsafe_queue& operator=(const threadsafe_queue& other) = delete;
	
	void push(T value)
	{
		std::lock_guard<std::mutex> lock(mut);
		data_queue.push(value);
		data_cond.notify_one();
	};
	
	void wait_and_pop(T& value)
	{
		std::unique_lock<std::mutex> lock(mut);
		data_cond.wait(lock, [this]
			{
				return !data_queue.empty();
			});
		value = data_queue.front();
		data_queue.pop();
	};

	std::shared_ptr<const T> wait_and_pop()
	{
		std::unique_lock<std::mutex> lock(mut);
		data_cond.wait(lock, [this]
			{
				return !data_queue.empty();
			});
		std::shared_ptr<const T> res(std::make_shared<const T>(data_queue.front()));
		data_queue.pop();
		return res;
	};

	bool try_pop(T& value)
	{
		std::lock_guard<std::mutex> lock(mut);
		if (data_queue.empty())
		{
			return false;
		}
		value = data_queue.front();
		data_queue.pop();
		return true;
	};

	std::shared_ptr<const T> try_pop()
	{
		std::lock_guard<std::mutex> lock(mut);
		if (data_queue.empty())
		{
			return nullptr;
		}
		std::shared_ptr<const T> res(std::make_shared<const T>(data_queue.front()));
		data_queue.pop();
		return res;
	};

	bool empty() const
	{
		std::lock_guard<std::mutex> lock(mut);
		return data_queue.empty();
	};
};

int main()
{
	threadsafe_queue<int> ourQueue{};
	ourQueue.push(10);
	auto p = ourQueue.try_pop();
	if (p)
	{
		std::cout << *p << std::endl;
	}
	else
	{
		//this should not happen!
		std::cout << "Cannot poop from the queue, as it is empty" << std::endl;
	}
	if (ourQueue.empty())
	{
		std::cout << "Our queue is empty!" << std::endl;
	}
}