#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <string>

namespace mylib
{
using namespace std;

struct chess_pos
{
	chess_pos(unsigned short num, unsigned size)
	{
		r = '1' + (size - (num / size)) - 1;
		c = 'a' + (num % size);
	}
	friend ostream& operator<<(ostream& out,chess_pos p) { out << p.c << p.r; return out;}
	
	char r;
	char c;
};
typedef unsigned uint;
class Matrix : public vector<vector<bool> >
{
public:
	Matrix(bool (&func)(int,int,int,int),unsigned board_size) :
		vector<vector<bool> >(board_size*board_size,vector<bool>(board_size*board_size))
	{	
		for(uint piece_x = 0; piece_x < board_size; piece_x++) for(uint piece_y = 0; piece_y < board_size; piece_y++)
		for(uint y = 0; y < board_size; y++) for(uint x = 0; x < board_size; x++)
			this->at(piece_y*board_size+piece_x).at(y*board_size+x) = func(x,y,piece_x,piece_y);
		
		this->board_size = board_size;
	}
	friend ostream& operator<<(ostream &stream, const Matrix &m)
	{
		stream << "   ";
		for(uint i=0; i < m.size();i++)
			stream << chess_pos(i,m.board_size).c << chess_pos(i,m.board_size).r << ' ';
		stream << endl;
		
		for(int row = 0; row < m.size(); row++)
		{
			stream << chess_pos(row,m.board_size).c << chess_pos(row,m.board_size).r << ' ';
			for(uint col = 0; col < m[row].size(); col++)
				stream << m[row][col] << "  ";	
			stream << endl;
		}
		return stream;
	}
private:
	unsigned board_size;
};

class BoolExpr
{
	public:
		struct xory
		{
			xory(unsigned _x,unsigned _y) : x(_x),y(_y) {}
			unsigned short x;
			unsigned short y;
		};
		
		BoolExpr(Matrix& m,unsigned board_size) : is_dnf(false)
		{
			expr = new vector<xory>;
			dnf = NULL;
			
			for(int r = 0; r < m.size(); r++)
				for(int c = 0; c < m.size(); c++)
					if((m[r][c] == true) && (r < c))
						expr->push_back(xory(r,c));
			
			this->board_size = board_size;
		}
		BoolExpr(vector<vector<short> >* _dnf,unsigned board_size) : is_dnf(true)
		{
			expr = NULL;
			dnf = _dnf;
			this->board_size = board_size;
		}
		BoolExpr(vector<xory>* _cnf,unsigned board_size) : is_dnf(false)
		{
			expr = _cnf;
			dnf = NULL;
			this->board_size = board_size;
		}
		
		~BoolExpr() { if(is_dnf) delete dnf; delete expr; }
		
		void toDNF(void (*process)(unsigned,unsigned)=NULL,stringstream* tmpr=NULL)
		{
			if(is_dnf)
				return;
				
			dnf = new vector<vector<short> >();

			for(int i = 0; i < expr->size(); i++)
			{
				Multiply(expr->at(i),*dnf);
				
				Simplify(dnf);
				if(tmpr && i!=0)
				{
					PrintExpr(expr,*tmpr,0,i+1);
					if(i+1 == expr->size())
						*tmpr << "(";
					else
						*tmpr << "˄ (";
					PrintDNF(dnf,*tmpr,0,0);
					*tmpr << ")";
					if(i != expr->size() -1 )
						*tmpr << "  = " << endl ;
					else
						*tmpr << endl;
				}
				if(process)
					(*process)(i,expr->size());
			}
			Simplify(dnf);

			//delete expr;
			is_dnf = true;
		}
		
		BoolExpr* getComplement()
		{
			if(!is_dnf)
				return NULL;
				
			vector<vector<short> > *cmpl = new vector<vector<short> >;
			
			for(vector<vector<short> >::iterator it = dnf->begin(); it < dnf->end(); it++)
			{
				vector<short> inv;
				for(int i = 0; i < board_size*board_size; i++)
				{
					bool found = false;
					for(int j = 0; j < it->size(); j++)
					{
						if(it->at(j) == i)
						{
							found = true;
							break;
						}
					}
					if(!found)
						inv.push_back(i);
				}
				cmpl->push_back(inv);
			}
			
			return new BoolExpr(cmpl,this->board_size);
		}
		
		vector<vector<short> >* getMaxConjs()
		{
			if(!is_dnf) return NULL;
			vector<vector<short> > *c = new vector<vector<short> >;
			int n = getMaxConjSize();
			for(int i = 0; i < dnf->size(); i++)
				if(dnf->at(i).size() == n)
					c->push_back(dnf->at(i));
					
			return c;
		}
		
		int getMaxConjSize()
		{
			if(!is_dnf)
				return -1;
			unsigned max = dnf->at(0).size();
			for(uint i =1; i < dnf->size(); i++)
			{
				if(dnf->at(i).size() > max)
					max = dnf->at(i).size();
			}
			return max;
		}
		
		bool isDNF() { return this->is_dnf; }
		
