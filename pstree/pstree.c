#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

struct pnode{
	int pid;
	int ppid;
	char name[100];
	struct pnode* nxtPro;
	int firstSon;
	int nxtBrother;
};

struct pnode** pArray = NULL;
int pNum = 0;
struct pnode head; 
int withPid = 0;
int withVersion = 0;


void findPid();
struct pnode* getProcessInfo(int);
void getSortArray();
void freeSpace();
int binSearch(int ppid);
void getSon();
int printNode();
void printTree(int, int, int, int, int*);
void printSon();

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
		assert(argv[i]);
		if (!strcmp("-V", argv[i]) || !strcmp("--version", argv[i])) withVersion = 1;
		if (!strcmp("-p", argv[i]) || !strcmp("--show-pids", argv[i])) withPid = 1;
	}
	
	if (withVersion == 1) printf("hello, this is xxs's pstree version 1.0!");
	else{
		findPid();
		getSortArray();
		getSon();
		printTree(0, 0, 0, 0, NULL);
		freeSpace();
	}
	printf("\n");
	return 0;
}




void findPid(){               //遍历目录，找到所有进程，并且用一个链表存起来
	DIR* dp = opendir("/proc");
	assert(dp);
	struct dirent* dirp = NULL;
    struct pnode* cur = &head;	
	while((dirp = readdir(dp)) != NULL){
		if (strspn(dirp->d_name, "0123456789") == strlen(dirp->d_name)){
			pNum += 1;
			cur->nxtPro = getProcessInfo(atoi(dirp->d_name));
		    cur = cur->nxtPro;	
			cur->nxtPro = NULL;
		}
	} 
}

struct pnode* getProcessInfo(int pid){            //读取进程下的stat文件，获取pid,ppid,name，存到pnode里
	struct pnode* pro = malloc(sizeof(struct pnode));
	pro->firstSon = pro->nxtBrother = -1; pro->nxtPro = NULL;
	pro->pid = pid;
	char pstat[24] = {0};
	sprintf(pstat, "/proc/%d/stat", pid);
	FILE* fstat = fopen(pstat, "r");
	assert(fstat);
	int tmpint; char tmpchar;
	fscanf(fstat, "%d (%s %c %d",&tmpint, pro->name, &tmpchar, &(pro->ppid));
	pro->name[strlen(pro->name) - 1] = 0;
	return pro;
}

void getSortArray(){                              //把链表转成数组，并且按照进程id排序，插入排序
	pArray = (struct pnode**)malloc(pNum * sizeof(struct pnode*));
	int tmpi = 0; struct pnode* cur = &head;
	while(cur->nxtPro != NULL){
		pArray[tmpi++] = cur->nxtPro;
		cur = cur->nxtPro;
	}
	for (int i = 0; i < pNum; ++i){
		struct pnode* curp = pArray[i];
		int cur = curp->pid;
		for (int j = i - 1; j >= 0; --j){
			if (cur >= pArray[j]->pid){
				pArray[j+1] = curp;
				break;
			}
			pArray[j+1] = pArray[j];
		}
	}	
}



void freeSpace(){                         //释放pArray和pnode的空间
	for (int i = 0; i < pNum; ++i){
		free(pArray[i]);
	}
	free(pArray);
}


int binSearch(int ppid){                   //根据进程id返回其在数组里的索引，二分查找
	int left = 0, right = pNum - 1;
	while(left <= right){
		int mid = (left + right) / 2;
		if (ppid == pArray[mid]->pid) return mid;
		if (ppid > pArray[mid]->pid) left = mid + 1;
		else right = mid - 1;
	}
	return -1;
}

void getSon(){                     //用孩子兄弟链表的方式在数组里构建出进程树；
	for (int i = 0; i < pNum; ++i){
		int ppid = pArray[i]->ppid;
		if (ppid == 0) continue;
		int pi = binSearch(ppid);
		assert(pi != -1); 
		int firstSon = pArray[pi]->firstSon;
		if (firstSon == -1){
			pArray[pi]->firstSon = i; 
		}
		else{
			struct pnode* tmp = pArray[firstSon];
			while(tmp->nxtBrother != -1) tmp = pArray[tmp->nxtBrother];
			tmp->nxtBrother = i;
		}
	}
}
	
