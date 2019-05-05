typedef struct _List List;

struct _List
{
    void **pListPointArray;                         //LIST数组指针
    int Total;                                      //元素个数
    void (*Add)(List *pList, void *pListPoint);     //添加
    void (*Remove)(List *pList, void *pListPoint);  //移除
    void (*Delete)(void *pList);                    //析构
 };

 List *ListCreate(void);