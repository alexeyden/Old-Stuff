#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL.h>
#include <time.h>
#include <math.h>
#include <sys/stat.h>

#include "config.h"

#define bool char
#define TRUE 1
#define FALSE 0

/* shape colors*/
#define COLOR_BG   0.12f, 0.14f, 0.13f
#define COLOR_TEXT 0.94f, 0.87f, 0.69f
#define COLOR_L  { 0.50f, 0.62f, 0.50f }
#define COLOR_J  { 0.37f, 0.70f, 0.54f }
#define COLOR_I  { 0.94f, 0.88f, 0.67f }
#define COLOR_Z  { 0.55f, 0.82f, 0.83f }
#define COLOR_S  { 0.58f, 0.75f, 0.95f }
#define COLOR_T  { 0.86f, 0.64f, 0.64f }
#define COLOR_O  { 0.44f, 0.31f, 0.31f }

/*  OpenGL and SDL stuff   */
SDL_Surface *surface;
int videoFlags;
bool win_active;

/* global game variables */
double shape_x,shape_y;
unsigned int shape_next;
unsigned int game_speed;
unsigned int game_score;
unsigned int msg_active;

/* messages text */
char msg_text_usage[] =
	"\nUsage: zbtetris [ OPTIONS ]\n"
	"Avaliable options:\n"
	"  -h, --help                     This help\n"
	"  -f, --fullscreen               Start game in fullscreen mode\n"
	"  -s, --size <width>x<height>    Set window size\n"
	"  -l, --level <level>            Set start level\n\n";

char msg_text_help[] =
	"        HELP        \n \n"
	"Keys:                      \n"
	" <left>  - move shape left \n"
	" <right> - move shape right\n"
	" <up>    - turn shape      \n"
	" <down>  - move shape down \n"
	" <q>     - exit            \n"
	" <n>     - new game        \n"
	" <s>     - show highscore  \n"
	" <h>     - help            ";

char msg_text_endgame[] =
	"     GAME OVER     \n \n"
	"Your score: %i\n \n"
	"Press                 \n"
	" <q> - quit        \n"
	" <n> - new game    ";

char msg_text_newgame[] =
	"     NEW GAME      \n \n"
	"   Are you serious?   \n \n"
	"Press                 \n"
	" <n>   - new game     \n"
	" <ESC> - continue     \n";

char msg_text_quit[] =
	"       QUIT        \n \n"
	" Really want to quit? \n \n"
	"Press                 \n"
	" <q>   - quit         \n"
	" <ESC> - coninue game \n";

enum
{
	MSG_NONE = -1,
	MSG_END_GAME = 0,
	MSG_QUIT = 1,
	MSG_SCORES = 2,
	MSG_HELP = 3,
	MSG_NEWGAME = 4
};

enum
{
	NEW_GAME = 0,
	HIGH_SCORE = 1,
	HELP = 2,
	QUIT = 3,
	MOVE_LEFT = 4,
	MOVE_RIGHT = 5,
	MOVE_DOWN = 6,
	TURN = 7,
	DROP = 8,
	CANCEL = 9,
	END_GAME = 10
};

/* basic game structures */

struct keybind
{
	SDLKey key;
	void (*on_key_func)(bool);
	char keyname[8];
	bool repeat;
};

struct config
{
	unsigned int width;
	unsigned int height;
	bool fullscreen;
}game_config;

struct vec2
{
	double x;
	double y;
};

struct rgb
{
	float r;
	float g;
	float b;
};

struct shape
{
	struct vec2 points[2][4];
	struct rgb color;
	struct vec2 size;
	int state;
}shape_cur;

struct font
{
	GLuint texture;
	GLuint list_base;
}game_font;

struct player
{
	char name[16];
	unsigned int score;
};

struct list
{
	struct vec2 pos;
	struct rgb color;
	struct list* next;
}*l_root;

void font_destroy(struct font*);

void game_new();
void game_quit();
void game_end();

int shp_check_intersect(struct shape*,int,int);
void shp_check_drop();
void shp_flip(struct shape*);

void on_key_new_game(bool);
void on_key_scores(bool);
void on_key_help(bool);
void on_key_quit(bool);
void on_key_move_left(bool);
void on_key_move_right(bool);
void on_key_move_down(bool);
void on_key_turn(bool);
void on_key_drop(bool);
void on_key_cancel(bool);

int highscore_save(char*);

