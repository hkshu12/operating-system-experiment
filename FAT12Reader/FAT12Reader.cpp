#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

typedef unsigned char u8;		//1字节
typedef unsigned short u16; //2字节
typedef unsigned int u32;		//4字节

int BytsPerSec; //每扇区字节数
int SecPerClus; //每簇扇区数
int RsvdSecCnt; //Boot记录占用的扇区数
int NumFATs;		//FAT表个数
int RootEntCnt; //根目录最大文件数
int FATSz;			//FAT扇区数
#pragma pack(1) /*指定按1字节对齐*/

//偏移11个字节
struct BPB
{
	u16 BPB_BytsPerSec; //每扇区字节数
	u8 BPB_SecPerClus;	//每簇扇区数
	u16 BPB_RsvdSecCnt; //Boot记录占用的扇区数
	u8 BPB_NumFATs;			//FAT表个数
	u16 BPB_RootEntCnt; //根目录最大文件数
	u16 BPB_TotSec16;
	u8 BPB_Media;
	u16 BPB_FATSz16; //FAT扇区数
	u16 BPB_SecPerTrk;
	u16 BPB_NumHeads;
	u32 BPB_HiddSec;
	u32 BPB_TotSec32; //如果BPB_FATSz16为0，该值为FAT扇区数
};
//BPB至此结束，长度25字节

//内容条目
struct Content
{
	char DIR_Name[11];
	u8 DIR_Attr; //文件属性
	char reserved[10];
	u16 DIR_WrtTime;
	u16 DIR_WrtDate;
	u16 DIR_FstClus; //开始簇号
	u32 DIR_FileSize;
};
//内容条目结束，32字节

#pragma pack() /*取消指定对齐，恢复缺省对齐*/

struct Command
{
	string cmd = "";
	set<char> options = {};
	vector<string> params = {};
	Command() = default;
	Command(string cmd);
};
Command::Command(string cmd)
{
	vector<string> split;
	split.clear();

	istringstream iss(cmd);
	string tmp;
	int count = 0;
	while (getline(iss, tmp, ' '))
	{
		count++;
		split.emplace_back(move(tmp));
	}
	if (split[0] == "ls")
	{
		this->cmd = split[0];
	}
	else if (split[0] == "cat")
	{
		this->cmd = split[0];
	}
	else if (split[0] == "exit")
	{
		this->cmd = split[0];
	}
	else
	{
		throw "Invalid command.";
	}
	for (int i = 1; i < count; i++)
	{
		if (split[i].find("-") == 0)
		{
			for (int j = 1; split[i][j] != '\0'; j++)
			{
				options.insert(split[i][j]);
			}
		}
		else
		{
			if (!this->params.empty())
			{
				throw "More params than expected.";
			}
			string tmp;
			istringstream url_iss(split[i]);
			while (getline(url_iss, tmp, '/'))
			{
				// 对路径中的“.”和“”不作处理
				if (tmp == "..")
				{
					if (this->params.empty())
					{
						throw "Try to visit outer space.";
					}
					this->params.pop_back();
				}
				else if (tmp == ".")
				{
					//pass
				}
				else if (tmp == "")
				{
					//pass
				}
				else
				{
					this->params.emplace_back(tmp);
				}
				move(tmp);
			}
		}
	}
};

struct File
{
	string fileName;
	int fileSize = 0;
	string content;
};

struct Dir
{
	string dirName;
	int numOfSubDirs = 0;
	int numOfSubFiles = 0;
	vector<Dir *> subDirs = {};
	vector<File *> subFiles = {};
};

void fillBPB(FILE *, BPB *);	//读取BPB相关信息
int getFATValue(FILE *, int); //读取num号FAT项所在的两个字节，并从这两个连续字节中取出FAT项的值
void getFileTree(FILE *, Content *, Dir *);
void createSubTrees(FILE *, Content *, Dir *, int);
void ls(vector<string>, Dir *, bool);
void cat(File *);
extern "C" void myPrint(const char *, int, const char *, int);

