//只筛选了插入其他字符的敏感词，没考虑汉字拼音、声调、首字母、拆分、单复数、时态等的干扰
#include<iostream>
#include<fstream>
#include<string>
#include<cstring>
#include<locale>
using namespace std;
struct mapping//用来建立敏感词树
{
	wstring dat;//记录敏感词英文单词的一个字母或者中文一个字的拼音
	mapping* next = NULL, * nei = NULL;//next指向下一个字母或者拼音，nei指向该该位置其他可能出现的敏感词的一部分
	bool iseng = true;
};
struct rela//记录原文和转化完的句子的对应关系
{
	int  beg=0, end=0;//记录该段字符串在原文中的起始位置和结束位置
	wstring dat;//记录该字符串
	rela* next = NULL;
	bool isnum = false, iseng = false;
};
struct sto//存储敏感词结果
{
	int line=0, end=0;//line记录所在行数，end记录该敏感词出现的最后一个字节的后一位
	wstring sw1, sw2;//sw1为敏感词，sw2为包含敏感词的原文
	sto* next = NULL;
}*store = new sto, * sto2 = store, * sto3;
rela* trans(wstring linestr)//转化字符串，去除除汉字，英文和数字以外字符，保存在链表out中，保留位置映射，输入是文件的一行原文
{
	int i, len = linestr.length();
	rela* out2, * out3, * out;
	out = new rela;
	out2 = out;
	for (i = 0; i < len; )
	{
		if (linestr[i] <= 122 && linestr[i] >= 97)//保留小写字母
		{
			out3 = new rela;
			out3->dat = linestr[i];
			out3->beg = i;
			out3->end = i;
			out3->iseng = true;
			out2->next = out3;
			out2 = out3;
		}
		else if (linestr[i] <= 90 && linestr[i] >= 65)//保留大写字母
		{
			out3 = new rela;
			out3->dat = linestr[i]+32;
			out3->beg = i;
			out3->end = i;
			out3->iseng = true;
			out2->next = out3;
			out2 = out3;
		}
		else if (linestr[i] <= 57 && linestr[i] >= 48)//保留数字
		{
			out3 = new rela;
			out3->dat = linestr[i];
			out3->beg = i;
			out3->end = i;
			out3->isnum = true;
			out2->next = out3;
			out2 = out3;
		}
		else if (linestr[i] > 127)//保留汉字
		{
			out3 = new rela;
			out3->dat = linestr[i];
			out3->beg = i;
			out3->end = i;
			out2->next = out3;
			out2 = out3;
		}
		i++;
	}
	return out->next;
}
mapping* cretree(wstring x, int i, mapping* root, int len)//递归将敏感词并入树中,返回根节点root，x为文件的一行字符串，i为当前x的字节位置，root为当前节点指针，len为x的长度
{
	if (i >= len)
		return NULL;
	mapping* p1 = root;
	if (p1 && p1->dat == x.substr(i,1))//树中已经有该部分内容
	{
		if (p1->next)//当前节点有子节点则继续搜索
		{
			cretree(x, i+1, p1->next, len);
		}
		else//当前节点无子节点则访问next空节点并返回指针
		{
			p1->next = cretree(x, i+1, p1->next, len);
		}
	}
	else if (p1 && p1->dat != x.substr(i, 1))//当前节点内容与x[i]不同
	{
		if (p1->nei)//有邻居则向邻居搜索
			cretree(x, i, p1->nei, len);
		else//无邻居则访问nei空节点并返回指针
			p1->nei = cretree(x, i, p1->nei, len);
	}
	else if (!p1)//当前节点为空，建立节点，并返回指针，若字符串x未完则继续向下访问next空节点
	{
		p1 = new mapping;
		p1->dat = x.substr(i, 1);
		if (x[i] > 127)
			p1->iseng = false;
		p1->next = cretree(x, i+1, p1->next, len);
	}
	return p1;
}
int com(mapping* root, rela* outx, wstring sw, int end)//查敏感词，root为敏感词树当前节点指针，outx为原文映射链表当前指针，sw为目前已经搜索到的敏感词分片,end为当前原文最后一个字节位置
{
	if (outx&&outx->isnum  &&root&& root->iseng)//若数字在英文中间则跳过
		return com(root, outx->next, sw, end);
	if (outx&&root&&outx->dat == root->dat)//当前outx所指的内容为敏感词分片
	{
		if (outx->beg - end - 1 > 20)//其他字符超过20个
			return 0;
		if (root->next && outx->next)//两者都还有后续部分，继续向下比较
		{
			return com(root->next, outx->next, sw + root->dat, outx->end);
		}
		else if (root->next)//敏感词还有后续分片，但该行原文已经没有有效部分，说明这段字符串不是敏感词
		{
			return 0;
		}
		else//敏感词已经到达叶节点，敏感词完整找到，这段字符串是敏感词，记录
		{
			sto3 = new sto;
			sto3->sw1 = sw + root->dat;
			sto3->end = outx->end + 1;
			return 1;
		}
	}
	else//当前outx所指的内容不是这一种敏感词分片，横向搜索
	{
		if (root&&root->nei&&outx)
		{
			return com(root->nei, outx, sw, outx->end);
		}
		else
		{
			return 0;
		}
	}
}
void show(wofstream& ans)//将结果写入目标文档
{
	sto2 = store->next;
	while (sto2)
	{
		ans << "Line" << sto2->line << ": <" << sto2->sw1 << "> " << sto2->sw2 << endl;
		sto2 = sto2->next;
	}
}
int main(int argc, char** argv)
{
	locale china("zh_CN.UTF-8");
	char** x = argv;
	x++;
	wifstream words;
	words.open(*x);
	x++;
	wifstream org;
	org.open(*x);
	x++;
	wofstream ans;
	ans.open(*x);//打开文件
	words.imbue(china);
	org.imbue(china);
	ans.imbue(china);
	wstring linestr;
	int total = 0, i = 1;//total记录总敏感词数
	mapping* root = NULL;//敏感词树的根节点
	rela* out;//映射链表头结点
	wstring a ;
	//创建敏感词树
	while (!words.eof())
	{
		words>> linestr ;
		root = cretree(linestr, 0, root, linestr.length());
	}
	while (!org.eof())
	{
		getline(org, linestr);
		out = trans(linestr);//转换储存到映射链表
		for (; out; out = out->next)//查找敏感词
		{
			if (com(root, out, a, out->end))
			{
				total++;
				sto3->line = i;
				sto3->sw2 = linestr.substr(out->beg, sto3->end - out->beg);
				sto2->next = sto3;
				sto2 = sto3;
			}
		}
		i++;
	}

	ans << "Total:" << total << endl;
	show(ans);

	words.close();
	org.close();
	ans.close();
	return 0;
}