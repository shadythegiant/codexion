/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   log.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: azahidi <azahidi@student.1337.ma>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/15 19:31:35 by azahidi           #+#    #+#             */
/*   Updated: 2026/11/15 19:31:39 by azahidi          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	log_state(t_coder *coder, const char *status)
{
	t_sim	*sim;

	sim = coder->sim;
	pthread_mutex_lock(&sim->log_mutex);
	if (sim_should_stop(sim) == 0)
	{
		printf("%lld %d %s\n", get_elapsed_time(sim->start_time_ms), coder->id,
			status);
	}
	pthread_mutex_unlock(&sim->log_mutex);
}

void	log_burnout(t_coder *coder)
{
	t_sim	*sim;

	sim = coder->sim;
	pthread_mutex_lock(&sim->log_mutex);
	if (!sim_should_stop(sim))
	{
		sim_set_stop(sim);
		printf("%lld %d burned out\n", get_elapsed_time(sim->start_time_ms),
			coder->id);
	}
	pthread_mutex_unlock(&sim->log_mutex);
}

void	log_compile_start(t_coder *coder)
{
	t_sim		*sim;
	long long	ts;

	sim = coder->sim;
	pthread_mutex_lock(&sim->log_mutex);
	if (!sim_should_stop(sim))
	{
		ts = get_elapsed_time(sim->start_time_ms);
		printf("%lld %d has taken a dongle\n", ts, coder->id);
		printf("%lld %d has taken a dongle\n", ts, coder->id);
		printf("%lld %d is compiling\n", ts, coder->id);
	}
	pthread_mutex_unlock(&sim->log_mutex);
}
