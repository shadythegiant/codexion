/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: azahidi <azahidi@student.1337.ma>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/21 00:08:38 by azahidi           #+#    #+#             */
/*   Updated: 2026/11/21 00:08:41 by azahidi          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	check_coder(t_sim *sim, int i, int *finished)
{
	long long	last_compile;
	long long	now;
	int			count;

	pthread_mutex_lock(&sim->coders[i].state_mutex);
	last_compile = sim->coders[i].last_compile_start;
	count = sim->coders[i].compiles_count;
	pthread_mutex_unlock(&sim->coders[i].state_mutex);
	if (sim->max_compiles > 0 && count >= sim->max_compiles)
		(*finished)++;
	now = get_time_ms();
	if (now < 0)
	{
		sim_set_stop(sim);
		wake_all_dongles(sim);
		return (1);
	}
	if (now - last_compile >= sim->time_to_burnout)
	{
		log_burnout(&sim->coders[i]);
		wake_all_dongles(sim);
		return (1);
	}
	return (0);
}

static int	scan_all_coders(t_sim *sim)
{
	int	i;
	int	finished;

	i = 0;
	finished = 0;
	while (i < sim->num_coders)
	{
		if (check_coder(sim, i, &finished))
			return (1);
		i++;
	}
	if (finished == sim->num_coders)
	{
		sim_set_stop(sim);
		wake_all_dongles(sim);
		return (1);
	}
	return (0);
}

void	*monitor_routine(void *arg)
{
	t_sim	*sim;

	sim = (t_sim *)arg;
	while (!sim_should_stop(sim))
	{
		if (scan_all_coders(sim))
			break ;
		usleep(500);
	}
	return (NULL);
}
