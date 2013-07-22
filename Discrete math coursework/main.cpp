#ifdef _WIN32
	#define WIN32
#endif

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Simple_Counter.H>
#include <FL/Fl_Radio_Round_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Button.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Check_Button.H>
#include <sstream>
#include "mylib.h"

#ifdef WIN32
	#include <Windows.h>
#elif __linux
	#include <pthread.h>
#endif

Fl_Simple_Counter* counter;
Fl_Window *window;
Fl_Radio_Round_Button* rtype1;
Fl_Radio_Round_Button* rtype2;
Fl_Choice* piece;
Fl_Progress* progress;
Fl_Button* start;
Fl_Text_Display* txt_out;
Fl_Window* txt_window;
Fl_Pack* txt_pack;
Fl_Pack* txt_buttons;
Fl_Button* txt_close;
Fl_Button* txt_copy;
Fl_Button* txt_save;
Fl_Check_Button* full_dnf;

#ifdef WIN32
	HANDLE calc_thread_h;
#elif __linux
	pthread_t calc_thread_h;
#endif

void on_calc_step(unsigned i,unsigned size)
{
	static char label_buf[32];
	std::stringstream s;
	s << "Раскрытие скобок: " << i << "/" << size;
	strcpy(label_buf,s.str().c_str());
	Fl::lock();
	progress->label(label_buf);
	progress->value(100*i/size);
	Fl::unlock();
}

struct thread_param
{
	unsigned size;
	mylib::pfunc p;
	bool intern;
	bool tmp_res;
};
#ifdef WIN32
DWORD WINAPI calc_thread( LPVOID lpParam )
#elif __linux
void* calc_thread(void* lpParam)
#endif
{
	using namespace mylib;
	thread_param* param = (thread_param*)lpParam;
	
	std::stringstream *sout = new std::stringstream;

	*sout << "#####################################" << std::endl;
	if(param->intern) *sout << "Внутренняя устойчивость, ";
	else *sout << "Внешняя устойчивость, ";
	*sout << param->p.name << ", " << param->size << "x" << param->size << std::endl;
	*sout << "#####################################" << std::endl << std::endl;

	*sout << "Матрица смежности" << std::endl;
	Matrix m(*(param->p.func),param->size);
	if(!param->intern)
		for(int i = 0; i < m.size(); i++)
			m[i][i]=true;
	*sout << m << endl;
	
	*sout << "Составление выражения по матрице:" << endl;
	if(param->intern)
	{
		BoolExpr expr(m,param->size);
		*sout << expr << endl;
		if(param->tmp_res)
		{
			*sout << "Приведение к ДНФ" << endl;
			expr.toDNF(&on_calc_step,sout);
		}
		else
			expr.toDNF(&on_calc_step);
		*sout << "ДНФ:" << endl;
		*sout << expr << endl;
		*sout << "Дополнения:\n";
		BoolExpr* cmpl = expr.getComplement();
		*sout << *cmpl << endl;
		*sout << "Число внутренней устойчивости:\n";
		*sout << cmpl->getMaxConjSize() << endl;
		*sout << "Расстановки с максимальным числом фигур" << endl;
		vector<vector<short> >* st = cmpl->getMaxConjs();
		for(int i = 0; i < st->size(); i++)
		{
			for(int j = 0; j < (*st)[i].size(); j++)
			{
				*sout << chess_pos(st->at(i).at(j),param->size) << " ";
			}
			*sout << endl;
		}
		delete st;
		delete cmpl;
	}
	else
	{
		BoolExprExt expr(m,param->size);
		*sout << expr << endl;
		if(param->tmp_res)
		{
			*sout << "Приведение к ДНФ:" << endl;
			expr.toDNF(sout,&on_calc_step);
			*sout << endl;
		}
		else
			expr.toDNF(NULL,&on_calc_step);
		*sout << "ДНФ:" << endl;
		*sout << expr << endl;
		*sout << "Число внешней устойчивости:" << endl;
		*sout << expr.getMinNumber() << endl;
		*sout << "Расстановки с минимальным числом фигур:" << endl;
		_bexpr *ms = expr.getMinSets();
		for(int i = 0; i < ms->size(); i++)
		{
			for(int j = 0; j < (*ms)[i].size(); j++)
			{
				*sout << chess_pos(ms->at(i).at(j),param->size) << " ";
			}
			*sout << endl;
		}
		delete ms;
	}

	delete param;

	Fl::awake(sout);

	return NULL;
}