/* highscore list*/
struct player high_score[10] = {
	{"Nobody", 10},
	{"Nobody", 30},
	{"Nobody", 50},
	{"Nobody", 100},
	{"Nobody", 200},
	{"Nobody", 300},
	{"Nobody", 400},
	{"Nobody", 500},
	{"Nobody", 1000},
	{"Nobody", 5000}
};

/* shapes */
struct shape shapes[7]  = {
	/*      shape points          |   turned shape points     |  color | size | */
	{ { {{1,2},{2,2},{3,2},{3,1}}, {{2,1},{2,2},{2,3},{3,3}} }, COLOR_L, {3,3}, 0 }, /* L */
	{ { {{1,1},{1,2},{2,2},{3,2}}, {{3,1},{2,1},{2,2},{2,3}} }, COLOR_J, {3,3}, 0 }, /* J */
	{ { {{1,2},{2,2},{3,2},{4,2}}, {{2,1},{2,2},{2,3},{2,4}} }, COLOR_I, {4,3}, 0 }, /* I */
	{ { {{1,1},{2,1},{2,2},{3,2}}, {{2,1},{2,2},{1,2},{1,3}} }, COLOR_Z, {3,2}, 0 }, /* Z */
	{ { {{1,2},{2,2},{2,1},{3,1}}, {{1,1},{1,2},{2,2},{2,3}} }, COLOR_S, {3,2}, 0 }, /* S */
	{ { {{1,2},{2,2},{3,2},{2,3}}, {{1,2},{2,1},{2,2},{2,3}} }, COLOR_T, {3,3}, 0 }, /* T */
	{ { {{1,1},{2,1},{1,2},{2,2}}, {{1,1},{2,1},{1,2},{2,2}} }, COLOR_O, {2,2}, 0 }  /* O */
};

const int KEYMAP_SIZE = 10;
const struct keybind keymap[10] = {
 	{ SDLK_n,      on_key_new_game   ,"n",     FALSE },
 	{ SDLK_s,      on_key_scores     ,"s",     FALSE },
 	{ SDLK_h,      on_key_help       ,"h",     FALSE },
 	{ SDLK_q,      on_key_quit       ,"q",     FALSE },
 	{ SDLK_LEFT,   on_key_move_left  ,"Left",  TRUE  },
 	{ SDLK_RIGHT,  on_key_move_right ,"Right", TRUE  },
 	{ SDLK_DOWN,   on_key_move_down  ,"Down",  TRUE  },
 	{ SDLK_UP,     on_key_turn       ,"Up",    FALSE },
 	{ SDLK_SPACE,  on_key_drop       ,"Space", FALSE },
 	{ SDLK_ESCAPE, on_key_cancel     ,"Esc",   FALSE }
};

struct keybind keys_local[5][2] =
{
 	/* end game */
	{
		{ SDLK_n,      on_key_new_game   ,"n",     FALSE },
		{ SDLK_q,      on_key_quit       ,"q",     FALSE },
	},

 	/* quit */
	{
		{ SDLK_q,      on_key_quit       ,"q",     FALSE },
		{ SDLK_ESCAPE, on_key_cancel     ,"Esc",   FALSE },
	},

 	/* high scores */
	{
		{ SDLK_ESCAPE, on_key_cancel     ,"Esc",   FALSE },
		{ SDLK_s,      on_key_scores     ,"s",     FALSE },
	},

 	/* help */
	{
		{ SDLK_ESCAPE, on_key_cancel     ,"Esc",   FALSE },
		{ SDLK_h,      on_key_help       ,"h",     FALSE },
	},

 	/* new game */
	{
		{ SDLK_n,      on_key_new_game   ,"n",     FALSE },
		{ SDLK_ESCAPE, on_key_cancel     ,"Esc",   FALSE }
	}
};

/* lists stuff */
struct list* l_find(double x,double y)
{
  struct list* iter = l_root;

  while(iter->pos.x != x && iter->pos.y != y && iter)
    iter = iter->next;

  return iter;
}

unsigned int l_length(void)
{
	struct list* iter = l_root;
	int i=0;

	for(i=0; iter; i++)
		iter = iter->next;

	return i;
}

struct list* l_get(unsigned int n)
{
    int i =0;
    struct list* iter = l_root;

    while(i++ != n && iter)
      iter = iter->next;

    return iter;
}

/* find and delete item with given x and y */
struct list* l_del(double x,double y)
{
	struct list* it = l_root;
	while(it)
	{
		if(it->next)
		{
			if((it->next->pos.x == x) && (it->next->pos.y == y))
			{
				struct list* to_del = it->next;
				it->next = it->next->next;
				free(to_del);
				break;
			}
		}

		it = it->next;
	}

