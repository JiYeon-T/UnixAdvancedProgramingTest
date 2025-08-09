#include "../common/apue.h"
#include "pthread.h"

// 一个主线程, 多个工作线程处理主线程分配给他们的任务

struct job {
    struct job *j_next;
    struct job *j_prev;
    pthread_t j_id; // tell which thread handle this job
    // more stuff here
};

struct queue {
    struct job *q_head;
    struct job *q_tail;
    pthread_rwlock_t q_lock;
};

// intialize a queue
int queue_init(struct queue *qp)
{
    int err;

    qp->q_head = NULL;
    qp->q_tail = NULL;
    err = pthread_rwlock_init(&qp->q_lock, NULL);
    if (err != 0)
        FACBT_ERR("err");

    // continue intialization

    return (0);
}

// insert(双向链表,头插法)
void job_insert(struct queue *qp, struct job *jp)
{
    pthread_rwlock_wrlock(&qp->q_lock);
    jp->j_prev = NULL; // 双向链表
    jp->j_next = qp->q_head; // 头插法
    if (qp->q_head != NULL) // list was not empty 
        qp->q_head->j_prev = jp;
    else // list was empty
        qp->q_tail = jp;
    qp->q_head = jp;
    pthread_rwlock_unlock(&qp->q_lock);
}

// append a job on the tail of the queue
void job_append(struct queue *qp, struct job * jp)
{
    pthread_rwlock_wrlock(&qp->q_lock);
    jp->j_prev = qp->q_tail; // insert at tail
    jp->j_next = NULL;
    if (qp->q_tail != NULL) // list not was empty
        qp->q_tail->j_next = jp;
    else // list was empty
        qp->q_tail = jp;
    qp->q_head = jp;
    pthread_rwlock_unlock(&qp->q_lock);
}

// remove a given job from a queue
void job_remove(struct queue *qp, struct job *jp)
{
    pthread_rwlock_wrlock(&qp->q_lock);
    if (jp == qp->q_head) { // 删除的是头节点
        qp->q_head = jp->j_next;
        if (jp == qp->q_tail) // 仅有一个节点的链表, 删除的也是尾节点
            qp->q_tail = NULL;
        else // 多个节点的链表
            jp->j_next->j_prev = jp->j_prev;
            // 不是双向链表吗? 这里不对把
            // jp->j_next->j_prev = NULL;
    } else if (jp == qp->q_tail) { // 删除的是尾节点
        qp->q_tail = jp->j_prev;
        if (jp == qp->q_head) // 仅有一个节点, 这里不可能走到, 第一个判断条件已经判断了是否为头节点
            qp->q_head = NULL;
        else // 多个节点, 删除尾节点
            jp->j_prev->j_next = jp->j_next;
    } else { // 删除的是中间节点
        jp->j_prev = jp->j_next->j_prev;
        jp->j_next = jp->j_prev->j_next;
    }
    pthread_rwlock_unlock(&qp->q_lock);
}

// find a job for the given thread ID
struct job * job_find(struct queue *qp, pthread_t id)
{
    struct job *jp;

    if (pthread_rwlock_rdlock(&qp->q_lock) != 0)
        return (NULL);
    
    for (jp = qp->q_head; jp != NULL; jp = jp->j_next) // 后向遍历
        if (pthread_equal(jp->j_id, id))
            break;
    pthread_rwlock_unlock(&qp->q_lock);

    return jp;
}

void job_queue_test(void)
{
    
}
