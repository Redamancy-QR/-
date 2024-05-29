#include "list.h"
#include "interrupt.h"
#include "global.h"

/**
 * plist_init - 初始化一个新的列表。
 * @plist: 指向要初始化的列表的指针。
 *
 * 设置列表的头和尾元素。
 */
void list_init(struct list *plist) {
    plist->head.prev = NULL;
    plist->head.next = &plist->tail;
    plist->tail.next = NULL;
    plist->tail.prev = &plist->head;
}

/**
 * list_insert_before - 在指定位置之前插入元素。
 * @posn: 要插入位置的前一个位置。
 * @elem: 要插入的元素。
 *
 * 在列表中的'posn'之前插入'elem'。在操作期间禁用中断以确保原子性。
 */
void list_insert_before(struct list_elem *posn, struct list_elem *elem) {
    /* 关闭中断以确保原子操作 */
    enum intr_status old_status = intr_disable();

    elem->next = posn;
    elem->prev = posn->prev;
    posn->prev->next = elem;
    posn->prev = elem;

    intr_set_status(old_status);
}

/**
 * list_push - 将一个元素推入列表的前面。
 * @plist: 要添加元素的列表。
 * @elem: 要添加的元素。
 *
 * 将'elem'添加到'plist'的开头。
 */
void list_push(struct list *plist, struct list_elem *elem) {
    list_insert_before(plist->head.next, elem);
}

/**
 * list_append - 将一个元素追加到列表的末尾。
 * @plist: 要添加元素的列表。
 * @elem: 要追加的元素。
 *
 * 将'elem'添加到'plist'的末尾。
 */
void list_append(struct list *plist, struct list_elem *elem) {
    list_insert_before(&plist->tail, elem);
}

/**
 * list_remove - 从列表中移除一个元素。
 * @elem: 要移除的元素。
 *
 * 断开'elem'与其相邻元素的连接。在操作期间禁用中断以确保原子性。
 */
void list_remove(struct list_elem *elem) {
    enum intr_status old_status = intr_disable();

    elem->prev->next = elem->next;
    elem->next->prev = elem->prev;

    intr_set_status(old_status);
}

/**
 * list_pop - 从列表中弹出第一个元素。
 * @plist: 要弹出元素的列表。
 *
 * 从'plist'中移除并返回第一个元素。
 */
struct list_elem *list_pop(struct list *plist) {
    struct list_elem *elem = plist->head.next;
    list_remove(elem);
    return elem;
}

/**
 * list_elem_find - 检查列表中是否存在一个元素。
 * @plist: 要搜索的列表。
 * @obj_elem: 要搜索的元素。
 *
 * 如果在'plist'中找到了'obj_elem'，则返回'true'，否则返回'false'。
 */
bool list_elem_find(struct list *plist, struct list_elem *obj_elem) {
    struct list_elem *iter = plist->head.next;
    while (iter != &plist->tail) {
        if (iter == obj_elem){
            return true;
        }
        iter = iter->next;
    }
    return false;
}

/**
 * list_traversal - 遍历列表并应用一个函数。
 * @plist: 要遍历的列表。
 * @func: 要应用的函数。
 * @arg: 要传递给函数的参数。
 *
 * 将'func'应用于'plist'的每个元素，直到它返回'true'。
 * 返回'func'返回'true'的元素，如果没有则返回'NULL'。
 */
struct list_elem *list_traversal(struct list *plist, function func, int arg) {
    if (list_empty(plist))
        return NULL;

    struct list_elem *iter = plist->head.next;
    while (iter != &plist->tail) {
        if (func(iter, arg)){
            return iter;
        }
        iter = iter->next;
    }
    return NULL;
}

/**
 * list_len - 计算列表的长度。
 * @plist: 要计算长度的列表。
 *
 * 返回'plist'中的元素数量。
 */
uint32_t list_len(struct list *plist) {
    uint32_t len = 0;
    struct list_elem *iter = plist->head.next;
    while (iter != &plist->tail) {
        ++len;
        iter = iter->next;
    }
    return len;
}

/**
 * list_empty - 检查列表是否为空。
 * @plist: 要检查的列表。
 *
 * 如果'plist'为空，则返回'true'，否则返回'false'。
 */
bool list_empty(struct list *plist) { 
    return plist->head.next == &plist->tail ? true : false; 
}