	if(l_root && (l_root->pos.x == x) && (l_root->pos.y == y))
	{
		struct list* tmp = l_root->next;
		free(l_root);
		l_root = tmp;
		return l_root;
	}

	return it;
}

/* append item */
struct list* l_append(double x,double y,struct rgb new_color)
{
  struct list* iter = l_root;

  if(iter != NULL)
  {
    while(iter->next)
      iter=iter->next;
  }

  struct list* new = (struct list*) malloc(sizeof(struct list));
  struct vec2 new_pos = {x,y};


  new->pos = new_pos;
  new->next = NULL;
  new->color = new_color;

  if(iter !=NULL)
    iter->next = new;
  else
    l_root = new;

  return new;
}

/* return number of items with given x and y,
   if one of parameters is zero (e.g. l_count(1.0,0.0))
   function will only search for elements with non-zero parameter
*/
unsigned int l_count(double x,double y)
{
	struct list* iter = l_root;

	unsigned int counter = 0;

	while(iter)
	{
		if(x && y)
			if((iter->pos.x == x) && (iter->pos.y == y))
				counter++;

		if(x && !y)
			if(iter->pos.x == x)
				counter++;

		if(!x && y)
			if(iter->pos.y == y)
				counter++;

		iter = iter->next;
	}

	return counter;
}

void l_free_all(void)
{
	if(l_root)
	{
		struct list* iter = l_root->next;

		while(l_root)
		{
			free(l_root);
			l_root = iter;

			if(iter)
				iter = iter->next;
		}
	}

	l_root = NULL;
}

/* ============ on_key_* handlers ============ */

void on_key_new_game(bool is_key_pressed)
{
	if(is_key_pressed)
	{
		if(msg_active == MSG_NEWGAME || msg_active == MSG_END_GAME)
		{
			game_new();
			msg_active = MSG_NONE;
		}
		else if(msg_active == MSG_NONE)
			msg_active = MSG_NEWGAME;
	}
}

void on_key_scores(bool is_key_pressed)
{
	if(is_key_pressed)
		msg_active = MSG_SCORES;
	else
		msg_active = MSG_NONE;
}

void on_key_help(bool is_key_pressed)
{
	if(is_key_pressed)
		msg_active = MSG_HELP;
	else
		msg_active = MSG_NONE;
}

void on_key_quit(bool is_key_pressed)
{
	if(msg_active == MSG_NONE)
	{
		if(!is_key_pressed)
			msg_active = MSG_QUIT;
	}
	else
	{
		if(is_key_pressed)
			game_quit();
	}
}

void on_key_move_left(bool is_pressed)
{
	if(msg_active == MSG_NONE && is_pressed == TRUE)
	{
		if(shp_check_intersect(&shape_cur,shape_x-1,shape_y) != 1)
			shape_x--;

		shp_check_drop();
	}
}

void on_key_move_right(bool pressed)
{
	if(msg_active == MSG_NONE && pressed == TRUE)
	{
		if(shp_check_intersect(&shape_cur,shape_x+1,shape_y) != 1)
			shape_x++;

		shp_check_drop();
	}
}

void on_key_move_down(bool pressed)
{
	if(pressed && msg_active == MSG_NONE)
	{
		if(shp_check_intersect(&shape_cur,shape_x,shape_y+1) != 1)
			shape_y++;

		shp_check_drop();
	}
}

void on_key_turn(bool pressed)
{
	if(pressed && msg_active == MSG_NONE)
	{
		struct shape flipped;
		flipped = shape_cur;
		shp_flip(&flipped);

		if(shp_check_intersect(&flipped,shape_x,shape_y) == 0)
			shp_flip(&shape_cur);

		shp_check_drop();
	}
}

void on_key_drop(bool pressed)
{
	if(pressed && msg_active == MSG_NONE)
	{
		while(shp_check_intersect(&shape_cur,shape_x,shape_y+1) != 1)
			shape_y++;

		shp_check_drop();
	}
}

void on_key_cancel(bool pressed)
{
	msg_active = MSG_NONE;
}

char* highscore_get_dir(void)
{
	struct stat fst;

	char* temp = (char*) malloc(sizeof(char)*(strlen(getenv("XDG_CONFIG_HOME"))+strlen("/zenburn_tetris/")+1));
	strcpy(temp,getenv("XDG_CONFIG_HOME"));
	strcat(temp,"/zenburn_tetris/");

	if(stat(temp,&fst) == -1)
		mkdir(temp,S_IRWXU | S_IXGRP | S_IRGRP | S_IXOTH);

	return temp;
}

