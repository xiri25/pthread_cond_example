#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <stdatomic.h>

typedef struct { 
    uint64_t pro_idx;
    uint64_t con_idx;
    pthread_mutex_t idxs_mutex;
    pthread_cond_t cond;
} context;

typedef struct {
    uint8_t i;
    uint8_t* buffers;
    _Atomic uint8_t* running;
    context* ctx;
} pro_args_t;

typedef struct {
    uint8_t i;
    uint8_t* buffers;
    _Atomic uint8_t* running;
    context* ctx;
} con_args_t;

void print_buffer(const uint8_t* buffer)
{
    printf("{%d, %d, %d}\n", buffer[0], buffer[1], buffer[2]);
}

void* producir(void* thread_args)
{
    pro_args_t *args = (pro_args_t*)thread_args;

    while(atomic_load(args->running))
    {
        if( pthread_mutex_lock(&args->ctx->idxs_mutex) == 0)
        {
            while( (args->ctx->pro_idx - args->ctx->con_idx) >= 3 ) //Condiciones para esperar
            {
                //printf("producir esperando\n");
                pthread_cond_wait(&args->ctx->cond, &args->ctx->idxs_mutex);
            }

            usleep(15000); //Lo que devuelve es lo que se escribe
            
            args->buffers[args->ctx->pro_idx % 3] = args->i;
            args->ctx->pro_idx++;

            print_buffer(args->buffers);
            
            if ( args->ctx->pro_idx > (UINT64_MAX - 1) )
            {
                break;
            }

            //args->ctx->pro_idx = args->ctx->pro_idx % UINT64_MAX;
            //printf("pro_idx = %ld, con_idx = %ld\n\n", args->ctx->pro_idx, args->ctx->con_idx);
            assert( pthread_cond_signal(&args->ctx->cond) == 0);
            assert( pthread_mutex_unlock(&args->ctx->idxs_mutex) == 0);
        } else {
            continue;
        }
    }

    return (void*)NULL;
}

void* consumir(void* thread_args)
{
    con_args_t *args = (con_args_t*)thread_args;

    while(atomic_load(args->running))
    {
        if( pthread_mutex_lock(&args->ctx->idxs_mutex) == 0)
        {
            while( args->ctx->pro_idx == args->ctx->con_idx )
            {
                //printf("consumir esperando\n");
                pthread_cond_wait(&args->ctx->cond, &args->ctx->idxs_mutex);
            }

            usleep(200000); //Lo que devuelve es lo que se escribe
            
            args->buffers[args->ctx->con_idx % 3] = args->i;
            args->ctx->con_idx++;

            print_buffer(args->buffers);
            
            if ( args->ctx->con_idx > (UINT64_MAX - 1) )
            {
                break;
            }

            //args->ctx->con_idx = args->ctx->con_idx % UINT64_MAX;
            //printf("pro_idx = %ld, con_idx = %ld\n\n", args->ctx->pro_idx, args->ctx->con_idx);
            assert( pthread_cond_signal(&args->ctx->cond) == 0);
            assert( pthread_mutex_unlock(&args->ctx->idxs_mutex) == 0);
        } else {
            continue;
        }
    }

    return (void*)NULL;
}

int main(void)
{
    uint8_t buffers[3] = {0, 0, 0};

    context ctx = {
        .pro_idx = 0,
        .con_idx = 0,
    };

    assert( pthread_mutex_init(&ctx.idxs_mutex,NULL) == 0 );
    assert( pthread_cond_init(&ctx.cond, NULL) == 0 );

    /* volatile para que el compilador no la optimize */
    _Atomic uint8_t running = 1;

    pro_args_t pro_args = {
        .buffers = &buffers[0],
        .i = 1,
        .running = &running,
        .ctx = &ctx,
    };

    con_args_t con_args = {
        .buffers = &buffers[0],
        .i = 2,
        .running = &running,
        .ctx = &ctx,
    };

    pthread_t pro_thread;
    assert( pthread_create(&pro_thread, NULL, producir, (void*)&pro_args) == 0);
    
    pthread_t con_thread;
    assert( pthread_create(&con_thread, NULL, consumir, (void*)&con_args) == 0);
   
    // Let the threads run for a while
    //printf("Press Enter to stop the threads...\n");
    getchar();
    atomic_store(&running, 0);

    // Wake up any waiting threads before joining
    pthread_mutex_lock(&ctx.idxs_mutex);
    pthread_cond_broadcast(&ctx.cond);
    pthread_mutex_unlock(&ctx.idxs_mutex);

    assert( pthread_join(pro_thread, NULL) == 0 );
    assert( pthread_join(con_thread, NULL) == 0 );

    assert( pthread_cond_destroy(&ctx.cond) == 0 );
    assert( pthread_mutex_destroy(&ctx.idxs_mutex) == 0 );
 
    return 0;
}
