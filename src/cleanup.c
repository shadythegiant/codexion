/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cleanup.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: azahidi <azahidi@student.1337.ma>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/21 00:04:42 by azahidi           #+#    #+#             */
/*   Updated: 2026/11/21 00:04:49 by azahidi          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	destroy_dongles(t_sim *sim, int count)
{
	int	i;

	if (!sim->dongles)
		return ;
	i = 0;
	while (i < count)
	{
		pthread_mutex_destroy(&sim->dongles[i].mutex);
		free(sim->dongles[i].queue.nodes);
		sim->dongles[i].queue.nodes = NULL;
		i++;
	}
	free(sim->dongles);
	sim->dongles = NULL;
}

void	destroy_coders(t_sim *sim, int count)
{
	int	i;

	if (!sim->coders)
		return ;
	i = 0;
	while (i < count)
	{
		pthread_mutex_destroy(&sim->coders[i].state_mutex);
		i++;
	}
	free(sim->coders);
	sim->coders = NULL;
}

void	destroy_sim_sync(t_sim *sim)
{
	pthread_cond_destroy(&sim->arb_cond);
	pthread_mutex_destroy(&sim->arb_mutex);
	pthread_mutex_destroy(&sim->log_mutex);
	pthread_mutex_destroy(&sim->sim_mutex);
}

void	cleanup_sim(t_sim *sim)
{
	destroy_coders(sim, sim->num_coders);
	destroy_dongles(sim, sim->num_coders);
	destroy_sim_sync(sim);
}