void game_quit(int ret_code)
{
	font_destroy(&game_font);

	char* hsdir;

	if((hsdir = highscore_get_dir()))
	{
		highscore_save(hsdir);
		free(hsdir);
	}

	SDL_Quit();
    exit(ret_code);
}

void shp_flip(struct shape* f)
{
	int width;
	int height;

	f->state = !f->state;

	if(f->state == 0)
	{
		width = f->size.x;
		height = f->size.y;
	}
	else
	{
		width = f->size.y;
		height = f->size.x;
	}

	int i;
	for(i=0;i<4;i++)
	{
		f->points[f->state][i].x = width - f->points[f->state][i].x + 1;
		f->points[f->state][i].y = height - f->points[f->state][i].y + 1;
	}
}

int check_fill(void)
{
	int j;
	int i;

	for(i=1;i<=20;i++)
	{
		if(l_count(0,i) >= 10)
		{
			for(j=1;j<=10;j++)
				l_del(j,i);

			struct list* iter = l_root;
			while (iter)
			{
				if(iter->pos.y < i)
					iter->pos.y++;

				iter = iter->next;
			}

			game_score++;
		}
	}

	return 1;
}

int shp_check_intersect(struct shape* sh,int shp_x,int shp_y)
{
	struct list* iter = l_root;

	do
	{
		int i;
		for(i=0;i<4;i++)
		{
			if(iter &&
			   iter->pos.x == sh->points[sh->state][i].x+shp_x &&
			   iter->pos.y == sh->points[sh->state][i].y+shp_y)
				return 1;

			if(sh->points[sh->state][i].x + shp_x < 1 ||
			   sh->points[sh->state][i].x + shp_x > 10 ||
			   sh->points[sh->state][i].y + shp_y > 20)
				return 1;
		}

		if(iter)
			iter = iter->next;

	} while(iter);

	return 0;
}

