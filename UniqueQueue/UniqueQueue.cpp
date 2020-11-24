//ожидания события при наличии mutex
#include <condition_variable>
//для принудительного взаимного исключения
#include <mutex>
#include <iostream>
#include <queue>
#include <memory>

template <typename T>
class threadsafe_queue
{
private:
	//для указания того, что данная переменная может изменяться даже в константном контексте
	mutable std::mutex mut;
	std::queue<T> data_queue;
	std::condition_variable data_cond;
public:
	//создание явного конструктора - иначе поля пустые будут либо не тем, чем надо проинициализируются
	explicit threadsafe_queue() {}; //конструктор по умолчанию
	
	explicit threadsafe_queue(const threadsafe_queue& other) //конструктор копирования
	{
		//(доступ в любое время только у одного потока)
		//деструктор разблокирует mutex.
		std::lock_guard<std::mutex> lock(mut);
		data_queue = other.data_queue;
	};
	
	//threadsafe_queue& operator=(const threadsafe_queue& other) = delete;
	
	void push(T value) //добавление в очередь [TO DO - добавление должно быть уникальным]
	{
		//блокируем, кладём, разблокируем
		std::lock_guard<std::mutex> lock(mut);
		data_queue.push(value);
		//Разблокирует все потоки, которые ожидают объект
		data_cond.notify_one();
	};
	
	//for consumer thread
	void wait_and_pop(T& value) //удаление из очереди
	{
		//заблокируется, пока не получит уведомление, что в поток что-то поместили
		//применяется, когда есть необходимость передачи владения или вызова других методов, 
		//отсутствующих в lock_guard. Можем и блокировать, и разблокировать. 
		std::unique_lock<std::mutex> lock(mut);
		//блокирует поток
		data_cond.wait(lock, [this]
			{
				//если очередь не пуста
				return !data_queue.empty();
			});
		//получаем начало, чтобы выкинуть его из очереди
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
		//предназначен для ситуаций, 
		//когда управлять временем существования объекта в памяти требуется нескольким владельцам.
		//умный указатель на первый элемент очереди
		std::shared_ptr<const T> res(std::make_shared<const T>(data_queue.front()));
		data_queue.pop();
		return res;
	};

	//for caller
	bool try_pop(T& value) //не будет ждать - посмотрит в очереди, заблокирует, удалит
	{
		std::lock_guard<std::mutex> lock(mut);
		if (data_queue.empty())
		{
			//вернём ложь, если очередь пуста 
			return false;
			//tell caller that we're not going to place anything in the "value"
			//because there's nothing in the queue 
		}
		value = data_queue.front();
		data_queue.pop();
		//если отработали успешно, вернём истину
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

	bool empty() const //проверка очереди на пустоту
	{
		std::lock_guard<std::mutex> lock(mut);
		return data_queue.empty();
	};
};


//TEST (без многопоточности)
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