string RED = "\033[31m";
string DEFAULT = "\033[0m";

int main()
{
	//加载FAT12文件
	FILE *fat12;
	fat12 = fopen("/mnt/c/Users/Jinyao/a.img", "rb");
	BPB bpb;
	BPB *bpb_ptr = &bpb;

	fillBPB(fat12, bpb_ptr);

	BytsPerSec = bpb_ptr->BPB_BytsPerSec;
	SecPerClus = bpb_ptr->BPB_SecPerClus;
	RsvdSecCnt = bpb_ptr->BPB_RsvdSecCnt;
	// NumFATs = bpb_ptr->BPB_NumFATs;
	NumFATs = 2;
	RootEntCnt = bpb_ptr->BPB_RootEntCnt;
	if (bpb_ptr->BPB_FATSz16 != 0)
	{
		FATSz = bpb_ptr->BPB_FATSz16;
	}
	else
	{
		FATSz = bpb_ptr->BPB_TotSec32;
	}

	Content content;
	Content *content_ptr = &content;

	Dir root;
	Dir *root_ptr = &root;
	root.dirName = "root";

	getFileTree(fat12, content_ptr, root_ptr);

	fclose(fat12);

	//开始命令输入解析与执行
	string cmdStr;
    string hint = ">";
	while (myPrint(hint.c_str(), hint.length(), DEFAULT.c_str(), DEFAULT.length()), getline(cin, cmdStr))
	{
		if (cmdStr == "")
		{
			continue;
		}
		try
		{
			struct Command line(cmdStr);
			if (line.cmd == "ls")
			{
				bool contain_l = false;
				for (auto it = line.options.begin(); it != line.options.end(); it++)
				{
					if (*it == 'l')
					{
						contain_l = true;
					}
					else
					{
						throw "Unknown options.";
					}
				}
				Dir *currentDir = root_ptr;
				bool notFound = true;
				vector<string> tmp(line.params);
				while (true)
				{
					if (line.params.size() == 0)
					{
						ls(tmp, currentDir, contain_l);
						notFound = false;
						break;
					}
					bool flag = false;
					for (auto it = currentDir->subDirs.begin(); it != currentDir->subDirs.end(); it++)
					{
						if ((*it)->dirName == line.params[0])
						{
							line.params.erase(line.params.begin());
							currentDir = *it;
							flag = true;
							break;
						}
					}
					if (!flag)
					{
						break;
					}
				}
				if (notFound)
				{
					throw "Directory not found.";
				}
			}
			else if (line.cmd == "cat")
			{
				if (!line.options.empty())
				{
					throw "Unknown options.";
				}
				if (line.params.empty())
				{
					throw "Empty parameters.";
				}
				Dir *currentDir = root_ptr;
				File *fileToCat = nullptr;
				bool notFound = true;
				while (true)
				{
					if (line.params.size() == 1)
					{
						for (auto it = currentDir->subFiles.begin(); it != currentDir->subFiles.end(); it++)
						{
							if ((*it)->fileName == line.params[0])
							{
								line.params.erase(line.params.begin());
								fileToCat = *it;
								notFound = false;
								break;
							}
						}
						if (!notFound)
						{
							cat(fileToCat);
						}
						break;
					}
					bool flag = false;
					for (auto it = currentDir->subDirs.begin(); it != currentDir->subDirs.end(); it++)
					{
						if ((*it)->dirName == line.params[0])
						{
							line.params.erase(line.params.begin());
							currentDir = *it;
							flag = true;
							break;
						}
					}
					if (!flag)
					{
						break;
					}
				}
				if (notFound)
				{
					throw "File not found.";
				}
			}
			else if (line.cmd == "exit")
			{
				string msg = "";
				break;
			}
		}
		catch (const char *msg)
		{
			cerr << msg << endl;
		}
	}
}