int win_resize(int new_width,int new_height)
{
    GLfloat ratio;

    if (new_height == 0)
	new_height = 1;

    ratio = (GLfloat) new_width / (GLfloat) new_height;

    glViewport(0, 0, (GLsizei) new_width,(GLsizei) new_height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(45.0f, ratio, 0.1f, 100.0f);

    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();

    return TRUE;
}

unsigned int on_timer(unsigned int interval,void* param)
{

	return interval;
}

void shp_check_drop(void)
{
	if(shp_check_intersect(&shape_cur,shape_x,shape_y+1) == 1)
	{
		int s = shape_cur.state;
		int i;
		for(i=0;i<4;i++)
			l_append(shape_cur.points[s][i].x+shape_x,
					 shape_cur.points[s][i].y+shape_y,
					 shape_cur.color);

		if(shape_y < 1)
		{
			game_end();

			int hst_i;
			for(hst_i=9;hst_i>=0;hst_i--)
			{
				if(game_score > high_score[hst_i].score)
				{
					high_score[hst_i].score = game_score;
					memset(high_score[hst_i].name,0,16);
					strncpy(high_score[hst_i].name,getenv("USER"),15);
					break;
				}
			}
		}

		if(game_score % 10 == 0 && game_score > 0 && game_speed > 20)
			game_speed -= 10;

		shape_cur = shapes[shape_next];
		shape_next = rand() % 7;

		shape_y = -1.0;
		shape_x =  5.0 - floor(shape_cur.size.x / 2);
	}
}

void on_key(SDL_keysym *keysym,bool is_keydown)
{
	if(msg_active == MSG_NONE)
	{
		int i;
		for(i=0;i < KEYMAP_SIZE;i++)
		{
			if(keysym->sym == keymap[i].key)
			{
				(*keymap[i].on_key_func)(is_keydown);
			}
		}
	}
	else
	{
		if(keysym->sym == keys_local[msg_active][0].key)
			(*keys_local[msg_active][0].on_key_func)(is_keydown);
		else if(keysym->sym == keys_local[msg_active][1].key)
			(*keys_local[msg_active][1].on_key_func)(is_keydown);
	}
}

int highscore_load(char* dir)
{
	FILE* hsfp;

	char* full_path = (char*)malloc(sizeof(char)*(strlen(dir)+strlen("scores")+1));
	strcpy(full_path,dir);
	strcat(full_path,"scores");

	if((hsfp = fopen(full_path,"r")))
	{
		char in_buf[64];
		int i=0;

		while(fgets(in_buf,63,hsfp))
		{
			if(!sscanf(in_buf,"%s %i",high_score[i].name,&high_score[i].score))
				break;

			i++;
		}

		fclose(hsfp);
	}
	else
	{
		free(full_path);
		return -1;
	}

	free(full_path);;

	return 0;
}

int highscore_save(char* dir)
{
	FILE* hsfp;

	char* full_path = (char*)malloc(sizeof(char)*(strlen(dir)+strlen("scores")+1));
	strcpy(full_path,dir);
	strcat(full_path,"scores");

	if(!(hsfp = fopen(full_path,"w")))
	{
		printf("%s",full_path);
		free(full_path);
		return -1;
	}

	int i;
	for(i=0;i<10;i++)
		fprintf(hsfp,"%s %i\n",high_score[i].name,high_score[i].score);

	free(full_path);
}

int font_load(char* file,struct font* ft_out)
{
	FILE* fp;

	unsigned short int bfType;
    long int bfOffBits;
    short int biPlanes;
    short int biBitCount;
    long int biSizeImage;

	struct texture
	{
		int height;
		int width;
		unsigned char* data;
	}font_texture;

	if ((fp = fopen(file, "rb")) == NULL)
        return -1;

    if(!fread(&bfType, sizeof(short int), 1, fp))
        return -1;

    if (bfType != 19778)
        return -1;

	fseek(fp, 8, SEEK_CUR);

	fread(&bfOffBits, sizeof(long int), 1, fp);

	fseek(fp,4,SEEK_CUR);

	fread(&font_texture.width, sizeof(int), 1, fp);
    fread(&font_texture.height, sizeof(int), 1, fp);

	fread(&biPlanes, sizeof(short int), 1, fp);
    if (biPlanes != 1)
        return -1;

    fread(&biBitCount, sizeof(short int), 1, fp);

    if (biBitCount != 24)
		return -1;

    biSizeImage = font_texture.width * font_texture.height * 3;
    font_texture.data = (unsigned char*) malloc(biSizeImage);

	fseek(fp, bfOffBits, SEEK_SET);

	fread(font_texture.data, biSizeImage, 1, fp);

	/* bgr -> rgb */
	unsigned char temp;
	int i;
	for (i = 0; i < biSizeImage; i += 3)
    {
        temp = font_texture.data[i];
        font_texture.data[i] = font_texture.data[i + 2];
        font_texture.data[i + 2] = temp;
    }

	glGenTextures(1,&ft_out->texture);
	glBindTexture(GL_TEXTURE_2D,ft_out->texture);

	glTexImage2D(GL_TEXTURE_2D, 0, 3, font_texture.width,
				 font_texture.height, 0, GL_RGB, GL_UNSIGNED_BYTE,
				 font_texture.data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	ft_out->list_base = glGenLists(256);

	float cx,cy;
	int loop;
	for (loop = 0; loop < 256; loop++)
    {
        cx = (float) (loop % 16) / 16.0f;
        cy = (float) (loop / 16) / 16.0f;

		glNewList(ft_out->list_base + loop, GL_COMPILE);
		 glBegin(GL_QUADS);
		  glTexCoord2f(cx, 1 - cy - 0.0625f);
		  glVertex3f(0.0f, 0.0f, 0.0f);

		  glTexCoord2f(cx + 0.0625f, 1 - cy - 0.0625f);
		  glVertex3f(0.5,0.0,0.0);

		  glTexCoord2f(cx + 0.0625f, 1 - cy);
		  glVertex3f(0.5f, 0.5f, 0.0f);

		  glTexCoord2f(cx, 1 - cy);
		  glVertex3f(0.0f, 0.5f, 0.0f);
		 glEnd();

		 glTranslatef(0.4f, 0.0f, 0.0f);
        glEndList();
    }

	free(font_texture.data);
	fclose(fp);

    return 0;
}

void font_destroy(struct font* ft)
{
	glDeleteLists(ft->list_base,256);
	glDeleteTextures(1,&ft->texture);
}


void font_render(struct font* ft,GLfloat x, GLfloat y, char *string, int set)
{
	glBindTexture(GL_TEXTURE_2D, ft->texture);
	glPushMatrix();
	glTranslatef(x,y,0.0f);
	glListBase(ft->list_base +(128*set));
	glCallLists(strlen(string),GL_BYTE,string);
	glPopMatrix();
}

int init_gfx(unsigned int width,unsigned int height,bool fullscreen)
{
    const SDL_VideoInfo *videoInfo;

    if(SDL_Init(SDL_INIT_VIDEO) < 0)
		return -1;

    videoInfo = SDL_GetVideoInfo();

    if (!videoInfo)
		return -1;

    videoFlags  = SDL_OPENGL;          /* Enable OpenGL in SDL */
    videoFlags |= SDL_GL_DOUBLEBUFFER; /* Enable double buffering */
    videoFlags |= SDL_HWPALETTE;       /* Store the palette in hardware */
    videoFlags |= SDL_RESIZABLE;       /* Enable window resizing */

    /* This checks to see if surfaces can be stored in memory */
    if (videoInfo->hw_available)
		videoFlags |= SDL_HWSURFACE;
    else
		videoFlags |= SDL_SWSURFACE;

    /* This checks if hardware blits can be done */
    if (videoInfo->blit_hw)
		videoFlags |= SDL_HWACCEL;

    /* Sets up OpenGL double buffering */
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	if(fullscreen)
		videoFlags |= SDL_FULLSCREEN;

    /* get a SDL surface */
    surface = SDL_SetVideoMode(width, height, 24, videoFlags);

    /* Verify there is a surface */
    if (!surface)
		return -1;

	/* setup OpenGL */
	glShadeModel(GL_SMOOTH);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glEnable(GL_TEXTURE_2D);

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	/* set window size */
    win_resize(width, height);

	glFlush();

    return 0;
}

void draw_message(struct vec2 rect[2],struct rgb color,struct rgb text_color,char* text)
{
	char* local_s = (char*) malloc(sizeof(char)*strlen(text)+1);
	strcpy(local_s,text);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	glColor3f(color.r,color.g,color.b);

	glBegin(GL_QUADS);
	glVertex3f( rect[0].x, rect[0].y, 0.0);
	glVertex3f( rect[0].x, rect[1].y, 0.0);
	glVertex3f( rect[1].x, rect[1].y, 0.0);
	glVertex3f( rect[1].x, rect[0].y, 0.0);
	glEnd();

	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);

	glColor3f(text_color.r,text_color.g,text_color.b);

	int lines_num=1;
	char* it = local_s;
	while(*it)
		if (*it++ == '\n') lines_num++;

	char* cur_str = strtok(local_s,"\n");

	int i = 1;
	while(cur_str)
	{
		font_render(&game_font,
					rect[0].x+(rect[1].x - rect[0].x)/2 - strlen(cur_str)*0.2,
					rect[0].y+(rect[1].y - rect[0].y)/2 + lines_num*0.3 - i*0.6,
					cur_str,0);

		cur_str = strtok(NULL,"\n");
		i++;
	}

	free(local_s);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}

void draw_block(double x,double y,struct rgb color)
{
	if(y<=0) return;

	struct vec2 glob_coord = { x-5.0f, (20.0f-y)+1.0 - 10.0f };

	glColor3f(color.r,color.g,color.b);
    glBegin(GL_QUADS);
      glVertex3f(glob_coord.x-1.0f, glob_coord.y,      0.0f); /* Top Left */
      glVertex3f(glob_coord.x,      glob_coord.y,      0.0f); /* Top Right */
      glVertex3f(glob_coord.x,      glob_coord.y-1.0f, 0.0f); /* Bottom Right */
      glVertex3f(glob_coord.x-1.0f, glob_coord.y-1.0f, 0.0f); /* Bottom Left */
    glEnd();
}

void draw_shape(double fig_x,double fig_y,struct shape* f)
{
	int i;
	for(i=0;i<4;i++)
		draw_block(f->points[f->state][i].x+fig_x,
				   f->points[f->state][i].y+fig_y,
				   f->color);
}

void render_ui(void)
{
/* render scores and level */
	/* FIXME: buffer overrun may be here */
	char scores_buf[64];
	snprintf(scores_buf,60,"Level: %i\nLines: %i",(1000 - game_speed) / 10 +1,game_score);

	struct vec2 scores_rect[2] = {{5.5,0.0},{10.0,2.0}};
	draw_message(scores_rect,(struct rgb){COLOR_BG},(struct rgb){COLOR_TEXT},scores_buf);

/* render messages */
	switch(msg_active)
	{
	case MSG_END_GAME:
	{
		char* end_text_buf = (char*) malloc(sizeof(char)*strlen(msg_text_endgame) + 17);
		snprintf(end_text_buf,strlen(msg_text_endgame)+16,msg_text_endgame,game_score);
		struct vec2 end_msg_rect[2] = {{-5.0,-3.0},{5.0,3.0}};
		draw_message(end_msg_rect,
					 (struct rgb){0.2,0.0,0.0},
					 (struct rgb){COLOR_TEXT},
					 end_text_buf);
		free(end_text_buf);
	}
	break;
	case MSG_QUIT:
	{
		struct vec2 quit_msg_rect[2] = {{-7.0,-4.0},{7.0,4.0}};
		draw_message(quit_msg_rect,
					 (struct rgb){COLOR_BG},
					 (struct rgb){COLOR_TEXT},
					 msg_text_quit);
	}
	break;
	case MSG_HELP:
	{
		struct vec2 help_msg_rect[2] = {{-7.0,-4.0},{7.0,4.0}};
		draw_message(help_msg_rect,
					 (struct rgb){COLOR_BG},
					 (struct rgb){COLOR_TEXT},
					 msg_text_help);
	}
	break;
	case MSG_SCORES:
	{
		/* FIXME: buffer overrun*/
		char hscore_text[256];
		memset(hscore_text,0,256*sizeof(char));

		int i;
		for(i=9;i>=0;i--)
		{
			strcat(hscore_text,high_score[i].name);

			char scrval_tmp[32];
			snprintf(scrval_tmp,30,"%i\n",high_score[i].score);

			char spaces[16];
			int spaces_count = 20 - strlen(high_score[i].name) - strlen(scrval_tmp);
			if(spaces_count)
			{
				memset(spaces,0,16*sizeof(char));
				for(;spaces_count>=0;spaces_count--)
				{
					spaces[spaces_count] = '.';
				}
			}

			strcat(hscore_text,spaces);
			strcat(hscore_text,scrval_tmp);
		}

		struct vec2 highscore_rect[2] = {{-5.0,-4.0},{5.0,4.0}};
		draw_message(highscore_rect,(struct rgb){0.3,0.3,0.3},(struct rgb){COLOR_TEXT},hscore_text);
	}
	break;
	case MSG_NEWGAME:
	{
		struct vec2 newgame_msg_rect[2] = {{-7.0,-4.0},{7.0,4.0}};
		draw_message(newgame_msg_rect,
					 (struct rgb){COLOR_BG},
					 (struct rgb){COLOR_TEXT},
					 msg_text_newgame);
	}
	break;
	default:
		break;
	}
}


int render(GLvoid)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -25.0f);

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);

	/* draw background */
	glColor3f(COLOR_BG);
    glBegin(GL_QUADS);
      glVertex3f(-5.0f,  10.0f, 0.0f);
      glVertex3f( 5.0f,  10.0f, 0.0f);
      glVertex3f( 5.0f, -10.0f, 0.0f);
      glVertex3f(-5.0f, -10.0f, 0.0f);
    glEnd();

	/* draw blocks */
	struct list* iter = l_root;;
	while(iter) {
		draw_block(iter->pos.x,iter->pos.y,iter->color);
		iter = iter->next;
	}

	/* draw shape */
	draw_shape(shape_x,shape_y,&shape_cur);

	/* draw grid */
	glColor4f(0.0f,0.0f,0.0f,1.0f);

	double i,j;
	for(i=-5.0;i<5.0;i++)
	{
		glBegin(GL_LINES);
			glVertex3f(i,10.0,0.0f);
			glVertex3f(i,-10.0,0.0f);
		glEnd();
	}
	for(j=-10.0;j<10.0;j++)
	{
		glBegin(GL_LINES);
			glVertex3f(-5.0,j,0.0f);
			glVertex3f(5.0,j,0.0f);
		glEnd();
	}

	/* draw next shape preview */
	glColor3f(COLOR_BG);
    glBegin(GL_QUADS);
	 glVertex3f(6.0f,  3.0f, 0.0f);
	 glVertex3f(6.0f,  7.0f, 0.0f);
	 glVertex3f(10.0f, 7.0f, 0.0f);
	 glVertex3f(10.0f, 3.0f, 0.0f);
    glEnd();

	draw_shape(11.0,3.0,&shapes[shape_next]);

	glColor3f(0.0,0.0,0.0);
	for(i=6.0;i<10.0;i++)
	{
		glBegin(GL_LINES);
		glVertex3f(i,3.0,0.0f);
		glVertex3f(i,7.0,0.0f);
		glEnd();
	}
	for(j=3.0;j<7.0;j++)
	{
		glBegin(GL_LINES);
		glVertex3f(6.0f,j,0.0f);
		glVertex3f(10.0f,j,0.0f);
		glEnd();
	}

	/* render UI elements */
	render_ui();

    /* Draw it to the screen */
    SDL_GL_SwapBuffers();

    return TRUE;
}

