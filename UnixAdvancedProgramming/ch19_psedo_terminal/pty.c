/**
 * Psedo terminal - 伪终端
*/
#include "ch19.h"
#include <termios.h>

#include "../common/apue.h"

/**
 * TODO:
 * spp_pty
 * pty 有一个 master 端, 一个 slave 端, btservice 操作的是 master 端, 外面应用操作的是 slave 端
 * 应用不用关注 btservice 里面的东西, 只需要关注 slave 在那儿创建和 init 的
*/

// int euv_pty_write(euv_pty_t *handle, uint8_t *buffer, int length, euv_write_cb cb)
// {
//     uv_buf_t buf;
//     euv_wreq_t *wreq;
//     int ret;

//     if (handle == NULL)
//         return -EINVAL;
    
//     wreq = (euv_wreq_t *)malloc(sizeof(euv_wreq_t));
//     if (!wreq)
//         return -ENOMEM;
//     wreq->req.data = (void*)handle;
//     wreq->buffer = buffer;
//     wreq->wreite_cb = cb;
//     buf = uv_buf_init((char*)buffer, length);

//     ret = uv_write(&wreq->req, (uv_stream_t*)&handle->uv_tty, &buf, 1, uv_write_callback);
//     if (ret != 0)
//         free(wreq);

//     return ret;
// }