void begin_calc_thread()
{
	progress->show();
	start->deactivate();

	int cur_p = piece->value();

	bool is_intern = false;
	if(rtype1->value())
		is_intern=true;

	unsigned size = counter->value();
	
	bool tmp_results = false;
	if(full_dnf->value())
		tmp_results = true;

	thread_param* pth = new thread_param;
	pth->p = mylib::pieces[cur_p];
	pth->size = size;
	pth->intern = is_intern;
	pth->tmp_res = tmp_results;

#ifdef WIN32
	DWORD thread_id;
	calc_thread_h = CreateThread(NULL,0,calc_thread,pth,0,&thread_id);
	if(!calc_thread_h)
		fl_alert("cannot create new thread");
#elif __linux
	if(pthread_create(&calc_thread_h,NULL,calc_thread,pth) != 0)
		fl_alert("cannot create new thread");
#endif
}

void end_calc_thread(std::stringstream *msg)
{
#ifdef WIN32
	WaitForSingleObject(calc_thread_h,100);
	CloseHandle(calc_thread_h);
#endif
	
	progress->value(0);
	if(!txt_out->buffer())
		txt_out->buffer(new Fl_Text_Buffer(0,64));

	txt_out->buffer()->text(msg->str().c_str());
	delete msg;

	txt_window->show();
}

void on_start(Fl_Widget* w,void* data)
{
	begin_calc_thread();
}

void on_txt_close(Fl_Widget* w,void* data)
{
	txt_window->hide();
	progress->hide();
	start->activate();
}

void on_txt_copy(Fl_Widget* w,void* data)
{
	char *t = txt_out->buffer()->text();
	Fl::copy(t,strlen(t),1);
	delete [] t;
}

void on_txt_save(Fl_Widget* w,void* data)
{
	Fl_Native_File_Chooser c;
	c.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	c.title("Сохранить");
	c.preset_file("result.txt");
	
	c.filter("Text (*.txt)");
	c.show();
	txt_out->buffer()->savefile(c.filename());
}

int main(int argc, char **argv) {
  window = new Fl_Window(400,300);
  Fl_Pack* pack = new Fl_Pack(100,0,200,300);
  pack->spacing(20);

  counter = new Fl_Simple_Counter(0,0,100,20,"Размер поля");
  counter->step(1.0);
  counter->minimum(2);
  counter->maximum(10);
  counter->value(4);

  rtype1 = new Fl_Radio_Round_Button(0,60,300,20,"Внутренняя устойчивость");
  rtype2 = new Fl_Radio_Round_Button(0,100,300,20,"Внешняя устойчивость");
  rtype1->value(1);

  piece = new Fl_Choice(0,150,300,20);
  for(unsigned int i=0; i < mylib::pieces_n; i++)
	  piece->add(mylib::pieces[i].name);
  piece->value(piece->menu());

  progress = new Fl_Progress(0,0,300,30,"Прогресс");
  progress->minimum(0); progress->maximum(100);
  progress->value(0);
  progress->hide();

  start = new Fl_Button(0,40,300,30,"Считать");
  start->callback(&on_start);
  
  
  full_dnf = new Fl_Check_Button(0,0,300,20,"Выводить промежуточные результаты");

  pack->add(piece);
  pack->add(rtype1);
  pack->add(rtype2);
  pack->add(counter);
  pack->add(full_dnf);
  pack->add(start);
  pack->add(progress);

  txt_window = new Fl_Window(500,500,"Результат");
  txt_pack = new Fl_Pack(0,0,500,500);
  
  txt_buttons = new Fl_Pack(0,0,500,30);
  txt_buttons->type(Fl_Pack::HORIZONTAL);
  
  txt_out = new Fl_Text_Display(0,0,500,470);
  txt_out->textfont(FL_COURIER);

  txt_close = new Fl_Button(0,0,100,30,"Закрыть");
  txt_copy = new Fl_Button(0,0,100,30,"Копировать");
  txt_save = new Fl_Button(0,0,100,30,"Сохранить");

  txt_close->callback(&on_txt_close);
  txt_copy->callback(&on_txt_copy);
  txt_save->callback(&on_txt_save);

  txt_pack->add(txt_out);
  txt_buttons->add(txt_close);
  txt_buttons->add(txt_copy);
  txt_buttons->add(txt_save);


  txt_window->callback(&on_txt_close);
  txt_window->resizable(txt_pack);
  txt_pack->add_resizable(*txt_out);

  txt_pack->add(txt_buttons);
  txt_window->add(txt_pack);

  window->label("Дискретка");
  window->add(pack);
  window->show(argc, argv);
  //Fl::scheme("windows");
  Fl::lock();
  while(Fl::wait() > 0)
  {
	  std::stringstream* msg;
	  if((msg = (std::stringstream*)Fl::thread_message()))
	  {
			end_calc_thread(msg);
	  }
  }
  return 0;
}