void ls(vector<string> path, Dir *dir_ptr, bool needDetails)
{
	string pathStr("/");
	for (auto it = path.begin(); it != path.end(); it++)
	{
		pathStr += (*it) + "/";
	}
	if (needDetails)
	{
		string tmp = pathStr;
		tmp.append(" ").append(to_string(dir_ptr->numOfSubDirs)).append(" ").append(to_string(dir_ptr->numOfSubFiles)).append(":\n");
		myPrint(tmp.c_str(), tmp.length(), DEFAULT.c_str(), DEFAULT.length());
		for (auto it = dir_ptr->subDirs.begin(); it != dir_ptr->subDirs.end(); it++)
		{
			if ((*it)->dirName != "." && (*it)->dirName != "..")
			{
				myPrint((*it)->dirName.c_str(), (*it)->dirName.length(), RED.c_str(), RED.length());
				string tmp = " ";
				tmp.append(to_string((*it)->numOfSubDirs));
				tmp.append(" ");
				tmp.append(to_string((*it)->numOfSubFiles));
				tmp.append("\n");
				myPrint(tmp.c_str(), tmp.length(), DEFAULT.c_str(), DEFAULT.length());
			}
			else
			{
				string tmp = (*it)->dirName;
				tmp.append("\n");
				myPrint(tmp.c_str(), tmp.length(), RED.c_str(), RED.length());
			}
		}
		for (auto it = dir_ptr->subFiles.begin(); it != dir_ptr->subFiles.end(); it++)
		{
			string tmp = (*it)->fileName;
			tmp.append(" ");
			tmp.append(to_string((*it)->fileSize));
			tmp.append("\n");
			myPrint(tmp.c_str(), tmp.length(), DEFAULT.c_str(), DEFAULT.length());
		}
		myPrint("\n", 1, DEFAULT.c_str(), DEFAULT.length());
	}
	else
	{
		string tmp = pathStr;
		tmp.append(":\n");
		myPrint(tmp.c_str(), tmp.length(), DEFAULT.c_str(), DEFAULT.length());
		for (auto it = dir_ptr->subDirs.begin(); it != dir_ptr->subDirs.end(); it++)
		{
			myPrint((*it)->dirName.c_str(), (*it)->dirName.length(), RED.c_str(), RED.length());
			string tmp = " ";
			myPrint(tmp.c_str(), tmp.length(), DEFAULT.c_str(), DEFAULT.length());
		}
		for (auto it = dir_ptr->subFiles.begin(); it != dir_ptr->subFiles.end(); it++)
		{
			string tmp = (*it)->fileName;
			tmp.append(" ");
			myPrint(tmp.c_str(), tmp.length(), DEFAULT.c_str(), DEFAULT.length());
		}
		myPrint("\n", 1, DEFAULT.c_str(), DEFAULT.length());
	}
	for (auto it = dir_ptr->subDirs.begin(); it != dir_ptr->subDirs.end(); it++)
	{
		if ((*it)->dirName == "." || (*it)->dirName == "..")
		{
			continue;
		}
		path.push_back((*it)->dirName);
		ls(path, *it, needDetails);
		path.pop_back();
	}
}

void cat(File *file_ptr)
{
	myPrint(file_ptr->content.c_str(), file_ptr->content.length(), DEFAULT.c_str(), DEFAULT.length());
	//string tmp = "\n";
	//myPrint(tmp.c_str(), tmp.length(), DEFAULT.c_str(), DEFAULT.length());
}

void fillBPB(FILE *fat12, BPB *bpb_ptr)
{
	//BPB从11字节开始,36字节结束
	fseek(fat12, 11, SEEK_SET);
	fread(bpb_ptr, 1, 25, fat12);
}

