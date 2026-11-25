/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: azahidi <azahidi@student.1337.ma>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/21 00:05:11 by azahidi           #+#    #+#             */
/*   Updated: 2026/11/21 00:05:13 by azahidi          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	get_ordered_dongles(t_coder *coder, t_dongle **first, t_dongle **second)
{
	if (coder->sim->num_coders == 1)
	{
		*first = coder->left_dongle;
		*second = NULL;
		return ;
	}
	if (coder->left_dongle->id < coder->right_dongle->id)
	{
		*first = coder->left_dongle;
		*second = coder->right_dongle;
	}
	else
	{
		*first = coder->right_dongle;
		*second = coder->left_dongle;
	}
}

void	wake_all_dongles(t_sim *sim)
{
	pthread_mutex_lock(&sim->arb_mutex);
	pthread_cond_broadcast(&sim->arb_cond);
	pthread_mutex_unlock(&sim->arb_mutex);
}

void	release_dongle(t_dongle *dongle, t_sim *sim)
{
	long long	now;

	pthread_mutex_lock(&dongle->mutex);
	dongle->in_use = 0;
	now = get_time_ms();
	if (now < 0)
		sim_set_stop(sim);
	else
		dongle->available_at_ms = now + sim->dongle_cooldown;
	pthread_mutex_unlock(&dongle->mutex);
}

void	release_pair(t_dongle *first, t_dongle *second, t_sim *sim)
{
	pthread_mutex_lock(&sim->arb_mutex);
	if (second)
		release_dongle(second, sim);
	if (first)
		release_dongle(first, sim);
	pthread_cond_broadcast(&sim->arb_cond);
	pthread_mutex_unlock(&sim->arb_mutex);
}

int	enqueue_pair(t_coder *c, t_dongle *first, t_dongle *second, t_request *req)
{
	req->coder_id = c->id;
	req->arrival_seq = ++c->sim->next_seq;
	pthread_mutex_lock(&first->mutex);
	if (heap_push(&first->queue, *req, c->sim->scheduler) != 0)
	{
		pthread_mutex_unlock(&first->mutex);
		return (0);
	}
	pthread_mutex_unlock(&first->mutex);
	pthread_mutex_lock(&second->mutex);
	if (heap_push(&second->queue, *req, c->sim->scheduler) != 0)
	{
		pthread_mutex_unlock(&second->mutex);
		pthread_mutex_lock(&first->mutex);
		heap_remove_coder(&first->queue, c->id, c->sim->scheduler);
		pthread_mutex_unlock(&first->mutex);
		return (0);
	}
	pthread_mutex_unlock(&second->mutex);
	pthread_cond_broadcast(&c->sim->arb_cond);
	return (1);
}