int printNode(int i){                //打印进程名(PID)，返回打印的长度
	if (withPid){
		char buf[150] = {0};
		sprintf(buf, "%s(%d)", pArray[i]->name, pArray[i]->pid);
		printf("%s", buf);
		return strlen(buf);
	}	
	else{
		printf("%s", pArray[i]->name);
		return strlen(pArray[i]->name);
	}
}


void printTree(int index, int offset, int newline, int len, int* arr){    //递归打印出树
	if (pArray[index]->ppid == 1 && pArray[index]->nxtBrother == -1) printf("%s(%d)", pArray[index]->name, pArray[index]->pid);
	assert(pArray[index]->pid != 2);
	assert(index >= 0 && index < pNum);
	int selfLen = 0;
	if (!newline){                   //这部分是打印自己
		if (index == 0){
			selfLen = printNode(index);
		}
		else{
			printf("--");
			selfLen = printNode(index);
		}
	}
	else{
		printf("\n");
		for (int i = 0; i < len; ++i){
			if (i == 0){
				for (int j = 0; j < abs(arr[0]); ++j) putchar(' ');
			}
			else{
				for (int j = 0; j < abs(arr[i])-abs(arr[i-1])-1; ++j) putchar(' ');
			}
			if (arr[i]>0) putchar('|'); else putchar(' ');
		}
	    if (pArray[index]->ppid == 1 && pArray[index]->nxtBrother == -1) printf("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"); //aaaaaaaaaaaaaaaaaaaaaaaaaa
		printf("\n");
	    if (pArray[index]->ppid == 1 && pArray[index]->nxtBrother == -1) printf("aaaaa"); //aaaaaaaaaaaaaaaaaaaaaaaaaa
		for (int i = 0; i < len; ++i){
			if (i == 0) for (int j = 0; j < abs(arr[0]); ++j) putchar(' ');
			else for (int j = 0; j < abs(arr[i])-abs(arr[i-1])-1; ++j) putchar(' ');
			if (i == len - 1) putchar('+');
			else {
				if (arr[i]>0) putchar('|'); else putchar(' ');
			}
		}
		selfLen = printNode(index); 
	}
	int son = pArray[index]->firstSon;                               
	if (son != -1){
		int* newarr = (int*) malloc((len+1) * sizeof(int));
		for (int i = 0; i < len; ++i) newarr[i] = arr[i];
		newarr[len] = offset + 2 + selfLen;          //这里让所有兄弟公用一个arr
		if (pArray[index]->nxtBrother == -1){
			newarr[len - 1] = -newarr[len-1];                    //如果自己没有兄弟了，就不需要在自己下面的行对应位置打印'|'，因此标记成负的
		}
				
		printTree(son, offset+2+selfLen, 0, len+1, newarr);
		struct pnode* tmp = pArray[son];
		while(true){
			if (tmp->nxtBrother == -1) break;
			printTree(tmp->nxtBrother, offset+2+selfLen, 1, len+1, newarr);
			assert(tmp->pid < pArray[tmp->nxtBrother]->pid);
			tmp = pArray[tmp->nxtBrother];
		}
	}
	if (index != 0 && pArray[index]->nxtBrother == -1) free(arr);      //没有下一个兄弟，说明是最后使用arr的，由它来释放空间

}



void printSon(){      //测试时用来打印进程的函数
	for (int i = 0; i < pNum; ++i){
		printf("%s %d %d  ", pArray[i]->name, pArray[i]->pid, pArray[i]->ppid);
		if (pArray[i]->firstSon != -1){
			struct pnode* son = pArray[pArray[i]->firstSon];
			while(true){
				printf("%s %d %d   ", son->name, son->pid, son->ppid);
				if (son->nxtBrother == -1) break;
				son = pArray[son->nxtBrother];
			}
		}		
		printf("\n");
	}
}
