#pragma once
#include <iostream>
#include <string>
#include <list>
#include <thread>
#include <condition_variable>
#include <chrono> 
#include <queue>
#include <mutex>
#include <functional>




class thread_pool_task__class
{


public:



	void set_number_thread(int number_threads)
	{
		list_treads.resize(number_threads);

		for (std::list<std::thread>::iterator it = list_treads.begin(); it != list_treads.end(); ++it)
		{
			(*it) = std::thread(&thread_pool_task__class::working_threads, this, this);
		}

		flag_close = false;
	}




	void add_task(std::function<void()> f_object)
	{
		{
			std::unique_lock<std::mutex> my_lock(mutex_);                   //Блокируем мьютекс

			deque_fobject.push_back(std::move(f_object));
		}

		CV_.notify_one();
	}





	~thread_pool_task__class()
	{
		{
			std::unique_lock<std::mutex> lock(mutex_);

			flag_close = true;                       //Установим флаг в "true" для функции run_loop, что пора завершать созданнй поток.
		}




		CV_.notify_all();                           //Будем все потоки, если они находится в ожидании.

		for (std::list<std::thread>::iterator it = list_treads.begin(); it != list_treads.end(); ++it)
		{
			(*it).join();
		}


		deque_fobject.clear();
		list_treads.clear();
	}







private:

	//-----------------------------------------------
	std::thread threads_;
	std::mutex mutex_;
	std::condition_variable CV_;
	std::list<std::thread>list_treads;

	std::deque <std::function<void()>> deque_fobject;
	bool flag_close;
	//-----------------------------------------------




	//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


	void working_threads(thread_pool_task__class* class_p)
	{

		std::function <void()> f_object;


		while (true)
		{

			//----------------------------------------------------------------
			{
				std::unique_lock<std::mutex> my_lock((*class_p).mutex_);                                                    //Блокируем мьютекс


				while ((*class_p).flag_close == false && (*class_p).deque_fobject.size() == 0)                               //Проверяем, если ли активные задачи в очереди, которые еще не взяты ни одним потоком.
				{
					//Задач, которые не взяты - нет. Можно засыпать.

					(*class_p).CV_.wait(my_lock);                                  //Усыпляем поток в ожидании пробуждения со стороны Пользователя, в данном случае при доабвлении задачи в "deque_fobject" в функции "run_loop"
				}


				//Поток пробудился и готов выполнять работу:



				//----------------------------------------------
				if ((*class_p).flag_close == true)
				{
					//Значит Пользователь вызвал завершения потока или вызвался десктрутор. Выходим полностью из самого первого цикла.
					break;
				}
				//----------------------------------------------


				f_object = std::move((*class_p).deque_fobject.front());       //Достаем функцию из очереди.

				(*class_p).deque_fobject.pop_front();                         //Удаляем задачу из очереди.

			}
			//----------------------------------------------------------------


			f_object();     //Выполянем Пользовательскую фунцуию.

		}

	}


};


