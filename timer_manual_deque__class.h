#pragma once
#include <iostream>
#include <string>
#include <chrono> 
#include <queue>



template<typename Chrono_Type> class timer_manual_deque__class
{


public:


	
	void add_timer(const size_t user_time, const std::function<void()> f_object)
	{
		const std::chrono::time_point<std::chrono::steady_clock> alarm_time = std::chrono::steady_clock::now() + Chrono_Type(user_time);   //ПРЕОБРАЗОВЫВАЕМ-Устанавливаем время от ТЕКУЩЕГО момента now() плюс время, которое Пользователь передал, которое должен отсчитывать таймер. То есть в "alarm_time" получается Абсолютное время в будущем до которого будет ждать метод std::condition_variable.wait_until(), который и принимает это асболютное время.

		priority_deq.emplace(alarm_time, std::move(f_object));    //Добавляем задачу в очередь.
	}



	void check__timer()
	{

		for (;;)
		{

			if (priority_deq.size() != 0 && priority_deq.top().alarm_time <= std::chrono::steady_clock::now())
			{
				//Значит время таймера уже равно срабатываемому времени или даже истекло, пора выполнять Пользовательскую задачу:

				priority_deq.top().f_object();

				priority_deq.pop();     //Удаляем эту задачу из очереди.

				//Проверяем дальше, пока не наткнемся на таймер вермя которого еще не истекло или пока очередь задач не станет равной нулю.

			}
			else
			{
				return;
			}

		}
		
	}
	



private:



	//----------------------------------------------------------------
	struct struct_task
	{
		std::chrono::time_point<std::chrono::steady_clock> alarm_time;       //[Читать до конца] Это тип времени, который хранит абсолютное время в "абстрактных" еденицах измерения(которые могут быть любыми единицами времени, поддерживаемыми std::chrono) от какого то момента в прошлом в "настоящий" момент, этот момент в прошлом определн то ли стандартом, то ли от реализации зависит, вообщем не имеет значения особого. То есть когда Пользователь доавбляет таймер и указывать жеаемое вермя срабатывания через некоторое время, ДЕЛАЕМ следуюющее - берем текущее время прибавляем к нему время таймера от Пользователя и получам Абсолютное время где-то в будущем в которое долен сработать таймер.
		std::function<void()> f_object;                                      //Лямбда-функция, которую нужно выполнить по истечению таймера.



		struct_task(std::chrono::time_point<std::chrono::steady_clock> alarm_time_, std::function<void()> f_object_init) //Конструктор для emplace
		{
			(*this).alarm_time = alarm_time_;
			(*this).f_object   = f_object_init;
		}
	};



	
	struct compare_time_points
	{
		bool operator()(const struct_task& a, const struct_task& b) const
		{
			return a.alarm_time > b.alarm_time;
		}
	};
	
	std::priority_queue<struct_task, std::vector<struct_task>, compare_time_points> priority_deq;  //Устанавливаем очередь с приоритетом от Меньшего к Большему, то есть меньшие значения вставляются в начало.

		
	/*
	static bool compare_time_points(const struct_task& a, const struct_task& b)
	{
		return a.alarm_time > b.alarm_time;
	}
	*/
	//----------------------------------------------------------------




};




