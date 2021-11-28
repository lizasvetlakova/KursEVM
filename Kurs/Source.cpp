#include <iostream>
#include <functional>
#include <thread>
#include <chrono>
#include <time.h>

using namespace std;

const int N = 4;   //кол-во процессоров и блоков памяти
const int K = 10;  //кол-во команд
const int M = 2;   //кол-во времени для обращения к памяти
const int Pn = 90; //процент данных сосредоточенных в одном блоке памяти
const int Kr = 80; //процент команд, не требующих обращения к памяти

enum { DENIED, ALLOWED, ACCESSED };
enum { REQUEST, FREE };

int ACCESS = DENIED; 
int commands[N][K];  
int lock[N] = { DENIED, DENIED, DENIED, DENIED }; 
int time_cnt = 0; 
chrono::time_point <chrono::steady_clock> t_begin; //начало выполнения
chrono::time_point <chrono::steady_clock> t_end; //конец

thread::id id1; //айди потоков
thread::id id2;
thread::id id3;
thread::id id4;

int memoryAccess(int request) //доступ к памяти для шины
{
	if (ACCESS == ALLOWED && request == REQUEST)
	{
		ACCESS = DENIED;
		return ACCESSED;
	}
	else if (ACCESS == DENIED && request == FREE)
		ACCESS = ALLOWED;
		
	return ACCESS;
}

void setCommands()
{
	/*надо сделать 
	распределение по заданным вероятностям случайным образом каким командам нужно обратиться к памяти 
	*/
	
	for (int j = 0; j < N; j++) {
		for (int i = 0; i < K; i++){
			commands[j][i] = 0; 
	    }
	}
	//пока что установка вручную с учётом того, что 80% без обращения к памяти
	commands[0][1] = 1; commands[0][7] = 3;
	commands[1][2] = 2; commands[1][7] = 1;
	commands[2][9] = 2; commands[2][7] = 3;
	commands[3][4] = 3; commands[3][7] = 4;
}

void printСommands()
{
	//матрица (команды/процессоры) в ячейках номера блоков памяти, если 0, то значит команда не требует обращения к памяти
	this_thread::sleep_for(chrono::milliseconds(500));
	cout << "# K\\Pr\tPr1\tPr2\tPr3\tPr4" << endl;

	for (int i = 0; i < K; i++)
	{
		cout << i + 1 << '\t' << commands[0][i] << '\t' << commands[1][i] << '\t' << commands[2][i] << '\t' << commands[3][i] << endl;

		if (i == K - 1)
		{
			ACCESS = ALLOWED;
			cout << endl << endl;
		}
	}
}

void bus(int id) //проц НЕ могут одновременно обращаться к памяти, если первый обращается к памяти, то все остальные ждут пока первый выполнит команду
{
	int access;

	for (int i = 0; i < K; i++)
	{
		if (commands[id - 1][i]) //выполняется команда + запрос памяти
		{
			cout << "\nREQUEST MEMORY " << commands[id - 1][i] << "\tPr " << id;

			while (true)
			{
				access = memoryAccess(REQUEST); //статус доступа к памяти

				if (access == ACCESSED) //если память свободна
				{
					cout << "\nACCESSED MEMORY " << commands[id - 1][i] << "\tPr " << id;
					this_thread::sleep_for(chrono::milliseconds(M * 50));
					break;
				}
				else //если память занята
				{
					cout << "\nWAITING MEMORY " << commands[id - 1][i] << "\tPr " << id;
					this_thread::sleep_for(chrono::milliseconds(200));
				}
			}
			memoryAccess(FREE); //разрешаем доступ памати, к которой ранее обратились
		}
		else
			this_thread::sleep_for(chrono::milliseconds(100));

		cout << "\nDONE COMMAND " << i + 1 << "\t\tPr " << id;
	}
}

void commutator(int id)
{
	//что-то похожее как для шины 
	//но процессоры могут одновременно работать с памятью, если память, к которой они обращаются не занята
}

void processor(int id)
{
	int tmp_cnt = 0;
	setCommands();

	while (id != 1 && ACCESS == DENIED) 
		this_thread::yield();

	if (id == 1){
		printСommands();
		cout << endl << "------------Bus-------------";
	}
	this_thread::sleep_for(chrono::milliseconds(1000));
	if (time_cnt == 0){
		time_cnt = 1;
		t_begin = chrono::steady_clock::now(); 
	}

	bus(id); 
	
	lock[id - 1] = 0;
	while (true)
	{
		if (!lock[0] && !lock[1] && !lock[2] && !lock[3]) 
		{
			time_cnt = 0;
			this_thread::sleep_for(chrono::milliseconds(500));
			break;
		}
	}
	lock[id - 1] = 1;
	
	bool flag = 1;
	while (flag)
	{
		if (id == 1)
		{
			t_end = std::chrono::steady_clock::now();
			auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_begin);
			cout << "\nTime: " << (double)(elapsed_ms.count() / 1000.0) - 0.5 << " seconds" << endl;
			flag = 0;
		}
	}

	//дальше работа коммутатора
	//commutator(id);
}

void threadFunction() //функция исполнения потока
{
	for (int i = 0; i < N; i++){
		if (!lock[i]){
			lock[i] = 1;
			switch (i) //определяем айди потока
			{
			case 0:
				id1 = this_thread::get_id();
				break;
			case 1:
				id2 = this_thread::get_id();
				break;
			case 2:
				id3 = this_thread::get_id();
				break;
			case 3:
				id4 = this_thread::get_id();
				break;
			}
			break;
		}
	}
	//вызываем соответствующую функцию процессора
	if (this_thread::get_id() == id1)
		processor(1);
	else if (this_thread::get_id() == id2)
		processor(2);
	else if (this_thread::get_id() == id3)
		processor(3);
	else if (this_thread::get_id() == id4)
		processor(4);
}

int main()
{
	//потоки 
	thread th1(threadFunction);
	thread th2(threadFunction);
	thread th3(threadFunction);
	thread th4(threadFunction);

	//блокировка вызывающего потока main
	th1.join();
	th2.join();
	th3.join();
	th4.join();

	cout << endl;
	system("pause");
	return 0;
}