template<typename Chrono_Type> class timer_auto_task__class
{


public:


	timer_auto_task__class()
	{
		timer_thread = std::thread(&timer_auto_task__class::run_loop, this, this);       //Запускаем бесконечный цикл проверки "таймера".

		flag_close = false;
	}

	~timer_auto_task__class()
	{
		{
			std::unique_lock<std::mutex> lock(mutex);

			flag_close = true;                            //Установим флаг в "true" для функции run_loop, что пора завершать созданнй поток.
		}

		CV_.notify_one();                                 //Будем поток, если он находится в ожидании или новый таймеров, и в ожидании наступления времени.

		timer_thread.join();


		//-------------------
		//Очистим очередь приоритетов:
		while (priority_deq.size() != 0)
		{
			priority_deq.pop();
		}
		//-------------------

	}

	void set_number_threads(int numm_threads)
	{
		thread_pool_task__class_.set_number_thread(numm_threads);      //Устанавливаем кол-во потоков которое будет обробатывать очередь std::function
	}

	void add_timer(const int user_time, const std::function<void()> f_object)
	{

		const std::chrono::time_point<std::chrono::steady_clock> alarm_time = std::chrono::steady_clock::now() + Chrono_Type(user_time);   //ПРЕОБРАЗОВЫВАЕМ-Устанавливаем время от ТЕКУЩЕГО момента now() плюс время, которое Пользователь передал, которое должен отсчитывать таймер. То есть в "alarm_time" получается Абсолютное время в будущем до которого будет ждать метод std::condition_variable.wait_until(), который и принимает это асболютное время.


		{
			std::unique_lock<std::mutex> lock(mutex);        //Блокируем мьютекс, так как доступ к очереди "priority_deq" осуществляется минимум из двух потоков.


			priority_deq.emplace(alarm_time, std::move(f_object));               //Добавляем в очередь с приоритетом это время "alarm_time", добавляем таким образом, что в самый первый элемент очереди помещается элемент с самым Меньшим значением "alarm_time", то есть получается очередь элементов в которых хранится абсолютное вермя, где то в будущем, от наиближайшего времени до убывающего в будущее время.
		}

		CV_.notify_one();                                  //Будем созданный поток, в случае, если он спит в ожидании хотя бы одного элемнета очереди "priority_deq" или он "взял в работу" первый элемент очереди "priority_deq" и спит в ожидании наступления времени, которое указано в этом первом элементе очереди - во втором случае будем мы его для того, чтобы он "перепроверил" изменилось ли первое значение элемента очереди, после того, как мы сейчас доабавили элемемент - с момента, как он заснул взяв в момент засыпания значения из первого элемента очереди.
	}





private:



	//-----------------------------------------------------------------------run_loop:начало-------------------------------------------------------------------
	void run_loop(timer_auto_task__class* Timer_class_p)
	{

		std::function <void()> f_object;


		while (true)
		{
			//Функция выполняет бесконечный цикл - до того, момента, как класс не будет уничтожен или Пользователь не вызовет завершения потока.

			
				{  //std::unique_lock---->Begin

					std::unique_lock<std::mutex> lock((*Timer_class_p).mutex);


				repeat:

					if ((*Timer_class_p).flag_close == false)
					{
						//Если flag_close == false, значит деструктор не вызывался, значит нужно продолжать работу таймера.

						if ((*Timer_class_p).priority_deq.size() == 0)
						{
							//Значит очередь пуста, то просто засыпаем в ожидании пока Пользователь не вызовет метод "set_timer" тем самым добавив таймер для отслеживания и пробудив поток:

							(*Timer_class_p).CV_.wait(lock);               
						}
						else
						{
							//Значит очередь не пуста, значит проверим время в Первом элементе очереди, ЕСЛИ оно к моменту проверки МЕНЬШЕ или РАВНО текущего времени в момент проверки:
							
							if ((*Timer_class_p).priority_deq.top().time_point_ >= std::chrono::steady_clock::now())
							{
								//если время в Первом элементе очереди БОЛЬШЕ текущего времени now() в момент проверки, то значит, что время срабатывания таймера находится все еще в будущем и значит уходим на досыпание вызывая wait_until():

								(*Timer_class_p).CV_.wait_until(lock, (*Timer_class_p).priority_deq.top().time_point_);   //Засыпаем в ожидании наступления абсолютного времени где то в будущем указанного в Первом элементе очереди "priority_deq".
							}
							else
							{
								//ЗНАЧИТ наступило время срабатывания таймера или даже уже просрочено и тем более нужно вызвать срабатывание таймера:

								f_object = std::move((*Timer_class_p).priority_deq.top().f_object);       //Достаем Пользовательскую задачу, которую нужно выполнить.

								(*Timer_class_p).priority_deq.pop();                           //Удаляем первый элемент из очереди.

								(*Timer_class_p).thread_pool_task__class_.add_task(std::move(f_object));  //Добавляем задачу в очередь задач в работу.
							}
						}
					}
					else
					{
						//Значит Пользователь вызвал завершения потока или вызвался десктрутор. Выходим полностью из самого первого цикла While и завергаем функцию run_loop и поток.
						break;
					}

					goto repeat;

				}  //std::unique_lock---->End


		}
	}
	//-----------------------------------------------------------------------run_loop:конец-------------------------------------------------------------------




	//----------------------------------------------------------------

	struct struct_task
	{
		std::chrono::time_point<std::chrono::steady_clock> time_point_;       //Это тип времени, который хранит абсолютное время в "абстрактных" еденицах измерения(которые могут быть любыми единицами времени, поддерживаемыми std::chrono) от какого то момента в прошлом в "настоящий" момент, этот момент в прошлом определн то ли стандартом, то ли от реализации зависит, вообщем не имеет значения особого.
		std::function<void()> f_object;



		struct_task(std::chrono::time_point<std::chrono::steady_clock> time_point_init, std::function<void()> f_object_init)
		{
			(*this).time_point_ = time_point_init;
			(*this).f_object = f_object_init;
		}
	};

	std::priority_queue<struct_task, std::vector<struct_task>, bool(*)(const struct_task&, const struct_task&)> priority_deq{ timer_auto_task__class::compare_time_points };  //Устанавливаем очередь с приоритетом от Меньшего к Большему, то есть меньшие значения вставляются в начало.


	static bool compare_time_points(const struct_task& a, const struct_task& b)
	{
		return a.time_point_ > b.time_point_;
	}
	//----------------------------------------------------------------



	//---------------------------------------------------------------
	std::mutex mutex;
	std::condition_variable CV_;
	std::thread timer_thread;
	bool flag_close;                  //Флаг уничтожения класса и завершения созданного потока.
	//---------------------------------------------------------------



	//----------------------------------------
	thread_pool_task__class thread_pool_task__class_;
	//----------------------------------------

};



