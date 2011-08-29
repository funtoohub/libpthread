/*
 * Copyright (c) 2011, Dongsheng Song <songdongsheng@live.cn>
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file sem.c
 * @brief Implementation Code of Semaphore Routines
 */

#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

#include <winsock2.h>

#include "arch.h"
#include "misc.h"

/**
 * Create an unnamed semaphore.
 * @param sem The pointer of the semaphore object.
 * @param pshared The pshared argument indicates whether this semaphore
 *        is to be shared between the threads (0 or PTHREAD_PROCESS_PRIVATE)
 *        of a process, or between processes (PTHREAD_PROCESS_SHARED).
 * @param value The value argument specifies the initial value for
 *        the semaphore.
 * @return If the function succeeds, the return value is 0.
 *         If the function fails, the return value is -1,
 *         with errno set to indicate the error.
 */
int sem_init(sem_t *sem, int pshared, unsigned int value)
{
    char buf[24] = {'\0'};
    arch_sem_t *pv;

    if (sem == NULL || value > (unsigned int) SEM_VALUE_MAX)
        return set_errno(EINVAL);

    if (NULL == (pv = (arch_sem_t *)calloc(1, sizeof(arch_sem_t))))
        return set_errno(ENOMEM);

    if (pshared != PTHREAD_PROCESS_PRIVATE) {
        sprintf(buf, "Global\\%p", pv);
    }

    if ((pv->handle = CreateSemaphore (NULL, value, SEM_VALUE_MAX, buf)) == NULL) {
        free(pv);
        return set_errno(ENOSPC);
    }

    *sem = pv;
    return 0;
}

/**
 * Acquire a semaphore.
 * @param sem The pointer of the semaphore object.
 * @return If the function succeeds, the return value is 0.
 *         If the function fails, the return value is -1,
 *         with errno set to indicate the error.
 */
int sem_wait(sem_t *sem)
{
    arch_sem_t *pv = (arch_sem_t *) sem;

    if (sem == NULL || pv == NULL)
        return set_errno(EINVAL);

    if (WaitForSingleObject(pv->handle, INFINITE) != WAIT_OBJECT_0)
        return set_errno(EINVAL);

    return 0;
}

/**
 * Try acquire a semaphore.
 * @param sem The pointer of the semaphore object.
 * @return If the function succeeds, the return value is 0.
 *         If the function fails, the return value is -1,
 *         with errno set to indicate the error.
 */
int sem_trywait(sem_t *sem)
{
    unsigned rc;
    arch_sem_t *pv = (arch_sem_t *) sem;

    if (sem == NULL || pv == NULL)
        return set_errno(EINVAL);

    if ((rc = WaitForSingleObject(pv->handle, 0)) == WAIT_OBJECT_0)
        return 0;

    if (rc == WAIT_TIMEOUT)
        return set_errno(EAGAIN);

    return set_errno(EINVAL);
}

/**
 * Try acquire a semaphore.
 * @param sem The pointer of the semaphore object.
 * @param abs_timeout The pointer of the structure that specifies an
 *        absolute timeout in seconds and nanoseconds since the Epoch,
 *        1970-01-01 00:00:00 +0000 (UTC).
 * @return If the function succeeds, the return value is 0.
 *         If the function fails, the return value is -1,
 *         with errno set to indicate the error.
 */
int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout)
{
    unsigned rc;
    arch_sem_t *pv = (arch_sem_t *) sem;

    if (sem == NULL || pv == NULL)
        return set_errno(EINVAL);

    if ((rc = WaitForSingleObject(pv->handle, arch_rel_time_in_ms(abs_timeout))) == WAIT_OBJECT_0)
        return 0;

    if (rc == WAIT_TIMEOUT)
        return set_errno(ETIMEDOUT);

    return set_errno(EINVAL);
}

/**
 * Release a semaphore.
 * @param sem The pointer of the semaphore object.
 * @return If the function succeeds, the return value is 0.
 *         If the function fails, the return value is -1,
 *         with errno set to indicate the error.
 */
int sem_post(sem_t *sem)
{
    arch_sem_t *pv = (arch_sem_t *) sem;

    if (sem == NULL || pv == NULL)
        return set_errno(EINVAL);

    if (ReleaseSemaphore(pv->handle, 1, NULL) == 0) {
        if (ERROR_TOO_MANY_POSTS == GetLastError())
            return set_errno(EOVERFLOW);
        return set_errno(EINVAL);
    }

    return 0;
}

