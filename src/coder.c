/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: azahidi <azahidi@student.1337.ma>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/21 00:03:49 by azahidi           #+#    #+#             */
/*   Updated: 2026/11/21 00:03:59 by azahidi          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	handle_single_coder(t_coder *coder, t_dongle *first)
{
	pthread_mutex_lock(&coder->sim->arb_mutex);
	pthread_mutex_lock(&first->mutex);
	first->in_use = 1;
	pthread_mutex_unlock(&first->mutex);
	pthread_mutex_unlock(&coder->sim->arb_mutex);
	log_state(coder, "has taken a dongle");
	while (!sim_should_stop(coder->sim))
		usleep(500);
	release_pair(first, NULL, coder->sim);
	return (0);
}

static int	update_compile_start(t_coder *coder)
{
	long long	now;

	now = get_time_ms();
	if (now < 0)
		return (0);
	pthread_mutex_lock(&coder->state_mutex);
	coder->last_compile_start = now;
	pthread_mutex_unlock(&coder->state_mutex);
	return (1);
}

static void	increment_compiles(t_coder *coder)
{
	if (sim_should_stop(coder->sim))
		return ;
	pthread_mutex_lock(&coder->state_mutex);
	coder->compiles_count++;
	pthread_mutex_unlock(&coder->state_mutex);
}

static int	coder_compile_phase(t_coder *coder)
{
	t_dongle	*first;
	t_dongle	*second;
	long long	deadline;

	get_ordered_dongles(coder, &first, &second);
	if (!second)
		return (handle_single_coder(coder, first));
	deadline = get_coder_deadline(coder);
	if (!acquire_pair(coder, first, second, deadline))
		return (0);
	if (!update_compile_start(coder))
	{
		release_pair(first, second, coder->sim);
		return (0);
	}
	log_compile_start(coder);
	smart_sleep(coder->sim->time_to_compile, coder->sim);
	increment_compiles(coder);
	release_pair(first, second, coder->sim);
	return (1);
}

void	*coder_routine(void *arg)
{
	t_coder	*coder;

	coder = (t_coder *)arg;
	while (!sim_should_stop(coder->sim))
	{
		if (!coder_compile_phase(coder))
			break ;
		if (sim_should_stop(coder->sim))
			break ;
		log_state(coder, "is debugging");
		smart_sleep(coder->sim->time_to_debug, coder->sim);
		if (sim_should_stop(coder->sim))
			break ;
		log_state(coder, "is refactoring");
		smart_sleep(coder->sim->time_to_refactor, coder->sim);
	}
	return (NULL);
}
