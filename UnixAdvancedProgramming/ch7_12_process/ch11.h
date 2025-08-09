#ifndef __TEST_H__
#define __TEST_H__

void exit_test(int argc, char *argv[]);
void env_test();
void process_memory_area_distributin(int argc, char *argv[]);
void classic_cmd_parse_realize(void);
void jump_test();
void jump_test2();
void process_resource_limits_test(void);

void process_test1(void);
void process_test2(void);
void process_test3(void);
void process_test4(void);
void process_test5(int argc, char *argv[]);
void rise_condition_test(void);
void exec_test1(void);
void print_argv_list(int argc, char *argv[]);
void uid_test1(void);
void bash_script_test(void);
void awk_script_test1(void);
void print_uid(void);
void system_m_test(void);
void tsys_test(int argc, char *argv[]);
void system_test(void);
void save_date_to_file1(void);
void save_date_to_file2(void);
void uid_test(void);
void process_counter_test(void);
void get_process_info(int argc, char *argv[]);
void get_user_info(void);
void process_schedule_test(int argc, char *argv[]);
void process_times_test(int argc, char *argv[]);

void unix_login_process(int argc, char *argv[]);
void process_grp_test1(void);
// int test_main(void);
int sig_hup_test(void);
void sig_usr_test(void);
void sig_alarm_test(void);
void sig_child_test(void);
void sig_test4(void);
int sig_test5(void);
unsigned int sleep1(unsigned int seconds);
unsigned int sleep2(unsigned int seconds);
void sig_alarm_test1(void);
void sig_test6(void);
void my_sig_test(void);
void sig_proc_mask_test1(void);
void sig_test7();
void sig_setjmp_test2(void);
void sig_suspend_test1(void);
void sig_suspend_test2(void);
void abort_test1(void);
void system_test1(void);
void signal_test9(void);
void sleep_test1(void);

extern void thread_test1(void);
extern void thread_exit_test2(void);
extern void thread_exit_test3(void);
extern void thread_cleanup_test1(void);
extern void thread_sync_test1(void);
extern void thread_lock_test2(void);
extern void lock_test1(void); 
extern void thread_lock_test3(void);
extern void job_queue_test(void);
extern void cond_test1(void);
extern void barrier_test(void);
extern void sysconf_test2(void);
extern void thread_property_test1(void);
extern int recursive_mutex_test1(void);
extern void getenv_test(void);
extern void getenv_test3(void);
void thread_signal_test1(void);
void fork_handler_test1(void);













int mq_test1(void);


#endif