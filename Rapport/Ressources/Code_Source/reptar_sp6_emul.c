/*
 * REPTAR Spartan6 FPGA emulation
 * Emulation "logic" part. Gateway between the emulation code and the backend.
 *
 * Copyright (c) 2013 HEIG-VD / REDS
 * Written by Romain Bornet
 *
 * This code is licensed under the GPL.
 */
#include <stddef.h>
#include <errno.h>


#include "qemu-common.h"
#include "qemu/thread.h"
#include "qemu/queue.h"

#include "qemu/sockets.h"

#include "reptar_sp6.h"
#include "reptar_sp6_emul.h"
#include "reptar_sp6_buttons.h"

#include "cJSON.h"

typedef struct SP6packet {
    QSIMPLEQ_ENTRY(SP6packet) entry;
    cJSON * packet;
} SP6packet;

typedef struct SP6EmulState {
    int sock;                               			/* Socket to command server */

    QSIMPLEQ_HEAD(cmd_list, SP6packet) cmd_list;   		/* Outgoing packets (commands) */
    QSIMPLEQ_HEAD(event_list, SP6packet) event_list;   	/* Ingoing packets (events) */

    QemuMutex cmd_mutex;                        		/* Condition variable and associated mutex */
    QemuCond cmd_cond;

    QemuThread eventThreadId;                   		/* Command thread sending commands to the GUI */
    QemuThread cmdThreadId;                   			/* Command thread sending commands to the GUI */

    int thread_terminate;                  				/* Flag to indicate that the the threads should terminate */

} SP6EmulState;

static SP6EmulState sp6_state;

/*
 * This function adds the cJSON pointer in the queue
 */
void *sp6_emul_cmd_post(cJSON *packet)
{
  if(sp6_state.thread_terminate)
  {
      cJSON_Delete(packet);
	  return NULL;
  }

  SP6packet *cmd;

  DBG("%s\n", __FUNCTION__);

  cmd = g_malloc(sizeof(SP6packet));
  cmd->packet = packet;

  qemu_mutex_lock(&sp6_state.cmd_mutex);

  DBG("%s Inserting into queue...\n", __FUNCTION__);
  QSIMPLEQ_INSERT_TAIL(&(sp6_state.cmd_list), cmd, entry);
  qemu_cond_signal(&sp6_state.cmd_cond);
  DBG("%s ...done\n", __FUNCTION__);

  qemu_mutex_unlock(&sp6_state.cmd_mutex);

  return NULL;
}

/*
 * This loop empties the queue as fast as it can, sending the stringified
 * JSON through the socket.
 */
static void *sp6_emul_cmd_process(void *arg) {

  SP6EmulState *sp6 = arg;
  SP6packet *cmd;
  char *rendered;
  unsigned int len;


  while (!sp6->thread_terminate)
  {
    /* Wait on command to process */
    qemu_mutex_lock(&sp6->cmd_mutex);
    qemu_cond_wait(&sp6->cmd_cond, &sp6->cmd_mutex);
    qemu_mutex_unlock(&sp6->cmd_mutex);

    if (sp6->thread_terminate)
        break;

    /* while not empty */
    while (!sp6->thread_terminate && !QSIMPLEQ_EMPTY(&sp6->cmd_list))
    {
        qemu_mutex_lock(&sp6->cmd_mutex);
        cmd 		= QSIMPLEQ_FIRST(&sp6->cmd_list);
        QSIMPLEQ_REMOVE_HEAD(&sp6->cmd_list, entry);

        rendered 	= cJSON_Print(cmd->packet);
        cJSON_Minify(rendered);
        len = strlen(rendered);

        rendered[len] 	= '\n';
        rendered[len+1] = '\0';

        if (write(sp6->sock, rendered, strlen(rendered)) != strlen(rendered))
        {
            fprintf(stderr, "%s: Write error on socket.\n", __FUNCTION__);
            sp6->thread_terminate = 1;
        }
        free(rendered);

        qemu_mutex_unlock(&sp6->cmd_mutex);
        cJSON_Delete(cmd->packet);
        g_free(cmd);
    }
  }

  /* Empty queue on exit */
  while(!QSIMPLEQ_EMPTY(&sp6->cmd_list))
  {
	  qemu_mutex_lock(&sp6->cmd_mutex);

      cmd 		= QSIMPLEQ_FIRST(&sp6->cmd_list);
      cJSON_Delete(cmd->packet);
      g_free(cmd);

	  qemu_mutex_unlock(&sp6->cmd_mutex);
  }

  /* Close connection to server here ... */
  DBG("%s thread exits!\n", __FUNCTION__);
  return NULL;

}

