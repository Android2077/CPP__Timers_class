#pragma once
#include <iostream>
#include <string>
#include <chrono> 



template<typename Chrono_Type> class timer_manual__class
{


public:



	void start()
	{
		total_pause_time = 0;

		start_ = std::chrono::steady_clock::now();
	}

	void pause()
	{
		pause_start = std::chrono::steady_clock::now();

		pause_enable_flag = 1;      //Устанавливаем флаг, что нажата пауза
	}

	void resume()
	{
		pause_end = std::chrono::steady_clock::now();

		unsigned long long pause_time = std::chrono::duration_cast<Chrono_Type>(pause_end - pause_start).count();

		pause_enable_flag = 0;        //Снимаем флаг, что нажата пауза

		total_pause_time = total_pause_time + pause_time;      //Прибавляем к общему времени паузы очередную завершившуся паузу.
	}

	const unsigned long long get__total_pause_time()
	{
		if (pause_enable_flag == 1)
		{
			//Значит пауза нажата:

			return  total_pause_time + std::chrono::duration_cast<Chrono_Type>(std::chrono::steady_clock::now() - pause_start).count();  //Возвращаем Пользователю время сколько прошло с начала последнего нажатия паузы, при условии, что пауза на данный моемнт еще не отжата до этого момента.
		}
		else
		{
			//Значит пауза не нажата:

			return total_pause_time;        //Возврощаем общее суммарно время всех нажатых пауз.
		}
	}

	const unsigned long long get__time()
	{
		end_ = std::chrono::steady_clock::now();

		return std::chrono::duration_cast<Chrono_Type>(end_ - start_).count() - get__total_pause_time();
	}




private:


	//-----------------------------------
	std::chrono::time_point<std::chrono::steady_clock> start_;
	std::chrono::time_point<std::chrono::steady_clock> end_;

	unsigned long long diff = 0;
	//-----------------------------------


	//-----------------------------------
	std::chrono::time_point<std::chrono::steady_clock> pause_start;
	std::chrono::time_point<std::chrono::steady_clock> pause_end;

	unsigned long long total_pause_time = 0;

	int pause_enable_flag = 0;      //0 - пауза на данный момент не активирована;  1 - пауза на данынй момент активирована.
	//-----------------------------------

};