void getFileTree(FILE *fat12, Content *content_ptr, Dir *root_ptr)
{
	int base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec; //根目录首字节的偏移数
	for (int i = 0; i < RootEntCnt; i++)
	{
		//逐个读取32字节的目录信息
		fseek(fat12, base, SEEK_SET);
		fread(content_ptr, 1, 32, fat12);
		base += 32;

		//检查DIR_Name，含非法字符表明该项为非目标项,跳过
		bool flag = false;
		for (int j = 0; j < 11; j++)
		{
			if (!(((content_ptr->DIR_Name[j] >= 48) && (content_ptr->DIR_Name[j] <= 57)) ||
						((content_ptr->DIR_Name[j] >= 65) && (content_ptr->DIR_Name[j] <= 90)) ||
						((content_ptr->DIR_Name[j] >= 97) && (content_ptr->DIR_Name[j] <= 122)) ||
						(content_ptr->DIR_Name[j] == ' ') || content_ptr->DIR_Name[j] == '.'))
			{
				flag = true; //非英文及数字、空格
				break;
			}
		}
		if (flag == true)
			continue; //非目标文件不输出

		if ((content_ptr->DIR_Attr & 0x10) == 0)
		{ //文件
			File *f_ptr = new File();
			f_ptr->fileName = "";
			f_ptr->content = "";
			string tmp = content_ptr->DIR_Name;
			string name = tmp.substr(0, 8);
			name = name.substr(name.find_first_not_of(' '), name.find_last_not_of(' ') + 1);
			string extension = tmp.substr(8);
			if (extension != "   ")
			{
				extension = extension.substr(extension.find_first_not_of(' '), extension.find_last_not_of(' ') + 1);
				f_ptr->fileName.append(name + '.' + extension);
			}
			else
			{
				f_ptr->fileName.append(name);
			}
			f_ptr->fileSize = content_ptr->DIR_FileSize;
			int cluster = content_ptr->DIR_FstClus;
			int FATValue = 0;
			int dataBase = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
			while (FATValue < 0xFF8)
			{
				FATValue = getFATValue(fat12, cluster);
				if (FATValue == 0xFF7)
				{
					// cout << "The cluster " << cluster << " is broken." << endl;
				}
				int startByte = dataBase + (cluster - 2) * SecPerClus * BytsPerSec;
				char buf[512] = {0}; //暂存从簇中读出的数据

				fseek(fat12, startByte, SEEK_SET);
				fread(buf, 1, 512, fat12);
				f_ptr->content.append(buf);

				cluster = FATValue;
			}
			root_ptr->subFiles.push_back(f_ptr);
			root_ptr->numOfSubFiles++;
		}
		else
		{ //目录
			Dir *dir_ptr = new Dir();
			dir_ptr->dirName = "";
			string tmp = content_ptr->DIR_Name;
			string name = tmp.substr(0, 8);
			dir_ptr->dirName.append(name.substr(name.find_first_not_of(' '), name.find_last_not_of(' ') + 1));

			int cluster = content_ptr->DIR_FstClus;
			createSubTrees(fat12, content_ptr, dir_ptr, cluster);
			root_ptr->subDirs.push_back(dir_ptr);
			root_ptr->numOfSubDirs++;
		}
	}
}