		friend ostream& operator<<(ostream &stream, BoolExpr &m)
		{
			if(m.isDNF())
				m.PrintDNF(m.dnf,stream);
			else
				m.PrintExpr(m.expr,stream);
			return stream;
		}
		unsigned size() { return board_size; }
	private:
		vector<xory>* expr;
		vector<vector<short> >* dnf;
		bool is_dnf;
		unsigned board_size;
		
		bool Contains(const vector<short> &subset,const vector<short> &set)
		{
			if(!subset.size())
				return false;
			for(uint i = 0; i < subset.size(); i++)
			{
				bool has = false;
				for(uint j = 0; j < set.size(); j++)
				{
					if(set[j] == subset[i])
					{
						has = true;
						break;
					}
				}
				if(!has)
					return false;
			}

			return true;
		}
		
		void Simplify(vector<vector<short> >* what)
		{
			for(vector<vector<short> >::iterator it=what->begin(); it < what->end(); it++)
				for(vector<vector<short> >::iterator it2=what->begin(); it2 < what->end(); it2++)
					if((it!=it2) && Contains(*it,*it2))
						(*it2).clear();


			for(uint i = 0; i < what->size(); i++)
				if(what->at(i).size() == 0)
				{
					what->erase(what->begin()+i);
					i--;
				}
		}
		
		void Multiply(xory what,vector<vector<short> > &v)
		{
			if(!v.size())
			{
				vector<short> a1(1,what.x);
				vector<short> b1(1,what.y);
				v.push_back(a1);
				v.push_back(b1);
				return;
			}
			
			vector<vector<short> > mul_b;
			
			for(vector<vector<short> >::iterator it = v.begin(); it < v.end(); it++)
			{
				bool found_x = Contains(vector<short>(1,what.x),*it);
				bool found_y = Contains(vector<short>(1,what.y),*it);
					
				vector<short> mb(*it);
				if(!found_y)
					mb.push_back(what.y);
				
				mul_b.push_back(mb);
				
				if(!found_x)
					it->push_back(what.x);
			}
			for(vector<vector<short> >::iterator it = mul_b.begin(); it < mul_b.end(); it++)
				v.push_back(*it);
		}
		
		void PrintDNF(vector<vector<short> >* __dnf,ostream& out,unsigned wrap=3,int begin=0)
		{
			const string or_str = "˅";
			const string and_str = "˄";
			
			for(vector<vector<short> >::iterator it = __dnf->begin()+begin; it < __dnf->end(); it++)
			{	
				out << "(";
				for(uint j = 0; j < it->size(); j++)
					out << chess_pos(it->at(j),this->board_size) << ((j == it->size()-1) ? "" : (" " + and_str + " "));
					
				out << ")";
				out << ((it == __dnf->end()-1) ? "" : (" " + or_str + " "));//endl;
				if(wrap != 0)
					out << endl;
			}
		}
		
		void PrintExpr(vector<xory>* __cnf,ostream& out,unsigned wrap=3,int begin=0)
		{
			const string or_str = "˅";
			const string and_str = "˄";
			
			int items = wrap;
			for(uint i =begin; i < __cnf->size(); i++)
			{
				chess_pos xpos(__cnf->at(i).x,this->board_size);
				chess_pos ypos(__cnf->at(i).y,this->board_size);

				out << "(" << xpos << " " + or_str + " " << ypos << 
					") " + ((i == __cnf->size()-1) ? "" : (" " + and_str + " "));
				
				if(items == 0) {
					items = wrap;
					if(wrap != 0)
					out << endl;
				}
				else
					items--;
			}
			if(wrap)
			out << endl;
		}
};

typedef vector<vector<short> > _bexpr;
class BoolExprExt
{
public:
	BoolExprExt(Matrix &m,unsigned size) : board_size(size)
	{
		expr = new _bexpr;
		for(int i = 0; i < size*size; i++)
		{
			vector<short> tmp;
			for(int j=0; j < size*size; j++)
			{
				if(m[i][j])// && (i<=j))
					tmp.push_back((short)j);
			}
			expr->push_back(tmp);
		}
		is_dnf = false;
	}
	BoolExprExt(vector<vector<short> >* e,unsigned size)
	{
		is_dnf = true;
		expr = e;
		board_size = size;
	}
	~BoolExprExt() { delete expr; }
	friend ostream& operator<<(ostream& out,BoolExprExt &ex)
	{
		Print(out,ex.expr,ex.is_dnf,ex.board_size,0,true);
		return out;
	}
	void toDNF(ostream *tmp_r=NULL,void (*on_step)(unsigned i,unsigned num)=NULL)
	{
		_bexpr *new_expr = new _bexpr;
	
		for(int i =0; i < expr->size(); i++)
		{
			Multiply(expr->at(i),new_expr);
			Simplify(new_expr);

			if(on_step)
				(*on_step)(i,expr->size());
			if(tmp_r)
			{
				if((i != expr->size() -1))
				{
					Print(*tmp_r,expr,false,board_size,i,false);
					*tmp_r << " ˄ ";
				}
				if(i!=0)
				{
					*tmp_r << "(";
					Print(*tmp_r,new_expr,true,board_size,0,false);
					*tmp_r << ") = " << endl;
				}
			}
		}
		delete expr;
		expr = new_expr;
		is_dnf = true;
	}
	unsigned getMinNumber()
	{
		unsigned min = expr->at(0).size();
		for(uint i =1; i < expr->size(); i++)
		{
			if(expr->at(i).size() < min)
				min = expr->at(i).size();
		}
		return min;
	}
	_bexpr* getMinSets()
	{
		if(!is_dnf) return NULL;
		vector<vector<short> > *c = new vector<vector<short> >;
		int n = getMinNumber();
		for(int i = 0; i < expr->size(); i++)
			if(expr->at(i).size() == n)
				c->push_back(expr->at(i));

		return c;
	}

private:
	vector<vector<short> >* expr;
	bool is_dnf;
	unsigned board_size;

