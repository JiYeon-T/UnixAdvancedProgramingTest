#ifndef __LIST_H__
#define __LIST_H__


/**
 * 双向链表
*/
struct list {
    struct list *prev;
    struct list *next;
};


#endif