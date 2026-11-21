/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_utils.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: azahidi <azahidi@student.1337.ma>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/21 00:04:22 by azahidi           #+#    #+#             */
/*   Updated: 2026/11/21 00:04:25 by azahidi          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	is_coder_runnable(t_coder *c, long long now)
{
	if (c->left_dongle->in_use || c->right_dongle->in_use)
		return (0);
	if (now < c->left_dongle->available_at_ms
		|| now < c->right_dongle->available_at_ms)
		return (0);
	return (1);
}

static int	is_highest_runnable(t_dongle *d, t_coder *c, t_request req,
		long long now)
{
	int		i;
	t_coder	*other;

	if (d->queue.size > 0 && d->queue.nodes[0].coder_id == c->id)
		return (1);
	i = 0;
	while (i < d->queue.size)
	{
		if (d->queue.nodes[i].coder_id != c->id)
		{
			other = &c->sim->coders[d->queue.nodes[i].coder_id - 1];
			if (is_coder_runnable(other, now)
				&& is_higher_priority(d->queue.nodes[i], req,
					c->sim->scheduler))
				return (0);
		}
		i++;
	}
	return (1);
}

static int	wait_step(t_sim *sim, t_dongle *first, t_dongle *second,
		long long now)
{
	long long		wake_ms;
	struct timespec	ts;

	if (!first->in_use && !second->in_use)
	{
		wake_ms = first->available_at_ms;
		if (second->available_at_ms > wake_ms)
			wake_ms = second->available_at_ms;
		if (wake_ms > now)
		{
			ts = ms_to_timespec(wake_ms);
			return (pthread_cond_timedwait(&sim->arb_cond, &sim->arb_mutex,
					&ts));
		}
	}
	return (pthread_cond_wait(&sim->arb_cond, &sim->arb_mutex));
}

static int	finish_pair(t_coder *c, t_dongle *first, t_dongle *second,
		int claim)
{
	pthread_mutex_lock(&first->mutex);
	heap_remove_coder(&first->queue, c->id, c->sim->scheduler);
	if (claim)
		first->in_use = 1;
	pthread_mutex_unlock(&first->mutex);
	pthread_mutex_lock(&second->mutex);
	heap_remove_coder(&second->queue, c->id, c->sim->scheduler);
	if (claim)
		second->in_use = 1;
	pthread_mutex_unlock(&second->mutex);
	if (claim)
		pthread_cond_broadcast(&c->sim->arb_cond);
	pthread_mutex_unlock(&c->sim->arb_mutex);
	return (claim);
}

int	acquire_pair(t_coder *c, t_dongle *first, t_dongle *second, long long dl)
{
	t_request	req;
	long long	now;
	int			st;

	req.deadline_ms = dl;
	pthread_mutex_lock(&c->sim->arb_mutex);
	if (!enqueue_pair(c, first, second, &req))
	{
		pthread_mutex_unlock(&c->sim->arb_mutex);
		return (0);
	}
	while (!sim_should_stop(c->sim))
	{
		now = get_time_ms();
		if (now < 0)
			break ;
		if (is_coder_runnable(c, now) && is_highest_runnable(first, c, req, now)
			&& is_highest_runnable(second, c, req, now))
			return (finish_pair(c, first, second, 1));
		st = wait_step(c->sim, first, second, now);
		if (st != 0 && st != ETIMEDOUT)
			break ;
	}
	return (finish_pair(c, first, second, 0));
}
