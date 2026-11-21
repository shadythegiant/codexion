/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init_2.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: azahidi <azahidi@student.1337.ma>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/20 21:15:33 by azahidi           #+#    #+#             */
/*   Updated: 2026/11/20 21:15:38 by azahidi          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	abort_threads(t_sim *sim, int created_count)
{
	int	j;

	sim_set_stop(sim);
	wake_all_dongles(sim);
	j = 0;
	while (j < created_count)
	{
		pthread_join(sim->coders[j].thread, NULL);
		j++;
	}
}

static int	start_thread_helper(t_sim *sim)
{
	if (pthread_create(&sim->monitor_thread, NULL, monitor_routine, sim) != 0)
	{
		abort_threads(sim, sim->num_coders);
		return (1);
	}
	return (0);
}

int	start_thread(t_sim *sim)
{
	int	i;

	i = 0;
	sim->start_time_ms = get_time_ms();
	if (sim->start_time_ms < 0)
		return (1);
	while (i < sim->num_coders)
	{
		sim->coders[i].last_compile_start = sim->start_time_ms;
		i++;
	}
	i = 0;
	while (i < sim->num_coders)
	{
		if (pthread_create(&sim->coders[i].thread, NULL, coder_routine,
				&sim->coders[i]) != 0)
		{
			abort_threads(sim, i);
			return (1);
		}
		i++;
	}
	return (start_thread_helper(sim));
}
