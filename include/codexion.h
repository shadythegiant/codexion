/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: azahidi <azahidi@student.1337.ma>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/21 01:35:04 by azahidi           #+#    #+#             */
/*   Updated: 2026/11/21 01:35:09 by azahidi          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <errno.h>
# include <limits.h>
# include <pthread.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <sys/time.h>
# include <time.h>
# include <unistd.h>

typedef enum e_sched_type
{
	SCHED_TYPE_FIFO,
	SCHED_TYPE_EDF
}						t_sched_type;

typedef struct s_request
{
	int					coder_id;
	long long			deadline_ms;
	unsigned long long	arrival_seq;
}						t_request;

typedef struct s_heap
{
	t_request			*nodes;
	int					size;
	int					capacity;
}						t_heap;

typedef struct s_dongle
{
	int					id;
	pthread_mutex_t		mutex;
	pthread_cond_t		cond;
	int					in_use;
	long long			available_at_ms;
	t_heap				queue;
}						t_dongle;

typedef struct s_sim	t_sim;

typedef struct s_coder
{
	int					id;
	pthread_t			thread;
	pthread_mutex_t		state_mutex;
	long long			last_compile_start;
	int					compiles_count;
	t_dongle			*left_dongle;
	t_dongle			*right_dongle;
	t_sim				*sim;
}						t_coder;

typedef struct s_sim
{
	int					num_coders;
	long long			time_to_burnout;
	long long			time_to_compile;
	long long			time_to_debug;
	long long			time_to_refactor;
	long long			dongle_cooldown;
	int					max_compiles;
	t_sched_type		scheduler;
	long long			start_time_ms;
	int					stop_sim;
	unsigned long long	next_seq;
	pthread_mutex_t		sim_mutex;
	pthread_mutex_t		log_mutex;
	pthread_mutex_t		arb_mutex;
	pthread_cond_t		arb_cond;
	pthread_t			monitor_thread;
	t_coder				*coders;
	t_dongle			*dongles;
}						t_sim;

/* time utils */
long long				get_time_ms(void);
long long				get_elapsed_time(long long start_time_ms);
struct timespec			ms_to_timespec(long long epoch_ms);
void					smart_sleep(long long duration_ms, t_sim *sim);

/* simulation utils */
int						sim_should_stop(t_sim *sim);
void					sim_set_stop(t_sim *sim);
long long				get_coder_deadline(t_coder *coder);
int						heap_remove_coder(t_heap *heap, int coder_id,
							t_sched_type sched);

/* coder & monitor routines */
void					*coder_routine(void *arg);
void					*monitor_routine(void *arg);

/* dongles & arbitration */
void					get_ordered_dongles(t_coder *coder, t_dongle **first,
							t_dongle **second);
void					wake_all_dongles(t_sim *sim);
void					release_dongle(t_dongle *dongle, t_sim *sim);
void					release_pair(t_dongle *first, t_dongle *second,
							t_sim *sim);
int						enqueue_pair(t_coder *c, t_dongle *first,
							t_dongle *second, t_request *req);
int						acquire_pair(t_coder *c, t_dongle *first,
							t_dongle *second, long long dl);

/* heap utils */
int						is_higher_priority(t_request a, t_request b,
							t_sched_type sched);
void					sift_up(t_heap *heap, int index, t_sched_type sched);
void					sift_down(t_heap *heap, int index, t_sched_type sched);
int						heap_push(t_heap *heap, t_request req,
							t_sched_type sched);

/* logging */
void					log_state(t_coder *coder, const char *status);
void					log_burnout(t_coder *coder);
void					log_compile_start(t_coder *coder);

/* init & cleanup */
int						parse_args(int argc, char **argv, t_sim *sim);
int						init_sim(t_sim *sim);
int						start_thread(t_sim *sim);
void					destroy_dongles(t_sim *sim, int count);
void					destroy_coders(t_sim *sim, int count);
void					destroy_sim_sync(t_sim *sim);
void					cleanup_sim(t_sim *sim);

#endif