void createSubTrees(FILE *fat12, Content *content_ptr, Dir *parent_ptr, int cluster)
{
	int value = 0;
	int dataBase = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
	while (value < 0xFF8)
	{
		if (value == 0xFF7)
		{
			// cout << "The cluster " << cluster << " is broken." << endl;
		}
		value = getFATValue(fat12, cluster);
		int startByte = dataBase + (cluster - 2) * SecPerClus * BytsPerSec;
		for (int i = 0; i < 16; i++)
		{
			fseek(fat12, startByte, SEEK_SET);
			fread(content_ptr, 1, 32, fat12);
			startByte += 32;

			//检查DIR_Name，含非法字符表明该项为非目标项,跳过
			bool flag = false;
			for (int j = 0; j < 11; j++)
			{
				if (!(((content_ptr->DIR_Name[j] >= 48) && (content_ptr->DIR_Name[j] <= 57)) ||
							((content_ptr->DIR_Name[j] >= 65) && (content_ptr->DIR_Name[j] <= 90)) ||
							((content_ptr->DIR_Name[j] >= 97) && (content_ptr->DIR_Name[j] <= 122)) ||
							(content_ptr->DIR_Name[j] == ' ') || content_ptr->DIR_Name[j] == '.'))
				{
					flag = true; //非英文及数字、空格
					break;
				}
			}
			if (flag == true)
				continue; //非目标文件不输出

			if ((content_ptr->DIR_Attr & 0x10) == 0)
			{ //文件
				File *f_ptr = new File();
				f_ptr->fileName = "";
				f_ptr->content = "";
				string tmp = content_ptr->DIR_Name;
				string name = tmp.substr(0, 8);
				name = name.substr(name.find_first_not_of(' '), name.find_last_not_of(' ') + 1);
				string extension = tmp.substr(8);
				if (extension != "   ")
				{
					extension = extension.substr(extension.find_first_not_of(' '), extension.find_last_not_of(' ') + 1);
					f_ptr->fileName.append(name + '.' + extension);
				}
				else
				{
					f_ptr->fileName.append(name);
				}
				f_ptr->fileSize = content_ptr->DIR_FileSize;
				int cluster = content_ptr->DIR_FstClus;
				int FATValue = 0;
				while (FATValue < 0xFF8)
				{
					FATValue = getFATValue(fat12, cluster);
					if (FATValue == 0xFF7)
					{
						// cout << "The cluster " << cluster << " is broken." << endl;
					}
					int startByte = dataBase + (cluster - 2) * SecPerClus * BytsPerSec;
					char buf[512] = {0}; //暂存从簇中读出的数据

					fseek(fat12, startByte, SEEK_SET);
					fread(buf, 1, 512, fat12);
					f_ptr->content.append(buf);

					cluster = FATValue;
				}

				parent_ptr->subFiles.push_back(f_ptr);
				parent_ptr->numOfSubFiles++;
			}
			else
			{ //目录
				Dir *dir_ptr = new Dir();
				dir_ptr->dirName = "";
				string tmp = content_ptr->DIR_Name;
				string name = tmp.substr(0, 8);
				dir_ptr->dirName.append(name.substr(name.find_first_not_of(' '), name.find_last_not_of(' ') + 1));
				if (dir_ptr->dirName == "." || dir_ptr->dirName == "..")
				{
					parent_ptr->subDirs.push_back(dir_ptr);
				}
				else
				{
					int cluster = content_ptr->DIR_FstClus;
					createSubTrees(fat12, content_ptr, dir_ptr, cluster);
					parent_ptr->subDirs.push_back(dir_ptr);
					parent_ptr->numOfSubDirs++;
				}
			}
		}
	}
}

int getFATValue(FILE *fat12, int num)
{
	//FAT1的偏移字节
	int fatBase = RsvdSecCnt * BytsPerSec;
	//FAT项的偏移字节
	int fatPos = fatBase + num * 3 / 2;
	//奇偶FAT项处理方式不同，分类进行处理，从0号FAT项开始
	int type = 0;
	if (num % 2 == 0)
	{
		type = 0;
	}
	else
	{
		type = 1;
	}

	//先读出FAT项所在的两个字节
	u16 bytes;
	u16 *bytes_ptr = &bytes;
	fseek(fat12, fatPos, SEEK_SET);
	fread(bytes_ptr, 1, 2, fat12);

	//u16为short，结合存储的小尾顺序和FAT项结构可以得到
	//type为0的话，取byte2的低4位和byte1构成的值，type为1的话，取byte2和byte1的高4位构成的值
	if (type == 0)
	{
		return int(bytes & 0x0fff);
	}
	else
	{
		return int(bytes >> 4);
	}
}