void on_event(SDL_Event event)
{
	switch(event.type)
	{
	case SDL_ACTIVEEVENT:
		if (event.active.gain == 0)
			win_active = FALSE;
		else
			win_active = TRUE;
		break;
	case SDL_VIDEORESIZE:
		surface = SDL_SetVideoMode( event.resize.w,event.resize.h,16, videoFlags );

		if (!surface)
		{
			fprintf(stderr,"Could not get a surface after resize: %s\n", SDL_GetError());
			game_quit(1);
		}
		win_resize(event.resize.w,event.resize.h);
		break;
	case SDL_KEYDOWN:
	case SDL_KEYUP:
		on_key(&event.key.keysym,(event.type == SDL_KEYDOWN) ? TRUE : FALSE);
		break;
	case SDL_QUIT:
		game_quit(0);
		break;
	default:
		break;
	}
}

void game_new(void)
{
	l_free_all();

	shape_cur = shapes[rand() % 7];
	shape_next = rand() % 7;

	shape_x = 5.0 - floor(shape_cur.size.x/2);
	shape_y = -1.0;

	game_speed = 1000;
	game_score = 0;

	msg_active = MSG_NONE;
}

void game_end(void)
{
	msg_active = MSG_END_GAME;

}

bool init(int argc, char **argv)
{
	win_active = TRUE;

	game_config.fullscreen = FALSE;
	game_config.width = 640;
	game_config.height = 480;

	/* parse cmdline */
	for(int i=1;i<argc;i++)
	{
		/* Level */
		if((strcmp(argv[i],"-l") == 0) || (strcmp(argv[i],"--level") == 0))
		{
			if(i+1 == argc)
			{
				printf("%s\n","FAIL: level value is not set");
				return FALSE;
			}

			char* endptr;
			unsigned int level = strtoul(argv[i+1],&endptr,10);
			if(!level)
			{
				printf("%s\n","FAIL: wrong level value");
				return FALSE;
			}
			else
			{
				game_speed = 1010 - level*10;
				i++;
			}
		}
		/* Window size */
		else if((strcmp(argv[i],"-s") == 0) || (strcmp(argv[i],"--size") == 0))
		{
			if(i+1 == argc)
			{
				printf("%s\n","FAIL: window size is not set");
				return FALSE;
			}

			if(sscanf(argv[i+1],"%ux%u",&game_config.width,&game_config.height) != 2)
			{
				printf("%s\n","FAIL: wrong window size");
				return FALSE;
			}

			i++;
		}
		/* Full screen */
		else if((strcmp(argv[i],"-f") == 0) || (strcmp(argv[i],"--fullscreen") == 0))
		{
			game_config.fullscreen = TRUE;
		}
		/* Usage */
		else if((strcmp(argv[i],"-h") == 0) || (strcmp(argv[i],"--help") == 0))
		{
			printf("%s",msg_text_usage);
			game_quit(0);
		}
		/* Wrong argument */
		else
		{
				printf("%s\n","FAIL: wrong parameter");
				printf("%s",msg_text_usage);
				game_quit(1);
		}
	}

	if(init_gfx(game_config.width,game_config.height,game_config.fullscreen) < 0)
	{
		printf("%s\n","FAIL: Graphics init failed");
		return FALSE;
	}
	if(font_load(DATA_DIR"/font.bmp",&game_font) < 0)
	{
		printf("%s %s\n","FAIL: Can not load font:",DATA_DIR"/font.bmp");
		return FALSE;
	}

	SDL_WM_SetCaption("Zenburn Tetris",NULL);

	char* hsdir = NULL;
	if((hsdir = highscore_get_dir()))
	{
		highscore_load(hsdir);
		free(hsdir);
	}

	return TRUE;
}

int main(int argc, char **argv)
{
	unsigned int ticks = 0;

	srand(time(NULL));

	game_new();

	if(!init(argc,argv))
		game_quit(1);

    SDL_Event event;
    while(TRUE)
	{
	    while(SDL_PollEvent(&event))
			on_event(event);

		if((SDL_GetTicks() - ticks) >= game_speed)
		{
			ticks = SDL_GetTicks();

			if(win_active && msg_active == MSG_NONE)
			{
				shape_y++;
				check_fill();

				shp_check_drop();
			}
		}

	    if(win_active)
			render();
	}

    game_quit(0);
    return(0);
}
