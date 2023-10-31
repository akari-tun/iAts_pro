#include "stddef.h"
#include "freertos/FreeRTOS.h"
#include "list.h"

//List类的析构函数
static void ListDelete(void *pList)
{
    //先释放指针数组
    if(((List *)pList)->pListPointArray != NULL)     
    {
        free(((List *)pList)->pListPointArray);
    }
     //再释放整个List类
    free(pList);                                    
}
//元素增加函数
static void ListAdd(List *pList, void *pListPoint)
{
    //申请比原来大一个存储单元的内存
    void **tListPointArray = malloc(sizeof(int *) * (pList->Total + 1));     
    int pListIndex;
    //拷贝
    for(pListIndex = 0; pListIndex < pList->Total; pListIndex++)        
    {
        //判断，如果有相同的元素存在
        if(pList->pListPointArray[pListIndex] == pListPoint)                 
        {   
            //释放现申请的内存
            free(tListPointArray);
            //返回                          
            return;                                                     
        }
        //拷贝
        tListPointArray[pListIndex] = pList->pListPointArray[pListIndex];
    }
   
    tListPointArray[pList->Total] = pListPoint;
    //总数加1
    pList->Total += 1;
    //释放原来的内存
    if(pList->pListPointArray != NULL) free(pList->pListPointArray);
    //将新的句柄替换原句柄
    pList->pListPointArray = tListPointArray;
}
//元素移除函数
static void ListRemove(List *pList, void *pListPoint)
{
    int pListIndex, tListIndex;
    void **tListPointArray;
    void **FreePointArray;
    void **SavePointArray;

    //总数为0时退出
    if(pList->Total == 0) return;       

    //申请比原来小一个存储单元的内存
    tListPointArray = malloc(sizeof(int) * (pList->Total - 1));
    //将刚申请的内存空间作为默认的释放空间
    FreePointArray = tListPointArray;
    //将已有的内存空间作为默认的存储空间
    SavePointArray = pList->pListPointArray;

    //查找移除点
    for(pListIndex = 0, tListIndex= 0; pListIndex < pList->Total; pListIndex++)
    {
        //当前点是移除点
        if(pList->pListPointArray[pListIndex] == pListPoint)
        {
            //改变释放内存指针
            FreePointArray = pList->pListPointArray;
            //改变保留内存指针
            SavePointArray = tListPointArray;
            //结束本次循环
            continue;                                           
        }

        //如果当前点不是移除点，拷贝序号小于总量减1
        if(tListIndex < (pList->Total -1))
        {
            //拷贝
            tListPointArray[tListIndex] = pList->pListPointArray[pListIndex];
            //拷贝序号加1
            tListIndex++;
        }
    }

    //根据保留的内存块改变总数的值
    pList->Total = (SavePointArray == tListPointArray) ? pList->Total - 1 : pList->Total;
    //释放该释放的不用的内存块
    if(FreePointArray != NULL) free(FreePointArray);
    //保留该保留的    
    pList->pListPointArray = SavePointArray;
}

//List构造函数
List *ListCreate(void)
{
    List *pList = (List *)malloc(sizeof(List));
    pList->Total = 0;
    pList->pListPointArray = NULL;
    pList->Add = ListAdd;
    pList->Remove = ListRemove;
    pList->Delete = ListDelete;
    return pList;
}