#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "common/apue.h"
#include "ch7_12_process/ch11.h"
#include "ch13_io/ch13.h"
#include "ch16_socket/ch16.h"
#include "ch17_IPC/ch17.h"
#include "ch19_psedo_terminal/ch19.h"


int main(int argc, char *argv[])
{
	printf("Hello, world start\n");

    // exit_test(argc, argv);jump_test
    // env_test();
    // process_memory_area_distributin(argc, argv);
    // jump_test();
    // jump_test2();
    // process_resource_limits_test();

    // process_test1();
    // process_test2();
    // process_test3();
    // process_test4();
    // process_test5(argc, argv);
    // rise_condition_test();

    /* chapter 8 */
    // exec_test1();
    // print_argv_list(argc, argv);
    // shell_test();
    // tsys_test(argc, argv);
    // system_test();
    // uid_test1();
    // bash_script_test();
    // awk_script_test1();
    // system_m_test();
    // system_m_test(); 
    // print_uid();
    // save_date_to_file1();
    // save_date_to_file2();
    // process_counter_test();
    // get_process_info(argc, argv);
    // get_user_info();
    // process_schedule_test(argc, argv);
    // process_times_test(argc, argv);

    /* chapter 9 */
    // unix_login_process(argc, argv);
    // process_grp_test1();
    // process_thread_infomes_test(argc, argv);
    // process_thread_infomes_test(argc, argv);
    // test_main();
    // sig_hup_test();
    // sig_usr_test();
    // sig_alarm_test();
    // sig_child_test();
    // sig_test4();
    // sig_test5();
    // sig_alarm_test1();
    // sig_test6();
    // my_sig_test();
    // sig_proc_mask_test1();
    // sig_test7();
    // sig_setjmp_test2();
    // sig_suspend_test1();
    // sig_suspend_test2();
    // abort_test1();
    // system_test1();
    // signal_test9();
    // sleep_test1();

    // mq_test1();

    // thread_test1();
    // thread_exit_test2();
    // thread_exit_test3();
    // thread_cleanup_test1();
    // thread_lock_test2();
    // lock_test1();
    // thread_lock_test3();
    // cond_test1();
    // barrier_test();
    // sysconf_test2();
    // thread_property_test1();
    // recursive_mutex_test1();
    // getenv_test();
    // getenv_test3();
    // thread_signal_test1();
    // fork_handler_test1();

    // daemon_process_test();
    // alread_runing();
    // io_test1();
    // file_record_test1();
    // file_lock_test(argc, argv);
    pselect_test();
    // char_process_test(argc, argv);
    // aio_test1(argc, argv);
    // aio_read_test1();
    // aio_suspend_test();
    // aio_lio_listhread_infoo_test();
    // aio_signal_test();
    // aio_signal_thread_test();
    // mmap_io_test(argc, argv);
    // pipe_test1();
    // pipe_test2(argc, argv);
    // parent_child_sync_test1();
    // pager_test2(argc, argv);
    // popen_test1();
    // coprocess_test1();
    // network_ipc_test1();
    // get_addr_info_test1(argc, argv);
    // socketpair_send_test(argc, argv);
    // socketpair_recv_test();
    // unix_socket_test1();
    // unix_socket_server_test2();
    // unix_socket_client_test2();
    // send_fd_server_test1();
    // recv_fd_client_test1();
    // start_uds_client();
    // buf_args(NULL, NULL);
    // start_open_server_v1(NULL, NULL);
    // start_open_client_v1(NULL, NULL);
    // uds_thread_test();

    /* chapter 18 */
    // change_term_stat();
    // term_state_flag();
    // term_ICANON_test();
    // term_get_name();
    // term_speed_test();
    // tty_test();
    // strdup_test();
    // ttyname_test();
    // getpass_test();
    // abnormal_terminal_test();
    // winsize_test();

    /* ch19 psedo terminal */
    // pty_test(argc, argv);
    // tty_test();

    if (errno != 0)
	    printf("Hello, world end with error:%d\n\n", errno);
    else
        printf("Hello, world end\n\n");

    return 0;
}