	static void Multiply(vector<short> &a,_bexpr* b)
	{
		if(b->size() == 0)
		{
			for(int i = 0; i < a.size(); i++)
				b->push_back(vector<short>(1,a[i]));
			return;
		}

		vector<_bexpr> *new_b = new vector<_bexpr>;
		for(int i = 0; i < a.size(); i++)
		{
			new_b->push_back(*b);
			for(int j = 0; j < new_b->at(i).size(); j++)
			{
				if(!BoolExprExt::Contains(vector<short>(1,a[i]),new_b->at(i).at(j)))
				{
					new_b->at(i).at(j).push_back(a[i]);
				}
			}
		}

		b->clear();
		for(int i =0 ; i < new_b->size(); i++)
		{
			for(int j = 0; j < new_b->at(i).size(); j++)
			{
				b->push_back(new_b->at(i).at(j));
			}
		}
		return;
	}

	static bool Contains(const vector<short> &subset,const vector<short> &set)
	{
		if(!subset.size())
			return false;
		for(uint i = 0; i < subset.size(); i++)
		{
			bool has = false;
			for(uint j = 0; j < set.size(); j++)
			{
				if(set[j] == subset[i])
				{
					has = true;
					break;
				}
			}
			if(!has)
				return false;
		}

		return true;
	}

	void Simplify(_bexpr* what)
	{
		for(vector<vector<short> >::iterator it=what->begin(); it < what->end(); it++)
			for(vector<vector<short> >::iterator it2=what->begin(); it2 < what->end(); it2++)
				if((it!=it2) && Contains(*it,*it2))
					(*it2).clear();


		for(uint i = 0; i < what->size(); i++)
			if(what->at(i).size() == 0)
			{
				what->erase(what->begin()+i);
				i--;
			}
	}

	static void Print(ostream &out,_bexpr *ex,bool dnf,unsigned size,unsigned begin,bool wrap)
	{
		const string or_str = " ˅ ";
		const string and_str = " ˄ ";
		const string* sep1;const string* sep2;
		if(!dnf) {
			sep1 = &and_str;
			sep2 = &or_str;
		}
		else {
			sep1 = &or_str;
			sep2 = &and_str;
		}
		for(int i =begin; i < ex->size(); i++)
		{
			out << "(";
			for(int j = 0; j < (*ex)[i].size(); j++)
			{
				out << chess_pos(ex->at(i).at(j),size);
				if(j != (*ex)[i].size()-1)
				{
					out << *sep2; 
				}
			}
			out << ")";
			if(i != ex->size()-1)
			{
				out << *sep1;	
			}


			if(!dnf && wrap)
				out << endl;
		}
	}
};

bool queen(int x,int y,int x0,int y0)
{
	return ((x == x0) ||
		   (y == y0) ||
		   (abs(float(x0-x)/float(y0-y)) == 1.0f)) &&
		   !((x==x0) && (y==y0));
}

bool knight(int x,int y,int x0,int y0)
{
	return (((abs(x-x0) == 2) && (abs(y-y0) == 1)) ||
		   ((abs(x-x0) == 1) && (abs(y-y0) == 2))) &&
		   !((x==x0) && (y==y0));
}

bool rook(int x,int y,int x0,int y0)
{
	return ((x == x0) || (y == y0)) && 
		   !((x==x0) && (y==y0));
}

bool bishop(int x,int y,int x0,int y0)
{
	return (abs(float(x0-x)/float(y0-y)) == 1.0f) &&
		   !((x==x0) && (y==y0));
}

bool king(int x,int y,int x0,int y0)
{
	return (abs(x0-x)<=1 && abs(y0-y)<=1) && !((x==x0) && (y==y0));
}
	
const int pieces_n = 5;
struct pfunc { const char *name; bool (*func)(int,int,int,int); };
pfunc pieces[pieces_n] = {
	{"Ферзь",&queen},
	{"Конь",&knight},
	{"Ладья",&rook},
	{"Слон",&bishop},
	{"Король",&king}
};

}