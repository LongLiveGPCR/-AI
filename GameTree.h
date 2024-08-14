#pragma once
#include<iostream>
#include<vector>
#include<array>
#include<algorithm>
class GameTree
{
public:
	class Node
	{
	public:
		enum State :uint8_t/*节省内存*/ { SPACE, BLACK, WHITE };
	private:
		friend class GameTree;
		static constexpr uint8_t boardSize = 15;
		static constexpr int maxEvaluteValue = INT_MAX;
		int value = 0;//叶节点表示估价函数的结果，MAX节点表示α值，MIN节点表示β值
		int evalutedValue = INT_MIN;//INT_MIN此处表示未计算过，而不是失败，失败为-INT_MAX
		unsigned short depth;//深度
		uint8_t lastX, lastY;//上一次落子的xy坐标
		Node* father;//父节点
		std::vector<Node*> children;//子节点
		//Node* firstChild = nullptr;
		//Node* nextBrother = nullptr;
		State **board;//棋盘
		int Evaluate()const//估价函数
		{
			static const auto EvaluateList = [](const std::array<State, 5>& v)//假定自己是白方
			{
				//判断颜色并记录棋子个数
				State lastColor = SPACE;
				uint8_t bitList = 0;//将棋链以二进制形式表示，如01101
				for (State i : v)
				{
					bitList <<= 1;
					if (i != SPACE)
					{
						if (i != lastColor)
						{
							if (lastColor == SPACE)//遇到的第一个棋子
								lastColor = i;
							else//有不同颜色的棋子
								return 0;
						}
						bitList |= 1;
					}
				}
				static constexpr int results[]
					= { 0,5,5,80,5,60,100,500,5,20,80,500,100,500,8000,100000,5,10,//0-17
					20,600,50,600,500,8000,80,600,500,6000,500,8000,100000,maxEvaluteValue };//18-31
				/*
				* 十进制-二进制-得分
				* 0-00000-0
				* 1-00001-5
				* 2-00010-5
				* 3-00011-80
				* 4-00100-5
				* 5-00101-60
				* 6-00110-100
				* 7-00111-500
				* 8-01000-5
				* 9-01001-20
				* 10-01010-80
				* 11-01011-500
				* 12-01100-100
				* 13-01101-500
				* 14-01110-8000
				* 15-01111-100000
				* 16-10000-5
				* 17-10001-10
				* 18-10010-20
				* 19-10011-600
				* 20-10100-50
				* 21-10101-600
				* 22-10110-500
				* 23-10111-5000
				* 24-11000-80
				* 25-11001-600
				* 26-11010-500
				* 27-11011-6000
				* 28-11100-500
				* 29-11101-5000
				* 30-11110-100000
				* 31-11111-MAX
				*/
				//对手返回负值，我方返回正值
				//乘以1.1是为了偏重防守
				if (bitList == 31)
				{
					return lastColor == WHITE ? maxEvaluteValue : -maxEvaluteValue;
				}
				return lastColor == WHITE ? results[bitList] : -results[bitList] * 11 / 10;
			};
			static const auto EvaluteSome = [](State** board, uint8_t lastX, uint8_t lastY)
			{
				int result = 0;
				for (uint8_t i = 0; i < 5; ++i)
				{
					//横向
					if (lastX + i < boardSize && lastX + i >= 4)
					{
						std::array<State, 5>v;
						for (uint8_t k = 0; k < 5; k++)
							v[k] = board[lastX + i - k][lastY];
						int t = EvaluateList(v);
						if (t == maxEvaluteValue || t == -maxEvaluteValue)//决出胜负直接返回
							return t;
						result += t;
					}
					//纵向
					if (lastY + i < boardSize && lastY + i >= 4)
					{
						std::array<State, 5>v;
						for (uint8_t k = 0; k < 5; k++)
							v[k] = board[lastX][lastY + i - k];
						int t = EvaluateList(v);
						if (t == maxEvaluteValue || t == -maxEvaluteValue)//决出胜负直接返回
							return t;
						result += t;
					}
					//左上-右下
					if (lastX + i < boardSize && lastX + i >= 4 && lastY + i < boardSize && lastY + i >= 4)
					{
						std::array<State, 5>v;
						for (uint8_t k = 0; k < 5; k++)
							v[k] = board[lastX + i - k][lastY + i - k];
						int t = EvaluateList(v);
						if (t == maxEvaluteValue || t == -maxEvaluteValue)//决出胜负直接返回
							return t;
						result += t;
					}
					//左下-右上
					if (lastX + i < boardSize && lastX + i >= 4 && lastY - i + 4 < boardSize && lastY - i >= 0)
					{
						std::array<State, 5>v;
						for (uint8_t k = 0; k < 5; k++)
							v[k] = board[lastX + i - k][lastY - i + k];
						int t = EvaluateList(v);
						if (t == maxEvaluteValue || t == -maxEvaluteValue)//决出胜负直接返回
							return t;
						result += t;
					}
				}
				return result;
			};
			int t = EvaluteSome(board, lastX, lastY); 
			if (t == maxEvaluteValue || t == -maxEvaluteValue)//决出胜负直接返回
				return t;
			return t + father->GetEvaluateValue() - EvaluteSome(father->board, lastX, lastY);
		}
		void CreateBoard()
		{
			if (board == nullptr)
			{
				board = new State * [boardSize];
				for (uint8_t i = 0; i < boardSize; ++i)
				{
					board[i] = new State[boardSize];
					memcpy_s(board[i], boardSize * sizeof(State), father->board[i], boardSize * sizeof(State));
				}
				board[lastX][lastY] = IsMaxNode() ? BLACK : WHITE;
			}
		}
		void ReleaseMemory()
		{
			if (board != nullptr && father != nullptr)//不是根节点
			{
				for (uint8_t i = 0; i < boardSize; ++i)
				{
					delete[] board[i];
				}
				delete[] board;
				board = nullptr;
			}
		}
	public:
		//非根节点的构造函数
		Node(Node* nf, uint8_t x, uint8_t y) :father(nf), lastX(x), lastY(y), depth(nf->depth + 1), board(nullptr)
		{
			//nextBrother = father->firstChild;
			//father->firstChild = this;
			father->children.push_back(this);
		}
		//根节点的构造函数
		Node(int _depth, uint8_t x, uint8_t y) :father(nullptr), depth(_depth), lastX(x), lastY(y),board(new State*[boardSize])
		{
			for (int i = 0; i < boardSize; ++i)
			{
				board[i] = new State[boardSize];
				memset(board[i], 0, boardSize * sizeof(State));
			}
			board[x][y] = IsMaxNode() ? BLACK : WHITE;
		}
		int GetEvaluateValue()
		{
			if (evalutedValue == INT_MIN)
				evalutedValue = (father == nullptr ? 0 : Evaluate());
			return evalutedValue;
		}
		inline bool IsMaxNode()const//默认计算机后手
		{
			return depth & 1u;//相当于depth%2
		}
		void Search(unsigned short _depth)
		{
			CreateBoard();
			if (_depth == 0 || this->GetEvaluateValue() == maxEvaluteValue || this->GetEvaluateValue() == -maxEvaluteValue)
			{
				this->value = this->GetEvaluateValue();
				ReleaseMemory();
				return;
			}
			/*
			* 记录是否创建新的节点，在以下两种情况下为true：
			* 1.当前节点深度为1
			* 2.当前为开局第一次搜索
			*/
			bool created = this->children.empty();
			//bool created = firstChild == nullptr;
			if (created)
			{
				children.reserve(boardSize * boardSize - 1);
				for (uint8_t i = 0; i < boardSize; i++)
				{
					for (uint8_t j = 0; j < boardSize; j++)
					{
						if (board[i][j] == SPACE)
						{
							new Node(this, i, j);
						}
					}
				}
			}
			if (IsMaxNode())
			{
				this->value = -maxEvaluteValue;
				if (created)
				{
					/*
					* 对于深度大于等于2的节点来说，它的子节点估价越大，它的子节点Search
					* 后value越有可能大，所以按照子节点估价从大到小排序，增加剪枝的概率
					* 而深度为1的节点在下一次搜索时也会成为深度为2的节点
					*/
					std::sort(this->children.begin(), this->children.end(), [](Node* a, Node* b) {
						a->CreateBoard();
						b->CreateBoard();
						return a->GetEvaluateValue() > b->GetEvaluateValue();
						});
				}
			}
			else
			{
				this->value = maxEvaluteValue;
				if (created)
				{
					std::sort(this->children.begin(), this->children.end(), [](Node* a, Node* b) {
						a->CreateBoard();
						b->CreateBoard();
						return a->GetEvaluateValue() < b->GetEvaluateValue();
						});
				}
			}
			//for (Node* child = firstChild; child != nullptr; child = child->nextBrother)
			for (Node* child : children)
			{
				child->Search(_depth - 1);
				//α - β 剪枝
				if (IsMaxNode())
				{
					if (child->value > this->value)
					{
						this->value = child->value;
						if (this->father && this->value >= this->father->value)
						{
							ReleaseMemory();
							return;//剪枝
						}
					}
				}
				else//MIN节点
				{
					if (child->value < this->value)
					{
						this->value = child->value;
						if (this->father && this->value <= this->father->value)
						{
							ReleaseMemory();
							return;//剪枝
						}
					}
				}
			}
			ReleaseMemory();
		}
		void DeleteAllButThis()
		{
			if (!father->board)//父节点不是根节点
				throw std::invalid_argument("this->father必须是根节点！");
			//防止释放父节点时将本节点delete
			/*
			if (this == father->firstChild)
			{
				father->firstChild = this->nextBrother;
			}
			else
			{
				Node* last = father->firstChild;
				while (last->nextBrother != this)
					last = last->nextBrother;
				last->nextBrother = this->nextBrother;
			}
			*/
			father->children.erase(std::find(father->children.begin(), father->children.end(), this));
			board = new State * [boardSize];
			for (int i = 0; i < boardSize; ++i)
			{
				board[i] = new State[boardSize];
				memcpy_s(board[i], boardSize * sizeof(State), father->board[i], boardSize * sizeof(State));
			}
			board[lastX][lastY] = IsMaxNode() ? BLACK : WHITE;
			delete father;
			father = nullptr;
		}
		bool IsFull()const
		{
			for (uint8_t i = 0; i < boardSize; ++i)
			{
				for (uint8_t j = 0; j < boardSize; ++j)
				{
					if (board[i][j] == State::SPACE)
						return false;
				}
			}
			return true;
		}
		inline State GetWinner()const
		{
			if (this->value == maxEvaluteValue)
			{
				return Node::WHITE;
			}
			else if (this->value == -maxEvaluteValue)
			{
				return Node::BLACK;
			}
			return Node::SPACE;
		}
		~Node()
		{
			if (board)
			{
				for (int i = 0; i < boardSize; ++i)
				{
					delete[] board[i];
				}
				delete[] board;
			}
			/*
			for (Node* n = firstChild; n != nullptr;)
			{
				Node* tmp = n->nextBrother;
				delete n;
				n = tmp;
			}
			*/
			for (Node* n : children)
				delete n;
		}
	};
private:
	Node* root;
	const unsigned short maxDepth;
public:
	GameTree(unsigned short _maxDepth) : root(nullptr), maxDepth(_maxDepth)
	{
		if (_maxDepth < 2)
			throw std::invalid_argument("最大层数必须大于等于2！");
	}
	std::pair<uint8_t, uint8_t>GetNextPos(uint8_t x, uint8_t y)
	{
		if (root)
		{
			//for (Node* node = root->firstChild; node != nullptr;node=node->nextBrother)//进入用户选择的分支
			for (Node* node : root->children)
			{
				if (node->lastX == x && node->lastY == y)
				{
					node->DeleteAllButThis();
					root = node;
					break;
				}
			}
		}
		else//第一次开局
		{
			root = new Node(1, x, y);
		}
		root->Search(maxDepth);
		if (root->IsFull())
			return std::make_pair(Node::boardSize, Node::boardSize);
		//for (Node* node = root->firstChild; node != nullptr; node = node->nextBrother)//选择分值最大的
		for (Node* node : root->children)
		{
			if (node->value == root->value)
			{
				node->DeleteAllButThis();
				root = node;
				break;
			}
		}
		return std::make_pair(root->lastX, root->lastY);
	}
	Node::State GetWinner()const
	{
		return root->GetWinner();
	}
	bool IsFull()const
	{
		return root->IsFull();
	}
	//强制放置棋子，不改变步数
	void PutChess(uint8_t x, uint8_t y,Node::State color)
	{
		root->board[x][y] = color;
	}
	void Run()
	{
		while (1)
		{
			int x, y;
			do
			{
				std::cout << "输入x,y坐标";
				std::cin >> x >> y;
			} while (x < 0 || y < 0 || x >= 15 || y >= 15 || (root && root->board[x][y] != Node::SPACE));
			GetNextPos(x, y);
			system("cls");
			for (int i = 0; i < Node::boardSize; i++)
			{
				for (int j = 0; j < Node::boardSize; j++)
				{
					if (root->board[i][j] == Node::SPACE)
						std::cout << "十 ";
					else if (root->board[i][j] == Node::BLACK)
						std::cout << "黑 ";
					else
						std::cout << "白 ";
				}
				std::cout << '\n';
			}
			if (root->value == Node::maxEvaluteValue)
			{
				std::cout << "电脑胜利！";
				break;
			}
			else if (root->value == -Node::maxEvaluteValue)
			{
				std::cout << "玩家胜利！";
				break;
			}
			else if (root->IsFull())
			{
				std::cout << "平局！";
				break;
			}
		}
	}	
	std::string GetBoardString()const
	{
		std::string res = "  0 1 2 3 4 5 6 7 8 9 1011121314\n";
		if (root)
		{
			for (size_t i = 0; i < 15; ++i)
			{
				res += std::format("{:2}", i);
				for (size_t j = 0; j < 15; ++j)
				{
					switch (root->board[i][j])
					{
					case Node::BLACK:
						res += "●";
						break;
					case Node::WHITE:
						res += "○";
						break;
					default:
						res += "十";
						break;
					}
				}
				res += '\n';
			}
		}
		else
		{
			for (size_t i = 0; i < 15; ++i)
			{
				res += std::format("{:2}十十十十十十十十十十十十十十十\n", i);
			}
		}
		return res;
	}
	bool CanPutChess(uint8_t x, uint8_t y)const
	{
		if (x >= Node::boardSize || y >= Node::boardSize)
			return false;
		if (root && root->board[x][y] != Node::SPACE)
			return false;
		return true;
	}
	~GameTree()
	{
		delete root;
	}
};