/**
 * Get the value of a semaphore.
 * @param sem The pointer of the semaphore object.
 * @param value The pointer of the current value of the semaphore.
 * @return If the function succeeds, the return value is 0.
 *         If the function fails, the return value is -1,
 *         with errno set to indicate the error.
 */
int sem_getvalue(sem_t *sem, int *value)
{
    long previous;
    arch_sem_t *pv = (arch_sem_t *) sem;

    switch (WaitForSingleObject(pv->handle, 0)) {
    case WAIT_OBJECT_0:
        if (!ReleaseSemaphore(pv->handle, 1, &previous))
            return set_errno(EINVAL);
        *value = previous + 1;
        return 0;
    case WAIT_TIMEOUT:
        *value = 0;
        return 0;
    default:
        return set_errno(EINVAL);
    }
}

/**
 * Destroy a semaphore.
 * @param sem The pointer of the semaphore object.
 * @return If the function succeeds, the return value is 0.
 *         If the function fails, the return value is -1,
 *         with errno set to indicate the error.
 */
int sem_destroy(sem_t *sem)
{
    arch_sem_t *pv = (arch_sem_t *) sem;

    if (pv == NULL)
        return set_errno(EINVAL);

    if (CloseHandle (pv->handle) == 0)
        return set_errno(EINVAL);

    free(pv);
    *sem = NULL;

    return 0;
}

/**
 * Open a named semaphore.
 * @param name The name of the semaphore object.
 * @param oflag If O_CREAT is specified in oflag, then the semaphore is
 *        created if it does not already exist. If both O_CREAT and O_EXCL
 *        are specified in oflag, then an error is returned if a semaphore
 *        with the given name already exists.
 * @param mode Ignored (The mode argument specifies the permissions to be
 *        placed on the new semaphore).
 * @return On success, returns the address of the new semaphore; On error,
 *         returns SEM_FAILED (NULL), with errno set to indicate the error.
 */
sem_t *sem_open(const char *name, int oflag, mode_t mode, unsigned int value)
{
    int len;
    char buffer[512];
    arch_sem_t *pv;

    if (value > (unsigned int) SEM_VALUE_MAX || (len = strlen(name)) > (int) sizeof(buffer) - 8 || len < 1) {
        set_errno(EINVAL);
        return NULL;
    }

    if (NULL == (pv = (arch_sem_t *)calloc(1, sizeof(arch_sem_t)))) {
        set_errno(ENOMEM);
        return NULL;
    }

    memcpy(buffer, "Global\\", 7);
    memcpy(buffer + 7, name, len);
    buffer[len + 7] = '\0';

    if ((pv->handle = CreateSemaphore (NULL, value, SEM_VALUE_MAX, buffer)) == NULL)
    {
        switch(GetLastError()) {
            case ERROR_ACCESS_DENIED:
                set_errno(EACCES);
                break;
            case ERROR_INVALID_HANDLE:
                set_errno(ENOENT);
                break;
            default:
                set_errno(ENOSPC);
                break;
        }
        free(pv);
        return NULL;
    } else {
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            if ((oflag & O_CREAT) && (oflag & O_EXCL)) {
                CloseHandle(pv->handle);
                free(pv);
                set_errno(EEXIST);
                return NULL;
            }
            return (sem_t *) pv;
        } else {
            if (!(oflag & O_CREAT)) {
                free(pv);
                set_errno(ENOENT);
                return NULL;
            }
        }
    }

    return (sem_t *) pv;
}

/**
 * Close a named semaphore.
 * @param sem The pointer of the semaphore object.
 * @return If the function succeeds, the return value is 0.
 *         If the function fails, the return value is -1,
 *         with errno set to indicate the error.
 * @remark Same as sem_destroy.
 */
int sem_close(sem_t *sem)
{
    return sem_destroy(sem);
}

/**
 * Remove a named semaphore.
 * @param name The name of the semaphore object.
 * @return If the function succeeds, the return value is 0.
 *         If the function fails, the return value is -1,
 *         with errno set to indicate the error.
 * @remark The semaphore object is destroyed when its last handle has been
 *         closed, so this function does nothing.
 */
int sem_unlink(const char *name)
{
    return 0;
}
