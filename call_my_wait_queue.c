#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <asm/current.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>

struct my_data {
    int pid;
    struct list_head list;
};

DECLARE_WAIT_QUEUE_HEAD(my_wait_queue); // 宣告一個等待佇列
LIST_HEAD(my_list);                     // 定義一個list head，儲存process資料
static DEFINE_MUTEX(my_mutex);          // 定義互斥鎖，保護共享資源
static int condition = 0;               // 等待佇列的條件

static int enter_wait_queue(void){
    int pid = (int)current->pid;                    // 獲取當前process的 PID
    struct my_data *entry;

    entry = kmalloc(sizeof(*entry), GFP_KERNEL);    // 分配記憶體
    entry->pid = pid;                               // 保存process的 PID
    list_add_tail(&entry->list, &my_list);          // 將資料加入linked_list

    // 當 condition == pid 時繼續執行
    wait_event_interruptible(my_wait_queue, condition == pid);  
    return 0;
}

static int clean_wait_queue(void){
    struct my_data *entry;
    list_for_each_entry(entry, &my_list, list) {
        condition = entry->pid;                     // 設置條件
        wake_up_interruptible(&my_wait_queue);      // 喚醒等待中的process
        msleep(100);                                // 延遲100ms -> FIFO
    }

    return 0;
}

SYSCALL_DEFINE1(call_my_wait_queue, int, id){
    switch (id){
        case 1:
            mutex_lock(&my_mutex);          // 獲取互斥鎖
            enter_wait_queue();             // 將process加入等待佇列
            mutex_unlock(&my_mutex);        // 釋放互斥鎖
            break;
        case 2:
            clean_wait_queue();             // 主程序呼叫，清理等待佇列
            break;
    }
    return 0;
}