/*
 * This loop receives and parses data from the socket, to cJSON objects.
 * Then the right driver callback is called.
 */
static void *sp6_emul_event_handle(void *arg)
{
  char inBuffer[1024]; 	/* Must be big enough to contain a least one json object */
  int readBytes;
  cJSON *root, *perifnode;

  int alreadyReadBytes = 0;
  SP6EmulState *sp6 = arg;

  while(!sp6->thread_terminate)
  {
	  /* Read from the socket. */
	  readBytes = read(sp6->sock,inBuffer+alreadyReadBytes,sizeof(inBuffer)-alreadyReadBytes);

	  DBG("%s: read %d \n", __FUNCTION__,readBytes);

	  if(readBytes == 0)
	  {
		  DBG("%s: Socket error %d \n", __FUNCTION__,errno);

		  sp6->thread_terminate=1;
		  qemu_cond_signal(&sp6_state.cmd_cond);
	  }
	  /* If something has been read. */
	  if(readBytes > 0)
		  alreadyReadBytes+=readBytes;

	  /* If something is present in the FIFO  */
	  if(alreadyReadBytes)
	  {
		  /* For each newLine delimited string */
		  while(1)
		  {
			  char * newLine = memchr(inBuffer,'\n',alreadyReadBytes);

			  if(!newLine)
			  {
				 /* If FIFO is full, but there is no newline, something went wrong. We discard the whole FIFO. */
				 if(alreadyReadBytes == sizeof(inBuffer))
					 alreadyReadBytes = 0;

				 break;
			  }


			  /* Replace newline by 0, so cJSON parses only one object */
			  *newLine		= '\0';

			  root 			= cJSON_Parse(inBuffer);

			  DBG("%s: cJSON_Parse done \n", __FUNCTION__);

			  if(root)
			  {
				  perifnode 	= cJSON_GetObjectItem(root,"perif");

				  if(strcmp(perifnode->valuestring,PERID_BTN) == 0)
						  reptar_sp6_btns_event_process(root);
				  else
				  {
					  DBG("%s: Error, unknow perif: %s \n", __FUNCTION__,perifnode->valuestring);
					  cJSON_Delete(root);
				  }

			  }
			  else
				  DBG("%s: Error, not valid JSON: %s \n", __FUNCTION__,inBuffer);

			  /*
			   * Update alreadyReadBytes. The +1 is for the discarded null char
			   */
			  alreadyReadBytes -= newLine-inBuffer+1;

			  /* Move the fifo:
			   * - Destination in inBuffer, base of the FIFO
			   * - Copy from newLine+1, so we discard the newLine
			   * - Copy the rest of the FIFO (alreadyReadBytes is up to date)
			   */
			  memmove(inBuffer,newLine+1,alreadyReadBytes);
		  }
	  }
  }
  DBG("%s thread exits!\n", __FUNCTION__);
  return NULL;
}

int sp6_emul_init(void)
{
    DBG("%s\n", __FUNCTION__);

    char host[255];

    QSIMPLEQ_INIT(&sp6_state.event_list);
    QSIMPLEQ_INIT(&sp6_state.cmd_list);

    qemu_mutex_init(&sp6_state.cmd_mutex);
    qemu_cond_init(&sp6_state.cmd_cond);

    sp6_state.thread_terminate = 0;

	/* Connect to server */
	snprintf(host, 255, "localhost:%d", TCP_PORT);
	//sp6_state.sock 	= inet_connect(host, SOCK_STREAM);
	sp6_state.sock = inet_connect(host, NULL);

	if (sp6_state.sock < 0) {
		fprintf(stderr, "%s: failed to connect to SP6 server\n", __FUNCTION__);
		fprintf(stderr, "%s: terminate thread\n", __FUNCTION__);
		return -1;
	}

    /* Thread for output commands */
    qemu_thread_create(&sp6_state.cmdThreadId, "cmd_process", sp6_emul_cmd_process, &sp6_state, QEMU_THREAD_JOINABLE);

    /* Thread for input events */
    qemu_thread_create(&sp6_state.eventThreadId, "event_handle", sp6_emul_event_handle, &sp6_state, QEMU_THREAD_JOINABLE);

    return 0;
}

int sp6_emul_exit(void)
{
    /* Stop the cmd processing thread */
    sp6_state.thread_terminate = 1;

    qemu_cond_signal(&sp6_state.cmd_cond);
    qemu_thread_join(&sp6_state.cmdThreadId);
    qemu_thread_join(&sp6_state.eventThreadId);

    close(sp6_state.sock);

    return 0;
}
