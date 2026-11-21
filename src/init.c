/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: azahidi <azahidi@student.1337.ma>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/21 00:07:36 by azahidi           #+#    #+#             */
/*   Updated: 2026/11/21 00:07:41 by azahidi          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	init_one_dongle(t_dongle *d, int id, int capacity)
{
	d->id = id;
	d->in_use = 0;
	d->available_at_ms = 0;
	d->next_seq = 0;
	d->queue.size = 0;
	d->queue.capacity = capacity;
	d->queue.nodes = malloc(sizeof(t_request) * capacity);
	if (!d->queue.nodes)
		return (1);
	if (pthread_mutex_init(&d->mutex, NULL) != 0)
	{
		free(d->queue.nodes);
		d->queue.nodes = NULL;
		return (1);
	}
	if (pthread_cond_init(&d->cond, NULL) != 0)
	{
		pthread_mutex_destroy(&d->mutex);
		free(d->queue.nodes);
		d->queue.nodes = NULL;
		return (1);
	}
	return (0);
}

static int	init_coders(t_sim *sim)
{
	int	i;

	sim->coders = malloc(sizeof(t_coder) * sim->num_coders);
	if (!sim->coders)
		return (1);
	i = 0;
	while (i < sim->num_coders)
	{
		sim->coders[i].id = i + 1;
		sim->coders[i].left_dongle = &sim->dongles[i];
		sim->coders[i].right_dongle = &sim->dongles[(i + 1) % sim->num_coders];
		sim->coders[i].sim = sim;
		sim->coders[i].last_compile_start = sim->start_time_ms;
		sim->coders[i].compiles_count = 0;
		if (pthread_mutex_init(&sim->coders[i].state_mutex, NULL) != 0)
		{
			destroy_coders(sim, i);
			return (1);
		}
		i++;
	}
	return (0);
}

static int	init_arb_resources(t_sim *sim)
{
	if (pthread_mutex_init(&sim->arb_mutex, NULL) != 0)
		return (1);
	if (pthread_cond_init(&sim->arb_cond, NULL) != 0)
	{
		pthread_mutex_destroy(&sim->arb_mutex);
		return (1);
	}
	return (0);
}

static int	init_sim_resources(t_sim *sim)
{
	if (pthread_mutex_init(&sim->sim_mutex, NULL) != 0)
		return (1);
	if (pthread_mutex_init(&sim->log_mutex, NULL) != 0)
	{
		pthread_mutex_destroy(&sim->sim_mutex);
		return (1);
	}
	if (init_arb_resources(sim) != 0)
	{
		pthread_mutex_destroy(&sim->log_mutex);
		pthread_mutex_destroy(&sim->sim_mutex);
		return (1);
	}
	sim->dongles = malloc(sizeof(t_dongle) * sim->num_coders);
	if (!sim->dongles)
	{
		destroy_sim_sync(sim);
		return (1);
	}
	return (0);
}

int	init_sim(t_sim *sim)
{
	int	i;

	sim->coders = NULL;
	sim->dongles = NULL;
	sim->next_seq = 0;
	if (init_sim_resources(sim) != 0)
		return (1);
	i = 0;
	while (i < sim->num_coders)
	{
		if (init_one_dongle(&sim->dongles[i], i, sim->num_coders) != 0)
		{
			destroy_dongles(sim, i);
			destroy_sim_sync(sim);
			return (1);
		}
		i++;
	}
	if (init_coders(sim) != 0)
	{
		destroy_dongles(sim, sim->num_coders);
		destroy_sim_sync(sim);
		return (1);
	}
	return (0);
}
